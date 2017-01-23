#include <stdio.h>
#include <syslog.h>
#include "input-keys.h"
#include <linux/input.h>
#include <linux/uinput.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#define KEY_POWERON KEY_LEFTBRACE
#define KEY_POWEROFF KEY_RIGHTBRACE

struct input_event const sync_event = {
    {0}, // time
    EV_SYN, // type
    0,      // code
    0,      // value
};

static void send_key(int uifd, struct input_event const &ev)
{
    int count = write(uifd, &ev, sizeof(ev));
    if (count != sizeof(ev))
        perror("write(input_event)");
    else {
        write(uifd, &sync_event, sizeof(sync_event));
    }
}

int main (int argc, char *argv[])
{
    openlog(0,0,0);
    int uifd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if(uifd < 0) {
        perror("/dev/uinput");
                return -1;
    }

    if (0 != ioctl(uifd, UI_SET_EVBIT, EV_KEY)) {
        perror("UI_SET_EVBIT(KEY)");
        return -1;
    }

    if (0 != ioctl(uifd, UI_SET_EVBIT, EV_SYN)) {
        perror("UI_SET_EVBIT(SYN)");
        return -1;
    }

    for (unsigned i = 0; i < num_input_keys; i++) {
        ioctl(uifd, UI_SET_KEYBIT, input_keys[i].ival);
    }
    ioctl(uifd, UI_SET_KEYBIT, KEY_POWERON);
    ioctl(uifd, UI_SET_KEYBIT, KEY_POWEROFF);

    struct uinput_user_dev uidev;
    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "%s", argv[0]);

    uidev.id.bustype = BUS_VIRTUAL;
    uidev.id.vendor  = 0x1234;
    uidev.id.product = 0xfedc;
    uidev.id.version = 1;
    write(uifd, &uidev, sizeof(uidev));

    if (0 != ioctl(uifd, UI_DEV_CREATE)) {
        perror("UI_DEV_CREATE");
        return -1;
    }

    struct input_event ev;
    ev.type = EV_KEY;
    for (unsigned i = 0; i < num_input_keys; i++) {
        printf("key(0x%03x) == %s\n",
               input_keys[i].ival,
               input_keys[i].sval);
        syslog(LOG_USER|LOG_INFO,
               "key(0x%03x) == %s\n",
               input_keys[i].ival,
               input_keys[i].sval);
        ev.code = input_keys[i].ival;
        ev.value = 1;
        send_key(uifd, ev);
        ev.value = 0;
        send_key(uifd, ev);
        sleep(1);
    }
    closelog();
    return 0;
}
