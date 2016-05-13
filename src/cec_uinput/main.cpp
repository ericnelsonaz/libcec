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

#include <linux/input.h>
#include <linux/uinput.h>
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

#define ARRAY_SIZE(__arr) (sizeof(__arr)/sizeof((__arr[0])))

typedef struct {
	libcec_configuration config;
	ICECCallbacks callbacks;
	int uifd;
} app_data_t ;

int CecLogMessage(void *cbParam, const cec_log_message message)
{
	printf("%s: level %d, message %s, cbParam %p\n", __func__,
	       message.level, message.message, cbParam);
	return 0;
}

typedef struct {
        cec_user_control_code	cec;
	int			input;
} key_map_t;

key_map_t const key_mapping[] = {
	{ CEC_USER_CONTROL_CODE_SELECT,				KEY_SELECT },
	{ CEC_USER_CONTROL_CODE_UP,				KEY_UP },
	{ CEC_USER_CONTROL_CODE_DOWN,				KEY_DOWN },
	{ CEC_USER_CONTROL_CODE_LEFT,				KEY_LEFT },
	{ CEC_USER_CONTROL_CODE_RIGHT,				KEY_RIGHT },
	{ CEC_USER_CONTROL_CODE_RIGHT_UP,			KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_RIGHT_DOWN,			KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_LEFT_UP,			KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_LEFT_DOWN,			KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_ROOT_MENU,			KEY_MENU },
	{ CEC_USER_CONTROL_CODE_SETUP_MENU,			KEY_MENU },
	{ CEC_USER_CONTROL_CODE_CONTENTS_MENU,			KEY_MENU },
	{ CEC_USER_CONTROL_CODE_FAVORITE_MENU,			KEY_MENU },
	{ CEC_USER_CONTROL_CODE_EXIT,				KEY_EXIT },
	{ CEC_USER_CONTROL_CODE_TOP_MENU,			KEY_MENU },
	{ CEC_USER_CONTROL_CODE_DVD_MENU,			KEY_MENU },
	{ CEC_USER_CONTROL_CODE_NUMBER_ENTRY_MODE,		KEY_NUMLOCK },
	{ CEC_USER_CONTROL_CODE_NUMBER11,			KEY_F11 },
	{ CEC_USER_CONTROL_CODE_NUMBER12,			KEY_F12 },
	{ CEC_USER_CONTROL_CODE_NUMBER0,			KEY_0 },
	{ CEC_USER_CONTROL_CODE_NUMBER1,			KEY_1 },
	{ CEC_USER_CONTROL_CODE_NUMBER2,			KEY_2 },
	{ CEC_USER_CONTROL_CODE_NUMBER3,			KEY_3 },
	{ CEC_USER_CONTROL_CODE_NUMBER4,			KEY_4 },
	{ CEC_USER_CONTROL_CODE_NUMBER5,			KEY_5 },
	{ CEC_USER_CONTROL_CODE_NUMBER6,			KEY_6 },
	{ CEC_USER_CONTROL_CODE_NUMBER7,			KEY_7 },
	{ CEC_USER_CONTROL_CODE_NUMBER8,			KEY_8 },
	{ CEC_USER_CONTROL_CODE_NUMBER9,			KEY_9 },
	{ CEC_USER_CONTROL_CODE_DOT,				KEY_DOT },
	{ CEC_USER_CONTROL_CODE_ENTER,				KEY_ENTER },
	{ CEC_USER_CONTROL_CODE_CLEAR,				KEY_CLEAR },
	{ CEC_USER_CONTROL_CODE_NEXT_FAVORITE,			KEY_FAVORITES },
	{ CEC_USER_CONTROL_CODE_CHANNEL_UP,			KEY_CHANNELUP },
	{ CEC_USER_CONTROL_CODE_CHANNEL_DOWN,			KEY_CHANNELDOWN },
	{ CEC_USER_CONTROL_CODE_PREVIOUS_CHANNEL,		KEY_PREVIOUS },
	{ CEC_USER_CONTROL_CODE_SOUND_SELECT,			KEY_SOUND },
	{ CEC_USER_CONTROL_CODE_INPUT_SELECT,			KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_DISPLAY_INFORMATION,		KEY_INFO },
	{ CEC_USER_CONTROL_CODE_HELP,				KEY_HELP },
	{ CEC_USER_CONTROL_CODE_PAGE_UP,			KEY_PAGEUP },
	{ CEC_USER_CONTROL_CODE_PAGE_DOWN,			KEY_PAGEDOWN },
	{ CEC_USER_CONTROL_CODE_POWER,				KEY_POWER },
	{ CEC_USER_CONTROL_CODE_VOLUME_UP,			KEY_VOLUMEUP },
	{ CEC_USER_CONTROL_CODE_VOLUME_DOWN,			KEY_VOLUMEDOWN },
	{ CEC_USER_CONTROL_CODE_MUTE,				KEY_MUTE },
	{ CEC_USER_CONTROL_CODE_PLAY,				KEY_PLAY },
	{ CEC_USER_CONTROL_CODE_STOP,				KEY_STOP },
	{ CEC_USER_CONTROL_CODE_PAUSE,				KEY_PAUSE },
	{ CEC_USER_CONTROL_CODE_RECORD,				KEY_RECORD },
	{ CEC_USER_CONTROL_CODE_REWIND,				KEY_REWIND },
	{ CEC_USER_CONTROL_CODE_FAST_FORWARD,			KEY_FASTFORWARD },
	{ CEC_USER_CONTROL_CODE_EJECT,				KEY_EJECTCD },
	{ CEC_USER_CONTROL_CODE_FORWARD,			KEY_FORWARD },
	{ CEC_USER_CONTROL_CODE_BACKWARD,			KEY_BACK },
	{ CEC_USER_CONTROL_CODE_STOP_RECORD,			KEY_STOPCD },
	{ CEC_USER_CONTROL_CODE_PAUSE_RECORD,			KEY_PAUSECD },
	{ CEC_USER_CONTROL_CODE_ANGLE,				KEY_ANGLE },
	{ CEC_USER_CONTROL_CODE_SUB_PICTURE,			KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_VIDEO_ON_DEMAND,		KEY_VIDEO },
	{ CEC_USER_CONTROL_CODE_ELECTRONIC_PROGRAM_GUIDE,	KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_TIMER_PROGRAMMING,		KEY_TIME },
	{ CEC_USER_CONTROL_CODE_INITIAL_CONFIGURATION,		KEY_CONFIG },
	{ CEC_USER_CONTROL_CODE_SELECT_BROADCAST_TYPE,		KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_SELECT_SOUND_PRESENTATION,	KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_PLAY_FUNCTION,			KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_PAUSE_PLAY_FUNCTION,		KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_RECORD_FUNCTION,		KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_PAUSE_RECORD_FUNCTION,		KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_STOP_FUNCTION,			KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_MUTE_FUNCTION,			KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_RESTORE_VOLUME_FUNCTION,	KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_TUNE_FUNCTION,			KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_SELECT_MEDIA_FUNCTION,		KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_SELECT_AV_INPUT_FUNCTION,	KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_SELECT_AUDIO_INPUT_FUNCTION,	KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_POWER_TOGGLE_FUNCTION,		KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_POWER_OFF_FUNCTION,		KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_POWER_ON_FUNCTION,		KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_F1_BLUE,			KEY_F1 },
	{ CEC_USER_CONTROL_CODE_F2_RED,				KEY_F2 },
	{ CEC_USER_CONTROL_CODE_F3_GREEN,			KEY_F3 },
	{ CEC_USER_CONTROL_CODE_F4_YELLOW,			KEY_F4 },
	{ CEC_USER_CONTROL_CODE_F5,				KEY_F5 },
	{ CEC_USER_CONTROL_CODE_DATA,				KEY_DATABASE },
	{ CEC_USER_CONTROL_CODE_AN_RETURN,			KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_AN_CHANNELS_LIST,		KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_MAX,				KEY_RESERVED },
	{ CEC_USER_CONTROL_CODE_UNKNOWN,			KEY_RESERVED },
};

int CecKeyPress(void *cbParam, const cec_keypress key)
{
	app_data_t const *data = (app_data_t const *)cbParam;
	int inputcode = KEY_RESERVED;
	for (unsigned i = 0; i < ARRAY_SIZE(key_mapping); i++) {
		if (key.keycode == key_mapping[i].cec) {
			inputcode = key_mapping[i].input;
			break;
		}
	}
	printf("%s: key 0x%02x, duration %u\n", __func__, key.keycode, key.duration);
	if (KEY_RESERVED != inputcode) {
		struct input_event ev;
		ev.type = EV_KEY;
		ev.code = inputcode;
		ev.value = (0 == key.duration);
                int count = write(data->uifd, &ev, sizeof(ev));
		if (count != sizeof(ev))
			perror("write(input_event)");
		else {
			ev.type = EV_SYN;
			ev.code = 0;
			ev.value = 0;
                        write(data->uifd, &ev, sizeof(ev));
			printf("---sync\n");
		}
	}
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
	libcec_configuration g_config;
	g_config.deviceTypes.Add(CEC_DEVICE_TYPE_RECORDING_DEVICE);
	strcpy(g_config.strDeviceName, "myosdname");
        app_data_t data;

	data.config.deviceTypes.Add(CEC_DEVICE_TYPE_RECORDING_DEVICE);
	if (1 < argc)
	        strcpy(data.config.strDeviceName, argv[1]);

	data.config.bActivateSource     = 0;
	if (2 < argc)
		data.config.iPhysicalAddress = strtoul(argv[2], 0, 0);;

	data.callbacks.CBCecLogMessage  = &CecLogMessage;
	data.callbacks.CBCecKeyPress    = &CecKeyPress;
	data.callbacks.CBCecCommand     = &CecCommand;
	data.callbacks.CBCecAlert       = &CecAlert;
	data.config.callbacks           = &data.callbacks;
	data.config.callbackParam	= &data;

	ICECAdapter* adapter = LibCecInitialise(&data.config);
	cec_adapter devices[10];
	uint8_t found = adapter->FindAdapters(devices, 10, NULL);
	if (found <= 0) {
		printf("no devices found\n");
		return 1;
	}
	if (!adapter->Open(devices[0].comm)){
		printf("Error opening %s\n", devices[0].comm);
	}

	data.uifd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	if(data.uifd < 0) {
		perror("/dev/uinput");
                return -1;
	}

	if (0 != ioctl(data.uifd, UI_SET_EVBIT, EV_KEY)) {
		perror("UI_SET_EVBIT(KEY)");
		return -1;
	}

	if (0 != ioctl(data.uifd, UI_SET_EVBIT, EV_SYN)) {
		perror("UI_SET_EVBIT(SYN)");
		return -1;
	}

	for (unsigned i=0; i < ARRAY_SIZE(key_mapping); i++) {
		ioctl(data.uifd, UI_SET_KEYBIT, key_mapping[i].input);
	}

	struct uinput_user_dev uidev;
	memset(&uidev, 0, sizeof(uidev));
        snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "cec-kbd");
	uidev.id.bustype = BUS_VIRTUAL;
	uidev.id.vendor  = 0x1234;
	uidev.id.product = 0xfedc;
	uidev.id.version = 1;
	write(data.uifd, &uidev, sizeof(uidev));

	if (0 != ioctl(data.uifd, UI_DEV_CREATE)) {
		perror("UI_DEV_CREATE");
		return -1;
	}

	while (1)
	{
		CEvent::Sleep(50);
	}

	adapter->Close();
	UnloadLibCec(adapter);
	return 0;
}
