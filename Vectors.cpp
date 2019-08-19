#include "Vectors.h"

Vector2::Vector2()
{
}

Vector2::Vector2(float x, float y)
	: x(x), y(y)
{
}

Vector3::Vector3()
{
}

Vector3::Vector3(float x, float y, float z)
	: x(x), y(y), z(z)
{
}

Vector3& Vector3::operator+=(const Vector3& vector)
{
	x += vector.x;
	y += vector.y;
	z += vector.z;

	return *this;
}

Vector3& Vector3::operator-=(const Vector3& vector)
{
	x -= vector.x;
	y -= vector.y;
	z -= vector.z;

	return *this;
}

Vector3& Vector3::operator*=(float number)
{
	x *= number;
	y *= number;
	z *= number;

	return *this;
}

Vector3& Vector3::operator/=(float number)
{
	x /= number;
	y /= number;
	z /= number;

	return *this;
}

// Vector 4
Vector4::Vector4()
{
}

Vector4::Vector4(float x, float y, float z, float w)
	: x(x), y(y), z(z), w(w)
{
}

Vector4& Vector4::operator+=(const Vector4& vector)
{
	x += vector.x;
	y += vector.y;
	z += vector.z;
	w += vector.w;

	return *this;
}

Vector4& Vector4::operator-=(const Vector4& vector)
{
	x -= vector.x;
	y -= vector.y;
	z -= vector.z;
	w -= vector.w;

	return *this;
}

Vector4& Vector4::operator*=(float number)
{
	x *= number;
	y *= number;
	z *= number;
	w *= number;

	return *this;
}

Vector4& Vector4::operator/=(float number)
{
	x /= number;
	y /= number;
	z /= number;
	w /= number;

	return *this;
}