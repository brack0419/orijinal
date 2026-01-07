#pragma once
#include <map>
#include <string>
#include <memory>
#include <d3d11.h>
#include "skinned_mesh.h"

class ResourceManager
{
public:
	ResourceManager(ID3D11Device* device) : device(device) {}
	~ResourceManager() = default;

	// スキンメッシュの読み込み（重複ロード防止機能付き）
	std::shared_ptr<skinned_mesh> load_skinned_mesh(const std::string& filename)
	{
		// すでに読み込み済みかチェック
		auto it = skinned_mesh_cache.find(filename);
		if (it != skinned_mesh_cache.end())
		{
			// 読み込み済みなら、保存しておいたポインタを返す
			return it->second;
		}

		// まだなら新しく読み込んで、キャッシュに保存する
		auto mesh = std::make_shared<skinned_mesh>(device, filename.c_str());
		skinned_mesh_cache[filename] = mesh;

		return mesh;
	}

private:
	ID3D11Device* device;
	// ファイル名をキーにしてデータを保存する辞書
	std::map<std::string, std::shared_ptr<skinned_mesh>> skinned_mesh_cache;
};
