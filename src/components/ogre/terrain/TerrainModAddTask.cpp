/*
 Copyright (C) 2009 Erik Hjortsberg <erik.hjortsberg@gmail.com>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "TerrainModAddTask.h"
#include "TerrainManager.h"
#include "TerrainMod.h"
#include <Mercator/TerrainMod.h>
#include <Mercator/Terrain.h>
#include <Eris/TerrainMod.h>

namespace EmberOgre
{

namespace Terrain
{

TerrainModAddTask::TerrainModAddTask(Mercator::Terrain& terrain, TerrainMod& terrainMod, TerrainManager& manager, TerrainModMap& terrainMods) :
	mTerrain(terrain), mTerrainMod(terrainMod), mManager(manager), mTerrainMods(terrainMods), mAppliedMod(0)
{

}

TerrainModAddTask::~TerrainModAddTask()
{
}

void TerrainModAddTask::executeTaskInBackgroundThread(Ember::Tasks::TaskExecutionContext& context)
{
	/// We need to save this pointer to use when the modifier is changed or deleted
	mAppliedMod = mTerrain.addMod(*mTerrainMod.getMercatorMod());

}

void TerrainModAddTask::executeTaskInMainThread()
{
	if (mAppliedMod) {
		mTerrainMods.insert(TerrainModMap::value_type(mTerrainMod.getErisMod()->getEntity()->getId(), mAppliedMod));

		std::vector<TerrainPosition> updatedPositions;
		updatedPositions.push_back(TerrainPosition(mAppliedMod->bbox().getCenter().x(), mAppliedMod->bbox().getCenter().y()));
		mManager.reloadTerrain(updatedPositions);
	}
}
}

}