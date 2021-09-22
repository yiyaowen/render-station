/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include "d3dcore/d3dcore.h"
#include "utils/debugger.h"
#include "utils/vmesh-utils.h"
#include "wave-simulator.h"

WaveSimulator::WaveSimulator(D3DCore* pCore,
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
	bool optimized) // TRUE to enable GPU CS optimization. FALSE to use CPU general computation.
	: Modifier(pCore, ritem->mesh.get(), grid), _optimized(optimized), pCore(pCore),
	ritem(ritem), _m(m), _n(n), _M(2 * m + 1), _N(2 * n + 1),
	_d(d), _c(c), _u(u), _hmin(hmin), _hmax(hmax), _disturbCD(t)
{
	_prevGeo.reset(copyObjectGeometry(grid));

	updateConstraints();

	if (_optimized) initComputeShaderResources();
}

/*                    /|\ y
 *     (m) vertices    |   (m) vertices     (2 * m + 1) vertices
 * @---@---@---@---@---@---@---@---@---@---@
 * |   |   |   |   |   |   |   |   |   |   | (n) vertices
 * @---@---@---@---@---@---@---@---@---@---@
 * |   |   |   |   |   |   |   |   |   |   |   \
 * @---@---@---@---@---@---@---@---@---@---@----  x
 * |   |   |   |   |   |   |   |   |   |   |   /
 * @---@---@---@---@---@---@---@---@---@---@
 * |   |   |   |   |   |   |   |   |   |   | (n) vertices
 * @---@---@---@---@---@---@---@---@---@---@
 * Total: (2*m + 1) * (2*n + 1) vertices    (2 * n + 1) vertices
*/

void WaveSimulator::update() {
	if (!_actived) return;

	if (_optimized) {
		updateWithComputeShaderOptimized();
	}
	else {
		updateWithCPUGeneralCompute();
	}
}

void WaveSimulator::updateWithCPUGeneralCompute() {
	// Wait disturb CD.
	if (pCore->timer->elapsedSecs > lastDisturbTime + _disturbCD) {

		lastDisturbTime = pCore->timer->elapsedSecs;

		UINT x = randint(1, _M - 2);
		UINT y = randint(1, _N - 2);
		float h = randfloat(_hmin, _hmax);
		float halfH = h * 0.5f;
		_geo->vertices[x + y * _M].pos.y = h;
		_geo->vertices[x - 1 + y * _M].pos.y = halfH;
		_geo->vertices[x + 1 + y * _M].pos.y = halfH;
		_geo->vertices[x + (y - 1) * _M].pos.y = halfH;
		_geo->vertices[x + (y + 1) * _M].pos.y = halfH;
	}

	// Wait update CD.
	if (pCore->timer->elapsedSecs > lastUpdateTime + _t) {

		lastUpdateTime = pCore->timer->elapsedSecs;

		for (UINT i = 1; i < _M - 1; ++i) {
			for (UINT j = 1; j < _N - 1; ++j) {

				// Write all data into _prevGeo directly and treat it as _nextGeo,
				// since the formula only depends current vertex for current time.
				// Thus we do not need to create an extra _nextGeo.
				auto& y_next = _prevGeo->vertices[i + j * _M].pos.y;
				auto& y_curr = _geo->vertices[i + j * _M].pos.y;
				auto& y_prev = _prevGeo->vertices[i + j * _M].pos.y;

				auto& y_left = _geo->vertices[i - 1 + j * _M].pos.y;
				auto& y_right = _geo->vertices[i + 1 + j * _M].pos.y;
				auto& y_above = _geo->vertices[i + (j - 1) * _M].pos.y;
				auto& y_below = _geo->vertices[i + (j + 1) * _M].pos.y;

				y_next = _a1 * y_curr + _a2 * y_prev + _a3 * (y_left + y_right + y_above + y_below);
			}
		}

		std::swap(_prevGeo, _geo);

		for (UINT i = 1; i < _M - 1; ++i) {
			for (UINT j = 1; j < _N - 1; ++j) {

				auto& y_nor = _geo->vertices[i + j * _M].normal;

				auto& y_left = _geo->vertices[i - 1 + j * _M].pos.y;
				auto& y_right = _geo->vertices[i + 1 + j * _M].pos.y;
				auto& y_above = _geo->vertices[i + (j - 1) * _M].pos.y;
				auto& y_below = _geo->vertices[i + (j + 1) * _M].pos.y;

				// Swap y-component and z-component due to Y-axis is the upward direction in the scene.
				//auto normal = XMVectorSet(y_left - y_right, y_above - y_below, 2 * _d, 0.0f);
				auto normal = XMVectorSet(y_left - y_right, 2 * _d, y_above - y_below, 0.0f);
				XMStoreFloat3(&y_nor, XMVector3Normalize(normal));
			}
		}
	}

	uploadStatedResource(pCore,
		_mesh->vertexBuffGPU.Get(), D3D12_RESOURCE_STATE_GENERIC_READ,
		_mesh->vertexUploadBuff.Get(), D3D12_RESOURCE_STATE_GENERIC_READ,
		_geo->vertices.data(), _geo->vertexDataSize());
}

void WaveSimulator::initComputeShaderResources() {
	createResourceHeap();
	createGridNodeTextures();
	createResourceDescriptors();

	createRootSignatureAndCpso();
}

void WaveSimulator::createRootSignatureAndCpso() {
	CD3DX12_ROOT_PARAMETER slotRootParameter[6];

	slotRootParameter[0].InitAsConstants(9, 0);
	CD3DX12_DESCRIPTOR_RANGE uavTable[5];
	for (int i = 0; i < 5; ++i) {
		uavTable[i].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, i);
		slotRootParameter[i + 1].InitAsDescriptorTable(1, &uavTable[i]);
	}

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(6, slotRootParameter, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_NONE);
	createRootSig(pCore, "wave_simulator", &rootSigDesc);

	// Compile optimization CS shaders.
	ShaderFuncEntryPoints csEntryPoint = {};
	
	csEntryPoint.cs = "DisturbWave";
	s_disturbWave = std::make_unique<Shader>(
		"wave_simulator_disturb_wave", L"shaders/cs-optimization/wave-simulation.hlsl", Shader::CS, csEntryPoint);
	
	csEntryPoint.cs = "CalcDisplacement";
	s_calcDisplacement = std::make_unique<Shader>(
		"wave_simulator_calc_displacement", L"shaders/cs-optimization/wave-simulation.hlsl", Shader::CS, csEntryPoint);
	
	csEntryPoint.cs = "CalcNormal";
	s_calcNormal = std::make_unique<Shader>(
		"wave_simulator_calc_normal", L"shaders/cs-optimization/wave-simulation.hlsl", Shader::CS, csEntryPoint);

	// Create optimization compute shader PSOs.
	D3D12_COMPUTE_PIPELINE_STATE_DESC optimizePsoDesc = {};
	optimizePsoDesc.pRootSignature = pCore->rootSigs["wave_simulator"].Get();
	bindShaderToCPSO(&optimizePsoDesc, s_disturbWave.get());
	optimizePsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	checkHR(pCore->device->CreateComputePipelineState(&optimizePsoDesc, IID_PPV_ARGS(&pCore->PSOs["wave_simulator_disturb_wave"])));

	bindShaderToCPSO(&optimizePsoDesc, s_calcDisplacement.get());
	checkHR(pCore->device->CreateComputePipelineState(&optimizePsoDesc, IID_PPV_ARGS(&pCore->PSOs["wave_simulator_calc_displacement"])));

	bindShaderToCPSO(&optimizePsoDesc, s_calcNormal.get());
	checkHR(pCore->device->CreateComputePipelineState(&optimizePsoDesc, IID_PPV_ARGS(&pCore->PSOs["wave_simulator_calc_normal"])));
}

void WaveSimulator::createGridNodeTextures() {
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = _M;
	texDesc.Height = _N;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	// Only store crest height data.
	texDesc.Format = DXGI_FORMAT_R32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_RESOURCE_DESC crestTexDesc = texDesc;
	crestTexDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	checkHR(pCore->device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&crestTexDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&prevResource)));

	checkHR(pCore->device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&crestTexDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&currResource)));

	checkHR(pCore->device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&crestTexDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&nextResource)));

	D3D12_RESOURCE_DESC mapTexDesc = texDesc;
	mapTexDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	mapTexDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	checkHR(pCore->device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&mapTexDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&displacementMap)));

	checkHR(pCore->device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&mapTexDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&normalMap)));
}

void WaveSimulator::createResourceHeap() {
	D3D12_DESCRIPTOR_HEAP_DESC tdhDesc = {}; // tdh: Texture Descriptor Heap
	tdhDesc.NumDescriptors = 7; // UAV for prev, curr, next. SRV and UAV for displacement and normal. 3 + 4 = 7.
	tdhDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	tdhDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	checkHR(pCore->device->CreateDescriptorHeap(&tdhDesc, IID_PPV_ARGS(&descHeap)));
}

void WaveSimulator::createResourceDescriptors() {
	auto cbvSrvUavDescSize = pCore->cbvSrvUavDescSize;

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(descHeap->GetCPUDescriptorHandleForHeapStart());
	prevUav_CPU = handle;
	currUav_CPU = handle.Offset(1, cbvSrvUavDescSize);
	nextUav_CPU = handle.Offset(1, cbvSrvUavDescSize);
	displacementSrv_CPU = handle.Offset(1, cbvSrvUavDescSize);
	displacementUav_CPU = handle.Offset(1, cbvSrvUavDescSize);
	normalSrv_CPU = handle.Offset(1, cbvSrvUavDescSize);
	normalUav_CPU = handle.Offset(1, cbvSrvUavDescSize);

	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(descHeap->GetGPUDescriptorHandleForHeapStart());
	prevUav_GPU = gpuHandle;
	currUav_GPU = gpuHandle.Offset(1, cbvSrvUavDescSize);
	nextUav_GPU = gpuHandle.Offset(1, cbvSrvUavDescSize);
	displacementSrv_GPU = gpuHandle.Offset(1, cbvSrvUavDescSize);
	displacementUav_GPU = gpuHandle.Offset(1, cbvSrvUavDescSize);
	normalSrv_GPU = gpuHandle.Offset(1, cbvSrvUavDescSize);
	normalUav_GPU = gpuHandle.Offset(1, cbvSrvUavDescSize);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC crestUavDesc = {};
	crestUavDesc.Format = DXGI_FORMAT_R32_FLOAT;
	crestUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	crestUavDesc.Texture2D.MipSlice = 0;

	D3D12_UNORDERED_ACCESS_VIEW_DESC mapUavDesc = crestUavDesc;
	mapUavDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

	pCore->device->CreateUnorderedAccessView(prevResource.Get(), nullptr, &crestUavDesc, prevUav_CPU);
	pCore->device->CreateUnorderedAccessView(currResource.Get(), nullptr, &crestUavDesc, currUav_CPU);
	pCore->device->CreateUnorderedAccessView(nextResource.Get(), nullptr, &crestUavDesc, nextUav_CPU);

	pCore->device->CreateShaderResourceView(displacementMap.Get(), &srvDesc, displacementSrv_CPU);
	pCore->device->CreateUnorderedAccessView(displacementMap.Get(), nullptr, &mapUavDesc, displacementUav_CPU);

	pCore->device->CreateShaderResourceView(normalMap.Get(), &srvDesc, normalSrv_CPU);
	pCore->device->CreateUnorderedAccessView(normalMap.Get(), nullptr, &mapUavDesc, normalUav_CPU);

}

void WaveSimulator::updateWithComputeShaderOptimized() {

	pCore->cmdList->SetComputeRootSignature(pCore->rootSigs["wave_simulator"].Get());

	ID3D12DescriptorHeap* descHeaps[] = { descHeap.Get() };
	pCore->cmdList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);

	// We should bind current crest texture resource here due to both
	// random-disturb and calc-update will use the crest data in it.
	pCore->cmdList->SetComputeRootDescriptorTable(2, currUav_GPU);

	// Wait disturb CD.
	if (pCore->timer->elapsedSecs > lastDisturbTime + _disturbCD) {

		lastDisturbTime = pCore->timer->elapsedSecs;

		int x = randint(1, _M - 2);
		int y = randint(1, _N - 2);
		float h = randfloat(_hmin, _hmax);

		/* TRICKY TRICKY TRICKY TRICKY TRICKY TRICKY TRICKY TRICKY TRICKY TRICKY */
		// Record a confusing problem here:
		// It will fail when use SetComputeRoot32BitConstant to bind a FLOAT variable to constant buffer;
		// however everything works well when use SetComputeRoot32BitConstants instead. I guess it is caused
		// by the way how data is transfered between CPU and GPU in DirectX 12. If we use pointer to finish
		// this work, all the data from CPU will be sent to GPU untouched (Actually only pointer is sent);
		// if we send the data (32 bit) self directly the variable types (e.g. UINT Vs. FLOAT) will conflict.

		// I falied to find any doc, article or blog about this problem until I write these codes.

		// Dispatch CS.
		pCore->cmdList->SetComputeRoot32BitConstants(0, 1, &h, 4);
		pCore->cmdList->SetComputeRoot32BitConstants(0, 1, &x, 7);
		pCore->cmdList->SetComputeRoot32BitConstants(0, 1, &y, 8);

		pCore->cmdList->SetPipelineState(pCore->PSOs["wave_simulator_disturb_wave"].Get());

		pCore->cmdList->Dispatch(1, 1, 1);
	}

	// Wait update CD.
	if (pCore->timer->elapsedSecs > lastUpdateTime + _t) {

		lastUpdateTime = pCore->timer->elapsedSecs;

		// Dispatch CS.
		pCore->cmdList->SetComputeRoot32BitConstants(0, 1, &_a1, 0);
		pCore->cmdList->SetComputeRoot32BitConstants(0, 1, &_a2, 1);
		pCore->cmdList->SetComputeRoot32BitConstants(0, 1, &_a3, 2);
		pCore->cmdList->SetComputeRoot32BitConstants(0, 1, &_d, 3);
		pCore->cmdList->SetComputeRoot32BitConstants(0, 1, &_M, 5);
		pCore->cmdList->SetComputeRoot32BitConstants(0, 1, &_N, 6);

		pCore->cmdList->SetComputeRootDescriptorTable(1, prevUav_GPU);
		pCore->cmdList->SetComputeRootDescriptorTable(3, nextUav_GPU);

		pCore->cmdList->SetComputeRootDescriptorTable(4, displacementUav_GPU);
		pCore->cmdList->SetComputeRootDescriptorTable(5, normalUav_GPU);

		UINT numGroupX = (UINT)ceilf(_M / 32.0f); // M = 32 in wave-simulation.hlsl
		UINT numGroupY = (UINT)ceilf(_N / 32.0f); // N = 32 in wave-simulation.hlsl

		pCore->cmdList->SetPipelineState(pCore->PSOs["wave_simulator_calc_displacement"].Get());

		pCore->cmdList->Dispatch(numGroupX, numGroupY, 1);

		pCore->cmdList->SetPipelineState(pCore->PSOs["wave_simulator_calc_normal"].Get());

		pCore->cmdList->Dispatch(numGroupX, numGroupY, 1);

		// Update crest data.
		copyStatedResource(pCore,
			prevResource.Get(), D3D12_RESOURCE_STATE_GENERIC_READ,
			currResource.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);
		copyStatedResource(pCore,
			currResource.Get(), D3D12_RESOURCE_STATE_GENERIC_READ,
			nextResource.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);
	}

	// Update target render item properties.
	ritem->displacementAndNormalMapDescHeap = descHeap.Get();
	ritem->hasDisplacementMap = 1;
	ritem->displacementMapHandle = displacementSrv_GPU;
	ritem->hasNormalMap = 1;
	ritem->normalMapHandle = normalSrv_GPU;
}

void WaveSimulator::updateConstraints() {
	
	// Coefficients:
	//
	//       4 - 8 c^2 t^2 / d^2              ut - 2              2 c^2 t^2 / d^2
	// a1 = ---------------------       a2 = --------       a3 = -----------------
	//             ut + 2                     ut + 2                   ut + 2
	//
	// Constraints:
	// 
	//              u + sqrt{ u^2 + 32 c^2 / d^2 }   ||    ||           d
	// 0 < max_t < --------------------------------  || OR ||  0 < c < ---- sqrt{ ut + 2 }
	//                       8 c^2 / d^2             ||    ||           2t

	_maxt = (_u + sqrtf(_u * _u + 32 * _c * _c / (_d * _d))) / (8 * _c * _c / (_d * _d));

	_t = _maxt * 0.5f;

	_a1 = (4 - 8 * _c * _c * _t * _t / (_d * _d)) / (_u * _t + 2);

	_a2 = (_u * _t - 2) / (_u * _t + 2);

	_a3 = (2 * _c * _c * _t * _t / (_d * _d)) / (_u * _t + 2);
}
