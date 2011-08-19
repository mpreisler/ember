//
// C++ Implementation: NumberAdapter
//
// Description: 
//
//
// Author: Erik Hjortsberg <erik.hjortsberg@gmail.com>, (C) 2007
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

#include <cstdlib>
#include "NumberAdapter.h"
#include "../../ColouredListItem.h"

namespace Ember {
namespace OgreView {

namespace Gui {

namespace Adapters {

namespace Atlas {

NumberAdapter::NumberAdapter(const ::Atlas::Message::Element& element, CEGUI::Combobox* textWindow)
: AdapterBase(element)
, mTextWindow(textWindow)
{
	if (textWindow) {
		addGuiEventConnection(textWindow->subscribeEvent(CEGUI::Window::EventTextChanged, CEGUI::Event::Subscriber(&NumberAdapter::window_TextChanged, this))); 
	}
	updateGui(mOriginalValue);
	mTextWindow->getPushButton()->setVisible(false);

}


NumberAdapter::~NumberAdapter()
{
}

void NumberAdapter::updateGui(const ::Atlas::Message::Element& element)
{
	AdapterSelfUpdateContext context(*this);
	if (mTextWindow) {
		std::stringstream ss;
		ss << element.asNum();
		mTextWindow->setText(ss.str());
	}
}

bool NumberAdapter::window_TextChanged(const CEGUI::EventArgs& e)
{
	if (!mSelfUpdate) {
		EventValueChanged.emit();
	}
	return true;
}



void NumberAdapter::fillElementFromGui()
{
	if (mOriginalValue.isInt()) {
		mEditedValue = ::Atlas::Message::Element(atoi(mTextWindow->getText().c_str()));
	} else {
		mEditedValue = ::Atlas::Message::Element(atof(mTextWindow->getText().c_str()));
	}
}

bool NumberAdapter::_hasChanges()
{
	return mOriginalValue.asNum() != getValue().asNum();
}

void NumberAdapter::addSuggestion(const std::string& suggestedValue)
{
	mTextWindow->addItem(new ColouredListItem(suggestedValue));
	mTextWindow->getPushButton()->setVisible(true);
}

}

}

}

}
}
