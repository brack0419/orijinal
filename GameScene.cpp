#include "GameScene.h"
#include "framework.h"
#include "shader.h"
#include "texture.h"
#include "misc.h"

#ifdef USE_IMGUI
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
#endif

using namespace DirectX;

void GameScene::initialize(framework* fw)
{
	HRESULT hr{ S_OK };
	ID3D11Device* device = fw->device.Get();

	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(scene_constants);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = 0;
	buffer_desc.MiscFlags = 0;
	buffer_desc.StructureByteStride = 0;
	hr = device->CreateBuffer(&buffer_desc, nullptr, constant_buffers[0].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	buffer_desc.ByteWidth = sizeof(parametric_constants);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = 0;
	buffer_desc.MiscFlags = 0;
	buffer_desc.StructureByteStride = 0;
	hr = device->CreateBuffer(&buffer_desc, nullptr, constant_buffers[1].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	// リソースの読み込み
	sprite_batches[0] = std::make_unique<sprite_batch>(device, L".\\resources\\screenshot.jpg", 1);
	skinned_meshes[0] = std::make_unique<skinned_mesh>(device, ".\\resources\\anis.fbx");

	framebuffers[0] = std::make_unique<framebuffer>(device, 1280, 720);
	framebuffers[1] = std::make_unique<framebuffer>(device, 1280 / 2, 720 / 2);
	bit_block_transfer = std::make_unique<fullscreen_quad>(device);

	create_ps_from_cso(device, "luminance_extraction_ps.cso", pixel_shaders[0].GetAddressOf());
	create_ps_from_cso(device, "blur_ps.cso", pixel_shaders[1].GetAddressOf());
}

void GameScene::update(framework* fw, float elapsed_time)
{
#ifdef USE_IMGUI
	ImGui::Begin("ImGUI");

	ImGui::SliderFloat("camera_position.x", &camera_position.x, -100.0f, +100.0f);
	ImGui::SliderFloat("camera_position.y", &camera_position.y, -100.0f, +100.0f);
	ImGui::SliderFloat("camera_position.z", &camera_position.z, -100.0f, -1.0f);

	ImGui::SliderFloat("light_direction.x", &light_direction.x, -1.0f, +1.0f);
	ImGui::SliderFloat("light_direction.y", &light_direction.y, -1.0f, +1.0f);
	ImGui::SliderFloat("light_direction.z", &light_direction.z, -1.0f, +1.0f);

	ImGui::SliderFloat("translation.x", &translation.x, -10.0f, +10.0f);
	ImGui::SliderFloat("translation.y", &translation.y, -10.0f, +10.0f);
	ImGui::SliderFloat("translation.z", &translation.z, -10.0f, +10.0f);

	ImGui::SliderFloat("scaling.x", &scaling.x, -10.0f, +10.0f);
	ImGui::SliderFloat("scaling.y", &scaling.y, -10.0f, +10.0f);
	ImGui::SliderFloat("scaling.z", &scaling.z, -10.0f, +10.0f);

	ImGui::SliderFloat("rotation.x", &rotation.x, -10.0f, +10.0f);
	ImGui::SliderFloat("rotation.y", &rotation.y, -10.0f, +10.0f);
	ImGui::SliderFloat("rotation.z", &rotation.z, -10.0f, +10.0f);

	ImGui::ColorEdit4("material_color", reinterpret_cast<float*>(&material_color));

	ImGui::SliderFloat("factors[0]", &factors[0], -1.5f, +1.5f);
	ImGui::SliderFloat("factors[1]", &factors[1], +0.0f, +500.0f);
	ImGui::SliderFloat("factors[2]", &factors[2], +0.0f, +1.0f);

	ImGui::SliderFloat("extraction_threshold", &parametric_constants.extraction_threshold, +0.0f, +5.0f);
	ImGui::SliderFloat("gaussian_sigma", &parametric_constants.gaussian_sigma, +0.0f, +10.0f);
	ImGui::SliderFloat("bloom_intensity", &parametric_constants.bloom_intensity, +0.0f, +10.0f);
	ImGui::SliderFloat("exposure", &parametric_constants.exposure, +0.0f, +10.0f);

	ImGui::End();
#endif
}

void GameScene::render(framework* fw, float elapsed_time)
{
	ID3D11DeviceContext* context = fw->immediate_context.Get();

	ID3D11RenderTargetView* null_render_target_views[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
	context->OMSetRenderTargets(_countof(null_render_target_views), null_render_target_views, 0);
	ID3D11ShaderResourceView* null_shader_resource_views[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT]{};
	context->VSSetShaderResources(0, _countof(null_shader_resource_views), null_shader_resource_views);
	context->PSSetShaderResources(0, _countof(null_shader_resource_views), null_shader_resource_views);

	FLOAT color[]{ 0.2f, 0.2f, 0.2f, 1.0f };
	context->ClearRenderTargetView(fw->render_target_view.Get(), color);
	context->OMSetRenderTargets(1, fw->render_target_view.GetAddressOf(), fw->depth_stencil_view.Get());

	context->PSSetSamplers(0, 1, fw->sampler_states[static_cast<size_t>(framework::SAMPLER_STATE::POINT)].GetAddressOf());
	context->PSSetSamplers(1, 1, fw->sampler_states[static_cast<size_t>(framework::SAMPLER_STATE::LINEAR)].GetAddressOf());
	context->PSSetSamplers(2, 1, fw->sampler_states[static_cast<size_t>(framework::SAMPLER_STATE::ANISOTROPIC)].GetAddressOf());
	context->PSSetSamplers(3, 1, fw->sampler_states[static_cast<size_t>(framework::SAMPLER_STATE::LINEAR_BORDER_BLACK)].GetAddressOf());
	context->PSSetSamplers(4, 1, fw->sampler_states[static_cast<size_t>(framework::SAMPLER_STATE::LINEAR_BORDER_WHITE)].GetAddressOf());

	context->OMSetBlendState(fw->blend_states[static_cast<size_t>(framework::BLEND_STATE::ALPHA)].Get(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(fw->depth_stencil_states[static_cast<size_t>(framework::DEPTH_STATE::ZT_ON_ZW_ON)].Get(), 0);
	context->RSSetState(fw->rasterizer_states[static_cast<size_t>(framework::RASTER_STATE::SOLID)].Get());

	D3D11_VIEWPORT viewport;
	UINT num_viewports{ 1 };
	context->RSGetViewports(&num_viewports, &viewport);

	float aspect_ratio{ viewport.Width / viewport.Height };
	DirectX::XMMATRIX P{ DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(30), aspect_ratio, 0.1f, 100.0f) };

	DirectX::XMVECTOR eye{ DirectX::XMLoadFloat4(&camera_position) };
	DirectX::XMVECTOR focus{ DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f) };
	DirectX::XMVECTOR up{ DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) };
	DirectX::XMMATRIX V{ DirectX::XMMatrixLookAtLH(eye, focus, up) };

	scene_constants data{};
	DirectX::XMStoreFloat4x4(&data.view_projection, V * P);
	data.light_direction = light_direction;
	data.camera_position = camera_position;
	context->UpdateSubresource(constant_buffers[0].Get(), 0, 0, &data, 0, 0);
	context->VSSetConstantBuffers(1, 1, constant_buffers[0].GetAddressOf());
	context->PSSetConstantBuffers(1, 1, constant_buffers[0].GetAddressOf());

	context->UpdateSubresource(constant_buffers[1].Get(), 0, 0, &parametric_constants, 0, 0);
	context->PSSetConstantBuffers(2, 1, constant_buffers[1].GetAddressOf());

	// Framebuffer pass
	framebuffers[0]->clear(context);
	framebuffers[0]->activate(context);

	context->OMSetDepthStencilState(fw->depth_stencil_states[static_cast<size_t>(framework::DEPTH_STATE::ZT_OFF_ZW_OFF)].Get(), 0);
	context->RSSetState(fw->rasterizer_states[static_cast<size_t>(framework::RASTER_STATE::CULL_NONE)].Get());
	sprite_batches[0]->begin(context);
	sprite_batches[0]->render(context, 0, 0, 1280, 720);
	sprite_batches[0]->end(context);

	context->OMSetDepthStencilState(fw->depth_stencil_states[static_cast<size_t>(framework::DEPTH_STATE::ZT_ON_ZW_ON)].Get(), 0);
	context->RSSetState(fw->rasterizer_states[static_cast<size_t>(framework::RASTER_STATE::SOLID)].Get());

	const DirectX::XMFLOAT4X4 coordinate_system_transforms[]{
		{ -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
		{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
		{ -1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
		{ 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
	};
	const float scale_factor = 0.01f;
	DirectX::XMMATRIX C{ DirectX::XMLoadFloat4x4(&coordinate_system_transforms[0]) * DirectX::XMMatrixScaling(scale_factor, scale_factor, scale_factor) };

	DirectX::XMMATRIX S{ DirectX::XMMatrixScaling(scaling.x, scaling.y, scaling.z) };
	DirectX::XMMATRIX R{ DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) };
	DirectX::XMMATRIX T{ DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z) };
	DirectX::XMFLOAT4X4 world;
	DirectX::XMStoreFloat4x4(&world, C * S * R * T);

	if (skinned_meshes[0])
	{
		int clip_index{ 0 };
		int frame_index{ 0 };
		static float animation_tick{ 0 };

		if (!skinned_meshes[0]->animation_clips.empty())
		{
			animation& animation{ skinned_meshes[0]->animation_clips.at(clip_index) };
			frame_index = static_cast<int>(animation_tick * animation.sampling_rate);
			if (frame_index > animation.sequence.size() - 1)
			{
				frame_index = 0;
				animation_tick = 0;
			}
			else
			{
				animation_tick += elapsed_time;
			}
			animation::keyframe& keyframe{ animation.sequence.at(frame_index) };
			skinned_meshes[0]->render(context, world, material_color, &keyframe);
		}
		else
		{
			skinned_meshes[0]->render(context, world, material_color, nullptr);
		}
	}

	framebuffers[0]->deactivate(context);

	// Post-processing
	framebuffers[1]->clear(context);
	framebuffers[1]->activate(context);
	context->OMSetDepthStencilState(fw->depth_stencil_states[static_cast<size_t>(framework::DEPTH_STATE::ZT_OFF_ZW_OFF)].Get(), 0);
	context->RSSetState(fw->rasterizer_states[static_cast<size_t>(framework::RASTER_STATE::CULL_NONE)].Get());
	bit_block_transfer->blit(context, framebuffers[0]->shader_resource_views[0].GetAddressOf(), 0, 1, pixel_shaders[0].Get());
	framebuffers[1]->deactivate(context);

	context->OMSetDepthStencilState(fw->depth_stencil_states[static_cast<size_t>(framework::DEPTH_STATE::ZT_OFF_ZW_OFF)].Get(), 0);
	context->RSSetState(fw->rasterizer_states[static_cast<size_t>(framework::RASTER_STATE::CULL_NONE)].Get());
	ID3D11ShaderResourceView* shader_resource_views[2]{ framebuffers[0]->shader_resource_views[0].Get(), framebuffers[1]->shader_resource_views[0].Get() };
	bit_block_transfer->blit(context, shader_resource_views, 0, 2, pixel_shaders[1].Get());
}

void GameScene::finalize(framework* fw)
{
	// 必要ならリソース解放処理
}