#pragma once
#include "pch.h"
#include "Vector3D.h"
#include "Vector4D.h"
#include "Quaternion.h"

class Matrix4x4
{
public:
    // コンストラクタ
    Matrix4x4() noexcept
    {
        setIdentity();
    }

    void setIdentity() noexcept
    {
        ::memset(m_mat, 0, sizeof(float) * 16);
        m_mat[0][0] = 1.f;
        m_mat[1][1] = 1.f;
        m_mat[2][2] = 1.f;
        m_mat[3][3] = 1.f;
    }

    static Matrix4x4 Identity() noexcept
    {
        Matrix4x4 mat;
        mat.setIdentity();
        return mat;
    }

    //// =======================
    //// FbxAMatrix 変換
    //// =======================
    //static Matrix4x4 ConvertMatrix(const FbxAMatrix& fbmat) noexcept
    //{
    //    Matrix4x4 mat;
    //    for (int row = 0; row < 4; ++row)
    //    {
    //        for (int col = 0; col < 4; ++col)
    //            mat.m_mat[row][col] = static_cast<float>(fbmat.Get(row, col));
    //    }
    //    return mat;
    //}

    // =======================
    // 回転行列設定
    // =======================
    void setRotationX(float x) noexcept
    {
        float c = std::cos(x);
        float s = std::sin(x);
        m_mat[1][1] = c;  m_mat[1][2] = s;
        m_mat[2][1] = -s; m_mat[2][2] = c;
    }

    void setRotationY(float y) noexcept
    {
        float c = std::cos(y);
        float s = std::sin(y);
        m_mat[0][0] = c;  m_mat[0][2] = -s;
        m_mat[2][0] = s;  m_mat[2][2] = c;
    }

    void setRotationZ(float z) noexcept
    {
        float c = std::cos(z);
        float s = std::sin(z);
        m_mat[0][0] = c;  m_mat[0][1] = s;
        m_mat[1][0] = -s; m_mat[1][1] = c;
    }

    void operator*=(const Matrix4x4& mat) noexcept
    {
        *this = *this * mat;
    }

    Matrix4x4 operator*(const Matrix4x4& mat) const noexcept
    {
        Matrix4x4 out;
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                out.m_mat[i][j] =
                    m_mat[i][0] * mat.m_mat[0][j] +
                    m_mat[i][1] * mat.m_mat[1][j] +
                    m_mat[i][2] * mat.m_mat[2][j] +
                    m_mat[i][3] * mat.m_mat[3][j];
            }
        }
        return out;
    }

    void setMatrix(const Matrix4x4& mat) noexcept
    {
        ::memcpy(m_mat, mat.m_mat, sizeof(float) * 16);
    }

    // =======================
    // 行列からベクトル変換
    // =======================
    static Vector3D Apply(const Matrix4x4& matrix, const Vector3D& vec) noexcept
    {
        float x = vec.x * matrix.m_mat[0][0] + vec.y * matrix.m_mat[1][0] + vec.z * matrix.m_mat[2][0] + matrix.m_mat[3][0];
        float y = vec.x * matrix.m_mat[0][1] + vec.y * matrix.m_mat[1][1] + vec.z * matrix.m_mat[2][1] + matrix.m_mat[3][1];
        float z = vec.x * matrix.m_mat[0][2] + vec.y * matrix.m_mat[1][2] + vec.z * matrix.m_mat[2][2] + matrix.m_mat[3][2];
        float w = vec.x * matrix.m_mat[0][3] + vec.y * matrix.m_mat[1][3] + vec.z * matrix.m_mat[2][3] + matrix.m_mat[3][3];

        // wで割る処理が必要な場合（射影変換後など）はここで行う
        // if(w != 1.0f && w != 0.0f) { x /= w; y /= w; z /= w; }

        return Vector3D(x, y, z);
    }

    // =======================
    // ビュー・投影・変換行列
    // =======================
    static Matrix4x4 setLookAtLH(const Vector3D& pos, const Vector3D& target, const Vector3D& up) noexcept
    {
        Matrix4x4 mat;

        Vector3D zaxis = (target - pos).GetSafeNormal();
        Vector3D xaxis = up.cross(zaxis).GetSafeNormal();
        Vector3D yaxis = zaxis.cross(xaxis);

        mat.m_mat[0][0] = xaxis.x; mat.m_mat[0][1] = yaxis.x; mat.m_mat[0][2] = zaxis.x; mat.m_mat[0][3] = 0.f;
        mat.m_mat[1][0] = xaxis.y; mat.m_mat[1][1] = yaxis.y; mat.m_mat[1][2] = zaxis.y; mat.m_mat[1][3] = 0.f;
        mat.m_mat[2][0] = xaxis.z; mat.m_mat[2][1] = yaxis.z; mat.m_mat[2][2] = zaxis.z; mat.m_mat[2][3] = 0.f;
        mat.m_mat[3][0] = -xaxis.dot(pos);
        mat.m_mat[3][1] = -yaxis.dot(pos);
        mat.m_mat[3][2] = -zaxis.dot(pos);
        mat.m_mat[3][3] = 1.f;

        return mat;
    }

    static Matrix4x4 setPerspectiveFovLH(float fov, float aspect, float znear, float zfar) noexcept
    {
        Matrix4x4 mat;
        float yscale = 1.f / std::tan(fov * 0.5f);
        float xscale = yscale / aspect;

        mat.m_mat[0][0] = xscale; mat.m_mat[0][1] = 0.f;     mat.m_mat[0][2] = 0.f;                mat.m_mat[0][3] = 0.f;
        mat.m_mat[1][0] = 0.f;    mat.m_mat[1][1] = yscale;  mat.m_mat[1][2] = 0.f;                mat.m_mat[1][3] = 0.f;
        mat.m_mat[2][0] = 0.f;    mat.m_mat[2][1] = 0.f;     mat.m_mat[2][2] = zfar / (zfar - znear); mat.m_mat[2][3] = 1.f;
        mat.m_mat[3][0] = 0.f;    mat.m_mat[3][1] = 0.f;     mat.m_mat[3][2] = -znear * zfar / (zfar - znear); mat.m_mat[3][3] = 0.f;

        return mat;
    }

    static Matrix4x4 setOrthoLH(float width, float height, float near_plane, float far_plane) noexcept
    {
        Matrix4x4 mat;
        float fRange = 1.f / (far_plane - near_plane);

        mat.m_mat[0][0] = 2.f / width; mat.m_mat[0][1] = 0.f;         mat.m_mat[0][2] = 0.f;        mat.m_mat[0][3] = 0.f;
        mat.m_mat[1][0] = 0.f;         mat.m_mat[1][1] = 2.f / height; mat.m_mat[1][2] = 0.f;        mat.m_mat[1][3] = 0.f;
        mat.m_mat[2][0] = 0.f;         mat.m_mat[2][1] = 0.f;         mat.m_mat[2][2] = fRange;    mat.m_mat[2][3] = 0.f;
        mat.m_mat[3][0] = 0.f;         mat.m_mat[3][1] = 0.f;         mat.m_mat[3][2] = -fRange * near_plane; mat.m_mat[3][3] = 1.f;

        return mat;
    }

    static Matrix4x4 setOrthoOffsetLH(float left, float right, float bottom, float top, float nearZ, float farZ) noexcept
    {
        Matrix4x4 mat;
        float rWidth = 1.f / (right - left);
        float rHeight = 1.f / (top - bottom);
        float fRange = 1.f / (farZ - nearZ);

        mat.m_mat[0][0] = 2.f * rWidth; mat.m_mat[0][1] = 0.f;       mat.m_mat[0][2] = 0.f;    mat.m_mat[0][3] = 0.f;
        mat.m_mat[1][0] = 0.f;          mat.m_mat[1][1] = 2.f * rHeight; mat.m_mat[1][2] = 0.f;    mat.m_mat[1][3] = 0.f;
        mat.m_mat[2][0] = 0.f;          mat.m_mat[2][1] = 0.f;       mat.m_mat[2][2] = fRange; mat.m_mat[2][3] = 0.f;
        mat.m_mat[3][0] = -(left + right) * rWidth;
        mat.m_mat[3][1] = -(top + bottom) * rHeight;
        mat.m_mat[3][2] = -nearZ * fRange;
        mat.m_mat[3][3] = 1.f;

        return mat;
    }

    static Matrix4x4 inverse(const Matrix4x4& matrix)
    {
        uint32_t a, i, j;
        Matrix4x4 out;
        Vector4D v, vec[3];
        float det = 1.0f;

        det = Matrix4x4::getDeterminant(matrix);
        if (det == 0) return out;
        for (i = 0; i < 4; i++)
        {
            for (j = 0; j < 4; j++)
            {
                if (j != i)
                {
                    a = j;
                    if (j > i) a = a - 1;
                    vec[a].x = (matrix.m_mat[j][0]);
                    vec[a].y = (matrix.m_mat[j][1]);
                    vec[a].z = (matrix.m_mat[j][2]);
                    vec[a].w = (matrix.m_mat[j][3]);
                }
            }
            v = vec[0].cross(vec[1], vec[2]);

            out.m_mat[0][i] = (float)pow(-1.0f, i) * v.x / det;
            out.m_mat[1][i] = (float)pow(-1.0f, i) * v.y / det;
            out.m_mat[2][i] = (float)pow(-1.0f, i) * v.z / det;
            out.m_mat[3][i] = (float)pow(-1.0f, i) * v.w / det;
        }

        return out;
    }

    static float getDeterminant(const Matrix4x4& matrix)
    {
        Vector4D mirror, v1, v2, v3;
        float det;

        v1 = Vector4D(matrix.m_mat[0][0], matrix.m_mat[1][0], matrix.m_mat[2][0], matrix.m_mat[3][0]);
        v2 = Vector4D(matrix.m_mat[0][1], matrix.m_mat[1][1], matrix.m_mat[2][1], matrix.m_mat[3][1]);
        v3 = Vector4D(matrix.m_mat[0][2], matrix.m_mat[1][2], matrix.m_mat[2][2], matrix.m_mat[3][2]);


        mirror = v1.cross(v2, v3);
        det = -(matrix.m_mat[0][3] * mirror.x + matrix.m_mat[1][3] * mirror.y + matrix.m_mat[2][3] * mirror.z +
            matrix.m_mat[3][3] * mirror.w);

        return det;
    }
    // クォータニオンから回転行列
    static Matrix4x4 QuaternionToMatrix(const Quaternion& q) noexcept
    {
        Matrix4x4 mat;
        float xx = q.x * q.x * 2.f;
        float yy = q.y * q.y * 2.f;
        float zz = q.z * q.z * 2.f;
        float xy = q.x * q.y * 2.f;
        float xz = q.x * q.z * 2.f;
        float yz = q.y * q.z * 2.f;
        float wx = q.w * q.x * 2.f;
        float wy = q.w * q.y * 2.f;
        float wz = q.w * q.z * 2.f;

        mat.m_mat[0][0] = 1.f - yy - zz;
        mat.m_mat[0][1] = xy + wz;
        mat.m_mat[0][2] = xz - wy;
        mat.m_mat[0][3] = 0.f;

        mat.m_mat[1][0] = xy - wz;
        mat.m_mat[1][1] = 1.f - xx - zz;
        mat.m_mat[1][2] = yz + wx;
        mat.m_mat[1][3] = 0.f;

        mat.m_mat[2][0] = xz + wy;
        mat.m_mat[2][1] = yz - wx;
        mat.m_mat[2][2] = 1.f - xx - yy;
        mat.m_mat[2][3] = 0.f;

        mat.m_mat[3][0] = 0.f;
        mat.m_mat[3][1] = 0.f;
        mat.m_mat[3][2] = 0.f;
        mat.m_mat[3][3] = 1.f;

        return mat;
    }

    // 平行移動行列
    static Matrix4x4 TransitionToMatrix(const Vector3D& trans) noexcept
    {
        Matrix4x4 mat;
        mat.setIdentity();
        mat.m_mat[3][0] = trans.x;
        mat.m_mat[3][1] = trans.y;
        mat.m_mat[3][2] = trans.z;
        return mat;
    }

    // =======================
    // Vector3D(オイラー角 degree)から回転行列
    // ※回転順序は Z(Roll) -> X(Pitch) -> Y(Yaw) としています
    // =======================
    static Matrix4x4 RotationToMatrix(const Vector3D& degree) noexcept
    {
        // Degree -> Radian 変換
        float radX = degree.x * MathUtility::DEG_TO_RAD;
        float radY = degree.y * MathUtility::DEG_TO_RAD;
        float radZ = degree.z * MathUtility::DEG_TO_RAD;

        Matrix4x4 rx, ry, rz;
        // setRotationX等は std::cos/sin を使うため Radian を渡す
        rx.setRotationX(radX);
        ry.setRotationY(radY);
        rz.setRotationZ(radZ);

        // 合成順序: Z(Roll) -> X(Pitch) -> Y(Yaw)
        return rz * rx * ry;
    }

    // スケーリング行列
    static Matrix4x4 ScalingToMatrix(const Vector3D& scale) noexcept
    {
        Matrix4x4 mat;
        mat.setIdentity();
        mat.m_mat[0][0] = scale.x;
        mat.m_mat[1][1] = scale.y;
        mat.m_mat[2][2] = scale.z;
        return mat;
    }

    void setTranslation(const Vector3D& trans) noexcept
    {
        m_mat[3][0] = trans.x;
        m_mat[3][1] = trans.y;
        m_mat[3][2] = trans.z;
    }

    void setScale(const Vector3D& trans) noexcept
    {
        m_mat[0][0] = trans.x;
        m_mat[1][1] = trans.y;
        m_mat[2][2] = trans.z;
    }

public:
    float m_mat[4][4] = {};
};
