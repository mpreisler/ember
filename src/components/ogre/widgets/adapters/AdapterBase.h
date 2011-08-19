//
// C++ Interface: AdapterBase
//
// Description: 
//
// Author: Martin Preisler <preisler.m@gmail.com>
// based on Atlas adapters by Erik Hjortsberg <erik.hjortsberg@gmail.com>, (C) 2007
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
#ifndef EMBEROGRE_GUI_ADAPTERS_ADAPTERBASE_H
#define EMBEROGRE_GUI_ADAPTERS_ADAPTERBASE_H

#include "framework/LoggingInstance.h"
#include <CEGUIEvent.h>
#include <Atlas/Message/Element.h>
#include <sigc++/signal.h>

namespace Ember {
namespace OgreView {

namespace Gui {

namespace Adapters {

/**
 * @brief Use an instance of this whenever you need to update the gui from the adapter.
 *
 * In order to prevent endless loops we need to set the mSelfUpdate variable to
 * true while we do the update. This will automate the setting of this, and the
 * unsetting when it's destroyed.
 * 
 * Code example (scope):
 * @code
 * {
 * 	AdapterSelfUpdateContext(*this);
 * 	Perform the update here
 * 	
 * 	The update context goes out of scope after the bracket
 * }
 * @endcode
*/
template<typename AdapterType>
class AdapterSelfUpdateContext
{
public:
	AdapterSelfUpdateContext(AdapterType& adapter):
		mAdapter(adapter)
	{
		mAdapter._setSelfUpdate(true);
	}
	
	~AdapterSelfUpdateContext()
	{
		mAdapter._setSelfUpdate(false);
	}
	
private:
	AdapterType& mAdapter;
};

/**
 * @brief Base class for all adapters.
 *
 * An adapter is a class which binds a series of gui elements to a 
 * Atlas::Message::Element. It acts like a bridge between the UI and the data
 * in an application.
 * In MVC terms, the adapter is the controller, while the gui elements are the
 * view and the Atlas::Message::Element is the model.
 * 
 * The adapter can either be used in a read only state, where you only show the
 * data contained, or in a read-write state where you also allow the user to
 * update the data.
 * When the adapter is used in a read-write mode, and the value held is being 
 * updated from the gui, the adapter will keep two values: the original value 
 * which is never changed and the new updated value. You can access these two
 * values from getValue() and getOriginalValue(). Every time you call
 * getValue() the adapter will calculate  the value of the element from the
 * gui contents.
 * 
 * Any subclass of this which listens to gui element updates must first check
 * that mSelfUpdate isn't true. If it's true, the gui update has been 
 * triggered by the adapter itself and no signals should be emitted.
 * 
 * An example:
 * @code
 * bool StringAdapter::window_TextChanged(const CEGUI::EventArgs& e)
 * {
 * 	if (!mSelfUpdate) {
 * 		EventValueChanged.emit();
 * 	}
 * 	return true;
 * }
 * @endcode
 * 
 * An adapter can also be marked for removal through remove(). It's however up
 * to the actual adapter implementation as well as code that acts on it to
 * perform the actual removal. The removal is not possible in all instances.
 */
template<typename TargetType>
class AdapterBase
{
public:
	/**
	* @brief Ctor
	*
	* Creates a new adapter and intializes it with the supplied value.
	* @param element The original value.
	*/
	AdapterBase(const TargetType& value):
		mOriginalValue(value),
		mEditedValue(value),
		mSelfUpdate(false),
		mRemoved(false)
	{}

	/**
	* @brief Dtor
	*/
	virtual ~AdapterBase()
	{
		disconnectAllGuiEventConnections();
	}
	
	/**
	 * @brief Sets the value, thus also updating the gui.
	 * 
	 * @param element The new value.
	 */
	void setValue(const TargetType& element)
	{
		updateGui(element);
		EventValueChanged.emit();
	}

	/**
	 * @brief Gets the current value as expressed by the gui.
	 * 
	 * If you've updated the value in the gui, for example by changing text in an editbox, this will return the text as expressed in an Element.
	 * The actual translation from the gui to an Element will be carried out by fillElementFromGui().
	 * @see fillElementFromGui()
	 * @return The current value.
	 */
	const TargetType& getValue()
	{
		fillElementFromGui();
		return mEditedValue;
	}
	
	/**
	 * @brief Returns the original value, as set when the adapter was created or when setValue() was called.
	 * 
	 * @see setValue()
	 * @return The original value
	 */
	const TargetType& getOriginalValue() const
	{
		return mOriginalValue;
	}
	
	/**
	 * @brief Emitted when the value has been changed from a gui update. You can access the new value through getValue().
	 * 
	 * @see getValue()
	 */
	sigc::signal<void> EventValueChanged;
	
	/**
	 * @brief Updates the gui with new values, without triggering EventValueChanged.
	 * 
	 * This is simialiar to setValue(), but won't trigger EventValueChanged. Use this when you want to update the gui, but not trigger any recursive update which might happen when you have other functions listening to EventValueChanged.
	 * @see setValue()
	 * @param element The new value.
	 */
	virtual void updateGui(const TargetType& element) = 0;
	
	/**
	 * @brief Returns true if the value has been changed through changes in the gui.
	 *
	 * @return True if the value has changed.
	 */
	bool hasChanges()
	{
		if (mRemoved)
		{
			return true;
		}
		
		try
		{
			return _hasChanges();
		}
		catch (const std::exception& ex)
		{
			S_LOG_WARNING("Error when checking for changes." << ex);
			return false;
		}
	}
	
	/**
	 * @brief Returns the changed element, and an empty Element if the original element has been deleted.
	 * 
	 * @return 
	 */
	TargetType getChangedElement()
	{
		if (mRemoved)
		{
			return TargetType();
		}
		
		return _getChangedElement();
	}
	
	/**
	 * @brief Deletes the element.
	 * 
	 * This will mark the element as deleted. This might not result in any actual deletion though, depending on what context you use the adapter in. Different adapters will also behave a little different.
	 */
	void remove()
	{
		mRemoved = true;
	}
	
	/**
	 * @brief True if value has been deleted.
	 * 
	 * @see remove()
	 * @return True if value has been deleted.
	 */
	bool isRemoved() const
	{
		return mRemoved;
	}
	
	/**
	 * @brief Adds a suggested value.
	 * How to present this is up to the various adapters, but most would probably show the suggestion in a combobox.
	 * @param suggestedValue The suggested value, as expressed in a string.
	 */
	virtual void addSuggestion(const std::string& suggestedValue)
	{
		// default implementation doesn't support suggestions
	}
	
	/**
	 * @brief Internal method, use AdapterSelfUpdateContext instead where possible!
	 */
	void _setSelfUpdate(bool selfUpdate)
	{
		mSelfUpdate = selfUpdate;
	}
	
protected:
	/**
	The original value.
	*/
	const TargetType& mOriginalValue;
	
	/**
	The value as translated from the gui elements. If no gui changes has been made, this will be the same as mOriginalElement.
	*/
	TargetType mEditedValue;
	
	/**
	Whenever the adapter is updating the gui this must be set to true first, and then back to false after the update has been done.
	The reason is that we listen for changes to the gui elements and update the internally held element as we receive element changed events. However, these events will both be triggered when the gui is updated by the user and when we update the gui from the code. This can lead to a never ending loop, where a gui update will trigger a value update which will trigger a gui update and so on.
	By setting and then checking for this value we can however make sure to not enter such a loop.
	All methods that listen to gui update events must first check that this isn't true before they can assume that the gui has been changed and that the internal value should be updated.
	*/
	bool mSelfUpdate;
	
	/**
	Will be true if the element is marked for removal.
	*/
	bool mRemoved;
	
	/**
	 * @brief Fills the internal mEditedElement element with values from the gui.
	 * 
	 * This is where you in your subclass have the logic where you parse the values in the gui elements into the element.
	 */
	virtual void fillElementFromGui() = 0;
	
	/**
	 * @brief Checks whether any change has occurred.
	 * 
	 * For some adapters this will be really straight forward, just checking the mOriginalElement and the mEditedElement, but for some others such as list or maps this will be a little more complex.
	 * @return True if a change has occurred.
	 */
	virtual bool _hasChanges() = 0;
	
	/**
	 * @brief Get's the changed element.
	 * 
	 * Called by getChangedElement(). By default this will call getValue(), but if your subclass is more complex you can override that behaviour here.
	 * @see getChangedElement()
	 * @see getValue()
	 * @return The changed value.
	 */
	virtual TargetType _getChangedElement()
	{
		return getValue();
	}
	
	/**
	 * @brief Adds a CEGUI event connection to the internal list, which all are disconnected when this instance is detroyed.
	 * 
	 * All connections that are made to CEGUI event should be disconnected when ever any instance of this is destroyed, to prevent dangling pointers. By calling this method with your connection instance you'll guarantee that that happens.
	 * @code
	 * addGuiEventConnection(textWindow->subscribeEvent(CEGUI::Window::EventTextChanged, CEGUI::Event::Subscriber(&FooAdapter::window_TextChanged, this))); 
	 * @endcode
	 * @param connection The event connection. 
	 */
	void addGuiEventConnection(CEGUI::Event::Connection connection)
	{
		mGuiEventConnections.push_back(connection);
	}
	
	/**
	 * @brief Disconnects all cegui event connections that were registered through addGuiEventConnection().
	 * 
	 * This method is called automatically when an instance of this is deleted, so you don't need to call this yourself unless you want to force a disconnection.
	 */
	void disconnectAllGuiEventConnections()
	{
		for (std::vector<CEGUI::Event::Connection>::iterator I = mGuiEventConnections.begin(); I != mGuiEventConnections.end(); ++I)
		{
			(*I)->disconnect();
		}
		
		mGuiEventConnections.clear();
	}
	
private:
	/**
	 * @brief The internal list of gui event connections, used for automatic disconnection when the instance is destroyed.
	 */
	std::vector<CEGUI::Event::Connection> mGuiEventConnections;

};

/**
 * @brief Wrapper struct for an adapter and the window that contains it.
 */
template<typename AdapterType>
struct AdapterWrapper
{
	AdapterType* Adapter;
	CEGUI::Window* ContainerWindow;
};

}

}

}

}

#endif
