/*
Serval DNA daemon
Copyright (C) 2012 Serval Project Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <signal.h>
#include "serval.h"
#include "conf.h"

static void crash_handler(int signal);


char crash_handler_clue[1024] = "no clue";

static void crash_handler(int signal)
{
  LOGF(LOG_LEVEL_FATAL, "Caught signal %s", alloca_signal_name(signal));
  LOGF(LOG_LEVEL_FATAL, "The following clue may help: %s", crash_handler_clue);
  dump_stack(LOG_LEVEL_FATAL);
  BACKTRACE;
  // Now die of the same signal, so that our exit status reflects the cause.
  INFOF("Re-sending signal %d to self", signal);
  kill(getpid(), signal);
  // If that didn't work, then die normally.
  INFOF("exit(%d)", -signal);
  exit(-signal);
}

