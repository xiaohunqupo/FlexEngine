#include "stdafx.hpp"

#include "Physics/PhysicsWorld.hpp"

IGNORE_WARNINGS_PUSH
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>

#include <LinearMath/btVector3.h>
IGNORE_WARNINGS_POP

#include "Cameras/BaseCamera.hpp"
#include "Cameras/CameraManager.hpp"
#include "FlexEngine.hpp"
#include "Helpers.hpp"
#include "Physics/PhysicsHelpers.hpp"
#include "Physics/PhysicsManager.hpp"
#include "Physics/RigidBody.hpp"
#include "Scene/BaseScene.hpp"
#include "Scene/GameObject.hpp"
#include "Scene/SceneManager.hpp"
#include "Window/Window.hpp"

namespace flex
{
	const u32 PhysicsWorld::MAX_SUBSTEPS = 32;

	PhysicsWorld::PhysicsWorld()
	{
	}

	PhysicsWorld::~PhysicsWorld()
	{
	}

	void PhysicsWorld::Initialize()
	{
		if (m_World == nullptr)
		{
			m_World = g_PhysicsManager->CreateWorld();

			m_World->setInternalTickCallback(PhysicsInternalTickCallback, this);

			m_World->getSolverInfo().m_globalCfm = 0.00001f;
		}
	}

	void PhysicsWorld::Destroy()
	{
		if (m_World != nullptr)
		{
			for (i32 i = m_World->getNumCollisionObjects() - 1; i >= 0; --i)
			{
				btCollisionObject* obj = m_World->getCollisionObjectArray()[i];
				btRigidBody* body = btRigidBody::upcast(obj);
				if (body != nullptr && body->getMotionState())
				{
					delete body->getMotionState();
				}
				m_World->removeCollisionObject(obj);
				delete obj;
			}

			delete m_World;
			m_World = nullptr;
		}
	}

	void PhysicsWorld::StepSimulation(sec deltaSeconds)
	{
		if (m_World != nullptr)
		{
			PROFILE_AUTO("Step physics simulation");

			if (!g_EngineInstance->IsSimulationPaused())
			{
				m_AccumulatedTime += deltaSeconds;
			}

			BaseScene* scene = g_SceneManager->CurrentScene();

			u32 numSubsteps = glm::min((u32)(m_AccumulatedTime / g_FixedDeltaTime), MAX_SUBSTEPS);

			for (u32 step = 0; step < numSubsteps; ++step)
			{
				m_World->stepSimulation(g_FixedDeltaTime, 1, g_FixedDeltaTime);
				scene->FixedUpdate();
				m_AccumulatedTime -= g_FixedDeltaTime;
			}

			// TODO: Tell bullet what remaining time is so it can interpolate?
		}
	}

	btDiscreteDynamicsWorld* PhysicsWorld::GetWorld()
	{
		return m_World;
	}

	btVector3 PhysicsWorld::GenerateDirectionRayFromScreenPos(i32 x, i32 y)
	{
		real frameBufferWidth = (real)g_Window->GetFrameBufferSize().x;
		real frameBufferHeight = (real)g_Window->GetFrameBufferSize().y;
		btScalar aspectRatio = frameBufferWidth / frameBufferHeight;

		BaseCamera* camera = g_CameraManager->CurrentCamera();
		real fov = camera->FOV;
		real tanFov = tanf(0.5f * fov);

		real pixelScreenX = 2.0f * ((x + 0.5f) / frameBufferWidth) - 1.0f;
		real pixelScreenY = 1.0f - 2.0f * ((y + 0.5f) / frameBufferHeight);

		real pixelCameraX = pixelScreenX * aspectRatio * tanFov;
		real pixelCameraY = pixelScreenY * tanFov;


		glm::mat4 cameraView = glm::inverse(camera->GetView());

		glm::vec4 rayOrigin(0, 0, 0, 1);
		glm::vec3 rayOriginWorld = cameraView * rayOrigin;

		glm::vec3 rayPWorld = cameraView * glm::vec4(pixelCameraX, pixelCameraY, 1.0f, 1.0f);
		btVector3 rayDirection = ToBtVec3(rayPWorld - rayOriginWorld);
		rayDirection.normalize();

		return rayDirection;
	}

	GameObject* PhysicsWorld::PickTaggedBody(const btVector3& rayStart, const btVector3& rayEnd, const std::string& tag, u32 mask /* = (u32)CollisionType::DEFAULT */)
	{
		GameObject* pickedGameObject = nullptr;

		btCollisionWorld::AllHitsRayResultCallback rayCallback(rayStart, rayEnd);
		rayCallback.m_collisionFilterGroup = (i32)mask;
		rayCallback.m_collisionFilterMask = (i32)mask;
		m_World->rayTest(rayStart, rayEnd, rayCallback);
		real closestDist2 = FLT_MAX;
		if (rayCallback.hasHit())
		{
			for (i32 i = 0; i < rayCallback.m_hitPointWorld.size(); ++i)
			{
				btVector3 pickPos = rayCallback.m_hitPointWorld[i];
				const btRigidBody* body = btRigidBody::upcast(rayCallback.m_collisionObjects[i]);
				if (body != nullptr)
				{
					GameObject* gameObject = static_cast<GameObject*>(body->getUserPointer());

					if (gameObject != nullptr && gameObject->HasTag(tag))
					{
						real dist2 = (pickPos - rayStart).length2();
						if (dist2 < closestDist2)
						{
							closestDist2 = dist2;
							pickedGameObject = gameObject;
						}
					}
				}
			}
		}

		return pickedGameObject;
	}

	bool PhysicsWorld::GetPointOnGround(const glm::vec3& startingPoint, glm::vec3& outPointOnGround)
	{
		btCollisionWorld::ClosestRayResultCallback rayCallback(ToBtVec3(startingPoint), ToBtVec3(startingPoint - VEC3_UP * 10000.0f));
		m_World->rayTest(rayCallback.m_rayFromWorld, rayCallback.m_rayToWorld, rayCallback);
		if (rayCallback.hasHit())
		{
			const btRigidBody* body = btRigidBody::upcast(rayCallback.m_collisionObject);
			if (body != nullptr)
			{
				outPointOnGround = ToVec3(rayCallback.m_hitPointWorld);
				return true;
			}
		}

		return false;
	}

	bool PhysicsWorld::GetPointOnGround(const btConvexShape* shape, const btTransform& from, const btTransform& to, glm::vec3& outPointOnGround, glm::vec3& outGroundNormal)
	{
		btCollisionWorld::ClosestConvexResultCallback sweepCallback(from.getOrigin(), to.getOrigin());
		m_World->convexSweepTest(shape, from, to, sweepCallback);
		if (sweepCallback.hasHit())
		{
			const btRigidBody* body = btRigidBody::upcast(sweepCallback.m_hitCollisionObject);
			if (body != nullptr)
			{
				outPointOnGround = ToVec3(sweepCallback.m_hitPointWorld);
				outGroundNormal = ToVec3(sweepCallback.m_hitNormalWorld);
				return true;
			}
		}

		return false;
	}

	const btRigidBody* PhysicsWorld::PickFirstBody(const btVector3& rayStart, const btVector3& rayEnd)
	{
		const btRigidBody* pickedBody = nullptr;

		btCollisionWorld::ClosestRayResultCallback rayCallback(rayStart, rayEnd);
		m_World->rayTest(rayStart, rayEnd, rayCallback);
		if (rayCallback.hasHit())
		{
			//btVector3 pickPos = rayCallback.m_hitPointWorld;
			const btRigidBody* body = btRigidBody::upcast(rayCallback.m_collisionObject);
			if (body != nullptr)
			{
				GameObject* pickedGameObject = static_cast<GameObject*>(body->getUserPointer());

				if (pickedGameObject != nullptr)
				{
					pickedBody = body;
				}

				//pickedBody->activate(true);
				//pickedBody->clearForces();
				//
				//btVector3 localPivot = body->getCenterOfMassTransform().inverse() * pickPos;
				//pickedBody->applyForce({ 0, 600, 0 }, localPivot);

				//savedState = pickedBody->getActivationState();
				//pickedBody->setActivationState(DISABLE_DEACTIVATION);

				//btPoint2PointConstraint* p2p = new btPoint2PointConstraint(*body, localPivot);
				//dynamicsWorld->addConstraint(p2p, true);
				//pickedConstraint = p2p;
				//btScalar mousePickClamping = 30.f;
				//p2p->m_setting.m_impulseClamp = mousePickClamping;
				//p2p->m_setting.m_tau = 0.001f;
			}
			//hitPos = pickPos;
			//pickingDist = (pickPos - rayFromWorld).length();
		}

		return pickedBody;
	}

	// CLEANUP:
	void PhysicsInternalTickCallback(btDynamicsWorld* world, btScalar timeStep)
	{
		FLEX_UNUSED(timeStep);

		PhysicsWorld* physWorld = static_cast<PhysicsWorld*>(world->getWorldUserInfo());

		std::set<std::pair<const btCollisionObject*, const btCollisionObject*>> collisionPairsFoundThisStep;

		i32 numManifolds = world->getDispatcher()->getNumManifolds();
		for (i32 i = 0; i < numManifolds; ++i)
		{
			btPersistentManifold* contactManifold = world->getDispatcher()->getManifoldByIndexInternal(i);
			const btCollisionObject* obA = contactManifold->getBody0();
			const btCollisionObject* obB = contactManifold->getBody1();

			GameObject* obAGameObject = static_cast<GameObject*>(obA->getUserPointer());
			GameObject* obBGameObject = static_cast<GameObject*>(obB->getUserPointer());

			i32 numContacts = contactManifold->getNumContacts();

			if (numContacts > 0 && obAGameObject && obBGameObject)
			{
				u32 obAFlags = obAGameObject->GetRigidBody()->GetPhysicsFlags();
				u32 obBFlags = obBGameObject->GetRigidBody()->GetPhysicsFlags();

				u32 bObATrigger = (obAFlags & (u32)PhysicsFlag::TRIGGER);
				u32 bObBTrigger = (obBFlags & (u32)PhysicsFlag::TRIGGER);
				// If exactly one of the two objects is a trigger (not both)
				if (bObATrigger ^ bObBTrigger)
				{
					const btCollisionObject* triggerCollisionObject = (bObATrigger ? obA : obB);
					const btCollisionObject* otherCollisionObject = (bObATrigger ? obB : obA);
					GameObject* trigger = (bObATrigger ? obAGameObject : obBGameObject);
					GameObject* other = (bObATrigger ? obBGameObject : obAGameObject);

					if (!other->IsStatic())
					{
						std::pair<const btCollisionObject*, const btCollisionObject*> pair(triggerCollisionObject, otherCollisionObject);
						collisionPairsFoundThisStep.insert(pair);

						if (physWorld->m_CollisionPairs.find(pair) == physWorld->m_CollisionPairs.end())
						{
							trigger->OnOverlapBegin(other);
							other->OnOverlapBegin(trigger);
						}
					}
				}
			}

			//for (i32 j = 0; j < numContacts; ++j)
			//{
			//	btManifoldPoint& pt = contactManifold->getContactPoint(j);
			//	if (pt.getDistance() < 0.0f)
			//	{
			//		const btVector3& ptA = pt.getPositionWorldOnA();
			//		const btVector3& ptB = pt.getPositionWorldOnB();
			//		const btVector3& normalOnB = pt.m_normalWorldOnB;
			//		//Print("contact: " + std::to_string(normalOnB.getX()) + ", " +
			//		//				std::to_string(normalOnB.getY()) + ", " + std::to_string(normalOnB.getZ()));
			//	}
			//}
		}

		std::set<std::pair<const btCollisionObject*, const btCollisionObject*>> differentPairs;
		std::set_difference(physWorld->m_CollisionPairs.begin(), physWorld->m_CollisionPairs.end(),
			collisionPairsFoundThisStep.begin(), collisionPairsFoundThisStep.end(),
			std::inserter(differentPairs, differentPairs.begin()));

		for (const auto& pair : differentPairs)
		{
			GameObject* triggerGameObject = static_cast<GameObject*>(pair.first->getUserPointer());
			GameObject* otherGameObject = static_cast<GameObject*>(pair.second->getUserPointer());
			triggerGameObject->OnOverlapEnd(otherGameObject);
			otherGameObject->OnOverlapEnd(triggerGameObject);
		}

		physWorld->m_CollisionPairs.clear();
		for (const auto& pair : collisionPairsFoundThisStep)
		{
			physWorld->m_CollisionPairs.insert(pair);
		}
	}


	btScalar CustomContactResultCallback::addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap,
		int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1)
	{
		FLEX_UNUSED(cp);
		FLEX_UNUSED(colObj0Wrap);
		FLEX_UNUSED(partId0);
		FLEX_UNUSED(index0);
		FLEX_UNUSED(colObj1Wrap);
		FLEX_UNUSED(partId1);
		FLEX_UNUSED(index1);

		bHit = true;

		return 0.0f;
	}
} // namespace flex

