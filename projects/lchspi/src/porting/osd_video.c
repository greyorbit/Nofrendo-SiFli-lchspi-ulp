/**
 * Copyright (c) 2025 CJS<greyorbit@foxmail.com>
 * SPDX-License-Identifier: MIT
 */
#include <rtthread.h>
#include <rtdevice.h>
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
static struct rt_semaphore lcd_sema;
static rt_device_t lcd_device;
static struct rt_device_graphic_info lcd_info;
static char fb[1]; // dummy
bitmap_t *myBitmap;
/* ----------------------------------------------------------------------------
 * function define
 * ---------------------------------------------------------------------------*/
/* initialise video */
static rt_err_t osd_videoFlushDone(rt_device_t dev, void *buffer) {
    rt_sem_release(&lcd_sema);
    return RT_EOK;
}

static int osd_videoInit(int width, int height) {
    static bool is_init = false;
    if (is_init) {
        return 0;
    }
    lcd_device = rt_device_find("lcd");
    RT_ASSERT(lcd_device != RT_NULL);
    rt_device_set_tx_complete(lcd_device, osd_videoFlushDone);
    if (rt_device_open(lcd_device, RT_DEVICE_OFLAG_RDWR) == RT_EOK) {
        if (rt_device_control(lcd_device, RTGRAPHIC_CTRL_GET_INFO, &lcd_info) == RT_EOK) {
            printf("Lcd lcd_info w:%d, h%d, bits_per_pixel %d\r\n", lcd_info.width, lcd_info.height, lcd_info.bits_per_pixel);
        } else {
            printf("Get lcd lcd_info failed\r\n");
            return -RT_ERROR;
        }
    } else {
        printf("Open lcd device failed\r\n");
        return -RT_ERROR;
    }

    uint16_t cf;
    if (16 == lcd_info.bits_per_pixel) {
        printf("RGB565\n");
        cf = RTGRAPHIC_PIXEL_FORMAT_RGB565;
    } else if (24 == lcd_info.bits_per_pixel) {
        cf = RTGRAPHIC_PIXEL_FORMAT_RGB888;
        printf("RGB888\n");
    } else {
        RT_ASSERT(0);
    }

    rt_device_control(lcd_device, RTGRAPHIC_CTRL_SET_BUF_FORMAT, &cf);

    rt_sem_init(&lcd_sema, "lcd_hal", 1, RT_IPC_FLAG_FIFO);
    uint8_t brightness = 80;
    rt_device_control(lcd_device, RTGRAPHIC_CTRL_SET_BRIGHTNESS, &brightness);
    is_init = true;
    return 0;
}

static void osd_videoShutdown(void) {}

/* set a video mode */
static int osd_videoSetMode(int width, int height) {
    return 0;
}

/* copy nes palette over to hardware */
uint16 myPalette[256];
static void osd_videoSetPalette(rgb_t *pal) {
    for (int i = 0; i < 256; i++) {
        myPalette[i] = (pal[i].b >> 3) + ((pal[i].g >> 2) << 5) + ((pal[i].r >> 3) << 11);
    }
}

/* clear all frames to a particular color */
static void osd_videoClear(uint8 color) {}

/* acquire the directbuffer for writing */
static bitmap_t *osd_videoLockWrite(void) {
    myBitmap = bmp_createhw((uint8 *)fb, NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT, NES_SCREEN_WIDTH * 2);
    return myBitmap;
}

/* release the resource */
static void osd_videoFreeWrite(int num_dirties, rect_t *dirty_rects) {
    // bmp_destroy(&myBitmap);
    printf("free_write num_dirties %d\n", num_dirties);
    if (dirty_rects != NULL) {
        printf("x %d, y %d, w %d, h %d\n", dirty_rects->x, dirty_rects->y, dirty_rects->w, dirty_rects->h);
    }
}

static uint16_t nesFrameBuf[NES_SCREEN_HEIGHT * NES_SCREEN_WIDTH];
static void osd_videoCustomBlit(bitmap_t *bmp, int num_dirties, rect_t *dirty_rects) {

    // printf("custom_blit num_dirties %d\n", num_dirties);
    if (dirty_rects != NULL) {
        // printf("x %d, y %d, w %d, h %d\n", dirty_rects->x, dirty_rects->y, dirty_rects->w, dirty_rects->h);
    }
    int i = 0;
    for (int h = 0; h < NES_SCREEN_HEIGHT; h++) {
        for (int w = 0; w < NES_SCREEN_WIDTH; w++) {
            nesFrameBuf[i++] = myPalette[bmp->data[w + h * NES_SCREEN_WIDTH]];
        }
    }

    int16_t x = (lcd_info.width - NES_SCREEN_WIDTH) / 2;
    int16_t y = (lcd_info.height - NES_SCREEN_HEIGHT) / 2;
    uint16_t w = NES_SCREEN_WIDTH;
    uint16_t h = NES_SCREEN_HEIGHT;
    if (w && h) {
        /*In case of some LCD need align to 2 or 4 pixel*/
        x = RT_ALIGN_DOWN(x, lcd_info.draw_align);
        y = RT_ALIGN_DOWN(y, lcd_info.draw_align);

        w = RT_ALIGN_DOWN(w, lcd_info.draw_align);
        h = RT_ALIGN_DOWN(h, lcd_info.draw_align);
        rt_graphix_ops(lcd_device)->set_window(x, y, x + w - 1, y + h - 1);

        rt_sem_control(&lcd_sema, RT_IPC_CMD_RESET, (void *)1);
        rt_sem_take(&lcd_sema, RT_WAITING_FOREVER);
        rt_graphix_ops(lcd_device)->draw_rect_async((const char *)nesFrameBuf, x, y, x + w - 1, y + h - 1);
    }
}

const viddriver_t videoDriver = {
    "Sifli LCD",         /* name */
    osd_videoInit,       /* init */
    osd_videoShutdown,   /* shutdown */
    osd_videoSetMode,    /* set_mode */
    osd_videoSetPalette, /* set_palette */
    osd_videoClear,      /* clear */
    osd_videoLockWrite,  /* lock_write */
    osd_videoFreeWrite,  /* free_write */
    osd_videoCustomBlit, /* custom_blit */
    false                /* invalidate flag */
};

extern void osd_getvideoinfo(vidinfo_t *info) {
    info->default_width = NES_SCREEN_WIDTH;
    info->default_height = NES_SCREEN_HEIGHT;
    info->driver = &videoDriver;
}