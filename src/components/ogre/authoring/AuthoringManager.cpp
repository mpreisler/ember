/*
 Copyright (C) 2009 Erik Hjortsberg

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

#include "AuthoringManager.h"
#include "AuthoringHandler.h"
#include "services/config/ConfigService.h"

namespace EmberOgre
{

namespace Authoring
{
AuthoringManager::AuthoringManager(Eris::View& view) :
	DisplayAuthoringVisualizations("displayauthoringvisualizations", this, "Displays authoring markers for all entities."), HideAuthoringVisualizations("hideauthoringvisualizations", this, "Hides authoring markers for all entities."), mView(view), mHandler(0)
{
	registerConfigListener("authoring", "visualizations", sigc::mem_fun(*this, &AuthoringManager::config_AuthoringVisualizations));
}

AuthoringManager::~AuthoringManager()
{
	delete mHandler;
}

void AuthoringManager::displayAuthoringVisualization()
{
	if (!mHandler) {
		mHandler = new AuthoringHandler(mView);
	}
}

void AuthoringManager::hideAuthoringVisualization()
{
	delete mHandler;
	mHandler = 0;
}

void AuthoringManager::runCommand(const std::string &command, const std::string &args)
{
	if (DisplayAuthoringVisualizations == command) {
		Ember::EmberServices::getSingleton().getConfigService()->setValue("authoring", "visualizations", true);
	} else if (HideAuthoringVisualizations == command) {
		Ember::EmberServices::getSingleton().getConfigService()->setValue("authoring", "visualizations", false);
	}
}

void AuthoringManager::config_AuthoringVisualizations(const std::string& section, const std::string& key, varconf::Variable& variable)
{
	if (variable.is_bool()) {
		if (static_cast<bool> (variable)) {
			displayAuthoringVisualization();
		} else {
			hideAuthoringVisualization();
		}
	}
}

void AuthoringManager::startMovement(EmberEntity& entity, EntityMover& mover)
{
	if (mHandler) {
		mHandler->startMovement(entity, mover);
	}
}

void AuthoringManager::stopMovement()
{
	if (mHandler) {
		mHandler ->stopMovement();
	}
}

}
}