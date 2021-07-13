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
 *
 *
 ***********************************************************/
#include <onlp/platformi/sfpi.h>
#include <onlplib/i2c.h>
#include <onlplib/file.h>
#include "platform_lib.h"

#define CPLD_MAX_PORT_NUM       8

static const int port_cpld_bus_index[NUM_OF_SFP_PORT] = 
{
    13, 13, 13, 13, 13, 13, 13, 13,
    14, 14, 14, 14, 14, 14, 14, 14,
    15, 15, 15, 15, 15, 15, 15, 15,
    16, 16, 16, 16, 16, 16, 16, 16,
    0, 0
};

static const int port_bus_index[NUM_OF_SFP_PORT] = 
{
    21, 22, 23, 24, 25, 26, 27, 28,
    29, 30, 31, 32, 33, 34, 35, 36,
    37, 38, 39, 40, 41, 42, 43, 44,
    45, 46, 47, 48, 49, 50, 51, 52,
    12, 11
};

static const int qsfp_port_cpld_bus_index[NUM_OF_QSFP_PORT_CPLD] = 
{
    13, 14, 15, 16
};

#define PORT_CPLD_BUS_INDEX(port)           (port_cpld_bus_index[port])
#define PORT_BUS_INDEX(port)                (port_bus_index[port])
#define QSFP_PORT_CPLD_BUS_INDEX(port_cpld) (qsfp_port_cpld_bus_index[port_cpld])

/* QSFP */
#define MODULE_EEPROM_QSFP_FORMAT           "/sys/bus/i2c/devices/%d-0050/eeprom"
#define MODULE_PRESENT_QSFP_CPLD_FORMAT     "/sys/bus/i2c/devices/%d-005f/module_present_%d"
#define MODULE_RESET_QSFP_CPLD_FORMAT       "/sys/bus/i2c/devices/%d-005f/module_reset_%d"
#define MODULE_LPMODE_QSFP_CPLD_FORMAT      "/sys/bus/i2c/devices/%d-005f/module_lp_mode_%d"
#define MODULE_PRESENT_ALL_QSFP_CPLD_FORMAT "/sys/bus/i2c/devices/%d-005f/module_present_all"

/* SFP */
#define MODULE_EEPROM_SFP_FORMAT            "/sys/bus/i2c/devices/%d-0050/eeprom"
#define MODULE_EEPROM_DOM_SFP_FORMAT        "/sys/bus/i2c/devices/%d-0051/eeprom"
#define MODULE_PRESENT_SFP_FORMAT           "/sys/bus/i2c/devices/0-005e/sfp_present"
#define MODULE_RXLOS_SFP_FORMAT             "/sys/bus/i2c/devices/0-005e/sfp_rx_loss"
#define MODULE_TXFAULT_SFP_FORMAT           "/sys/bus/i2c/devices/0-005e/sfp_tx_fault"
#define MODULE_TXDISABLE_SFP_FORMAT         "/sys/bus/i2c/devices/0-005e/sfp_tx_disable"

#define MODULE_IS_PRESENT_SFP0_MASK         0x2
#define MODULE_IS_PRESENT_SFP1_MASK         0x1
#define MODULE_IS_PRESENT_ALL_SFP_MASK      0x3

#define MODULE_RXLOS_SFP0_MASK              0x2
#define MODULE_RXLOS_SFP1_MASK              0x1
#define MODULE_TXFAULT_SFP0_MASK            0x2
#define MODULE_TXFAULT_SFP1_MASK            0x1
#define MODULE_TXDISABLE_SFP0_MASK          0x2
#define MODULE_TXDISABLE_SFP1_MASK          0x1


/************************************************************
 *
 * SFPI Entry Points
 *
 ***********************************************************/
int
onlp_sfpi_init(void)
{
    DIAG_PRINT("%s", __FUNCTION__);
    /* Called at initialization time */
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{
    int p;
    AIM_BITMAP_CLR_ALL(bmap);

    for(p = 0; p < NUM_OF_SFP_PORT; p++) {
        AIM_BITMAP_SET(bmap, p);
    }

    DIAG_PRINT("%s", __FUNCTION__);
    
    return ONLP_STATUS_OK;
}

int onlp_sfpi_is_present(int port)
{
    /*
     * Return 1 if present.
     * Return 0 if not present.
     * Return < 0 if error.
     */
    int present;

    if (IS_SFP_PORT(port)) 
    {
        if (onlp_file_read_int(&present, MODULE_PRESENT_SFP_FORMAT) < 0) 
        {
            AIM_LOG_ERROR("Unable to read present status from port(%d)\r\n", port);
            return ONLP_STATUS_E_INTERNAL;
        }

        DIAG_PRINT("%s, SFP Present value:0x%x, ", __FUNCTION__, present);
        
        if (IS_SFP0_PORT(port))
        {
            present = present & MODULE_IS_PRESENT_SFP0_MASK;
            present = present >> 1;
            present = !present;
        
        }
        else if (IS_SFP1_PORT(port))
        {
            present = present & MODULE_IS_PRESENT_SFP1_MASK;
            present = !present;
        }
        else
        {
            return ONLP_STATUS_E_INVALID;
        }
        DIAG_PRINT("%s, SFP Port:%d Present value:0x%x, ", __FUNCTION__, port, present);
    }
    else if (IS_QSFP_PORT(port))
    {
        if (onlp_file_read_int(&present, MODULE_PRESENT_QSFP_CPLD_FORMAT, 
                PORT_CPLD_BUS_INDEX(port), (port+1)) < 0) 
        {
            AIM_LOG_ERROR("Unable to read present status from port(%d)\r\n", port);
            return ONLP_STATUS_E_INTERNAL;
        }
        DIAG_PRINT("%s, QSFP Port:%d Present value:0x%x, ", __FUNCTION__, port, present);
    }
    else
    {
        return ONLP_STATUS_E_INVALID;
    }
    
    return present;
}

int
onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    DIAG_PRINT("%s", __FUNCTION__);
    uint32_t bytes[NUM_OF_QSFP_PORT_CPLD+NUM_OF_SFP_PORT_CPLD] = { 0 };
    int present_all;
    int i = 0;
    FILE* fp;

    /* Get 4 QSFP PORT CPLD present_all status */
    for (i = 0; i < NUM_OF_QSFP_PORT_CPLD; i++)
    {
        int count = 0;
        char file[64] = {0};

        sprintf(file, MODULE_PRESENT_ALL_QSFP_CPLD_FORMAT, QSFP_PORT_CPLD_BUS_INDEX(i));      
        fp = fopen(file, "r");

        if(fp == NULL) 
        {
            AIM_LOG_ERROR("Unable to open the module_present_all device file from port_cpld(%d)\r\n", i+1);
            return ONLP_STATUS_E_INTERNAL;
        }
        count = fscanf(fp, "%x", bytes+i);
        fclose(fp);
        
        if(count != 1) 
        {
            /* Likely a CPLD read timeout. */
            AIM_LOG_ERROR("Unable to read module_present_all status from port_cpld(%d)\r\n", i+1);
            return ONLP_STATUS_E_INTERNAL;
        }

        DIAG_PRINT("%s, port_cpld:%d, present_all:0x%x", __FUNCTION__, i+1, bytes[i]);
    }

    /* Get SFP present_all status */
    if (onlp_file_read_int(&present_all, MODULE_PRESENT_SFP_FORMAT) < 0) 
    {
        AIM_LOG_ERROR("Unable to read SFP present_all status from FPGA\r\n");
        return ONLP_STATUS_E_INTERNAL;
    }
    
    DIAG_PRINT("%s, SFP present_all:0x%x from FPGA", __FUNCTION__, present_all);

    present_all = ~present_all;
    present_all &= MODULE_IS_PRESENT_ALL_SFP_MASK;
    /* exchange bit 0 and bit 1 */
    present_all = ((present_all & MODULE_IS_PRESENT_SFP0_MASK) >> 1) | 
                  ((present_all & MODULE_IS_PRESENT_SFP1_MASK) << 1);

    DIAG_PRINT("%s, SFP present_all:0x%x", __FUNCTION__, present_all);
    bytes[i] = present_all;

    for (i = 0; i < NUM_OF_QSFP_PORT_CPLD+NUM_OF_SFP_PORT_CPLD; i++)
    {
        DIAG_PRINT("%s, bytes[%d]:0x%x", __FUNCTION__, i, bytes[i]);
    }

    /* Convert to 64 bit integer in port order */
    uint64_t presence_all = 0 ;
    for (i = AIM_ARRAYSIZE(bytes) - 1; i >= 0; i--)
    {
        presence_all <<= 8;
        presence_all |= bytes[i];
    }

    /* Populate bitmap */
    for (i = 0; presence_all; i++)
    {
        AIM_BITMAP_MOD(dst, i, (presence_all & 1));
        presence_all >>= 1;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    /* auto generate from sfp.c */
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{
    /*
     * Read the SFP eeprom into data[]
     *
     * Return MISSING if SFP is missing.
     * Return OK if eeprom is read
     */
    int size = 0;
    if(port < 0 || port >= NUM_OF_SFP_PORT)
        return ONLP_STATUS_E_INTERNAL;

    DIAG_PRINT("%s, port:%d, busid:%d", __FUNCTION__, port, PORT_BUS_INDEX(port));

    if (IS_SFP_PORT(port)) 
    {
        if(onlp_file_read(data, 256, &size, MODULE_EEPROM_SFP_FORMAT, 
                PORT_BUS_INDEX(port)) != ONLP_STATUS_OK) 
        {
            AIM_LOG_ERROR("Unable to read eeprom from port(%d)\r\n", port);
            return ONLP_STATUS_E_INTERNAL;
        }
    
    }
    else if (IS_QSFP_PORT(port))
    {
        if(onlp_file_read(data, 256, &size, MODULE_EEPROM_QSFP_FORMAT, 
                PORT_BUS_INDEX(port)) != ONLP_STATUS_OK) 
        {
            AIM_LOG_ERROR("Unable to read eeprom from port(%d)\r\n", port);
            return ONLP_STATUS_E_INTERNAL;
        }
    }
    else
    {
        return ONLP_STATUS_E_INVALID;
    }

    if(size != 256) 
    {
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_dom_read(int port, uint8_t data[256])
{
    FILE* fp;
    char file[64] = {0};

    DIAG_PRINT("%s, port:%d, busid:%d", __FUNCTION__, port, PORT_BUS_INDEX(port));

    if (IS_SFP_PORT(port))
    {
        sprintf(file, MODULE_EEPROM_SFP_FORMAT, PORT_BUS_INDEX(port));
        fp = fopen(file, "r");
        if(fp == NULL) {
            AIM_LOG_ERROR("Unable to open the eeprom device file of port(%d)", port);
            return ONLP_STATUS_E_INTERNAL;
        }

        if (fseek(fp, 256, SEEK_CUR) != 0) {
            fclose(fp);
            AIM_LOG_ERROR("Unable to set the file position indicator of port(%d)", port);
            return ONLP_STATUS_E_INTERNAL;
        }

        int ret = fread(data, 1, 256, fp);
        fclose(fp);
        if (ret != 256) {
            AIM_LOG_ERROR("Unable to read the eeprom device file of port(%d)", port);
            return ONLP_STATUS_E_INTERNAL;
        }
    }
    else
    {
        return ONLP_STATUS_E_INVALID;
    }
    
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_dev_readb(int port, uint8_t devaddr, uint8_t addr)
{
    int ret = 0;
    int bus = PORT_BUS_INDEX(port);

    ret = onlp_i2c_readb(bus, devaddr, addr, ONLP_I2C_F_FORCE);
    DIAG_PRINT("%s, port:%d, devaddr:%d, addr:%d, ret:%d(0x%02X)", __FUNCTION__, port, devaddr, addr, ret, ret);
    return ret; 
}

int
onlp_sfpi_dev_writeb(int port, uint8_t devaddr, uint8_t addr, uint8_t value)
{
    int ret = 0;
    int bus = PORT_BUS_INDEX(port);

    ret = onlp_i2c_writeb(bus, devaddr, addr, value, ONLP_I2C_F_FORCE);
    DIAG_PRINT("%s, port:%d, devaddr:%d, addr:%d, value:%d(0x%02X), ret:%d", __FUNCTION__, port, devaddr, addr, value, value, ret);
    return ret;
}

int
onlp_sfpi_dev_readw(int port, uint8_t devaddr, uint8_t addr)
{
    int ret = 0;
    int bus = PORT_BUS_INDEX(port);

    ret = onlp_i2c_readw(bus, devaddr, addr, ONLP_I2C_F_FORCE);
    DIAG_PRINT("%s, port:%d, devaddr:%d, addr:%d, ret:%d(0x%04X)", __FUNCTION__, port, devaddr, addr, ret, ret);
    return ret;
}

int
onlp_sfpi_dev_writew(int port, uint8_t devaddr, uint8_t addr, uint16_t value)
{
    int ret = 0;
    int bus = PORT_BUS_INDEX(port);

    ret = onlp_i2c_writew(bus, devaddr, addr, value, ONLP_I2C_F_FORCE);
    DIAG_PRINT("%s, port:%d, devaddr:%d, addr:%d, value:%d(0x%04X), ret:%d", __FUNCTION__, port, devaddr, addr, value, value, ret);
    return ret;
}

int onlp_sfpi_control_supported(int port, onlp_sfp_control_t control, int *supported)
{
    if (supported == NULL)
    {
        AIM_LOG_INFO("%s:%d fail[%d]\n", __FUNCTION__, __LINE__, ONLP_STATUS_E_PARAM);
        return ONLP_STATUS_E_PARAM;
    }

    *supported = 0;
    switch (control)
    {
        case ONLP_SFP_CONTROL_RESET:
        case ONLP_SFP_CONTROL_LP_MODE:
        {
            if(IS_QSFP_PORT(port))
            {
                *supported = 1;
            }
            else
            {
                *supported = 0;
            }
        }
            break;
        case ONLP_SFP_CONTROL_RX_LOS:
        case ONLP_SFP_CONTROL_TX_FAULT:
        case ONLP_SFP_CONTROL_TX_DISABLE:
        {
            if(IS_SFP_PORT(port))
            {
                *supported = 1;
            }
            else
            {
                *supported = 0;
            }
        }
            break;

        default:
            *supported = 0;
            break;
    }

    DIAG_PRINT("%s, port:%d, control:%d(%s), supported:%d", __FUNCTION__, port, control, sfp_control_to_str(control), *supported);
    return ONLP_STATUS_OK;
}


int
onlp_sfpi_control_set(int port, onlp_sfp_control_t control, int value)
{
    int rv;  
    int supported = 0;

    if ((onlp_sfpi_control_supported(port, control, &supported) == ONLP_STATUS_OK) && 
        (supported == 0))
    {
        AIM_LOG_INFO("%s:%d fail[%d]\n", __FUNCTION__, __LINE__, ONLP_STATUS_E_UNSUPPORTED);
        return ONLP_STATUS_E_UNSUPPORTED;
    }
    
    DIAG_PRINT("%s, port:%d, control:%d(%s), value:0x%X", __FUNCTION__, port, control, sfp_control_to_str(control), value);
    
    /* ONLP_SFP_CONTROL_RESET: write-only. */
    switch(control)
    {
        case ONLP_SFP_CONTROL_RESET:
        {
            if(IS_QSFP_PORT(port)) 
            {
                if (onlp_file_write_int(value, MODULE_RESET_QSFP_CPLD_FORMAT, 
                        PORT_CPLD_BUS_INDEX(port), (port+1)) < 0) 
                {
                    AIM_LOG_ERROR("Unable to set reset status to port(%d)\r\n", port);
                    rv = ONLP_STATUS_E_INTERNAL;
                }
                else 
                {
                    rv = ONLP_STATUS_OK;
                }
                DIAG_PRINT("%s, Write QSFP port:%d RESET value:0x%x, ", __FUNCTION__, port, value);
            }
            else
            {
                rv = ONLP_STATUS_E_UNSUPPORTED;
            }
            break;
        }
        case ONLP_SFP_CONTROL_LP_MODE:
        {
            if(IS_QSFP_PORT(port)) 
            {
                if (onlp_file_write_int(value, MODULE_LPMODE_QSFP_CPLD_FORMAT, 
                        PORT_CPLD_BUS_INDEX(port), (port+1)) < 0) 
                {
                    AIM_LOG_ERROR("Unable to set lp_mode status to port(%d)\r\n", port);
                    rv = ONLP_STATUS_E_INTERNAL;
                }
                else 
                {
                    rv = ONLP_STATUS_OK;
                }
                DIAG_PRINT("%s, Write QSFP port:%d LPMODE value:0x%x, ", __FUNCTION__, port, value);
            }
            else
            {
                rv = ONLP_STATUS_E_UNSUPPORTED;
            }
            break;
        }
        case ONLP_SFP_CONTROL_TX_DISABLE:
        {
            if(IS_SFP_PORT(port)) 
            {
                int cpld_val;
                /* Read before write */
                if (onlp_file_read_int(&cpld_val, MODULE_TXDISABLE_SFP_FORMAT) < 0) 
                {
                    AIM_LOG_ERROR("Unable to read tx_disabled status from port(%d)\r\n", port);
                    rv = ONLP_STATUS_E_INTERNAL;
                    return rv;
                }

                DIAG_PRINT("%s, Read Current CPLD SFP TXDISABLE  value:0x%x, ", __FUNCTION__, cpld_val);

                /* Base on sfp port to write TXDISABLE value */
                if (IS_SFP0_PORT(port))
                {
                    if (value)  /* value = 1*/
                    {       
                        cpld_val |= MODULE_TXDISABLE_SFP0_MASK;
                    }
                    else        /* value = 0*/
                    {       
                        cpld_val &= ~MODULE_TXDISABLE_SFP0_MASK;
                    }

                }
                else if (IS_SFP1_PORT(port))
                {
                    if (value)  /* value = 1*/
                    {       
                        cpld_val |= MODULE_TXDISABLE_SFP1_MASK;
                    }
                    else        /* value = 0*/
                    {       
                        cpld_val &= ~MODULE_TXDISABLE_SFP1_MASK;
                    }
                }
                else
                {
                    rv = ONLP_STATUS_E_INTERNAL;
                    return rv;
                }
            
                if (onlp_file_write_int(cpld_val, MODULE_TXDISABLE_SFP_FORMAT) < 0) 
                {
                    AIM_LOG_ERROR("Unable to set tx_disabled status to port(%d)\r\n", port);
                    rv = ONLP_STATUS_E_INTERNAL;
                }
                else 
                {
                    rv = ONLP_STATUS_OK;
                }
                DIAG_PRINT("%s, Write SFP port:%d TXDISABLE value:0x%x, ", __FUNCTION__, port, cpld_val);
                
            }
            else {
                rv = ONLP_STATUS_E_UNSUPPORTED;
            }
            break;
        }

        default:
            rv = ONLP_STATUS_E_UNSUPPORTED;
            break;
    }

    if (rv == ONLP_STATUS_E_UNSUPPORTED)
    {
        AIM_LOG_INFO("%s:%d fail[%d]\n", __FUNCTION__, __LINE__, ONLP_STATUS_E_UNSUPPORTED);
    }

    return rv;
}

int
onlp_sfpi_control_get(int port, onlp_sfp_control_t control, int* value)
{
    int rv;
    int cpld_val;
    int supported = 0;

    if (value == NULL)
    {
        AIM_LOG_INFO("%s:%d fail[%d]\n", __FUNCTION__, __LINE__, ONLP_STATUS_E_PARAM);
        return ONLP_STATUS_E_PARAM;
    }

    if ((onlp_sfpi_control_supported(port, control, &supported) == ONLP_STATUS_OK) && 
        (supported == 0))
    {
        AIM_LOG_INFO("%s:%d fail[%d]\n", __FUNCTION__, __LINE__, ONLP_STATUS_E_UNSUPPORTED);
        return ONLP_STATUS_E_UNSUPPORTED;
    }
    *value = 0;
    
    /* ONLP_SFP_CONTROL_RX_LOS , ONLP_SFP_CONTROL_TX_FAULT are read-only. */
    switch(control)
    {
        case ONLP_SFP_CONTROL_RESET:
        {
            if(IS_QSFP_PORT(port))
            {
                if (onlp_file_read_int(&cpld_val,  MODULE_RESET_QSFP_CPLD_FORMAT, 
                        PORT_CPLD_BUS_INDEX(port), (port+1)) < 0) 
                {
                    AIM_LOG_ERROR("Unable to read reset status from port(%d)\r\n", port);
                    rv = ONLP_STATUS_E_INTERNAL;
                }
                else
                {
                    rv = ONLP_STATUS_OK;
                }

                DIAG_PRINT("%s, Read Current CPLD QSFP RESET value:0x%x, ", __FUNCTION__, cpld_val);
                *value = cpld_val;
            }
            else
            {
                rv = ONLP_STATUS_E_UNSUPPORTED;
            }
            break;
        }
        case ONLP_SFP_CONTROL_LP_MODE:
        {
            if(IS_QSFP_PORT(port))
            {
                if (onlp_file_read_int(&cpld_val,  MODULE_LPMODE_QSFP_CPLD_FORMAT, 
                        PORT_CPLD_BUS_INDEX(port), (port+1)) < 0) 
                {
                    AIM_LOG_ERROR("Unable to read lp_mode status from port(%d)\r\n", port);
                    rv = ONLP_STATUS_E_INTERNAL;
                }
                else
                {
                    rv = ONLP_STATUS_OK;
                }

                DIAG_PRINT("%s, Read Current CPLD QSFP LPMODE value:0x%x, ", __FUNCTION__, cpld_val);
                *value = cpld_val;
            }
            else
            {
                rv = ONLP_STATUS_E_UNSUPPORTED;
            }
            break;
        }
        case ONLP_SFP_CONTROL_RX_LOS:
        {
            if(IS_SFP_PORT(port))
            {
                if (onlp_file_read_int(&cpld_val, MODULE_RXLOS_SFP_FORMAT) < 0)
                {
                    AIM_LOG_ERROR("Unable to read rx_loss status from port(%d)\r\n", port);
                    rv = ONLP_STATUS_E_INTERNAL;
                }
                else
                {
                    rv = ONLP_STATUS_OK;
                }

                DIAG_PRINT("%s, Read Current CPLD SFP RXLOS value:0x%x, ", __FUNCTION__, cpld_val);

                if (IS_SFP0_PORT(port))
                {
                    cpld_val = cpld_val & MODULE_RXLOS_SFP0_MASK;
                    cpld_val = cpld_val >> 1;
                
                }
                else if (IS_SFP1_PORT(port))
                {
                    cpld_val = cpld_val & MODULE_RXLOS_SFP1_MASK;
                }
                else
                {
                    return ONLP_STATUS_E_INVALID;
                }

                DIAG_PRINT("%s, Read Current SFP port:%d RXLOS value:0x%x, ", __FUNCTION__, port, cpld_val);
                *value = cpld_val;
            }
            else
            {
                rv = ONLP_STATUS_E_UNSUPPORTED;
            }
            break;
        }
        case ONLP_SFP_CONTROL_TX_FAULT:
        {
            if (IS_SFP_PORT(port)) 
            {
                if (onlp_file_read_int(&cpld_val, MODULE_TXFAULT_SFP_FORMAT) < 0) {
                    AIM_LOG_ERROR("Unable to read tx_fault status from port(%d)\r\n", port);
                    rv = ONLP_STATUS_E_INTERNAL;
                }
                else 
                {
                    rv = ONLP_STATUS_OK;
                }

                DIAG_PRINT("%s, Read Current CPLD SFP TXFAULT value:0x%x, ", __FUNCTION__, cpld_val);

                if (IS_SFP0_PORT(port))
                {
                    cpld_val = cpld_val & MODULE_TXFAULT_SFP0_MASK;
                    cpld_val = cpld_val >> 1;
                
                }
                else if (IS_SFP1_PORT(port))
                {
                    cpld_val = cpld_val & MODULE_TXFAULT_SFP1_MASK;
                }
                else
                {
                    return ONLP_STATUS_E_INVALID;
                }

                DIAG_PRINT("%s, Read Current SFP port:%d TXFAULT value:0x%x, ", __FUNCTION__, port, cpld_val);
                *value = cpld_val;
            }
            else 
            {
                rv = ONLP_STATUS_E_UNSUPPORTED;
            }
            break;
        }
        case ONLP_SFP_CONTROL_TX_DISABLE:
        {
            if (IS_SFP_PORT(port))
            {
                if (onlp_file_read_int(&cpld_val, MODULE_TXDISABLE_SFP_FORMAT) < 0) 
                {
                    AIM_LOG_ERROR("Unable to read tx_disabled status from port(%d)\r\n", port);
                    rv = ONLP_STATUS_E_INTERNAL;
                }
                else 
                {
                    rv = ONLP_STATUS_OK;
                }

                DIAG_PRINT("%s, Read Current CPLD SFP TXDISABLE value:0x%x, ", __FUNCTION__, cpld_val);

                if (IS_SFP0_PORT(port))
                {
                    cpld_val = cpld_val & MODULE_TXDISABLE_SFP0_MASK;
                    cpld_val = cpld_val >> 1;
                }
                else if (IS_SFP1_PORT(port))
                {
                    cpld_val = cpld_val & MODULE_TXDISABLE_SFP1_MASK;
                }
                else
                {
                    return ONLP_STATUS_E_INVALID;
                }

                DIAG_PRINT("%s, Read Current SFP port:%d TXDISABLE value:0x%x, ", __FUNCTION__, port, cpld_val);
                *value = cpld_val;
            }
            else {
                rv = ONLP_STATUS_E_UNSUPPORTED;
            }
            break;
        }

        default:
            rv = ONLP_STATUS_E_UNSUPPORTED;
    }

    DIAG_PRINT("%s, port:%d, control:%d(%s), value:0x%X", __FUNCTION__, port, control, sfp_control_to_str(control), *value);

    if (rv == ONLP_STATUS_E_UNSUPPORTED)
    {
        AIM_LOG_INFO("%s:%d fail[%d]\n", __FUNCTION__, __LINE__, ONLP_STATUS_E_UNSUPPORTED);
    }

    return rv;
}

int
onlp_sfpi_denit(void)
{
    DIAG_PRINT("%s", __FUNCTION__);
    return ONLP_STATUS_OK;
}

