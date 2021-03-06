//
// C++ Implementation: AvatarLogger
//
// Description:
//
//
// Author: Erik Hjortsberg <erik.hjortsberg@gmail.com>, (C) 2008
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.//
//
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "AvatarLogger.h"
#include "Avatar.h"
#include "EmberEntity.h"
#include "GUIManager.h"
#include "services/config/ConfigService.h"
#include "framework/Time.h"


#include "framework/osdir.h"

#include <sigc++/signal.h>

namespace Ember {
namespace OgreView {

AvatarLogger::AvatarLogger(EmberEntity& avatarEntity)
: mChatLogger(0)
{
	assert(&avatarEntity);

	//Put log files in a "logs" subdirectory of the home directory.
	const std::string dir = EmberServices::getSingleton().getConfigService().getHomeDirectory() + "/logs/";
	try {
		//make sure the directory exists

		oslink::directory osdir(dir);

		if (!osdir.isExisting()) {
#ifdef __WIN32__
			mkdir(dir.c_str());
#else
			mkdir(dir.c_str(), S_IRWXU);
#endif
		}
		//perform setup of the stream
		std::stringstream logFileSS;
		logFileSS << dir << "/" << avatarEntity.getName() << "_chatlog.log";
		mChatLogger = std::auto_ptr<std::ofstream>(new std::ofstream(logFileSS.str().c_str(), std::ios::app));
		S_LOG_VERBOSE("Chat Logging set to write in [ " << logFileSS.str() << " ]");

		*mChatLogger << "-------------------------------------------------------" << std::endl;
		*mChatLogger << "Chat Logging Initialized at " <<  Time::getLocalTimeStr() << std::endl;
		*mChatLogger << "-------------------------------------------------------" << std::endl;

		//wait with connecting until everything has been properly set up
		GUIManager::getSingleton().AppendIGChatLine.connect(sigc::mem_fun(*this, &AvatarLogger::GUIManager_AppendIGChatLine));

	} catch (const std::exception& ex) {
		S_LOG_FAILURE("Error when creating directory for logs." << ex);
	}
}


AvatarLogger::~AvatarLogger()
{
	*mChatLogger << "-------------------------------------------------------" << std::endl;
	*mChatLogger << "Chat Logging Ended at " <<  Time::getLocalTimeStr() << std::endl;
	*mChatLogger << "-------------------------------------------------------" << std::endl;
}

void AvatarLogger::GUIManager_AppendIGChatLine(const std::string& message, EmberEntity* entity)
{
	*mChatLogger << "[" << Time::getLocalTimeStr() << "] <" <<  entity->getName() << "> says: " << message << std::endl;
}

AvatarLoggerParent::AvatarLoggerParent(Avatar& avatar)
{
	//we either already have an entity, or we need to wait until it's creeated
	mLogger = std::auto_ptr<AvatarLogger>(new AvatarLogger(avatar.getEmberEntity()));
}



}
}
