/**
 * Copyright (c) 2025 CJS<greyorbit@foxmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "rtthread.h"
#include "bf0_hal.h"
#include "drv_common.h"
#include "drv_io.h"
#include "stdio.h"
#include "dfs_file.h"
#include "spi_msd.h"
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "nofrendo/nofrendo.h"

#define FS_ROOT "root"
#define FS_ROOT_PATH "/"
#define FS_ROOT_OFFSET 0X00000000
#define FS_ROOT_LEN 500 * 1024 * 1024 // 500M

#define FS_MSIC "misc"
#define FS_MSIC_PATH "/misc"
#define FS_MSIC_OFFSET (FS_ROOT_OFFSET + FS_ROOT_LEN)
#define FS_MSIC_LEN 500 * 1024 * 1024 // 500MB

#define FS_BLOCK_SIZE 0x200

#define KEY2 43
/* ----------------------------------------------------------------------------
 * function declaration
 * ---------------------------------------------------------------------------*/

/* ----------------------------------------------------------------------------
 * variable define
 * ---------------------------------------------------------------------------*/
char fileName[256] = "SuperMarioBros.nes";
static int file_cnt = 0;
static uint32_t file_index;
/* ----------------------------------------------------------------------------
 * function define
 * ---------------------------------------------------------------------------*/

struct rt_device *fal_mtd_msd_device_create(char *name, long offset, long len) {

    rt_device_t msd = rt_device_find("sd0");
    if (msd == NULL) {
        rt_kprintf("Error: the flash device name (sd0) is not found.\n");
        return NULL;
    }
    struct msd_device *msd_dev = (struct msd_device *)msd->user_data;

    struct msd_device *msd_file_dev = (struct msd_device *)rt_malloc(sizeof(struct msd_device));
    if (msd_file_dev) {
        msd_file_dev->parent.type = RT_Device_Class_MTD;
#ifdef RT_USING_DEVICE_OPS
        msd_file_dev->parent.ops = msd_dev->parent.ops;
#else
        msd_file_dev->parent.init = msd_dev->parent.init;
        msd_file_dev->parent.open = msd_dev->parent.open;
        msd_file_dev->parent.close = msd_dev->parent.close;
        msd_file_dev->parent.read = msd_dev->parent.read;
        msd_file_dev->parent.write = msd_dev->parent.write;
        msd_file_dev->parent.control = msd_dev->parent.control;
#endif
        msd_file_dev->offset = offset;
        msd_file_dev->spi_device = msd_dev->spi_device;
        msd_file_dev->geometry.bytes_per_sector = FS_BLOCK_SIZE;
        msd_file_dev->geometry.block_size = FS_BLOCK_SIZE;
        msd_file_dev->geometry.sector_count = len;

        rt_device_register(&msd_file_dev->parent, name, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_REMOVABLE | RT_DEVICE_FLAG_STANDALONE);
        rt_kprintf("fal_mtd_msd_device_create dev:sd0 part:%s offset:0x%x, size:0x%x\n", name, msd_file_dev->offset,
                   msd_file_dev->geometry.sector_count);
        return RT_DEVICE(&msd_file_dev->parent);
        ;
    }
    return NULL;
}

int mnt_init(void) {
    uint16_t time_out = 100;
    while (time_out--) {
        rt_thread_mdelay(30);
        if (rt_device_find("sd0"))
            break;
    }
    fal_mtd_msd_device_create(FS_ROOT, FS_ROOT_OFFSET >> 9, FS_ROOT_LEN >> 9);
    fal_mtd_msd_device_create(FS_MSIC, FS_MSIC_OFFSET >> 9, FS_MSIC_LEN >> 9);
    if (dfs_mount(FS_ROOT, FS_ROOT_PATH, "elm", 0, 0) == 0) // fs exist
    {
        rt_kprintf("mount fs on flash to root success\n");
    } else {
        // auto mkfs, remove it if you want to mkfs manual
        rt_kprintf("mount fs on flash to root fail\n");
        if (dfs_mkfs("elm", FS_ROOT) == 0) // Format file system
        {
            rt_kprintf("make elm fs on flash sucess, mount again\n");
            if (dfs_mount(FS_ROOT, "/", "elm", 0, 0) == 0)
                rt_kprintf("mount fs on flash success\n");
            else {
                rt_kprintf("mount to fs on flash fail\n");
                return RT_ERROR;
            }
        } else
            rt_kprintf("dfs_mkfs elm flash fail\n");
    }
    extern int mkdir(const char *path, mode_t mode);
    mkdir(FS_MSIC_PATH, 0);
    if (dfs_mount(FS_MSIC, FS_MSIC_PATH, "elm", 0, 0) == 0) // fs exist
    {
        rt_kprintf("mount fs on flash to FS_MSIC success\n");
    } else {
        // auto mkfs, remove it if you want to mkfs manual
        rt_kprintf("mount fs on flash to FS_MISC fail\n");
        if (dfs_mkfs("elm", FS_MSIC) == 0) // Format file system
        {
            rt_kprintf("make elm fs on flash sucess, mount again\n");

            if (dfs_mount(FS_MSIC, "/misc", "elm", 0, 0) == 0)
                rt_kprintf("mount fs on flash success\n");
            else
                rt_kprintf("mount to fs on flash fail err=%d\n", rt_get_errno());
        } else
            rt_kprintf("dfs_mkfs elm flash fail\n");
    }
    return RT_EOK;
}
INIT_ENV_EXPORT(mnt_init);

int file_list(const char *name, int num) {
    DIR *p_dir;
    int err = 0;
    struct dirent *entry;
    struct stat statbuf;

    p_dir = opendir("/");

    if (p_dir == NULL) {
        printf("Can not open dir\n");
        return -EIO;
    }
    int cnt = 0;
    while ((entry = readdir(p_dir)) != NULL) {
        if (stat("/", &statbuf) == 0) {
            if (S_ISDIR(statbuf.st_mode)) {
                if (strstr(entry->d_name, ".nes") != NULL) {
                    if (name != NULL) {
                        if (num == cnt) {
                            strcpy(name, entry->d_name);
                            break;
                        }
                    } else {
                        printf("%d: %s\n", cnt, entry->d_name);
                    }
                    cnt++;
                }
            }
        }
    }
    closedir(p_dir);

    return cnt;
}

void key2_irq(void *args) {
    if (rt_pin_read(KEY2) == 1) {
        rt_kprintf("KEY2 pressed\n");
    } else {
        rt_kprintf("KEY2 released\n");
    }
}

void gpio_init(void) {
    rt_pin_mode(KEY2, PIN_MODE_INPUT);
    rt_pin_attach_irq(KEY2, PIN_IRQ_MODE_RISING_FALLING, key2_irq, RT_NULL);
    rt_pin_irq_enable(KEY2, PIN_IRQ_ENABLE);
}

int main(void) {
    printf("Hello Nofrendo!\n");
    printf("HEAP_BEGIN: %p, HEAP_END: %p\n", (void *)HEAP_BEGIN, (void *)HEAP_END);
    printf("HEAP_SIZE: %d\n", (int)(HEAP_END) - (int)(HEAP_BEGIN));
    gpio_init();
    file_index = HAL_Get_backup(RTC_BACKUP_MODULE_RECORD);
    file_cnt = file_list(NULL, 0);
    printf("file_cnt %d, file_index %d\n", file_cnt, file_index);
    if (file_index >= file_cnt) {
        file_index = 0;
    }
    file_list(fileName, file_index);
    file_index++;
    HAL_Set_backup(RTC_BACKUP_MODULE_RECORD, file_index);
    printf("fileName %s\n", fileName);
    char *file = &fileName[0];
    nofrendo_main(1, &file);
    while (1) {
        rt_thread_mdelay(1000);
        printf("never reach here\n");
    }
    return 0;
}
