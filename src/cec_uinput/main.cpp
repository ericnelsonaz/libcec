/*
 * This file is part of the libCEC(R) library.
 *
 * libCEC(R) is Copyright (C) 2011-2013 Pulse-Eight Limited.  All rights reserved.
 * libCEC(R) is an original work, containing original code.
 *
 * libCEC(R) is a trademark of Pulse-Eight Limited.
 *
 * This program is dual-licensed; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Alternatively, you can license this library under a commercial license,
 * please contact Pulse-Eight Licensing for more information.
 *
 * For more information contact:
 * Pulse-Eight Licensing       <license@pulse-eight.com>
 *     http://www.pulse-eight.com/
 *     http://www.pulse-eight.net/
 */

#include "../env.h"
#include "../include/cec.h"

#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <signal.h>
#include "../lib/platform/os.h"
#include "../lib/implementations/CECCommandHandler.h"
#include "../lib/platform/util/StdString.h"
#include "../lib/platform/threads/threads.h"

using namespace CEC;
using namespace std;
using namespace PLATFORM;
#include "../../include/cecloader.h"

#define CEC_CONFIG_VERSION CEC_CLIENT_VERSION_CURRENT;

int CecLogMessage(void *cbParam, const cec_log_message message)
{
	printf("%s: level %d, message %s, cbParam %p\n", __func__,
	       message.level, message.message, cbParam);
	return 0;
}

int CecKeyPress(void *UNUSED(cbParam), const cec_keypress UNUSED(key))
{
	printf("%s\n", __func__);
	return 0;
}

int CecCommand(void *UNUSED(cbParam), const cec_command UNUSED(command))
{
	printf("%s\n", __func__);
	return 0;
}

int CecAlert(void *cbParam, const libcec_alert type, const libcec_parameter param)
{
	printf("%s: type %d, param %d/%p, cbparam %p\n", __func__, type, param.paramType, param.paramData, cbParam);
	if (CEC_ALERT_CONNECTION_LOST == type) {
		printf("connection lost\n");
	}
	return 0;
}

int main (int argc, char *argv[])
{
	for (int i = 0; i < argc; i++) {
		printf("argv[%d] == %s\n", i, argv[i]);
	}

	printf("creating configuration\n");
	libcec_configuration g_config;
	g_config.deviceTypes.Add(CEC_DEVICE_TYPE_RECORDING_DEVICE);
	strcpy(g_config.strDeviceName, "myosdname");
	g_config.bActivateSource     = 1;

	ICECCallbacks g_callbacks;
	g_callbacks.CBCecLogMessage  = &CecLogMessage;
	g_callbacks.CBCecKeyPress    = &CecKeyPress;
	g_callbacks.CBCecCommand     = &CecCommand;
	g_callbacks.CBCecAlert       = &CecAlert;
	g_config.callbacks           = &g_callbacks;

	ICECAdapter* g_parser = LibCecInitialise(&g_config);
	cec_adapter devices[10];
	uint8_t found = g_parser->FindAdapters(devices, 10, NULL);
	if (found <= 0) {
		printf("no devices found\n");
		return 1;
	}
	printf("%d devices found\n", found);
	if (!g_parser->Open(devices[0].comm)){
		printf("Error opening %s\n", devices[0].comm);
	}
	printf("device %s opened\n", devices[0].comm);

	while (1)
	{
		CEvent::Sleep(50);
	}

	g_parser->Close();
	UnloadLibCec(g_parser);
	return 0;
}
