#pragma once
#include <d3d11.h>
#include <directxmath.h>
#include <memory>
#include "skinned_mesh.h"

class GameObject
{
public:
	// 座標変換データ
	DirectX::XMFLOAT3 position = { 0, 0, 0 };
	DirectX::XMFLOAT3 rotation = { 0, 0, 0 }; // x:Pitch, y:Yaw, z:Roll
	DirectX::XMFLOAT3 scale = { 1, 1, 1 };

	// 描画データ
	std::shared_ptr<skinned_mesh> mesh;
	DirectX::XMFLOAT4 color = { 1, 1, 1, 1 };

	// アニメーション制御用
	float animation_tick = 0.0f;
	float animation_speed = 1.0f;

	// コンストラクタ
	GameObject(std::shared_ptr<skinned_mesh> m) : mesh(m) {}
	virtual ~GameObject() = default;

	// 更新処理
	virtual void update(float elapsed_time)
	{
		// アニメーション時間を進める
		animation_tick += elapsed_time * animation_speed;
	}

	// 描画処理
	virtual void render(ID3D11DeviceContext* context)
	{
		if (!mesh) return;

		// ワールド行列の作成 (Scale -> Rotate -> Translate)
		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);

		// 座標系の補正（元コードにあった処理）
		const DirectX::XMFLOAT4X4 coordinate_system_transforms
		{
			-1, 0, 0, 0,
			 0, 1, 0, 0,
			 0, 0, 1, 0,
			 0, 0, 0, 1
		};
		const float scale_factor = 0.01f;
		DirectX::XMMATRIX C = DirectX::XMLoadFloat4x4(&coordinate_system_transforms) * DirectX::XMMatrixScaling(scale_factor, scale_factor, scale_factor);

		// 最終的なワールド行列
		DirectX::XMFLOAT4X4 world;
		DirectX::XMStoreFloat4x4(&world, C * S * R * T);

		// アニメーション再生
		if (!mesh->animation_clips.empty())
		{
			// とりあえず0番目のクリップを再生
			const auto& animation = mesh->animation_clips.at(0);
			int frame_index = static_cast<int>(animation_tick * animation.sampling_rate);

			// ループ処理
			if (frame_index > animation.sequence.size() - 1)
			{
				frame_index = 0;
				animation_tick = 0;
			}

			const auto& keyframe = animation.sequence.at(frame_index);
			mesh->render(context, world, color, &keyframe);
		}
		else
		{
			// アニメーションなし
			mesh->render(context, world, color, nullptr);
		}
	}
};
