/*
** Render Station @ https://gitee.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include <memory>

#include "utils/geometry-utils.h"

// Forward declaration to avoid circular reference.
struct D3DCore;
struct Vmesh;

class Modifier {
public:
	Modifier(D3DCore* pCore, Vmesh* mesh, ObjectGeometry* geo);

	virtual void update() = 0;

	inline Vmesh* mesh() { return _mesh; }
	inline void changeMesh(Vmesh* newMesh) { _mesh = newMesh; }

	inline bool actived() { return _actived; }
	inline void setActived(bool value) { _actived = value; }

protected:
	D3DCore* pCore = nullptr;
	Vmesh* _mesh = nullptr;
	std::unique_ptr<ObjectGeometry> _geo = nullptr;

	bool _actived = true;
};