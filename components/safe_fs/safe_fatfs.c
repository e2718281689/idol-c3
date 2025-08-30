// safe_fatfs.c

#include <stdio.h>  // For standard file I/O (fopen, fclose, fread, fwrite, fseek, ftell)
#include <string.h> // For string manipulation (e.g., strcpy, strlen)
#include <dirent.h> // For directory operations (opendir, readdir, closedir)
#include <errno.h>  // For error handling (errno)
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "ff.h"

// 定义一个静态的互斥锁句柄
extern SemaphoreHandle_t spi_mutex;

esp_err_t safe_fatfs_init(void)
{
    // // 防止重复初始化
    // if (s_fatfs_mutex) {
    //     return ESP_OK;
    // }

    // s_fatfs_mutex = xSemaphoreCreateMutex();
    // if (s_fatfs_mutex == NULL) {
    //     // ESP_LOGE("SAFE_FATFS", "Failed to create mutex!"); // 建议添加日志
    //     return ESP_FAIL;
    // }
    return ESP_OK;
}

// 宏定义，用于简化每个函数的加锁和解锁逻辑
#define FATFS_LOCK()                                       \
    do {                                                   \
        xSemaphoreTake(spi_mutex, portMAX_DELAY);      \
    } while (0)

#define FATFS_UNLOCK()                                     \
    do {                                                   \
        xSemaphoreGive(spi_mutex);                     \
    } while (0)


FRESULT safe_f_open(FIL* fp, const TCHAR* path, BYTE mode)
{
    FATFS_LOCK();
    FRESULT res = f_open(fp, path, mode);
    FATFS_UNLOCK();
    return res;
}

FRESULT safe_f_close(FIL* fp)
{
    FATFS_LOCK();
    FRESULT res = f_close(fp);
    FATFS_UNLOCK();
    return res;
}

FRESULT safe_f_read(FIL* fp, void* buff, UINT btr, UINT* br)
{
    FATFS_LOCK();
    FRESULT res = f_read(fp, buff, btr, br);
    FATFS_UNLOCK();
    return res;
}

FRESULT safe_f_write(FIL* fp, const void* buff, UINT btw, UINT* bw)
{
    FATFS_LOCK();
    FRESULT res = f_write(fp, buff, btw, bw);
    FATFS_UNLOCK();
    return res;
}

FRESULT safe_f_tell(FIL* fp)
{
    FATFS_LOCK();
    FRESULT res = f_tell(fp);
    FATFS_UNLOCK();
    return res;
}
    
FRESULT safe_f_size(FIL* fp)
{
    FATFS_LOCK();
    FRESULT res = f_size(fp);
    FATFS_UNLOCK();
    return res;
}


FRESULT safe_f_lseek(FIL* fp, FSIZE_t ofs)
{
    FATFS_LOCK();
    FRESULT res = f_lseek(fp, ofs);
    FATFS_UNLOCK();
    return res;
}

FRESULT safe_f_stat(const TCHAR* path, FILINFO* fno)
{
    FATFS_LOCK();
    FRESULT res = f_stat(path, fno);
    FATFS_UNLOCK();
    return res;
}

FRESULT safe_f_unlink(const TCHAR* path)
{
    FATFS_LOCK();
    FRESULT res = f_unlink(path);
    FATFS_UNLOCK();
    return res;
}

FRESULT safe_f_rename(const TCHAR* path_old, const TCHAR* path_new)
{
    FATFS_LOCK();
    FRESULT res = f_rename(path_old, path_new);
    FATFS_UNLOCK();
    return res;
}

FRESULT safe_f_opendir(FF_DIR* dp, const TCHAR* path)
{
    FATFS_LOCK();
    FRESULT res = f_opendir(dp, path);
    FATFS_UNLOCK();
    return res;
}

FRESULT safe_f_readdir(FF_DIR* dp, FILINFO* fno)
{
    FATFS_LOCK();
    FRESULT res = f_readdir(dp, fno);
    FATFS_UNLOCK();
    return res;
}

FRESULT safe_f_closedir(FF_DIR* dp)
{
    FATFS_LOCK();
    FRESULT res = f_closedir(dp);
    FATFS_UNLOCK();
    return res;
}
