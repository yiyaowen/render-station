/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include "utils/geometry-utils.h"
#include "modifier.h"

class WaveSimulator : public Modifier {
public:
	WaveSimulator(D3DCore* pCore,
		RenderItem* ritem, // Initial target render item.
		float d, // The distance between two adjacent vertices in the grid.
		UINT m, // Half horizontal node count. See geometry-utils.cpp for generateGrid(xxx).
		UINT n, // Half vertical node count. See geometry-utils.cpp for generateGrid(xxx).
		ObjectGeometry* grid,
		float c, // The spread velocity of wave.
		float u, // The damping grade with velocity dimension.
		float hmin, // The mi ndisturbance height.
		float hmax, // The max disturbance height.
		float t, // The interval seconds between 2 random disturbance.
		bool optimized); // TRUE to enable GPU CS optimization. FALSE to use CPU general computation.

	// Inovke Update() every frame to simulate a wave animation.
	void update() override;

private:
	bool _optimized = true;

	void updateWithCPUGeneralCompute();

	// GPU optimization.

	void initComputeShaderResources();

	void createRootSignatureAndCpso();

	void createGridNodeTextures();

	void createResourceHeap();

	void createResourceDescriptors();

	void updateWithComputeShaderOptimized();

	std::unique_ptr<Shader> s_disturbWave = nullptr;
	std::unique_ptr<Shader> s_calcDisplacement = nullptr;
	std::unique_ptr<Shader> s_calcNormal = nullptr;

	ComPtr<ID3D12DescriptorHeap> descHeap = nullptr;

	ComPtr<ID3D12Resource> prevResource = nullptr, currResource = nullptr, nextResource = nullptr;
	ComPtr<ID3D12Resource> displacementMap = nullptr, normalMap = nullptr;

	D3D12_CPU_DESCRIPTOR_HANDLE prevUav_CPU = {}, currUav_CPU = {}, nextUav_CPU = {};
	D3D12_GPU_DESCRIPTOR_HANDLE prevUav_GPU = {}, currUav_GPU = {}, nextUav_GPU = {};
	D3D12_CPU_DESCRIPTOR_HANDLE displacementSrv_CPU = {}, displacementUav_CPU = {},
		normalSrv_CPU = {}, normalUav_CPU = {};
	D3D12_GPU_DESCRIPTOR_HANDLE displacementSrv_GPU = {}, displacementUav_GPU = {},
		normalSrv_GPU = {}, normalUav_GPU = {};

private:
	D3DCore* pCore = nullptr;

	RenderItem* ritem = nullptr;

	UINT _m = 0, _n = 0, _M = 0, _N = 0;

	float _d = 0.0f; // The distance between two adjacent vertices in the grid.

	float _c = 0.0f; // The spread velocity of wave.

	float _u = 0.0f; // The damping grade with velocity dimension.

	float _hmin = 0.0f; // The min disturbance height.

	float _hmax = 0.0f; // The max disturbance height.

	float _maxt = 0.0f; // Maximum update interval seconds.

	float _t = 0.0f; // Actual update interval seconds. It is acceptable to set _t = _maxt / 2.

	void updateConstraints();

	float _a1 = 0.0f, _a2 = 0.0f, _a3 = 0.0f; // Wave equation's coefficients.

	// Intermidiate variables.
	float lastUpdateTime = 0.0f;

	float lastDisturbTime = 0.0f;
	float _disturbCD = 0.0f;

	// ObjectGeometry* _geo = nullptr // Already initialized in Modifier.
	std::unique_ptr<ObjectGeometry> _prevGeo = nullptr;

public:
	inline float spreadVelocity() { return _c; }
	inline void setSpreadVelocity(float value) { _c = value; updateConstraints(); }

	inline float dampingGrade() { return _u; }
	inline void setDampingGrade(float value) { _u = value; updateConstraints(); }

	inline float disturbCD() { return _disturbCD; }
	inline void setDisturbCD(float value) { _disturbCD = value; }
};