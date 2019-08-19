#pragma once
#include <array>
#include <Vectors.h>
#include <Offsets.h>
#include <Windows.h>
#include <Globals.h>
#include <Memory.h>

namespace Engine
{
	class Camera
	{
	public:
		std::array<float, 16> GetViewMatrix();
		Vector2 WorldToScreen(Vector3 position);
	};
}