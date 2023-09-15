#pragma once
#include "math_types.h"
#include <vector>

namespace dev5
{
	using namespace end;

	struct joint_t
	{
		float4x4 transform;
		int parent;
	};

	using joint_set_t = std::vector<joint_t>;

	struct keyframe_t
	{
		float time;
		joint_set_t joints;
	};

	using keyframe_set_t = std::vector<keyframe_t>;

	struct anim_clip_t
	{
		float duration;
		keyframe_set_t keyframes;
	};
}