#include "Graphics/Camera.h"

Camera::Camera(uint32_t width, uint32_t height)
{
	m_Width = width;
	m_Height = height;
	m_Aspect = m_Width / m_Height;

	Update();
}

Camera::~Camera()
{
}

void Camera::Update()
{
	// プロジェクション行列の計算
	if (m_ProjMove == ProjectionMode::Perspective)
		m_Proj = Matrix4x4::setPerspectiveFovLH(m_FovY, m_Aspect, m_Near, m_Far);
	else
		m_Proj = Matrix4x4::setOrthoLH(m_Width, m_Height, m_Near, m_Far);

	// ビュー行列の計算
	m_View = Matrix4x4::setLookAtLH(m_Position, m_Target, m_Upward);
	// ビュー逆行列の計算
	m_ViewInv = Matrix4x4::inverse(m_View);

	// カメラ前方方向の計算
	m_Forward = Vector3D(m_ViewInv.m_mat[2][0], m_ViewInv.m_mat[2][1], m_ViewInv.m_mat[2][2]);
	// カメラ右方向
	m_Right = Vector3D(m_ViewInv.m_mat[0][0], m_ViewInv.m_mat[0][1], m_ViewInv.m_mat[0][2]);

	// カメラの回転行列を取得
	m_cameraRotation = m_ViewInv;
	m_cameraRotation.m_mat[3][0] = 0.0f;
	m_cameraRotation.m_mat[3][1] = 0.0f;
	m_cameraRotation.m_mat[3][2] = 0.0f;
	m_cameraRotation.m_mat[3][3] = 1.0f;

	// 注視点と視点までの距離を計算
	Vector3D toPos;
	toPos = m_Position - m_Target;
	m_targetToPositionLen = toPos.length();

	m_IsDirty = false;
}

//! @brief 注視点を原点としてカメラを回転させる
//! @param[in] 回転させるクォータニオン
void Camera::RotateOriginTarget(const Quaternion& qRot)
{
}

//! @brief カメラを動かす
//! @param[in] 移動量
void Camera::Move(const Vector3D& move)
{
	m_Position += move;
	m_Target += move;
	m_IsDirty = true;
}

//! @brief 注視点を動かす
//! @param[in] 移動量
void Camera::MoveTarget(const Vector3D& move)
{
	m_Target += move;
	m_IsDirty = true;
}

//! @brief 視点を動かす
//! @param[in] 移動量
void Camera::MovePosition(const Vector3D& move)
{
	m_Position += move;
	m_IsDirty = true;
}

//! @brief カメラの前方方向に移動
void Camera::MoveForward(float moveForward)
{
	auto forward = m_Forward * moveForward;
	Move(forward);
}

void Camera::MoveRight(float moveRight)
{
	auto right = m_Right * moveRight;
	Move(right);
}

void Camera::MoveUp(float moveUp)
{
	auto upward = m_Upward * moveUp;
	Move(upward);
}

void Camera::Reset()
{
	SetPositionAndTarget(Vector3D(0.0f, 0.0f, -5.0f), Vector3D(0.0f, 0.0f, 0.0f));
}

//! @brief カメラの位置を設定
void Camera::SetPosition(const Vector3D& pos)
{
	m_Position = pos;
	m_IsDirty = true;
}

void Camera::SetPosition(float x, float y, float z)
{
	SetPosition(Vector3D(x, y, z));
}

void Camera::SetPositionAndTarget(const Vector3D& pos, const Vector3D& target)
{
	m_Position = pos;
	m_Target = target;
	m_IsDirty = true;
}

void Camera::SetUpward(const Vector3D& up)
{
	m_Upward = up;
	m_IsDirty = true;
}

void Camera::SetFovY(const float& fovY)
{
	m_FovY = fovY;
	m_IsDirty = true;
}

void Camera::SetHeight(const float& height)
{
	if (height == 0)
		return;
	m_Height = height;
	m_IsDirty = true;
}

void Camera::SetWidth(const float& width)
{
	if (width == 0)
		return;
	m_Width = width;
	m_IsDirty = true;
}

void Camera::SetFar(const float& f)
{
	m_Far = f;
	m_IsDirty = true;
}

void Camera::SetNear(const float& n)
{
	m_Near = n;
	m_IsDirty = true;
}

void Camera::SetProjectionMode(const int& mode)
{
	if (mode < 0 && 1 < mode)
		return;

	m_ProjMove = static_cast<ProjectionMode>(mode);
	m_IsDirty = true;
}

const Matrix4x4& Camera::GetView()
{
	if (m_IsDirty)
		Update();

	return m_View;
}

const Matrix4x4& Camera::GetProj()
{
	if (m_IsDirty)
		Update();

	return m_Proj;
}

const Matrix4x4& Camera::GetViewProj()
{
	if (m_IsDirty)
		Update();

	return m_View * m_Proj;
}

const Matrix4x4& Camera::GetViewInv()
{
	if (m_IsDirty)
		Update();

	return m_ViewInv;
}

