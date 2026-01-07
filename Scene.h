#pragma once
#include <d3d11.h>

class framework;

class Scene
{
public:
	virtual ~Scene() = default;
	virtual void initialize(framework* fw) = 0;
	virtual void update(framework* fw, float elapsed_time) = 0;
	virtual void render(framework* fw, float elapsed_time) = 0;
	virtual void finalize(framework* fw) = 0;
};
