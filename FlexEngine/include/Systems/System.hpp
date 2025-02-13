#pragma once

namespace flex
{
	enum class SystemType
	{
		PROPERTY_COLLECTION_MANAGER,
		PLUGGABLES,
		ROAD_MANAGER,
		TERMINAL_MANAGER,
		TRACK_MANAGER,
		CART_MANAGER,
		PARTICLE_MANAGER,

		_NONE
	};

	class System
	{
	public:
		virtual ~System() = default;

		virtual void Initialize() = 0;
		virtual void Destroy() = 0;
		virtual void Update() = 0;
		virtual void OnPreSceneChange() {}

		virtual void DrawImGui() {}

	};
} // namespace flex
