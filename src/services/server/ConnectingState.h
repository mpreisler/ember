/*
 Copyright (C) 2010 Erik Hjortsberg <erik.hjortsberg@gmail.com>

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

#ifndef CONNECTINGSTATE_H_
#define CONNECTINGSTATE_H_

#include "StateBase.h"

#include "Connection.h"
#include <sigc++/trackable.h>
#include <sigc++/connection.h>

namespace Ember
{

class ConnectedState;

/**
 * @brief State for when a connection to a server is in progress.
 */
class ConnectingState: public virtual StateBase<ConnectedState>
{
public:
	ConnectingState(IState& parentState, const std::string& host, short port);
	virtual ~ConnectingState();

	virtual void destroyChildState();

	void connect();

private:

	/**
	 * @brief Holds our connection to the server
	 */
	Connection mConnection;

	/**
	 * @brief Track the GotFailure connection, so to sever it when aborting and thus avoiding infinite loops.
	 */
	sigc::connection mFailureConnection;

	void gotFailure(const std::string& msg);

	void connected();

	void statusChanged(Eris::BaseConnection::Status status);
};

}

#endif /* CONNECTINGSTATE_H_ */