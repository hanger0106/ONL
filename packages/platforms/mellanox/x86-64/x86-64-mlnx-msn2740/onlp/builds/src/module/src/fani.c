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
 * Fan Platform Implementation Defaults.
 *
 ***********************************************************/
#include <fcntl.h>
#include <onlplib/file.h>
#include <onlplib/mmap.h>
#include <onlp/platformi/fani.h>
#include "platform_lib.h"

#define PREFIX_PATH        "/bsp/fan/"
#define PREFIX_MODULE_PATH "/bsp/module/"

#define FAN_STATUS_OK  1

#define PERCENTAGE_MIN 60.0
#define PERCENTAGE_MAX 100.0
#define RPM_MAGIC_MIN  153.0
#define RPM_MAGIC_MAX  255.0

#define PSU_FAN_RPM_MIN 11700.0
#define PSU_FAN_RPM_MAX 19500.0

#define PROJECT_NAME
#define LEN_FILE_NAME 80

#define FAN_RESERVED        0
#define FAN_1_ON_MAIN_BOARD 1
#define FAN_2_ON_MAIN_BOARD 2
#define FAN_3_ON_MAIN_BOARD 3
#define FAN_4_ON_MAIN_BOARD 4

#define FAN_1_ON_PSU1       5
#define FAN_1_ON_PSU2       6

static int min_fan_speed[CHASSIS_FAN_COUNT+1] = {0};
static int max_fan_speed[CHASSIS_FAN_COUNT+1] = {0};

typedef struct fan_path_S
{
    char status[LEN_FILE_NAME];
    char r_speed_get[LEN_FILE_NAME];
    char r_speed_set[LEN_FILE_NAME];
    char min[LEN_FILE_NAME];
    char max[LEN_FILE_NAME];
}fan_path_T;

#define _MAKE_FAN_PATH_ON_MAIN_BOARD(prj,id) \
    { #prj"fan"#id"_status", \
      #prj"fan"#id"_speed_get", \
      #prj"fan"#id"_speed_set", \
      #prj"fan"#id"_min", \
      #prj"fan"#id"_max" }

#define MAKE_FAN_PATH_ON_MAIN_BOARD(prj,id) _MAKE_FAN_PATH_ON_MAIN_BOARD(prj,id)

#define MAKE_FAN_PATH_ON_PSU(psu_id, fan_id) \
    {"psu"#psu_id"_status", \
     "psu"#psu_id"_fan"#fan_id"_speed_get", "", "", "",}

static fan_path_T fan_path[] =  /* must map with onlp_fan_id */
{
    MAKE_FAN_PATH_ON_MAIN_BOARD(PROJECT_NAME, FAN_RESERVED),
    MAKE_FAN_PATH_ON_MAIN_BOARD(PROJECT_NAME, FAN_1_ON_MAIN_BOARD),
    MAKE_FAN_PATH_ON_MAIN_BOARD(PROJECT_NAME, FAN_2_ON_MAIN_BOARD),
    MAKE_FAN_PATH_ON_MAIN_BOARD(PROJECT_NAME, FAN_3_ON_MAIN_BOARD),
    MAKE_FAN_PATH_ON_MAIN_BOARD(PROJECT_NAME, FAN_4_ON_MAIN_BOARD),
    MAKE_FAN_PATH_ON_PSU(1 ,1),
    MAKE_FAN_PATH_ON_PSU(2, 1)
};

#define MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(id) \
    { \
        { ONLP_FAN_ID_CREATE(FAN_##id##_ON_MAIN_BOARD), "Chassis Fan "#id, 0 }, \
        0x0, \
        (ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_PERCENTAGE | \
         ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_SET_RPM), \
        0, \
        0, \
        ONLP_FAN_MODE_INVALID, \
    }

#define MAKE_FAN_INFO_NODE_ON_PSU(psu_id, fan_id) \
    { \
        { ONLP_FAN_ID_CREATE(FAN_##fan_id##_ON_PSU##psu_id), "Chassis PSU-"#psu_id" Fan "#fan_id, 0 }, \
        0x0, \
        (ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE), \
        0, \
        0, \
        ONLP_FAN_MODE_INVALID, \
    }

/* Static fan information */
onlp_fan_info_t linfo[] = {
    { }, /* Not used */
    MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(1),
    MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(2),
    MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(3),
    MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(4),
    MAKE_FAN_INFO_NODE_ON_PSU(1,1),
    MAKE_FAN_INFO_NODE_ON_PSU(2,1)
};

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_FAN(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

#define OPEN_READ_FILE(prefix_module, fan_path, data, nbytes, len)			\
	if (onlp_file_read((uint8_t*)data, nbytes, &len, "%s%s", prefix_module, fan_path) < 0)	\
       return ONLP_STATUS_E_INTERNAL;           \
	else													\
		AIM_LOG_VERBOSE("read data: %s\n", r_data);			\


/* No support in _onlp_fani_read_fan_eepron() function as System Fans does not contain eeprom. */

static int
_onlp_fani_info_get_fan(int local_id, onlp_fan_info_t* info)
{
    int   len = 0, nbytes = 10;
    float range = 0;
    float temp  = 0;
    char  r_data[10]   = {0};

    /* get fan status
    */
    OPEN_READ_FILE(PREFIX_MODULE_PATH, fan_path[local_id].status, r_data, nbytes, len);
    if (atoi(r_data) != FAN_STATUS_OK) {
        return ONLP_STATUS_OK;
    }
    info->status |= ONLP_FAN_STATUS_PRESENT;

     /* get fan speed
     */
    OPEN_READ_FILE(PREFIX_PATH, fan_path[local_id].r_speed_get, r_data, nbytes, len);
    info->rpm = atoi(r_data);

    /* check failure */
    if (info->rpm <= 0) {
      info->status |= ONLP_FAN_STATUS_FAILED;
      return ONLP_STATUS_OK;
    }

    if (ONLP_FAN_CAPS_GET_PERCENTAGE & info->caps) {
        /* get fan min speed
         */
        OPEN_READ_FILE(PREFIX_PATH, fan_path[local_id].min, r_data, nbytes, len);
        min_fan_speed[local_id] = atoi(r_data);

        /* get fan max speed
         */
        OPEN_READ_FILE(PREFIX_PATH, fan_path[local_id].max, r_data, nbytes, len);
        max_fan_speed[local_id] = atoi(r_data);

        /* get speed percentage from rpm */
        range = max_fan_speed[local_id] - min_fan_speed[local_id];
        if (range > 0) {
            temp = ((float)info->rpm - (float)min_fan_speed[local_id]) / range * 40.0 + 60.0;
            if (temp < PERCENTAGE_MIN) {
                temp = PERCENTAGE_MIN;
            }
            info->percentage = (int)temp;
        } else {
            return ONLP_STATUS_E_INTERNAL;
        }
    }

    return ONLP_STATUS_OK;
}

static int
_onlp_fani_info_get_fan_on_psu(int local_id, int psu_id, onlp_fan_info_t* info)
{
    int   len = 0, nbytes = 10;
    char  r_data[10]   = {0};
    float rpms_per_perc = 0.0;
    float temp = 0.0;

    /* get fan status
    */
    OPEN_READ_FILE(PREFIX_MODULE_PATH, fan_path[local_id].status, r_data, nbytes, len);
    if (atoi(r_data) != FAN_STATUS_OK) {
        return ONLP_STATUS_OK;
    }
    info->status |= ONLP_FAN_STATUS_PRESENT;

    /* get fan speed
    */
    OPEN_READ_FILE(PREFIX_PATH, fan_path[local_id].r_speed_get, r_data, nbytes, len);
    info->rpm = atoi(r_data);

    /* check failure */
    if (info->rpm <= 0) {
      info->status |= ONLP_FAN_STATUS_FAILED;
      return ONLP_STATUS_OK;
    }

    /* get speed percentage from rpm */
    rpms_per_perc = PSU_FAN_RPM_MIN / PERCENTAGE_MIN;
    temp = (float)info->rpm / rpms_per_perc;
    if (temp < PERCENTAGE_MIN) {
      temp = PERCENTAGE_MIN;
    }
    info->percentage = (int)temp;

    /* Serial number and model for PSU fan is the same as for appropriate PSU */
    if (FAN_1_ON_PSU1 == local_id) {
        if (0 != psu_read_eeprom(PSU1_ID, NULL, info))
            return ONLP_STATUS_E_INTERNAL;
    } else if (FAN_1_ON_PSU2 == local_id) {
        if (0 != psu_read_eeprom(PSU2_ID, NULL, info))
            return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/*
 * This function will be called prior to all of onlp_fani_* functions.
 */
int
onlp_fani_init(void)
{
    return ONLP_STATUS_OK;
}

int
onlp_fani_info_get(onlp_oid_t id, onlp_fan_info_t* info)
{
    int rc = 0;
    int local_id = 0;
    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);

    *info = linfo[local_id];

    switch (local_id)
        {
        case FAN_1_ON_PSU1:
            rc = _onlp_fani_info_get_fan_on_psu(local_id, PSU1_ID, info);
            break;
        case FAN_1_ON_PSU2:
            rc = _onlp_fani_info_get_fan_on_psu(local_id, PSU2_ID, info);
            break;
        case FAN_1_ON_MAIN_BOARD:
        case FAN_2_ON_MAIN_BOARD:
        case FAN_3_ON_MAIN_BOARD:
        case FAN_4_ON_MAIN_BOARD:
            rc =_onlp_fani_info_get_fan(local_id, info);
            break;
        default:
            rc = ONLP_STATUS_E_INVALID;
            break;
        }

    return rc;
}

/*
 * This function sets the speed of the given fan in RPM.
 *
 * This function will only be called if the fan supprots the RPM_SET
 * capability.
 *
 * It is optional if you have no fans at all with this feature.
 */
int
onlp_fani_rpm_set(onlp_oid_t id, int rpm)
{
    float temp = 0.0;
    int   rv = 0, local_id = 0, nbytes = 10;
    char  r_data[10]   = {0};
    onlp_fan_info_t* info = NULL;

    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);
    info = &linfo[local_id];

    if (0 == (ONLP_FAN_CAPS_SET_RPM & info->caps)) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    /* reject rpm=0% (rpm=0%, stop fan) */
    if (0 == rpm) {
        return ONLP_STATUS_E_INVALID;
    }

    /* Set fan speed
       Converting percent to driver value.
       Driver accept value in range between 153 and 255.
       Value 153 is minimum rpm.
       Value 255 is maximum rpm.
    */
    if (local_id > sizeof(min_fan_speed)/sizeof(min_fan_speed[0])) {
        return ONLP_STATUS_E_INTERNAL;
    }
    if (max_fan_speed[local_id] - min_fan_speed[local_id] < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }
    if (rpm < min_fan_speed[local_id] || rpm > max_fan_speed[local_id]) {
        return ONLP_STATUS_E_PARAM;
    }

    temp = (rpm - min_fan_speed[local_id]) * (RPM_MAGIC_MAX - RPM_MAGIC_MIN) /
        (max_fan_speed[local_id] - min_fan_speed[local_id]) + RPM_MAGIC_MIN;

    snprintf(r_data, sizeof(r_data), "%d", (int)temp);
    nbytes = strnlen(r_data, sizeof(r_data));
    rv = onlp_file_write((uint8_t*)r_data, nbytes, "%s%s", PREFIX_PATH,
            fan_path[local_id].r_speed_set);
	if (rv < 0) {
		return ONLP_STATUS_E_INTERNAL;
	}

    return ONLP_STATUS_OK;
}

/*
 * This function sets the fan speed of the given OID as a percentage.
 *
 * This will only be called if the OID has the PERCENTAGE_SET
 * capability.
 *
 * It is optional if you have no fans at all with this feature.
 */
int
onlp_fani_percentage_set(onlp_oid_t id, int p)
{
    float temp = 0.0;
    int   rv = 0, local_id = 0, nbytes = 10;
    char  r_data[10]   = {0};
    onlp_fan_info_t* info = NULL;

    VALIDATE(id);
    local_id = ONLP_OID_ID_GET(id);
    info = &linfo[local_id];

    if (0 == (ONLP_FAN_CAPS_SET_PERCENTAGE & info->caps)) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    /* reject p=0% (p=0%, stop fan) */
    if (0 == p) {
        return ONLP_STATUS_E_INVALID;
    }

    if (p < PERCENTAGE_MIN || p > PERCENTAGE_MAX) {
        return ONLP_STATUS_E_PARAM;
    }

    /* Set fan speed
       Converting percent to driver value.
       Driver accept value in range between 153 and 255.
       Value 153 is 60%.
       Value 255 is 100%.
    */
    temp = (p - PERCENTAGE_MIN) * (RPM_MAGIC_MAX - RPM_MAGIC_MIN) /
        (PERCENTAGE_MAX - PERCENTAGE_MIN) + RPM_MAGIC_MIN;

    snprintf(r_data, sizeof(r_data), "%d", (int)temp);
    nbytes = strnlen(r_data, sizeof(r_data));
    rv = onlp_file_write((uint8_t*)r_data, nbytes, "%s%s", PREFIX_PATH,
            fan_path[local_id].r_speed_set);
	if (rv < 0) {
		return ONLP_STATUS_E_INTERNAL;
	}

    return ONLP_STATUS_OK;
}

int
onlp_fani_get_min_rpm(int id)
{
    int   len = 0, nbytes = 10;
    char  r_data[10]   = {0};

    if (onlp_file_read((uint8_t*)r_data, nbytes, &len, "%s%s", PREFIX_PATH, fan_path[id].min) < 0)
        return ONLP_STATUS_E_INTERNAL;
  
    return atoi(r_data);
}
