//
// C++ Interface: IntegratedChatWidget
//
// Description:
//
//
// Author: Erik Hjortsberg <erik.hjortsberg@gmail.com>, (C) 2005
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
#ifndef EMBEROGREINTEGRATEDCHATWIDGET_H
#define EMBEROGREINTEGRATEDCHATWIDGET_H

#include "Widget.h"
#include "WidgetPool.h"
#include "services/config/ConfigListenerContainer.h"
#include <Eris/Entity.h>
#include <OgreMovableObject.h>

namespace Ember {
namespace OgreView {

namespace Camera
{
class MainCamera;
}

namespace Model {
class ModelRepresentation;
}
class GUIManager;
class Widget;
class EmberEntity;
class World;

namespace Gui {



/**

Shows chat bubbles over npc's heads when they say something.
The bubbles will disappear after a while (actually fade out) or when the player moves away. This can be set in the config file.

If the npc has a list of suggested responses these will be shown in a list of clickable buttons to the right.

@author Erik Hjortsberg
*/
class IngameChatWidget : public Widget, public ConfigListenerContainer {




	class EntityObserver;
	class Label;
	class MovableObjectListener : public Ogre::MovableObject::Listener
	{
		public:
			MovableObjectListener(EntityObserver& entityObserver, Model::ModelRepresentation& modelRepresentation);
			virtual ~MovableObjectListener();

			virtual bool objectRendering (const Ogre::MovableObject * movableObject, const Ogre::Camera * camera);
			void setObserving(bool isObserving);

		private:
			EntityObserver& mEntityObserver;
			Model::ModelRepresentation& mModelRepresentation;
			bool mIsObserving;
	};

	class EntityObserver : public virtual sigc::trackable
	{
		public:
			EntityObserver(IngameChatWidget& chatWidget, Model::ModelRepresentation& modelRepresentation);
			virtual ~EntityObserver();
			void updateLabel(const Ogre::Camera * camera);

		protected:
			IngameChatWidget& mChatWidget;
			Model::ModelRepresentation& mModelRepresentation;
			Label* mLabel;
			MovableObjectListener mMovableObjectListener;
			Eris::Entity::AttrChangedSlot mExternalSlot; //, mNameSlot;

			void showLabel();
			void hideLabel();

			void entity_VisibilityChanged(bool visible);
			void entity_BeingDeleted();
			void entity_Say(const Atlas::Objects::Root& talk);
			void entity_attributeChanged(const Atlas::Message::Element& attributeValue);


	};

	class ChatText;
	/**
	Holds the actual chat window and keeps track of fading, catching clicks etc.
	*/
	class Label : public virtual sigc::trackable
	{
		public:
			/**

			*/
			Label(CEGUI::Window* window, CEGUI::WindowManager* windowManager, IngameChatWidget& containerWidget, const std::string& prefix);

			virtual ~Label();

			/**

			*/
			void updateText(const std::string& line);


			/**
			 * @brief Gets the entity the window belongs to
			 */
			EmberEntity* getEntity();

			void setModelRepresentation(Model::ModelRepresentation* modelRepresentation);

			void setActive(bool active);
			bool getActive() const;
			void markForRender();

			CEGUI::Window* getWindow();
			/**
			call this each frame to update the window
			*/
			void frameStarted( const Ogre::FrameEvent & event );

			/**
			positions the window on top of the entity
			*/
			void placeWindowOnEntity();

			IngameChatWidget& getIngameChatWidget() { return mContainerWidget;}

			void updateEntityName();

			/**
			 * @brief Removes the attached chat text widget (i.e. the widget which shows what an entity says, and any optional response buttons).
			 */
			void removeChatText();

			/**
			 * @brief Resets the label.
			 *
			 * This involves removing any chat texts.
			 */
			void reset();

		protected:

			CEGUI::Window* mWindow;
			Model::ModelRepresentation* mModelRepresentation;
			std::vector<CEGUI::Window*> mResponseTextWidgets;
			CEGUI::WindowManager* mWindowManager;
			IngameChatWidget& mContainerWidget;

			bool mActive;
			const std::string mPrefix;
			bool mRenderNextFrame;
			ChatText* mChatText;
// 			CEGUI::Window* mNameWidget;

	};

	class LabelCreator : public WidgetPool<IngameChatWidget::Label>::WidgetCreator
	{
		public:
			LabelCreator(IngameChatWidget& ingameChatWidget);
			virtual ~LabelCreator();
			virtual IngameChatWidget::Label* createWidget(unsigned int currentPoolSize);
		protected:
			IngameChatWidget& mIngameChatWidget;
			CEGUI::Window* mLayout;
	};

	class ChatText : public virtual sigc::trackable
	{
		public:
			ChatText(CEGUI::Window* window, const std::string& prefix);
			virtual ~ChatText() {}

			void updateText( const std::string & line);

			/**
			call this each frame to update the window
			*/
			bool frameStarted( const Ogre::FrameEvent & event );

			float getElapsedTimeSinceLastUpdate();

			/**
			increases the elapsed time with the supplied amount
			*/
			void increaseElapsedTime(float timeSlice);

			void attachToLabel(Label* label);

		protected:
			std::vector<CEGUI::Window*> mResponseTextWidgets;
			Label* mLabel;
			CEGUI::Window* mWindow;
			CEGUI::Window* mTextWidget;
			CEGUI::Window* mResponseWidget;
			float mElapsedTimeSinceLastUpdate;
			std::string mPrefix;

			bool buttonResponse_Click(const CEGUI::EventArgs& args);

			/**
			 * @brief Removes all response buttons.
			 */
			void clearResponses();
	};


	class ChatTextCreator : public WidgetPool<IngameChatWidget::ChatText>::WidgetCreator
	{
		public:
			ChatTextCreator(IngameChatWidget& ingameChatWidget);
			virtual ~ChatTextCreator();
			virtual IngameChatWidget::ChatText* createWidget(unsigned int currentPoolSize);
		protected:
			IngameChatWidget& mIngameChatWidget;
			CEGUI::Window* mLayout;
	};

typedef std::map<std::string, Label*> LabelMap;
typedef std::vector<Label*> LabelStore;
typedef std::stack<Label*> LabelStack;
typedef std::vector<EntityObserver*> EntityObserverStore;
typedef std::vector<Eris::TypeInfo*> TypeInfoStore;
friend class IngameChatWidget::EntityObserver;
public:
	IngameChatWidget();
    virtual ~IngameChatWidget();
	void buildWidget();
	virtual void frameStarted(const Ogre::FrameEvent & event);

	void removeWidget(const std::string& windowName);

	void removeEntityObserver(EntityObserver* observer);

	WidgetPool<Label>& getLabelPool();
	WidgetPool<ChatText>& getChatTextPool();

	float getTimeShown();

	CEGUI::Window* getLabelSheet();

protected:

	/**
	 * @brief Listen to the Eris::View::EntitySeen event.
	 * @param entity The seen entity.
	 */
	void View_EntitySeen(Eris::Entity* entity);

	/**
	 * @brief Listen to the Eris::View::EntityCreated event.
	 * @param entity The created entity.
	 */
	void View_EntityCreated(Eris::Entity* entity);

	/**
	 * @brief Listen to the world created event, and attach listeners.
	 * @param world The new world.
	 */
	void EmberOgre_WorldCreated(World& world);

	void Config_TimeShown(const std::string& section, const std::string& key, varconf::Variable& variable);
	void Config_DistanceShown(const std::string& section, const std::string& key, varconf::Variable& variable);

	/**
	 * @brief Called whenever an entity has arrived from the server.
	 *
	 * Either through being seen or being created. This will inspect the entity and set up any observers if needed.
	 * @param entity A newly created or seen entity.
	 */
	void entityArrivedFromServer(EmberEntity& entity);


	EntityObserverStore mEntityObservers;

	TypeInfoStore mLabelTypes;

	/**
	 * @brief The length in seconds a window should be shown after it has been activated.
	 */
	float mTimeShown;

	/**
	 * @brief How far away, in meters, the window should be visible.
	 */
	float mDistanceShown;

	LabelCreator mLabelCreator;
	WidgetPool<Label> mLabelPool;

	ChatTextCreator mChatTextCreator;
	WidgetPool<ChatText> mChatTextPool;

	CEGUI::Window* mLabelSheet;

	std::string mAvatarEntityId;

	Camera::MainCamera* mCamera;

};

inline float IngameChatWidget::ChatText::getElapsedTimeSinceLastUpdate() {
	return mElapsedTimeSinceLastUpdate;
}

inline float IngameChatWidget::getTimeShown() {
	return mTimeShown;
}


}
}

}

#endif
