/*
 Copyright (C) 2004  Erik Hjortsberg

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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "EmberEntity.h"
#include "WorldEmberEntity.h"
#include "EmberOgre.h"
#include "Avatar.h"
#include "Convert.h"
#include "Scene.h"
#include "WorldAttachment.h"
#include "ForestRenderingTechnique.h"

#include "terrain/TerrainParser.h"
#include "terrain/TerrainManager.h"
#include "terrain/TerrainHandler.h"
#include "terrain/TerrainShaderParser.h"
#include "terrain/TerrainDefPoint.h"
#include "environment/Foliage.h"
#include "environment/Environment.h"
#include "environment/CaelumEnvironment.h"
#include "environment/SimpleEnvironment.h"

#include "services/EmberServices.h"
#include "services/config/ConfigService.h"

#include "framework/Exception.h"

#include "TerrainPageDataProvider.h"
#include "components/ogre/SceneManagers/EmberPagingSceneManager/include/EmberPagingSceneManager.h"
#include "components/ogre/SceneManagers/EmberPagingSceneManager/include/EmberPagingSceneManagerAdapter.h"

#include "main/Application.h"

#include <Eris/Timeout.h>
#include <Eris/View.h>

#include <OgreSceneManager.h>

namespace Ember
{
namespace OgreView
{

WorldEmberEntity::WorldEmberEntity(const std::string& id, Eris::TypeInfo* ty, Eris::View* vw, Scene& scene) :
	EmberEntity(id, ty, vw, scene), mTerrainManager(new Terrain::TerrainManager(new EmberPagingSceneManagerAdapter(static_cast<EmberPagingSceneManager&> (scene.getSceneManager())), scene, *EmberOgre::getSingleton().getShaderManager(), Application::getSingleton().EventEndErisPoll)), mFoliage(0), mEnvironment(0), mFoliageInitializer(0), mHasBeenInitialized(false), mPageDataProvider(new TerrainPageDataProvider(mTerrainManager->getHandler())), mSceneManager(static_cast<EmberPagingSceneManager&> (scene.getSceneManager())), mScene(scene)
{
	mSceneManager.registerProvider(mPageDataProvider);
	mWorldPosition.LatitudeDegrees = 0;
	mWorldPosition.LongitudeDegrees = 0;
	Ogre::SceneNode* worldNode = mSceneManager.getRootSceneNode()->createChildSceneNode("entity_" + getId());
	if (worldNode) {
		setAttachment(new WorldAttachment(*this, *worldNode, *mTerrainManager));
	} else {
		throw Exception("Could not create world node.");
	}
	EmberOgre::getSingleton().EventTerrainManagerCreated.emit(*mTerrainManager);
	mTerrainManager->getHandler().EventAfterTerrainUpdate.connect(sigc::mem_fun(*this, &WorldEmberEntity::terrainManager_AfterTerrainUpdate));
}

WorldEmberEntity::~WorldEmberEntity()
{
	ISceneRenderingTechnique* technique = mScene.removeRenderingTechnique("forest");
	delete technique;
	mSceneManager.registerProvider(0);
	delete mPageDataProvider;
	delete mFoliage;
	delete mEnvironment;
	if (mTerrainManager) {
		EmberOgre::getSingleton().EventTerrainManagerBeingDestroyed.emit();
	}
	delete mTerrainManager;
	if (mTerrainManager) {
		EmberOgre::getSingleton().EventTerrainManagerDestroyed.emit();
	}
}

void WorldEmberEntity::init(const Atlas::Objects::Entity::RootEntity &ge, bool fromCreateOp)
{
	//Set some of the positioning values to sane ones. These values should never be anything else.
	m_position = WFMath::Point<3>::ZERO();
	m_velocity = WFMath::Vector<3>::ZERO();
	m_orientation = WFMath::Quaternion().identity();
	m_acc = WFMath::Vector<3>::ZERO();

	registerConfigListener("graphics", "foliage", sigc::mem_fun(*this, &WorldEmberEntity::Config_Foliage));

	EmberEntity::init(ge, fromCreateOp);

	mEnvironment = new Environment::Environment(*mTerrainManager, new Environment::CaelumEnvironment(&mSceneManager, EmberOgre::getSingleton().getRenderWindow(), mScene.getMainCamera()), new Environment::SimpleEnvironment(&mSceneManager, EmberOgre::getSingleton().getRenderWindow(), mScene.getMainCamera()));
	EventEnvironmentCreated.emit();

	mScene.addRenderingTechnique("forest", new ForestRenderingTechnique(*mEnvironment->getForest()));

	//we will wait with creating the terrain and initializing the environment until we've got a onVisibilityChanged call, since the Eris::Calendar functionality depends on the world entity object to be fully constructed and initialized to work. By waiting until onVisibilityChanged is called we guarantee that the Calendar will get the correct server time

}

void WorldEmberEntity::Config_Foliage(const std::string& section, const std::string& key, varconf::Variable& variable)
{
	if (variable.is_bool() && static_cast<bool>(variable)) {
		if (!mFoliage) {
			//create the foliage
			mFoliage = new Environment::Foliage(*mTerrainManager);
			EventFoliageCreated.emit();
			if (!mHasBeenInitialized) {
				mFoliageInitializer = std::auto_ptr<DelayedFoliageInitializer>(new DelayedFoliageInitializer(*mFoliage, *getView(), 1000, 15000));
			} else {
				mFoliage->initialize();
			}
		}
	} else {
		delete mFoliage;
		mFoliage = 0;
		mFoliageInitializer.reset();
	}

}

void WorldEmberEntity::onAttrChanged(const std::string& str, const Atlas::Message::Element& v)
{
	//If the terrain hasn't been initialized yet (which will happen when the visibility changes, as it does right after the entity has been initialized) we'll ignore changes to the terrain, since it will be parsed anyway.
	if (str == "terrain" && !mHasBeenInitialized) {
		Eris::Entity::onAttrChanged(str, v);
	} else {
		EmberEntity::onAttrChanged(str, v);
	}
}

void WorldEmberEntity::onMoved()
{
	//It should never be possible to move the world, so we'll skip calling EmberEntity::onMoved() and go directly to Eris::Entity
	Eris::Entity::onMoved();
}

void WorldEmberEntity::onVisibilityChanged(bool vis)
{
	//we do our initialization of the terrain and environment here instead of at onInit since that way we can guarantee that Eris::Calendar will work as it should (which is used to get the correct server time)
	if (!mHasBeenInitialized) {
		mEnvironment->initialize();
		if (mTerrainManager) {
			mTerrainAfterUpdateConnection = mTerrainManager->getHandler().EventWorldSizeChanged.connect(sigc::mem_fun(*this, &WorldEmberEntity::terrain_WorldSizeChanged));
			mTerrainManager->getHandler().setLightning(mEnvironment->getSun());
			bool hasValidShaders = false;
			Terrain::TerrainShaderParser terrainShaderParser(*mTerrainManager);
			if (hasAttr("terrain")) {
				Terrain::TerrainParser terrainParser;
				const Atlas::Message::Element& terrainElement = valueOfAttr("terrain");
				if (terrainElement.isMap()) {
					const Atlas::Message::MapType& terrainMap(terrainElement.asMap());
					if (terrainMap.count("surfaces")) {
						const Atlas::Message::Element& surfaceElement(terrainMap.find("surfaces")->second);
						terrainShaderParser.createShaders(surfaceElement);
						hasValidShaders = true;
					}
				}
				mTerrainManager->getHandler().updateTerrain(terrainParser.parseTerrain(terrainElement, getPosition().isValid() ? getPosition() : WFMath::Point<3>::ZERO()));
				if (!hasValidShaders) {
					terrainShaderParser.createDefaultShaders();
					hasValidShaders = true;
				}
			}

			if (!hasValidShaders) {
				terrainShaderParser.createDefaultShaders();
				hasValidShaders = true;
			}

		}

		//TODO: Parse world location data when it's available
		mEnvironment->setWorldPosition(mWorldPosition.LongitudeDegrees, mWorldPosition.LatitudeDegrees);

		mHasBeenInitialized = true;
	}

	Eris::Entity::onVisibilityChanged(vis);
}

void WorldEmberEntity::terrain_WorldSizeChanged()
{
	mTerrainAfterUpdateConnection.disconnect();
	//wait a little with initializing the foliage
	if (mFoliage) {
		mFoliageInitializer = std::auto_ptr<DelayedFoliageInitializer>(new DelayedFoliageInitializer(*mFoliage, *getView(), 1000, 15000));
	}
}

void WorldEmberEntity::terrainManager_AfterTerrainUpdate(const std::vector<WFMath::AxisBox<2> >& areas, const std::set<Terrain::TerrainPage*>& pages)
{
	updateEntityPosition(this, areas);
}

void WorldEmberEntity::updateEntityPosition(EmberEntity* entity, const std::vector<WFMath::AxisBox<2> >& areas)
{
	entity->adjustPosition();
	for (unsigned int i = 0; i < entity->numContained(); ++i) {
		EmberEntity* containedEntity = static_cast<EmberEntity*> (entity->getContained(i));
		updateEntityPosition(containedEntity, areas);
	}
}

void WorldEmberEntity::onLocationChanged(Eris::Entity *oldLocation)
{
	Eris::Entity::onLocationChanged(oldLocation);
}

void WorldEmberEntity::addArea(Terrain::TerrainArea& area)
{
	mTerrainManager->getHandler().addArea(area);
}

void WorldEmberEntity::addTerrainMod(Terrain::TerrainMod* mod)
{
	mTerrainManager->getHandler().addTerrainMod(mod);
}

float WorldEmberEntity::getHeight(const WFMath::Point<2>& localPosition) const
{
	//Note that there's no need to adjust the localPosition since the world always is at location 0,0,0 with not rotation
	float height = 0;

	if (mTerrainManager->getHeight(WFMath::Point<2>(localPosition.x(), localPosition.y()), height)) {
		return height;
	}
	return EmberEntity::getHeight(localPosition);
}

void WorldEmberEntity::updateTerrain(const std::vector<Terrain::TerrainDefPoint>& terrainDefinitionPoints)
{
	mTerrainManager->getHandler().updateTerrain(terrainDefinitionPoints);
}

Environment::Environment* WorldEmberEntity::getEnvironment() const
{
	return mEnvironment;
}

Environment::Foliage* WorldEmberEntity::getFoliage() const
{
	return mFoliage;
}

Terrain::TerrainManager& WorldEmberEntity::getTerrainManager()
{
	return *mTerrainManager;
}

DelayedFoliageInitializer::DelayedFoliageInitializer(Environment::Foliage& foliage, Eris::View& view, unsigned int intervalMs, unsigned int maxTimeMs) :
	mFoliage(foliage), mView(view), mIntervalMs(intervalMs), mMaxTimeMs(maxTimeMs), mTimeout(new Eris::Timeout(intervalMs)), mTotalElapsedTime(0)
{
	//don't load the foliage directly, instead wait some seconds for all terrain areas to load
	//the main reason is that new terrain areas will invalidate the foliage causing a reload
	//by delaying the foliage we can thus in most cases avoid those reloads
	//wait three seconds
	mTimeout->Expired.connect(sigc::mem_fun(this, &DelayedFoliageInitializer::timout_Expired));

}

DelayedFoliageInitializer::~DelayedFoliageInitializer()
{
}

void DelayedFoliageInitializer::timout_Expired()
{
	//load the foliage if either all queues entities have been loaded, or 15 seconds has elapsed
	if (mView.lookQueueSize() == 0 || mTotalElapsedTime > mMaxTimeMs) {
		mFoliage.initialize();
	} else {
		mTotalElapsedTime += mIntervalMs;
		mTimeout->reset(mIntervalMs);
	}
}

}
}
