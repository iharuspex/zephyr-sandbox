#include <stdint.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/fs_interface.h>
#include <zephyr/drivers/disk.h>
#include <zephyr/storage/disk_access.h>
#include <ff.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#define MOUNT_POINT "/SD:"

FATFS fatfs_fs;

static struct fs_mount_t fat_fs_mnt = {
    .type = FS_FATFS,
    .mnt_point = MOUNT_POINT,
    .fs_data = &fatfs_fs,
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

static int fatfs_mount()
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

        // Mount drive
        fat_fs_mnt.storage_dev = (void *)"SD";

        ret = fs_mount(&fat_fs_mnt);
        if (ret != 0) {
            LOG_ERR("Mount failed: %d", ret);
            break;
        }

        LOG_INF("SD card mounted at %s", MOUNT_POINT);
    } while (0);

    return ret;
}

int main(void) 
{
    int ret;

    ret = fatfs_mount();
    if (ret != 0) {
        return EXIT_FAILURE;
    }
    
    ls_dir(MOUNT_POINT);

    return EXIT_SUCCESS;
}