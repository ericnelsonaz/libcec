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
#include "keymap.h"

#include <linux/input.h>
#include <linux/uinput.h>
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <limits.h>
#include <string>
#include <sstream>
#include <signal.h>
#include <stdlib.h>
#include <syslog.h>
#include "../lib/CECTypeUtils.h"
#include "../lib/platform/os.h"
#include "../lib/implementations/CECCommandHandler.h"
#include "../lib/platform/util/StdString.h"
#include "../lib/platform/threads/threads.h"

using namespace CEC;
using namespace std;
using namespace PLATFORM;
#include "../../include/cecloader.h"

#define CEC_CONFIG_VERSION CEC_CLIENT_VERSION_CURRENT;

/*
 * Many keys don't pass transparently through to our app,
 * so hack around this by using placeholders for power on
 * and power off events.
 *
 * When they hit Javascript, the event.keyCode value will be
 *  221 for power-off and
 *  219 for power-on
 */
#define KEY_POWERON KEY_LEFTBRACE
#define KEY_POWEROFF KEY_RIGHTBRACE

#define ARRAY_SIZE(__arr) (sizeof(__arr)/sizeof((__arr[0])))

#define STATE_UNKNOWN   -1
#define STATE_OFF       0
#define STATE_ON        1

typedef struct {
    libcec_configuration config;
    ICECCallbacks callbacks;
    ICECAdapter* adapter;
    int uifd;
    int tv_state;
} app_data_t ;

int CecLogMessage(void *cbParam, const cec_log_message message)
{
    printf("%s: level %d, message %s, cbParam %p\n", __func__,
           message.level, message.message, cbParam);
    return 0;
}

struct key_map_t default_key_mapping[] = {
    { CEC_USER_CONTROL_CODE_SELECT,                         KEY_SELECT },
    { CEC_USER_CONTROL_CODE_UP,                             KEY_UP },
    { CEC_USER_CONTROL_CODE_DOWN,                           KEY_DOWN },
    { CEC_USER_CONTROL_CODE_LEFT,                           KEY_LEFT },
    { CEC_USER_CONTROL_CODE_RIGHT,                          KEY_RIGHT },
    { CEC_USER_CONTROL_CODE_RIGHT_UP,                       KEY_UP },
    { CEC_USER_CONTROL_CODE_RIGHT_DOWN,                     KEY_DOWN },
    { CEC_USER_CONTROL_CODE_LEFT_UP,                        KEY_UP },
    { CEC_USER_CONTROL_CODE_LEFT_DOWN,                      KEY_DOWN },
    { CEC_USER_CONTROL_CODE_ROOT_MENU,                      KEY_MENU },
    { CEC_USER_CONTROL_CODE_SETUP_MENU,                     KEY_MENU },
    { CEC_USER_CONTROL_CODE_CONTENTS_MENU,                  KEY_MENU },
    { CEC_USER_CONTROL_CODE_FAVORITE_MENU,                  KEY_MENU },
    { CEC_USER_CONTROL_CODE_EXIT,                           KEY_ESC },
    { CEC_USER_CONTROL_CODE_TOP_MENU,                       KEY_MENU },
    { CEC_USER_CONTROL_CODE_DVD_MENU,                       KEY_MENU },
    { CEC_USER_CONTROL_CODE_NUMBER_ENTRY_MODE,              KEY_NUMLOCK },
    { CEC_USER_CONTROL_CODE_NUMBER11,                       KEY_F11 },
    { CEC_USER_CONTROL_CODE_NUMBER12,                       KEY_F12 },
    { CEC_USER_CONTROL_CODE_NUMBER0,                        KEY_0 },
    { CEC_USER_CONTROL_CODE_NUMBER1,                        KEY_1 },
    { CEC_USER_CONTROL_CODE_NUMBER2,                        KEY_2 },
    { CEC_USER_CONTROL_CODE_NUMBER3,                        KEY_3 },
    { CEC_USER_CONTROL_CODE_NUMBER4,                        KEY_4 },
    { CEC_USER_CONTROL_CODE_NUMBER5,                        KEY_5 },
    { CEC_USER_CONTROL_CODE_NUMBER6,                        KEY_6 },
    { CEC_USER_CONTROL_CODE_NUMBER7,                        KEY_7 },
    { CEC_USER_CONTROL_CODE_NUMBER8,                        KEY_8 },
    { CEC_USER_CONTROL_CODE_NUMBER9,                        KEY_9 },
    { CEC_USER_CONTROL_CODE_DOT,                            KEY_DOT },
    { CEC_USER_CONTROL_CODE_ENTER,                          KEY_ENTER },
    { CEC_USER_CONTROL_CODE_CLEAR,                          KEY_CLEAR },
    { CEC_USER_CONTROL_CODE_NEXT_FAVORITE,                  KEY_FAVORITES },
    { CEC_USER_CONTROL_CODE_CHANNEL_UP,                     KEY_PAGEUP },
    { CEC_USER_CONTROL_CODE_CHANNEL_DOWN,                   KEY_PAGEDOWN },
    { CEC_USER_CONTROL_CODE_PREVIOUS_CHANNEL,               KEY_PREVIOUS },
    { CEC_USER_CONTROL_CODE_SOUND_SELECT,                   KEY_SOUND },
    { CEC_USER_CONTROL_CODE_INPUT_SELECT,                   KEY_RESERVED },
    { CEC_USER_CONTROL_CODE_DISPLAY_INFORMATION,            KEY_INFO },
    { CEC_USER_CONTROL_CODE_HELP,                           KEY_HELP },
    { CEC_USER_CONTROL_CODE_PAGE_UP,                        KEY_PAGEUP },
    { CEC_USER_CONTROL_CODE_PAGE_DOWN,                      KEY_PAGEDOWN },
    { CEC_USER_CONTROL_CODE_POWER,                          KEY_POWER },
    { CEC_USER_CONTROL_CODE_VOLUME_UP,                      KEY_VOLUMEUP },
    { CEC_USER_CONTROL_CODE_VOLUME_DOWN,                    KEY_VOLUMEDOWN },
    { CEC_USER_CONTROL_CODE_MUTE,                           KEY_MUTE },
    { CEC_USER_CONTROL_CODE_PLAY,                           KEY_PLAY },
    { CEC_USER_CONTROL_CODE_STOP,                           KEY_STOP },
    { CEC_USER_CONTROL_CODE_PAUSE,                          KEY_PAUSE },
    { CEC_USER_CONTROL_CODE_RECORD,                         KEY_RECORD },
    { CEC_USER_CONTROL_CODE_REWIND,                         KEY_REWIND },
    { CEC_USER_CONTROL_CODE_FAST_FORWARD,                   KEY_FASTFORWARD },
    { CEC_USER_CONTROL_CODE_EJECT,                          KEY_EJECTCD },
    { CEC_USER_CONTROL_CODE_FORWARD,                        KEY_FORWARD },
    { CEC_USER_CONTROL_CODE_BACKWARD,                       KEY_BACK },
    { CEC_USER_CONTROL_CODE_STOP_RECORD,                    KEY_STOPCD },
    { CEC_USER_CONTROL_CODE_PAUSE_RECORD,                   KEY_PAUSECD },
    { CEC_USER_CONTROL_CODE_ANGLE,                          KEY_ANGLE },
    { CEC_USER_CONTROL_CODE_SUB_PICTURE,                    KEY_RESERVED },
    { CEC_USER_CONTROL_CODE_VIDEO_ON_DEMAND,                KEY_VIDEO },
    { CEC_USER_CONTROL_CODE_ELECTRONIC_PROGRAM_GUIDE,       KEY_PROGRAM },
    { CEC_USER_CONTROL_CODE_TIMER_PROGRAMMING,              KEY_TIME },
    { CEC_USER_CONTROL_CODE_INITIAL_CONFIGURATION,          KEY_CONFIG },
    { CEC_USER_CONTROL_CODE_SELECT_BROADCAST_TYPE,          KEY_RESERVED },
    { CEC_USER_CONTROL_CODE_SELECT_SOUND_PRESENTATION,      KEY_PRESENTATION },
    { CEC_USER_CONTROL_CODE_PLAY_FUNCTION,                  KEY_PLAY},
    { CEC_USER_CONTROL_CODE_PAUSE_PLAY_FUNCTION,            KEY_PLAYPAUSE },
    { CEC_USER_CONTROL_CODE_RECORD_FUNCTION,                KEY_RECORD },
    { CEC_USER_CONTROL_CODE_PAUSE_RECORD_FUNCTION,          KEY_PAUSECD },
    { CEC_USER_CONTROL_CODE_STOP_FUNCTION,                  KEY_STOP },
    { CEC_USER_CONTROL_CODE_MUTE_FUNCTION,                  KEY_MUTE },
    { CEC_USER_CONTROL_CODE_RESTORE_VOLUME_FUNCTION,        KEY_RESERVED },
    { CEC_USER_CONTROL_CODE_TUNE_FUNCTION,                  KEY_RESERVED },
    { CEC_USER_CONTROL_CODE_SELECT_MEDIA_FUNCTION,          KEY_MEDIA },
    { CEC_USER_CONTROL_CODE_SELECT_AV_INPUT_FUNCTION,       KEY_RESERVED },
    { CEC_USER_CONTROL_CODE_SELECT_AUDIO_INPUT_FUNCTION,    KEY_AUDIO },
    { CEC_USER_CONTROL_CODE_POWER_TOGGLE_FUNCTION,          KEY_POWER },
    { CEC_USER_CONTROL_CODE_POWER_OFF_FUNCTION,             KEY_POWEROFF },
    { CEC_USER_CONTROL_CODE_POWER_ON_FUNCTION,              KEY_POWERON },
    { CEC_USER_CONTROL_CODE_F1_BLUE,                        KEY_F1 },
    { CEC_USER_CONTROL_CODE_F2_RED,                         KEY_F2 },
    { CEC_USER_CONTROL_CODE_F3_GREEN,                       KEY_F3 },
    { CEC_USER_CONTROL_CODE_F4_YELLOW,                      KEY_F4 },
    { CEC_USER_CONTROL_CODE_F5,                             KEY_F5 },
    { CEC_USER_CONTROL_CODE_DATA,                           KEY_DATABASE },
    { CEC_USER_CONTROL_CODE_AN_RETURN,                      KEY_EXIT },
    { CEC_USER_CONTROL_CODE_AN_CHANNELS_LIST,               KEY_RESERVED },
    { CEC_POWER_ON,                                         KEY_LEFTBRACE },
    { CEC_POWER_OFF,                                        KEY_RIGHTBRACE },
    { CEC_USER_CONTROL_CODE_UNKNOWN,                        KEY_UNKNOWN },
};

static struct key_map_t *key_map = default_key_mapping;
static unsigned num_keys = ARRAY_SIZE(default_key_mapping);

struct input_event const sync_event = {
    {0}, // time
    EV_SYN, // type
    0,      // code
    0,      // value
};

static void send_key(app_data_t const &data,
                     struct input_event const &ev)
{
    int count = write(data.uifd, &ev, sizeof(ev));
    if (count != sizeof(ev))
        perror("write(input_event)");
    else {
        write(data.uifd, &sync_event, sizeof(sync_event));
    }
}

static int compare_key_map(const void *key, const void *mapent)
{
    struct key_map_t const *target = (struct key_map_t *)mapent;
    cec_user_control_code code = *(cec_user_control_code const *)key;
    return code-target->cec_code;
}

int CecKeyPress(void *cbParam, const cec_keypress key)
{
    app_data_t const *data = (app_data_t const *)cbParam;
    struct key_map_t *ent = (struct key_map_t *)
                            bsearch(&key.keycode,
                                    key_map,
                                    num_keys,
                                    sizeof(*key_map),
                                    compare_key_map);
    if (ent) {
        struct input_event ev;
        ev.type = EV_KEY;
        ev.code = ent->input_key;
        ev.value = (0 == key.duration);
        send_key(*data, ev);
        syslog(LOG_INFO|LOG_USER, "cec 0x%02x -> 0x%02x", key.keycode, ent->input_key);
    } else
        syslog(LOG_INFO|LOG_USER, "no mapping for cec code 0x%02x", key.keycode);

    return 0;
}

int CecCommand(void *cbParam, const cec_command command)
{
    app_data_t *data = (app_data_t *)cbParam;
    cec_user_control_code cec_code = CEC_USER_CONTROL_CODE_UNKNOWN;

    printf("%s: message from %s to %s\n",
           __func__,
           CCECTypeUtils::ToString(command.initiator),
           CCECTypeUtils::ToString(command.destination));
    if (CECDEVICE_TV == command.initiator) {
        if (CEC_OPCODE_STANDBY == command.opcode) {
            printf("--- standby\n");
            cec_code = CEC_POWER_OFF;
        } else if (CEC_OPCODE_ROUTING_CHANGE == command.opcode) {
            uint16_t iNewAddress = ((uint16_t)command.parameters[2] << 8) | ((uint16_t)command.parameters[3]);
            printf("--- new active route 0x%04x\n", iNewAddress);
            if (CEC_DEVICE_TYPE_TV == command.parameters[3]) {
                printf("--- TV is now active\n");
                cec_code = CEC_POWER_ON;
            }
        } else if (CEC_OPCODE_REPORT_POWER_STATUS == command.opcode) {
            cec_power_status powerStatus = (cec_power_status)command.parameters[0];
            bool on = (CEC_POWER_STATUS_ON == powerStatus);
            printf("--- power status %d (%s)\n", powerStatus, on ? "on" : "off or standby");
            cec_code = on ? CEC_POWER_ON : CEC_POWER_OFF;
        } else if (CEC_OPCODE_ACTIVE_SOURCE == command.opcode) {
            uint16_t iAddress = ((uint16_t)command.parameters[0] << 8) | ((uint16_t)command.parameters[1]);
            printf("--- set active source %d\n", iAddress);
            if (CEC_DEVICE_TYPE_TV == iAddress) {
                printf("--- power on here\n");
                cec_code = CEC_POWER_ON;
            }
        } else
            printf("unknown opcode 0x%x\n", command.opcode);

        if (cec_code != CEC_USER_CONTROL_CODE_UNKNOWN) {
            cec_keypress keypress;
            keypress.keycode = cec_code;

            // press
            keypress.duration = 10;
            CecKeyPress(cbParam, keypress);

            // release
            keypress.duration = 0;
            CecKeyPress(cbParam, keypress);

            if ((cec_code == CEC_POWER_ON)
                &&
                (data->tv_state < STATE_ON)) {
                /* power on. select our output */
                printf("--- set the active source to our output\n");
                data->adapter->SetActiveSource();
            }
            data->tv_state = (cec_code == CEC_POWER_OFF)
                            ? STATE_OFF : STATE_ON;
        }
    } else
        printf("not TV\n");

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
    openlog(0,0,0);
    libcec_configuration g_config;
    g_config.deviceTypes.Add(CEC_DEVICE_TYPE_RECORDING_DEVICE);
    strcpy(g_config.strDeviceName, "myosdname");
    app_data_t data;

    data.tv_state = STATE_UNKNOWN;
    data.config.deviceTypes.Add(CEC_DEVICE_TYPE_RECORDING_DEVICE);
    if (1 < argc)
        strcpy(data.config.strDeviceName, argv[1]);

    data.config.bActivateSource     = 0;
    if (2 < argc) {
        data.config.iPhysicalAddress = strtoul(argv[2], 0, 0);
        if (3 < argc) {
            struct key_map_t *map;
            unsigned count = read_key_map(argv[3], &map);
            if (count) {
                char fullname[PATH_MAX];
                syslog(LOG_INFO|LOG_USER, "using keys from %s",
                       realpath(argv[3], fullname));
                key_map = map;
                num_keys = count;
            }
        }
    }

    if (key_map == default_key_mapping)
        syslog(LOG_INFO|LOG_USER,
               "using default (compiled-in) key mappings");

    data.callbacks.CBCecLogMessage  = &CecLogMessage;
    data.callbacks.CBCecKeyPress    = &CecKeyPress;
    data.callbacks.CBCecCommand     = &CecCommand;
    data.callbacks.CBCecAlert       = &CecAlert;
    data.config.callbacks           = &data.callbacks;
    data.config.callbackParam   = &data;

    ICECAdapter* adapter = data.adapter = LibCecInitialise(&data.config);
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

    for (unsigned i=0; i < num_keys; i++) {
        ioctl(data.uifd, UI_SET_KEYBIT, key_map[i].input_key);
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
