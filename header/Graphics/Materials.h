#pragma once
#include "pch.h"
#include "Math/Vector3D.h"

// 読み取り用マテリアル構造体
struct Material
{
	Vector3D m_Diffuse; // ディフューズ色
	Vector3D m_Specular; // スペキュラー色
	float m_Alpha = 1.0f;    // アルファ値
	float m_Shininess; // 鏡面反射強度
	TextureID m_DiffuseTexId; // ディフューズテクスチャパスID
	TextureID m_NormalTexId; // ノーマルテクスチャパスID
	TextureID m_GLTFMetaricRoughnessTexId; // GLTFのメタリックラフネステクスチャパスID
	TextureID m_ShininessTexId; // シャイネステクスチャパスID
	TextureID m_SpecularTexId; // スペキュラテクスチャパスID
};

// GPU側に送るマテリアル構造体
struct alignas(256) MaterialBuffer
{
	Vector3D Difuuse;	//!< 基本色
	float Alpha;		//!< 透過成分
	Vector3D Specular;	//!< 鏡面反射
	float Shininess;	//!< 鏡面反射強度
};