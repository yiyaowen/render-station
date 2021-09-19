/*
** Render Station @ https://gitee.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include "d3dcore/d3dcore.h"
#include "wave-simulator.h"

WaveSimulator::WaveSimulator(D3DCore* pCore,
	Vmesh* mesh, // Initial target mesh. There is no problem to set it to null and change the mesh later.
	float d, // The distance between two adjacent vertices in the grid.
	UINT m, // Half horizontal node count. See geometry-utils.cpp for generateGrid(xxx).
	UINT n, // Half vertical node count. See geometry-utils.cpp for generateGrid(xxx).
	ObjectGeometry* grid,
	float c, // The spread velocity of wave.
	float u, // The damping grade with velocity dimension.
	float hmin, // The mi ndisturbance height.
	float hmax, // The max disturbance height.
	float t) // The interval seconds between 2 random disturbance.
	: Modifier(pCore, mesh, grid),
	_m(m), _n(n), _M(2 * m + 1), _N(2 * n + 1), _d(d), _c(c), _u(u), _hmin(hmin), _hmax(hmax), _disturbCD(t)
{
	_prevGeo.reset(copyObjectGeometry(grid));

	updateConstraints();
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

	// Wait disturb CD.
	if (pCore->timer->elapsedSecs > lastDisturbTime + _disturbCD) {

		lastDisturbTime = pCore->timer->elapsedSecs;

		UINT x = randint(1, _M - 2);
		UINT y = randint(1, _N - 2);
		float h = randfloat(_hmin, _hmax);
		float halfH = h * 0.5f;
		_geo->vertices[x + y * _M].pos.y = h;
		_geo->vertices[x-1 + y * _M].pos.y = halfH;
		_geo->vertices[x+1 + y * _M].pos.y = halfH;
		_geo->vertices[x + (y-1) * _M].pos.y = halfH;
		_geo->vertices[x + (y+1) * _M].pos.y = halfH;
	}
	
	// Wait update CD.
	if (pCore->timer->elapsedSecs > lastUpdateTime + _t) {

		lastUpdateTime = pCore->timer->elapsedSecs;

		for (UINT i = 1; i < _M - 1; ++i) {
			for (UINT j = 1; j < _N - 1; ++j) {

				// We can write the computed data into _prevGeo directly
				// since the formula only depends current vertex for current time.
				// Thus we do not need to create an extra _nextGeo.
				auto& y_next = _prevGeo->vertices[i + j * _M].pos.y;
				auto& y_curr = _geo->vertices[i + j * _M].pos.y;
				auto& y_prev = _prevGeo->vertices[i + j * _M].pos.y;

				auto& y_left = _geo->vertices[i-1 + j * _M].pos.y;
				auto& y_right = _geo->vertices[i + 1 + j * _M].pos.y;
				auto& y_above = _geo->vertices[i + (j-1) * _M].pos.y;
				auto& y_below = _geo->vertices[i + (j+1) * _M].pos.y;

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
