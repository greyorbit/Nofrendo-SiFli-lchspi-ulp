/**
 * Copyright (c) 2025 CJS<greyorbit@foxmail.com>
 * SPDX-License-Identifier: MIT
 */
#include <rtthread.h>
#include <rtdef.h>
#include <rtdevice.h>
#include <mem_section.h>
#include "bf0_hal.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "nofrendo/noftypes.h"
#include "nofrendo/event.h"
#include "nofrendo/gui.h"
#include "nofrendo/log.h"
#include "nofrendo/nes/nes.h"
#include "nofrendo/nes/nes_pal.h"
#include "nofrendo/nes/nesinput.h"
#include "nofrendo/nofconfig.h"
#include "nofrendo/osd.h"

#define OSD_HEAP_SIZE (1024 * 1024 * 7)
/* ----------------------------------------------------------------------------
 * function declaration
 * ---------------------------------------------------------------------------*/
extern const viddriver_t videoDriver;
extern void osd_inputInit(void);
/* ----------------------------------------------------------------------------
 * variable define
 * ---------------------------------------------------------------------------*/
static struct rt_timer osdTimer;
static void (*timer_callback)(void) = NULL;
struct rt_memheap osd_heap;
L2_RET_BSS_SECT_BEGIN(app_psram_ret_cache)
ALIGN(4) uint8_t osd_heap_mem[OSD_HEAP_SIZE] L2_RET_BSS_SECT(app_psram_ret_cache);
L2_RET_BSS_SECT_END
/* ----------------------------------------------------------------------------
 * function define
 * ---------------------------------------------------------------------------*/
static int osd_heap_init(void) {
    rt_err_t ret = rt_memheap_init(&osd_heap, "osd_heap", (void *)osd_heap_mem, OSD_HEAP_SIZE);
    if (ret != RT_EOK) {
        printf("osd_heap_init failed, ret=%d\n", ret);
    }
    return ret;
}
INIT_PREV_EXPORT(osd_heap_init);

static void osd_timer_expiry(void *dummy) {
    if (timer_callback)
        timer_callback();
}

static int logprint(const char *string) {
    return printf("%s", string);
}

extern void *mem_alloc(int size, bool prefer_fast_memory) {
    void *addr;
    // addr = malloc(size);
    addr = rt_memheap_alloc(&osd_heap, size);
    if (addr != NULL) {
        // printf("malloc %d bytes at %p\n", size, addr);
    } else {
        printf("malloc %d bytes failed\n", size);
        NVIC_SystemReset();
    }
    return addr;
}

extern void *mem_free(void *addr) {
    rt_memheap_free(addr);
    // free(addr);
    //  printf("free %p\n", addr);
}

extern void osd_setsound(void (*playfunc)(void *buffer, int size)) {
    printf("osd set sound\n");
}

extern void osd_getsoundinfo(sndinfo_t *info) {
    printf("osd get sound info\n");
}

extern int osd_init(void) {
    printf("osd init\n");
    nofrendo_log_chain_logfunc(logprint);
    osd_inputInit();
    return 0;
}

extern void osd_shutdown(void) {
    printf("osd shutdown\n");
}

extern int osd_main(int argc, char *argv[]) {
    config.filename = argv[0];
    return main_loop(argv[0], system_autodetect);
}

extern int osd_installtimer(int frequency, void *func, int funcsize, void *counter, int countersize) {
    /* these arguments are used only for djgpp, which needs to lock code/data */
    UNUSED(counter);
    UNUSED(countersize);
    UNUSED(funcsize);

    printf("Timer install,  freq=%d\n", frequency);

    rt_timer_init(&osdTimer, "", osd_timer_expiry, NULL, 0, RT_TIMER_FLAG_PERIODIC);
    rt_tick_t ticks = rt_tick_from_millisecond(1000 / frequency);
    rt_timer_control(&osdTimer, RT_TIMER_CTRL_SET_TIME, &ticks);
    rt_timer_start(&osdTimer);
    timer_callback = func;
    return 0;
}

extern void osd_fullname(char *fullname, const char *shortname) {
    strncpy(fullname, shortname, PATH_MAX);
}

extern char *osd_newextension(char *string, char *ext) {
    // dirty: assume both extensions is 3 characters
    size_t l = strlen(string);
    string[l - 3] = ext[1];
    string[l - 2] = ext[2];
    string[l - 1] = ext[3];
    return string;
}

extern int osd_makesnapname(char *filename, int len) {
    return -1;
}