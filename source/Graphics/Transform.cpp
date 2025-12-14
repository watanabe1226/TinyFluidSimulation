#include "Graphics/Transform.h"

Transform::Transform()
{
	Update();
}

Transform::~Transform()
{
}

void Transform::SetPosition(float x, float y, float z)
{
	m_Position.x = x;
	m_Position.y = y;
	m_Position.z = z;
	Update();
}

void Transform::SetPosition(const Vector3D& pos)
{
	m_Position = pos;
	Update();
}

void Transform::SetRotation(float x, float y, float z)
{
	m_Rotation.x = x;
	m_Rotation.y = y;
	m_Rotation.z = z;
	m_Quaternion = Quaternion::Euler(m_Rotation);
	Update();
}

void Transform::SetRotation(const Vector3D& rot)
{
	m_Rotation = rot;
	m_Quaternion = Quaternion::Euler(m_Rotation);
	Update();
}

void Transform::SetScale(float x, float y, float z)
{
	m_Scale.x = x;
	m_Scale.y = y;
	m_Scale.z = z;
	Update();
}

void Transform::SetScale(const Vector3D& scale)
{
	m_Scale = scale;
	Update();
}

void Transform::AddTranslation(float x, float y, float z)
{
	m_Position.x += x;
	m_Position.y += y;
	m_Position.z += z;
	Update();
}

void Transform::AddTranslation(const Vector3D& trans)
{
	m_Position += trans;
	Update();
}

const Vector3D& Transform::GetPosition() const
{
	return m_Position;
}

const Vector3D& Transform::GetRotation() const
{
	return m_Rotation;
}

const Quaternion& Transform::GetQuaternion() const
{
	return m_Quaternion;
}

const Vector3D& Transform::GetScale() const
{
	return m_Scale;
}

const Vector3D& Transform::GetForward() const
{
	return m_Forward;
}

const Vector3D& Transform::GetRight() const
{
	return m_Right;
}

const Vector3D& Transform::GetUp() const
{
	return m_Up;
}

const Matrix4x4& Transform::GetWorld() const
{
	return m_World;
}

const Matrix4x4& Transform::GetView() const
{
	return m_View;
}

void Transform::Update()
{
	auto scale = Matrix4x4::ScalingToMatrix(m_Scale);
	auto rot = Matrix4x4::QuaternionToMatrix(m_Quaternion);
	auto pos = Matrix4x4::TransitionToMatrix(m_Position);
	m_World = scale * rot * pos;

	// 0çsñ⁄: Right (Xé≤)
	m_Right.x = m_World.m_mat[0][0];
	m_Right.y = m_World.m_mat[0][1];
	m_Right.z = m_World.m_mat[0][2];
	m_Right.GetSafeNormal();

	// 1çsñ⁄: Up (Yé≤)
	m_Up.x = m_World.m_mat[1][0];
	m_Up.y = m_World.m_mat[1][1];
	m_Up.z = m_World.m_mat[1][2];
	m_Up.GetSafeNormal();

	// 2çsñ⁄: Forward (Zé≤)
	m_Forward.x = m_World.m_mat[2][0];
	m_Forward.y = m_World.m_mat[2][1];
	m_Forward.z = m_World.m_mat[2][2];
	m_Forward.GetSafeNormal();

	m_View = Matrix4x4::setLookAtLH(m_Position, m_Forward, m_Up);
}
