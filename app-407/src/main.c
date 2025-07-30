#include <stdint.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/fs_interface.h>
#include <zephyr/drivers/disk.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include "core/lv_obj.h"
#include "core/lv_obj_style_gen.h"
#include "lv_api_map_v8.h"
#include "lv_init.h"
#include "misc/lv_color.h"
#include "misc/lv_palette.h"
#include "misc/lv_timer.h"
#include "zephyr/drivers/display.h"
#include "zephyr/drivers/regulator.h"
#include <zephyr/storage/disk_access.h>
#include <ff.h>
#include <lvgl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#define DISPLAY_WIDTH 76
#define DISPLAY_HEIGHT 284
#define PIXEL_SIZE 2
static uint8_t buf[DISPLAY_WIDTH * DISPLAY_HEIGHT * PIXEL_SIZE];

static const struct device *lcd_led_reg = DEVICE_DT_GET(DT_NODELABEL(lcd_led_reg));
const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

#define MOUNT_POINT "/SD:"

FATFS fatfs_fs;

static struct fs_mount_t fat_fs_mnt = {
    .type = FS_FATFS,
    .mnt_point = MOUNT_POINT,
    .fs_data = &fatfs_fs,
    .storage_dev = (void *)"SD",
};

static void ls_dir(const char *path)
{
    struct fs_dir_t dirp;
    struct fs_dirent entry;

    fs_dir_t_init(&dirp);

    int res = fs_opendir(&dirp, path);
    if (res != 0) {
        LOG_ERR("fs_opendir failed (%d)", res);
        return;
    }

    LOG_INF("Contents of %s", path);
    while(1) {
        res = fs_readdir(&dirp, &entry);
        if (res != 0 || entry.name[0] == 0) {
            break;
        }

        if (entry.type == FS_DIR_ENTRY_DIR) {
            printk("[DIR ] %s\n", entry.name);
        } else {
            printk("[FILE] %s (%zu bytes)\n", entry.name, entry.size);
        }
    }

    fs_closedir(&dirp);
}

static int fatfs_mount(void)
{
    int ret;

    do {
        static const char *disk_pdrv = "SD";
        uint64_t memory_size_mb;
        uint32_t sector_count;
        uint32_t sector_size;

        ret = disk_access_init(disk_pdrv);
        if (ret != 0) {
            LOG_ERR("Disk access init failed: %d", ret);
            break;
        }

        ret = disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_COUNT, 
                                &sector_count);
        if (ret != 0) {
            LOG_ERR("Unable to get sector count");
            break;
        }

        ret = disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_SIZE, 
                                &sector_size);
        if (ret != 0) {
            LOG_ERR("Unable to get sector size");
            break;
        }

        LOG_INF("Sector size %u", sector_size);

        memory_size_mb = (uint64_t)sector_count * sector_size;
        LOG_INF("Memory size (MB) %u", (uint32_t)(memory_size_mb >> 20));

        ret = fs_mount(&fat_fs_mnt);
        if (ret != 0) {
            LOG_ERR("Mount failed: %d", ret);
            break;
        }

        LOG_INF("SD card mounted at %s", MOUNT_POINT);
    } while (0);

    return ret;
}

void fill_rgb565(uint16_t color)
{
    struct display_buffer_descriptor desc = {
        .width = DISPLAY_WIDTH,
        .height = DISPLAY_HEIGHT,
        .pitch = DISPLAY_WIDTH,
        .buf_size = sizeof(buf),
    };

    for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++) {
        buf[i * 2]     = (color >> 8) & 0xFF;
        buf[i * 2 + 1] = color & 0xFF;
    }

    int ret = display_write(display, 0, 0, &desc, buf);
    if (ret != 0) {
        LOG_ERR("Display write failed: %d", ret);
    }
}

#define BLUE_COLOR lv_color_hex(0x0000FF)

int main(void) 
{
    int ret;

    if (!device_is_ready(lcd_led_reg)) {
        LOG_ERR("LED regulator is not ready");
        return EXIT_FAILURE;
    }

    if (!device_is_ready(display)) {
        LOG_ERR("Display is not ready");
    }

    regulator_enable(lcd_led_reg);

    // fill_rgb565(0xFFE0);

    lv_init();
    lv_obj_t *scr = lv_scr_act();

    lv_obj_set_style_bg_color(scr, BLUE_COLOR, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    ret = fatfs_mount();
    if (ret != 0) {
        return EXIT_FAILURE;
    }
    
    ls_dir(MOUNT_POINT);

    while (1) {
        lv_timer_handler();
        k_sleep(K_MSEC(10));
    }

    return EXIT_SUCCESS;
}