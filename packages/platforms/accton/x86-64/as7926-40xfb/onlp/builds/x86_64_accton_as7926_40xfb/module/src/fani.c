/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2017 Accton Technology Corporation.
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
#include <onlp/platformi/fani.h>
#include "platform_lib.h"

enum fan_id {
    FAN_1_ON_FAN_BOARD = 1,
    FAN_2_ON_FAN_BOARD,
    FAN_3_ON_FAN_BOARD,
    FAN_4_ON_FAN_BOARD,
    FAN_5_ON_FAN_BOARD,
    FAN_6_ON_FAN_BOARD,
    FAN_7_ON_FAN_BOARD,
    FAN_8_ON_FAN_BOARD,
    FAN_9_ON_FAN_BOARD,
    FAN_10_ON_FAN_BOARD,
    FAN_1_ON_PSU_1,
    FAN_1_ON_PSU_2,
    ONLP_FAN_MAX
};

#define MAX_PSU_FAN_SPEED 23000

#define CREATE_FAN_INFO_ON_MAIN_BOARD(fid)        \
    { \
        { ONLP_FAN_ID_CREATE(FAN_##fid##_ON_FAN_BOARD), "Fan "#fid, ONLP_OID_CHASSIS, { 0 }, 0 }, \
        0x0,\
        ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE,\
        0,\
        0,\
    }

#define CREATE_FAN_INFO_ON_PSU(psu_id, fan_id) \
    { \
        { ONLP_FAN_ID_CREATE(FAN_##fan_id##_ON_PSU_##psu_id), "PSU-"#psu_id " Fan "#fan_id, ONLP_PSU_ID_CREATE(psu_id), { 0 }, 0 }, \
        0x0,\
        ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE,\
        0,\
        0, \
    }

#define AIM_FREE_IF_PTR(p) \
    do \
    { \
        if (p) { \
            aim_free(p); \
            p = NULL; \
        } \
    } while (0)

/* Static fan information */
onlp_fan_info_t finfo[] = {
    { }, /* Not used */
    CREATE_FAN_INFO_ON_MAIN_BOARD(1),
    CREATE_FAN_INFO_ON_MAIN_BOARD(2),
    CREATE_FAN_INFO_ON_MAIN_BOARD(3),
    CREATE_FAN_INFO_ON_MAIN_BOARD(4),
    CREATE_FAN_INFO_ON_MAIN_BOARD(5),
    CREATE_FAN_INFO_ON_MAIN_BOARD(6),
    CREATE_FAN_INFO_ON_MAIN_BOARD(7),
    CREATE_FAN_INFO_ON_MAIN_BOARD(8),
    CREATE_FAN_INFO_ON_MAIN_BOARD(9),
    CREATE_FAN_INFO_ON_MAIN_BOARD(10),
    CREATE_FAN_INFO_ON_PSU(1,1),
    CREATE_FAN_INFO_ON_PSU(2,1)
};

static int
_onlp_fani_info_get_fan(int fid, onlp_fan_info_t* info)
{
    char *fandir = NULL;
    int   len = 0;
    int   value;

    /* get fan present status
     */
    ONLP_TRY(onlp_file_read_int(&value, "%s""fan%d_present", FAN_SYSFS_PATH, fid));
    if (value == 0) {
        ONLP_OID_STATUS_FLAG_CLR(info, PRESENT);
        return ONLP_STATUS_OK; /* fan is not present */
    }
    else {
        ONLP_OID_STATUS_FLAG_SET(info, PRESENT);
    }

    /* get fan direction
     */
    len = onlp_file_read_str(&fandir, "%s""fan%d_dir", FAN_SYSFS_PATH, fid);
    if (fandir && len) {
        if (strncmp(fandir, "B2F", strlen("B2F")) == 0) {
            info->dir = ONLP_FAN_DIR_B2F;
        }
        else {
            info->dir = ONLP_FAN_DIR_F2B;
        }
    }
    AIM_FREE_IF_PTR(fandir);

    /* get fan speed
     */
    ONLP_TRY(onlp_file_read_int(&value, "%s""fan%d_input", FAN_SYSFS_PATH, fid));
    info->rpm = value;

    /* get fan fault status (turn on when any one fails)
     */
    if (info->rpm == 0) {
        ONLP_OID_STATUS_FLAG_SET(info, FAILED);
    }
    else {
        ONLP_OID_STATUS_FLAG_CLR(info, FAILED);
    }

    /* get speed percentage from rpm
     */
    ONLP_TRY(onlp_file_read_int(&value, "%s""fan%d_max", FAN_SYSFS_PATH, fid));
    if (value)
        info->percentage = (info->rpm * 100)/value;

    return ONLP_STATUS_OK;
}

static int
_onlp_fani_info_get_fan_on_psu(int pid, onlp_fan_info_t* info)
{
    char *fandir = NULL;
    int   len = 0;
    int   val = 0;

    ONLP_TRY(onlp_file_read_int(&val, "%s""psu%d_present", PSU_SYSFS_PATH, pid));
    if (val != PSU_STATUS_PRESENT) {
        ONLP_OID_STATUS_FLAG_CLR(info, PRESENT);
        return ONLP_STATUS_OK;
    }
    else {
        ONLP_OID_STATUS_FLAG_SET(info, PRESENT);
    }

    /* get fan direction
     */
    len = onlp_file_read_str(&fandir, "%s""psu%d_fan_dir", PSU_SYSFS_PATH, pid);
    if (fandir && len) {
        if (strncmp(fandir, "B2F", strlen("B2F")) == 0) {
            info->dir = ONLP_FAN_DIR_B2F;
        }
        else {
            info->dir = ONLP_FAN_DIR_F2B;
        }
    }
    AIM_FREE_IF_PTR(fandir);

    /* get fan speed
     */
    ONLP_TRY(onlp_file_read_int(&val, "%s""psu%d_fan1_input", PSU_SYSFS_PATH, pid));
    info->rpm = val;

    /* get fan fault status (turn on when failed)
     */
    if (info->rpm == 0) {
        ONLP_OID_STATUS_FLAG_SET(info, FAILED);
    }
    else {
        ONLP_OID_STATUS_FLAG_CLR(info, FAILED);
    }

    /* get speed percentage from rpm
     */
    info->percentage = (info->rpm * 100)/MAX_PSU_FAN_SPEED;

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
onlp_fani_percentage_set(onlp_oid_id_t id, int p)
{
    int fid = ONLP_OID_ID_GET(id);

    /* reject p=0 (p=0, stop fan) */
    if (p == 0){
        AIM_LOG_ERROR("Zero percentage is not allowed\r\n");
        return ONLP_STATUS_E_INVALID;
    }

    if (fid < FAN_1_ON_FAN_BOARD || fid > FAN_10_ON_FAN_BOARD) {
        AIM_LOG_ERROR("Invalid fan id(%d)\r\n", fid);
        return ONLP_STATUS_E_INVALID;
    }

    return onlp_file_write_int(p, "%s%s%d%s", FAN_SYSFS_PATH, "fan", fid, "_pwm");
}

/*
 * This function will be called prior to all of onlp_fani_* functions.
 */
int
onlp_fani_sw_init(void)
{
    return ONLP_STATUS_OK;
}

int
onlp_fani_sw_denit(void)
{
    return ONLP_STATUS_OK;
}

int
onlp_fani_info_get(onlp_oid_id_t id, onlp_fan_info_t* info)
{
    int rc = 0;
    int fid;

    fid = ONLP_OID_ID_GET(id);
    *info = finfo[fid];

    switch (fid) {
    case FAN_1_ON_PSU_1:
    case FAN_1_ON_PSU_2:
        rc = _onlp_fani_info_get_fan_on_psu(fid-FAN_1_ON_PSU_1+1, info);
        break;
    case FAN_1_ON_FAN_BOARD:
    case FAN_2_ON_FAN_BOARD:
    case FAN_3_ON_FAN_BOARD:
    case FAN_4_ON_FAN_BOARD:
    case FAN_5_ON_FAN_BOARD:
    case FAN_6_ON_FAN_BOARD:
    case FAN_7_ON_FAN_BOARD:
    case FAN_8_ON_FAN_BOARD:
    case FAN_9_ON_FAN_BOARD:
    case FAN_10_ON_FAN_BOARD:
        rc =_onlp_fani_info_get_fan(fid, info);
        break;
    default:
        AIM_LOG_ERROR("Invalid fan id(%d)\r\n", fid);
        rc = ONLP_STATUS_E_INVALID;
        break;
    }

    return rc;
}

static int
chassis_fani_hdr_get__(int id, onlp_oid_hdr_t* hdr)
{
    int value;
    *hdr = finfo[id].hdr;

    ONLP_TRY(onlp_file_read_int(&value, "%s""fan%d_present", FAN_SYSFS_PATH, id));
    if (value == 0) {
        ONLP_OID_STATUS_FLAG_CLR(hdr, PRESENT);
        return ONLP_STATUS_OK; /* fan is not present */
    }
    else {
        ONLP_OID_STATUS_FLAG_SET(hdr, PRESENT);
    }

    ONLP_TRY(onlp_file_read_int(&value, "%s""fan%d_input", FAN_SYSFS_PATH, id));
    if (value == 0) {
        ONLP_OID_STATUS_FLAG_SET(hdr, FAILED);
    }
    else {
        ONLP_OID_STATUS_FLAG_CLR(hdr, FAILED);
    }

    return ONLP_STATUS_OK;
}

static int
psu_fani_hdr_get__(int id, onlp_oid_hdr_t* hdr)
{
    int v;
    *hdr = finfo[id].hdr;

    ONLP_TRY(onlp_file_read_int(&v, "%s""psu%d_present", PSU_SYSFS_PATH, id));
    if (v != PSU_STATUS_PRESENT) {
        ONLP_OID_STATUS_FLAG_CLR(hdr, PRESENT);
        return ONLP_STATUS_OK;
    }
    else {
        ONLP_OID_STATUS_FLAG_SET(hdr, PRESENT);
    }

    ONLP_TRY(onlp_file_read_int(&v, "%s""psu%d_fan1_input", PSU_SYSFS_PATH, id));
    if (v == 0) {
        ONLP_OID_STATUS_FLAG_SET(hdr, FAILED);
    }
    else {
        ONLP_OID_STATUS_FLAG_CLR(hdr, FAILED);
    }

    return ONLP_STATUS_OK;
}

int
onlp_fani_hdr_get(onlp_oid_t oid, onlp_oid_hdr_t* hdr)
{
    int fid = ONLP_OID_ID_GET(oid);

    switch (fid) {
        case FAN_1_ON_PSU_1:
        case FAN_1_ON_PSU_2:
            return psu_fani_hdr_get__(fid-FAN_1_ON_PSU_1+1, hdr);
        case FAN_1_ON_FAN_BOARD:
        case FAN_2_ON_FAN_BOARD:
        case FAN_3_ON_FAN_BOARD:
        case FAN_4_ON_FAN_BOARD:
        case FAN_5_ON_FAN_BOARD:
        case FAN_6_ON_FAN_BOARD:
        case FAN_7_ON_FAN_BOARD:
        case FAN_8_ON_FAN_BOARD:
        case FAN_9_ON_FAN_BOARD:
        case FAN_10_ON_FAN_BOARD:
            return chassis_fani_hdr_get__(fid, hdr);
        default:
            AIM_LOG_ERROR("Invalid fan id(%d)\r\n", fid);
            break;
    }

    return ONLP_STATUS_E_PARAM;
}

int
onlp_fani_id_validate(onlp_oid_id_t id)
{
    return ONLP_OID_ID_VALIDATE_RANGE(id, 1, ONLP_FAN_MAX-1);
}
