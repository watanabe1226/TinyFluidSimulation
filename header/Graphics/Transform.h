#pragma once
#include "Math/Matrix4x4.h"
#include "Math/Quaternion.h"

struct alignas(256) TransformBuffer
{
public:
	Matrix4x4 World = Matrix4x4::Identity(); // ワールド変換行列
	Matrix4x4 View = Matrix4x4::Identity();  // ビュー変換行列
	Matrix4x4 Proj = Matrix4x4::Identity();  // プロジェクション変換行列

	// メモリ確保時に256バイト境界に合わせるカスタムnew
	void* operator new(size_t size)
	{
		return _aligned_malloc(size, 256);
	}

	// 配列版 new[]
	void* operator new[](size_t size)
	{
		return _aligned_malloc(size, 256);
	}

	// メモリ解放用のカスタムdelete
	void operator delete(void* ptr)
	{
		_aligned_free(ptr);
	}

	// 配列版 delete[]
	void operator delete[](void* ptr)
	{
		_aligned_free(ptr);
	}
};

class Transform
{
public:
	Transform();
	~Transform();

	void SetPosition(float x, float y, float z);
	void SetPosition(const Vector3D& pos);
	void SetRotation(float x, float y, float z);
	void SetRotation(const Vector3D& rot);
	void SetScale(float x, float y, float z);
	void SetScale(const Vector3D& scale);

	void AddTranslation(float x, float y, float z);
	void AddTranslation(const Vector3D& trans);

	const Vector3D& GetPosition() const;
	const Vector3D& GetRotation() const;
	const Quaternion& GetQuaternion() const;
	const Vector3D& GetScale() const;
	const Vector3D& GetForward() const;
	const Vector3D& GetRight() const;
	const Vector3D& GetUp() const;

	const Matrix4x4& GetWorld() const;
	const Matrix4x4& GetView() const;

private:
	void Update();

	Matrix4x4 m_World;
	Matrix4x4 m_View;
	Vector3D m_Position = Vector3D();
	Vector3D m_Rotation = Vector3D(); // degree 度
	Quaternion m_Quaternion = Quaternion();
	Vector3D m_Scale = Vector3D(1.0f);

	Vector3D m_Forward = Vector3D(0, 0, 1);
	Vector3D m_Right = Vector3D(1, 0, 0);
	Vector3D m_Up = Vector3D(0, 1, 0);

};