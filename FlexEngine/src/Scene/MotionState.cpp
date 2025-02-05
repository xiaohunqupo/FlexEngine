﻿#include "stdafx.hpp"

#include "Scene/MotionState.hpp"

#include "Transform.hpp"

namespace flex
{
	MotionState::MotionState(Transform* correspondingTransform)	:
		m_Transform(correspondingTransform)
	{
	}

	void MotionState::getWorldTransform(btTransform& centerOfMassWorldTrans) const
	{
		centerOfMassWorldTrans = btTransform(ToBtQuaternion(m_Transform->GetWorldRotation()), ToBtVec3(m_Transform->GetWorldPosition()));
	}

	void MotionState::setWorldTransform(const btTransform& centerOfMassWorldTrans)
	{
		m_Transform->SetFromBtTransform(centerOfMassWorldTrans);
	}
} // namespace flex
