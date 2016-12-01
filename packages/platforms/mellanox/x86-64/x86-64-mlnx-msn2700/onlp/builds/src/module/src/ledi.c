/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *        http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 *
 * </bsn.cl>
 ************************************************************
 *
 *
 *
 ***********************************************************/
#include <onlp/platformi/ledi.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <onlplib/mmap.h>

#include "platform_lib.h"

#define prefix_path "/bsp/led/led_"
#define driver_value_len 50

#define LED_MODE_OFF         "none"
#define LED_MODE_GREEN       "green"
#define LED_MODE_RED         "red"
#define LED_MODE_BLUE        "blue"
#define LED_MODE_GREEN_BLINK "green_blink"
#define LED_MODE_RED_BLINK   "red_blink"
#define LED_MODE_BLUE_BLINK  "blue_blink"
#define LED_MODE_AUTO        "cpld_control"

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_LED(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

/* LED related data
 */
enum onlp_led_id
{
    LED_RESERVED = 0,
    LED_SYSTEM,
    LED_FAN1,
    LED_FAN2,
    LED_FAN3,
    LED_FAN4,
    LED_PSU,
};

typedef struct led_light_mode_map {
    enum onlp_led_id id;
    char* driver_led_mode;
    enum onlp_led_mode_e onlp_led_mode;
} led_light_mode_map_t;

led_light_mode_map_t led_map[] = {
{LED_SYSTEM, LED_MODE_OFF,         ONLP_LED_MODE_OFF},
{LED_SYSTEM, LED_MODE_GREEN,       ONLP_LED_MODE_GREEN},
{LED_SYSTEM, LED_MODE_RED,         ONLP_LED_MODE_RED},
{LED_SYSTEM, LED_MODE_RED_BLINK,   ONLP_LED_MODE_RED_BLINKING},
{LED_SYSTEM, LED_MODE_GREEN_BLINK, ONLP_LED_MODE_GREEN_BLINKING},
{LED_SYSTEM, LED_MODE_AUTO,        ONLP_LED_MODE_AUTO},

{LED_FAN1, LED_MODE_OFF,         ONLP_LED_MODE_OFF},
{LED_FAN1, LED_MODE_GREEN,       ONLP_LED_MODE_GREEN},
{LED_FAN1, LED_MODE_RED,         ONLP_LED_MODE_RED},
{LED_FAN1, LED_MODE_RED_BLINK,   ONLP_LED_MODE_RED_BLINKING},
{LED_FAN1, LED_MODE_GREEN_BLINK, ONLP_LED_MODE_GREEN_BLINKING},
{LED_FAN1, LED_MODE_AUTO,        ONLP_LED_MODE_AUTO},

{LED_FAN2, LED_MODE_OFF,         ONLP_LED_MODE_OFF},
{LED_FAN2, LED_MODE_GREEN,       ONLP_LED_MODE_GREEN},
{LED_FAN2, LED_MODE_RED,         ONLP_LED_MODE_RED},
{LED_FAN2, LED_MODE_RED_BLINK,   ONLP_LED_MODE_RED_BLINKING},
{LED_FAN2, LED_MODE_GREEN_BLINK, ONLP_LED_MODE_GREEN_BLINKING},
{LED_FAN2, LED_MODE_AUTO,        ONLP_LED_MODE_AUTO},

{LED_FAN3, LED_MODE_OFF,         ONLP_LED_MODE_OFF},
{LED_FAN3, LED_MODE_GREEN,       ONLP_LED_MODE_GREEN},
{LED_FAN3, LED_MODE_RED,         ONLP_LED_MODE_RED},
{LED_FAN3, LED_MODE_RED_BLINK,   ONLP_LED_MODE_RED_BLINKING},
{LED_FAN3, LED_MODE_GREEN_BLINK, ONLP_LED_MODE_GREEN_BLINKING},
{LED_FAN3, LED_MODE_AUTO,        ONLP_LED_MODE_AUTO},

{LED_FAN4, LED_MODE_OFF,         ONLP_LED_MODE_OFF},
{LED_FAN4, LED_MODE_GREEN,       ONLP_LED_MODE_GREEN},
{LED_FAN4, LED_MODE_RED,         ONLP_LED_MODE_RED},
{LED_FAN4, LED_MODE_RED_BLINK,   ONLP_LED_MODE_RED_BLINKING},
{LED_FAN4, LED_MODE_GREEN_BLINK, ONLP_LED_MODE_GREEN_BLINKING},
{LED_FAN4, LED_MODE_AUTO,        ONLP_LED_MODE_AUTO},

{LED_PSU, LED_MODE_OFF,         ONLP_LED_MODE_OFF},
{LED_PSU, LED_MODE_GREEN,       ONLP_LED_MODE_GREEN},
{LED_PSU, LED_MODE_RED,         ONLP_LED_MODE_RED},
{LED_PSU, LED_MODE_RED_BLINK,   ONLP_LED_MODE_RED_BLINKING},
{LED_PSU, LED_MODE_GREEN_BLINK, ONLP_LED_MODE_GREEN_BLINKING},
{LED_PSU, LED_MODE_AUTO,        ONLP_LED_MODE_AUTO}
};

static char file_names[][10] =  /* must map with onlp_led_id */
{
    "reserved",
    "status",
    "fan1",
    "fan2",
    "fan3",
    "fan4",
    "psu"
};

/*
 * Get the information for the given LED OID.
 */
static onlp_led_info_t linfo[] =
{
    { }, /* Not used */
    {
        { ONLP_LED_ID_CREATE(LED_SYSTEM), "Chassis LED 1 (SYSTEM LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING |
        ONLP_LED_CAPS_RED | ONLP_LED_CAPS_RED_BLINKING | ONLP_LED_CAPS_AUTO,
    },
    {
        { ONLP_LED_ID_CREATE(LED_FAN1), "Chassis LED 2 (FAN1 LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING |
        ONLP_LED_CAPS_RED | ONLP_LED_CAPS_RED_BLINKING | ONLP_LED_CAPS_AUTO,
    },
    {
        { ONLP_LED_ID_CREATE(LED_FAN2), "Chassis LED 3 (FAN2 LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING |
        ONLP_LED_CAPS_RED | ONLP_LED_CAPS_RED_BLINKING | ONLP_LED_CAPS_AUTO,
    },
    {
        { ONLP_LED_ID_CREATE(LED_FAN3), "Chassis LED 4 (FAN3 LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING |
        ONLP_LED_CAPS_RED | ONLP_LED_CAPS_RED_BLINKING | ONLP_LED_CAPS_AUTO,
    },
    {
        { ONLP_LED_ID_CREATE(LED_FAN4), "Chassis LED 5 (FAN4 LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING |
        ONLP_LED_CAPS_RED | ONLP_LED_CAPS_RED_BLINKING | ONLP_LED_CAPS_AUTO,
    },
    {
        { ONLP_LED_ID_CREATE(LED_PSU), "Chassis LED 6 (PSU LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING |
        ONLP_LED_CAPS_RED | ONLP_LED_CAPS_RED_BLINKING | ONLP_LED_CAPS_AUTO,
    }
};

static int driver_to_onlp_led_mode(enum onlp_led_id id, char* driver_led_mode)
{
    int i, nsize = sizeof(led_map)/sizeof(led_map[0]);

    for (i = 0; i < nsize; i++)
    {
        if (id == led_map[i].id &&
            !strncmp(led_map[i].driver_led_mode, driver_led_mode, driver_value_len))
        {
            return led_map[i].onlp_led_mode;
        }
    }

    return 0;
}

static char* onlp_to_driver_led_mode(enum onlp_led_id id, onlp_led_mode_t onlp_led_mode)
{
    int i, nsize = sizeof(led_map)/sizeof(led_map[0]);

    for (i = 0; i < nsize; i++)
    {
        if (id == led_map[i].id && onlp_led_mode == led_map[i].onlp_led_mode)
        {
            return led_map[i].driver_led_mode;
        }
    }

    return LED_MODE_OFF;
}

/*
 * This function will be called prior to any other onlp_ledi_* functions.
 */
int
onlp_ledi_init(void)
{
    /*
     * TODO setting UI LED to off when it will be supported on SN2700
     */

    return ONLP_STATUS_OK;
}

int
onlp_ledi_info_get(onlp_oid_t id, onlp_led_info_t* info)
{
    int  local_id = 0;
    char data[driver_value_len] = {0};
    char fullpath[50] = {0};

    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);

    /* get fullpath */
    snprintf(fullpath, sizeof(fullpath), "%s%s", prefix_path, file_names[local_id]);

    /* Set the onlp_oid_hdr_t and capabilities */
    *info = linfo[ONLP_OID_ID_GET(id)];

    /* Get LED mode */
    if (deviceNodeReadString(fullpath, data, sizeof(data), 0) != 0) {
        DEBUG_PRINT("%s(%d)\r\n", __FUNCTION__, __LINE__);
        return ONLP_STATUS_E_INTERNAL;
    }

    info->mode = driver_to_onlp_led_mode(local_id, data);

    /* Set the on/off status */
    if (info->mode != ONLP_LED_MODE_OFF) {
        info->status |= ONLP_LED_STATUS_ON;
    }

    return ONLP_STATUS_OK;
}

/*
 * Turn an LED on or off.
 *
 * This function will only be called if the LED OID supports the ONOFF
 * capability.
 *
 * What 'on' means in terms of colors or modes for multimode LEDs is
 * up to the platform to decide. This is intended as baseline toggle mechanism.
 */
int
onlp_ledi_set(onlp_oid_t id, int on_or_off)
{
    VALIDATE(id);

    if (!on_or_off) {
        return onlp_ledi_mode_set(id, ONLP_LED_MODE_OFF);
    }

    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * This function puts the LED into the given mode. It is a more functional
 * interface for multimode LEDs.
 *
 * Only modes reported in the LED's capabilities will be attempted.
 */
int
onlp_ledi_mode_set(onlp_oid_t id, onlp_led_mode_t mode)
{
    int  local_id;
    char fullpath[50] = {0};

    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);
    snprintf(fullpath, sizeof(fullpath), "%s%s", prefix_path, file_names[local_id]);

    if (deviceNodeWrite(fullpath, onlp_to_driver_led_mode(local_id, mode), driver_value_len, 0) != 0)
    {
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/*
 * Generic LED ioctl interface.
 */
int
onlp_ledi_ioctl(onlp_oid_t id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

