/**
 * Copyright (c) 2025 CJS<greyorbit@foxmail.com>
 * SPDX-License-Identifier: MIT
 */
#include <rtthread.h>
#include <rtdevice.h>
#include "bf0_hal.h"
#include "nofrendo/noftypes.h"
#include "nofrendo/event.h"
#include "nofrendo/gui.h"
#include "nofrendo/log.h"
#include "nofrendo/nes/nes.h"
#include "nofrendo/nes/nes_pal.h"
#include "nofrendo/nes/nesinput.h"
#include "nofrendo/nofconfig.h"
#include "nofrendo/osd.h"

/* ----------------------------------------------------------------------------
 * function declaration
 * ---------------------------------------------------------------------------*/

/* ----------------------------------------------------------------------------
 * variable define
 * ---------------------------------------------------------------------------*/
static uint8_t input_buf[64];
static uint8_t key_value;
static uint8_t prev_key_value;
/* ----------------------------------------------------------------------------
 * function define
 * ---------------------------------------------------------------------------*/
static rt_err_t osd_inputCallback(rt_device_t dev, rt_size_t size) {
    rt_device_read(dev, 0, input_buf, size);
    key_value = input_buf[0];
    return RT_EOK;
}

void osd_inputInit(void) {
    static bool is_init = false;
    if (is_init) {
        return;
    }
    rt_device_t uartDev;
    uartDev = rt_device_find("uart1");
    rt_device_set_rx_indicate(uartDev, osd_inputCallback);
    printf("osd input init\n");
    is_init = true;
}

extern void osd_getinput(void) {
    const int ev[32] = {event_joypad1_a,
                        event_joypad1_b,
                        event_joypad1_start,
                        event_joypad1_select,
                        event_joypad1_up,
                        event_joypad1_down,
                        event_joypad1_left,
                        event_joypad1_right,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0};
    event_t evh;
    if (key_value == 0xFF) {
        printf("reset\n");
        NVIC_SystemReset();
    }

    if (prev_key_value || key_value) {
        for (uint8_t i = 0; i < 8; i++) {
            if (key_value & (1 << i)) {
                evh = event_get(ev[i]);
                if (evh) {
                    printf("press i:%d\n", i);
                    evh(1);
                }
            } else {
                evh = event_get(ev[i]);
                if (evh) {
                    printf("release i:%d\n", i);
                    evh(0);
                }
            }
        }
    }
    prev_key_value = key_value;
}

extern void osd_getmouse(int *x, int *y, int *button) {}