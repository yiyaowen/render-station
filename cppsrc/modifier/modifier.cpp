/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include "modifier.h"
#include "utils/vmesh-utils.h"

Modifier::Modifier(D3DCore* pCore, Vmesh* mesh, ObjectGeometry* geo)
	: pCore(pCore), _mesh(mesh)
{
	_geo.reset(copyObjectGeometry(geo));
}
