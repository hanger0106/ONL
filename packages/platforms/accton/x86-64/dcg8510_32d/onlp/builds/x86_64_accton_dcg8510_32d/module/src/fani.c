/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2014 Accton Technology Corporation.
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
#include <onlplib/file.h>
#include <onlp/platformi/fani.h>

#include "platform_lib.h"

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_FAN(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

#define MAX_FAN_SPEED    21000

enum fan_id {
    FAN_1_ON_FAN_BOARD = 1,
    FAN_2_ON_FAN_BOARD,
    FAN_3_ON_FAN_BOARD,
    FAN_4_ON_FAN_BOARD,
    FAN_5_ON_FAN_BOARD,
    FAN_6_ON_FAN_BOARD,
};

#define FAN_BOARD_PATH    "/sys/switch/fan/"

#define CHASSIS_FAN_INFO(fid)        \
    { \
        { ONLP_FAN_ID_CREATE(FAN_##fid##_ON_FAN_BOARD), "Chassis Fan-"#fid, 0 },\
        0x0,\
        ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE,\
        0,\
        0,\
        ONLP_FAN_MODE_INVALID,\
    }

/* Static fan information */
onlp_fan_info_t finfo[] = {
    { }, /* Not used */
    CHASSIS_FAN_INFO(1),
    CHASSIS_FAN_INFO(2),
    CHASSIS_FAN_INFO(3),
    CHASSIS_FAN_INFO(4),
    CHASSIS_FAN_INFO(5),
    CHASSIS_FAN_INFO(6),
};

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
    int  value = 0, fid,fmid,len;
    char path[64] = {0};
    VALIDATE(id);

    fid = ONLP_OID_ID_GET(id);
    *info = finfo[fid];

    /* get fan present status
     */
    //  /sys/switch/fan/fan1/status :1(good)
    sprintf(path, "%sfan%d/status", FAN_BOARD_PATH, fid);
    if (0 > onlp_file_read_int(&value, path ))
        return ONLP_STATUS_E_INTERNAL;
    if (value != 1) {
        return ONLP_STATUS_OK;  /* fan is not present */
    }
    info->status |= ONLP_FAN_STATUS_PRESENT;

    /* get front fan rpm
     */
    // fan[n]/motor[n]/speed
    info->rpm=0;
    for(fmid=1;fmid<=CHASSIS_FAN_MOTOR_COUNT;fmid++){
        sprintf(path, "%sfan%d/motor%d/speed", FAN_BOARD_PATH, fid,fmid);
        if (0 > onlp_file_read_int(&value, path ))
            return ONLP_STATUS_E_INTERNAL;
        if(info->rpm==0)
            info->rpm = value;
        else if (info->rpm > value) //take the min value from front/rear fan speed
            info->rpm = value;
    }

    /* set fan status based on rpm
     */
    if (!info->rpm) {
        info->status |= ONLP_FAN_STATUS_FAILED;
        return ONLP_STATUS_OK;
    }

    /* get speed percentage from rpm 
     */
    info->percentage = (info->rpm * 100)/MAX_FAN_SPEED;

    /* set fan direction
     */
    ///sys_switch/fan/fan[n]/motor[n]/direction
    sprintf(path, "%sfan%d/motor%d/direction", FAN_BOARD_PATH, fid,1);
    if (0 > onlp_file_read_int(&value, path ))
        return ONLP_STATUS_E_INTERNAL;
    if(value == 0)
        info->status |= ONLP_FAN_STATUS_F2B;
    else if (value == 1)
        info->status |= ONLP_FAN_STATUS_B2F;
    /* Get model name */
    // /sys_switch/fan/fan[n]/model_name
    sprintf(path, "%sfan%d/model_name", FAN_BOARD_PATH, fid);
    if (0 > onlp_file_read((uint8_t*)info->model,sizeof(info->model),&len, path ))
        return ONLP_STATUS_E_INTERNAL;   
 
    /* Get serial*/
    // /sys_switch/fan/fan[n]/serial_number
    sprintf(path, "%sfan%d/serial_number", FAN_BOARD_PATH, fid);
    if (0 > onlp_file_read((uint8_t*)info->serial,sizeof(info->model),&len, path ))
        return ONLP_STATUS_E_INTERNAL; 
    return ONLP_STATUS_OK;
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
    return ONLP_STATUS_E_UNSUPPORTED;
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
    int fid,fmid;
    fid = ONLP_OID_ID_GET(id);
    char path[64] = {0};
    ///sys_switch/fan/fan[n]/motor[n]/ratio
    for(fmid=1;fmid<=CHASSIS_FAN_MOTOR_COUNT;fmid++){
        sprintf(path, "%d > %sfan%d/motor%d/ratio", p,FAN_BOARD_PATH,fid,fmid);
        if ( 0 < onlp_file_write_int(p, path))
            return ONLP_STATUS_E_INTERNAL;    
    }   
    return ONLP_STATUS_OK;
}

/*
 * This function sets the fan speed of the given OID as per
 * the predefined ONLP fan speed modes: off, slow, normal, fast, max.
 *
 * Interpretation of these modes is up to the platform.
 *
 */
int
onlp_fani_mode_set(onlp_oid_t id, onlp_fan_mode_t mode)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * This function sets the fan direction of the given OID.
 *
 * This function is only relevant if the fan OID supports both direction
 * capabilities.
 *
 * This function is optional unless the functionality is available.
 */
int
onlp_fani_dir_set(onlp_oid_t id, onlp_fan_dir_t dir)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * Generic fan ioctl. Optional.
 */
int
onlp_fani_ioctl(onlp_oid_t id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}
