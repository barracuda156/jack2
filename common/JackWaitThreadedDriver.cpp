/*
 Copyright (C) 2001 Paul Davis
 Copyright (C) 2004-2008 Grame

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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#ifdef WIN32
#pragma warning (disable : 4786)
#endif

#include "JackWaitThreadedDriver.h"
#include "JackGlobals.h"
#include "JackClient.h"
#include "JackEngineControl.h"
#include "JackException.h"
#include "JackError.h"

namespace Jack
{

bool JackWaitThreadedDriver::Init()
{
    return (fStarter.Start() == 0);
}

bool JackWaitThreadedDriver::Execute()
{
    try {
        // Process a null cycle until NetDriver has started
        while (!fStarter.fRunning && fThread.GetStatus() == JackThread::kRunning) {
            fDriver->ProcessNull();
        }

        // Set RT
        if (fDriver->IsRealTime()) {
            jack_log("JackWaitThreadedDriver::Init IsRealTime");
            // Will do "something" on OSX only...
            fThread.SetParams(GetEngineControl()->fPeriod, GetEngineControl()->fComputation, GetEngineControl()->fConstraint);
            if (fThread.AcquireRealTime(GetEngineControl()->fPriority) < 0) {
                jack_error("AcquireRealTime error");
            } else {
                set_threaded_log_function();
            }
        }

        // Switch to keep running even in case of error
        while (fThread.GetStatus() == JackThread::kRunning) {
            fDriver->Process();
        }
        return false;
    } catch (JackDriverException& e) {
        e.PrintMessage();
        jack_log("Driver is restarted");
        fThread.DropRealTime();
        // Thread in kIniting status again...
        fThread.SetStatus(JackThread::kIniting);
        if (Init()) {
            // Thread in kRunning status again...
            fThread.SetStatus(JackThread::kRunning);
            return true;
        } else {
            return false;
        }
	}
}

} // end of namespace