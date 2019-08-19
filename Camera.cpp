#include "Camera.h"

namespace Engine
{
	std::array<float, 16> Camera::GetViewMatrix()
	{
		auto base = *reinterpret_cast<Camera * *>(Offsets::gamerenderer + reinterpret_cast<uint64_t>(GetModuleHandle(NULL)));

		return *Memory::Ptr<std::array<float, 16>*>(base, Offsets::gamerenderer_viewmatrix);
	}

	// This method has not been tested, look here for good information: http://songho.ca/opengl/gl_anglestoaxes.html
	Vector2 Camera::WorldToScreen(Vector3 pos)
	{
		Vector2 screen;
		std::array<float, 16> matrix = GetViewMatrix();

		Vector4 clipCoords;
		clipCoords.x = pos.x * matrix[0] + pos.y * matrix[4] + pos.z * matrix[8] + matrix[12];
		clipCoords.y = pos.x * matrix[1] + pos.y * matrix[5] + pos.z * matrix[9] + matrix[13];
		clipCoords.w = pos.x * matrix[3] + pos.y * matrix[7] + pos.z * matrix[11] + matrix[15];

		if ((double)clipCoords.w < 0.100000001490116)
			return Vector2(-1, -1);

		Vector3 SP;
		SP.x = clipCoords.x / clipCoords.w;
		SP.y = clipCoords.y / clipCoords.w;

		screen.x = (Globals::g_iWindowWidth / 2 * SP.x) + (SP.x + Globals::g_iWindowWidth / 2);
		screen.y = -(Globals::g_iWindowHeight / 2 * SP.y) + (SP.y + Globals::g_iWindowHeight / 2);

		return screen;
	}
}