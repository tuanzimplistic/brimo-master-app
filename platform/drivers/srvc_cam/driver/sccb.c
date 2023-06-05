/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * SCCB (I2C like) driver.
 *
 */
#include "sccb.h"
#include "sensor.h"
#include "esp_log.h"
#include "hwa_i2c_master.h"

#define LITTLETOBIG(x)          ((x<<8)|(x>>8))

static const char* TAG = "sccb";

int SCCB_Init(int pin_sda, int pin_scl)
{
    return 0;
}

int SCCB_Deinit(void)
{
    return 0;
}

uint8_t SCCB_Probe(void)
{
    uint8_t slave_addr = 0x0;

    I2C_inst_t x_i2c_inst;
    s8_I2C_Get_Inst (I2C_CAMERA, &x_i2c_inst);

    for (size_t i = 0; i < CAMERA_MODEL_MAX; i++)
    {
        if (slave_addr == camera_sensor[i].sccb_addr)
        {
            continue;
        }
        slave_addr = camera_sensor[i].sccb_addr;
        s8_I2C_Set_Slave_Addr (x_i2c_inst, slave_addr);
        if (s8_I2C_Write (x_i2c_inst, NULL, 0) == I2C_OK)
        {
            return slave_addr;
        }
    }
    return 0;
}

uint8_t SCCB_Read(uint8_t slv_addr, uint8_t reg)
{
    I2C_inst_t x_i2c_inst;
    s8_I2C_Get_Inst (I2C_CAMERA, &x_i2c_inst);
    s8_I2C_Set_Slave_Addr (x_i2c_inst, slv_addr);

    uint8_t data=0;
    if (s8_I2C_Read_Mem (x_i2c_inst, &reg, sizeof (reg), &data, sizeof (data)) == I2C_OK)
    {
        return data;
    }

    ESP_LOGE(TAG, "SCCB_Read Failed addr:0x%02x, reg:0x%02x, data:0x%02x", slv_addr, reg, data);
    return -1;
}

uint8_t SCCB_Write(uint8_t slv_addr, uint8_t reg, uint8_t data)
{
    I2C_inst_t x_i2c_inst;
    s8_I2C_Get_Inst (I2C_CAMERA, &x_i2c_inst);
    s8_I2C_Set_Slave_Addr (x_i2c_inst, slv_addr);

    if (s8_I2C_Write_Mem (x_i2c_inst, &reg, sizeof (reg), &data, sizeof (data)) == I2C_OK)
    {
        return 0;
    }

    ESP_LOGE(TAG, "SCCB_Write Failed addr:0x%02x, reg:0x%02x, data:0x%02x", slv_addr, reg, data);
    return -1;
}

uint8_t SCCB_Read16(uint8_t slv_addr, uint16_t reg)
{
    I2C_inst_t x_i2c_inst;
    s8_I2C_Get_Inst (I2C_CAMERA, &x_i2c_inst);
    s8_I2C_Set_Slave_Addr (x_i2c_inst, slv_addr);

    uint8_t data=0;
    uint16_t reg_htons = LITTLETOBIG(reg);
    if (s8_I2C_Read_Mem (x_i2c_inst, (uint8_t *)&reg_htons, sizeof (reg_htons), &data, sizeof (data)) == I2C_OK)
    {
        return data;
    }

    ESP_LOGE(TAG, "SCCB_Read Failed addr:0x%02x, reg:0x%04x, data:0x%02x", slv_addr, reg, data);
    return -1;
}

uint8_t SCCB_Write16(uint8_t slv_addr, uint16_t reg, uint8_t data)
{
    I2C_inst_t x_i2c_inst;
    s8_I2C_Get_Inst (I2C_CAMERA, &x_i2c_inst);
    s8_I2C_Set_Slave_Addr (x_i2c_inst, slv_addr);

    uint16_t reg_htons = LITTLETOBIG(reg);
    if (s8_I2C_Write_Mem (x_i2c_inst, (uint8_t *)&reg_htons, sizeof (reg_htons), &data, sizeof (data)) == I2C_OK)
    {
        return 0;
    }

    ESP_LOGE(TAG, "SCCB_Write Failed addr:0x%02x, reg:0x%04x, data:0x%02x", slv_addr, reg, data);
    return -1;
}
