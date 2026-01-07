#pragma once
#include "Scene.h"
#include <memory>
#include <d3d11.h>
#include <wrl.h>
#include <directxmath.h>

#include "sprite.h"
#include "sprite_batch.h"
#include "geometric_primitive.h"
#include "static_mesh.h"
#include "skinned_mesh.h"
#include "framebuffer.h"
#include "fullscreen_quad.h"

class GameScene : public Scene
{
public:
	GameScene() = default;
	~GameScene() override = default;

	void initialize(framework* fw) override;
	void update(framework* fw, float elapsed_time) override;
	void render(framework* fw, float elapsed_time) override;
	void finalize(framework* fw) override;

private:
	struct scene_constants
	{
		DirectX::XMFLOAT4X4 view_projection;
		DirectX::XMFLOAT4 light_direction;
		DirectX::XMFLOAT4 camera_position;
	};

	struct parametric_constants
	{
		float extraction_threshold{ 0.8f };
		float gaussian_sigma{ 1.0f };
		float bloom_intensity{ 1.0f };
		float exposure{ 1.0f };
	};
	parametric_constants parametric_constants;

	Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffers[8];

	DirectX::XMFLOAT4 camera_position{ 0.0f, 0.0f, -10.0f, 1.0f };
	DirectX::XMFLOAT4 light_direction{ 0.0f, 0.0f, -1.0f, 0.0f };

	DirectX::XMFLOAT3 translation{ 0, 0, 0 };
	DirectX::XMFLOAT3 scaling{ 1, 1, 1 };
	DirectX::XMFLOAT3 rotation{ 0, 0, 0 };
	DirectX::XMFLOAT4 material_color{ 1 ,1, 1, 1 };

	std::unique_ptr<sprite> sprites[8];
	std::unique_ptr<sprite_batch> sprite_batches[8];
	std::unique_ptr<static_mesh> static_meshes[8];
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shaders[8];
	std::unique_ptr<skinned_mesh> skinned_meshes[8];

	float factors[4]{ 0.0f, 121.438332f };

	std::unique_ptr<framebuffer> framebuffers[8];
	std::unique_ptr<fullscreen_quad> bit_block_transfer;
};
