#pragma once
#include "pch.h"
#include "Math/Matrix4x4.h"
#include "Math/Quaternion.h"
#include "Math/MathUtility.h"
class Camera
{
public:
	enum ProjectionMode
	{
		Perspective,
		Photographic, //!< 正射影法
	};

	Camera(uint32_t width, uint32_t height);
	~Camera();
	void Update();
	void RotateOriginTarget(const Quaternion& qRot);
	void Move(const Vector3D& move);
	void MoveTarget(const Vector3D& move);
	void MovePosition(const Vector3D& move);
	void MoveForward(float moveForward);
	void MoveRight(float moveRight);
	void MoveUp(float moveUp);
	void Reset();

	void SetPosition(const Vector3D& pos);
	void SetPosition(float x, float y, float z);
	void SetPositionAndTarget(const Vector3D& pos, const Vector3D& target);
	void SetUpward(const Vector3D& up);
	void SetFovY(const float& fovY);
	void SetHeight(const float& height);
	void SetWidth(const float& width);
	void SetFar(const float& f);
	void SetNear(const float& n);
	void SetProjectionMode(const int& mode);

	const Vector3D& GetPosition() const { return m_Position; }
	const Vector3D& GetTarget() const { return m_Target; }
	const Vector3D& GetForward() const { return m_Forward; }
	const Vector3D& GetRight() const { return m_Right; }
	const float& GetFar() const { return m_Far; }
	const float& GetNear() const { return m_Near; }
	const float& GetFovY() const { return m_FovY; }
	const float& GetAspect() const { return m_Aspect; }
	const Matrix4x4& GetView();
	const Matrix4x4& GetProj();
	const Matrix4x4& GetViewProj();
	const Matrix4x4& GetViewInv();

private:
	Vector3D m_Position = Vector3D(0.0f, 5.0f, -10.0f); //!< カメラ位置
	Vector3D m_Target = Vector3D(0.0f, 0.0f, 0.0f); //!< カメラの注視点
	Vector3D m_Upward = Vector3D(0.0f, 1.0f, 0.0f); //!< カメラの上方向
	Matrix4x4 m_View = Matrix4x4(); //!< ビュー行列
	Matrix4x4 m_ViewInv = Matrix4x4(); //!< ビュー行列の逆行列
	Matrix4x4 m_Proj = Matrix4x4();//!< プロジェクション行列
	Matrix4x4 m_cameraRotation = Matrix4x4(); //!< カメラの回転行列
	Vector3D m_Forward = Vector3D(0.0f, 0.0f, 1.0f); //!< カメラの前方方向
	Vector3D m_Right = Vector3D(1.0f, 0.0f, 0.0f); //!< カメラの右方向
	float m_targetToPositionLen = 1.0f;	//注視点と視点まで距離

	float m_Near = 1.0f;
	float m_Far = 10000.0f;
	float m_FovY = MathUtility::DegreeToRadian(60.0f);
	float m_Aspect = 1.0f;
	float m_Width = 1.0f;
	float m_Height = 1.0f;
	ProjectionMode m_ProjMove = ProjectionMode::Perspective;
	bool m_IsDirty = false;
};