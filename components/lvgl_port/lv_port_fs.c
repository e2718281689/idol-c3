/**
 * @file lv_port_fs.c
 * @brief LVGL file system port using native FatFs API for ESP-IDF
 */

/*Enable this file at the top*/
#if 1

/*********************
 * INCLUDES
 *********************/
// #include "lv_port_fs_template.h"
#include "lvgl.h"
#include <stdio.h>  // For standard file I/O (fopen, fclose, fread, fwrite, fseek, ftell)
#include <string.h> // For string manipulation (e.g., strcpy, strlen)
#include <dirent.h> // For directory operations (opendir, readdir, closedir)
#include <errno.h>  // For error handling (errno)
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "ff.h"
#include "safe_fatfs.h" // 包含线程安全的 FatFs 封装头文件


/*********************
 * DEFINES
 *********************/
static const char *TAG = "LV_FS";
/**********************
 * TYPEDEFS
 **********************/

/**********************
 * STATIC PROTOTYPES
 **********************/
static void fs_init(void);

// 函数原型声明
static void *fs_open(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode);
static lv_fs_res_t fs_close(lv_fs_drv_t *drv, void *file_p);
static lv_fs_res_t fs_read(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br);
static lv_fs_res_t fs_write(lv_fs_drv_t *drv, void *file_p, const void *buf, uint32_t btw, uint32_t *bw);
static lv_fs_res_t fs_seek(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence);
static lv_fs_res_t fs_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p);
static void *fs_dir_open(lv_fs_drv_t *drv, const char *path);
static lv_fs_res_t fs_dir_read(lv_fs_drv_t *drv, void *rddir_p, char *fn);
static lv_fs_res_t fs_dir_close(lv_fs_drv_t *drv, void *rddir_p);
static lv_fs_res_t fs_remove(lv_fs_drv_t *drv, const char *path);
static lv_fs_res_t fs_rename(lv_fs_drv_t *drv, const char *oldname, const char *newname);


/**********************
 * STATIC VARIABLES
 **********************/
// 文件系统操作的互斥锁
// extern SemaphoreHandle_t spi_mutex;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 * MACROS
 **********************/

/**********************
 * GLOBAL FUNCTIONS
 **********************/

void lv_port_fs_init(void)
{
    fs_init();

    static lv_fs_drv_t fs_drv;
    lv_fs_drv_init(&fs_drv);

    // 将盘符 'A' 映射到 FatFs 的逻辑驱动器 "0:"
    // 如果您有多个驱动器，可以注册多个 lv_fs_drv_t
    fs_drv.letter = 'A';
    fs_drv.open_cb = fs_open;
    fs_drv.close_cb = fs_close;
    fs_drv.read_cb = fs_read;
    fs_drv.write_cb = fs_write;
    fs_drv.seek_cb = fs_seek;
    fs_drv.tell_cb = fs_tell;
    // fs_drv.remove_cb = fs_remove;
    // fs_drv.rename_cb = fs_rename;

    fs_drv.dir_open_cb = fs_dir_open;
    fs_drv.dir_close_cb = fs_dir_close;

    lv_fs_drv_register(&fs_drv);
}

/**********************
 * STATIC FUNCTIONS
 **********************/

/* 实际的SD卡和文件系统挂载应该在这里之前完成 */
static void fs_init(void)
{
    // 这个函数保持为空，因为文件系统的初始化（挂载SD卡）
    // 通常在应用程序的早期阶段完成，而不是在LVGL端口初始化时。
    // 确保在调用 lv_port_fs_init() 之前，您的 SD 卡已经挂载成功。
}

// 辅助函数：将 LVGL 路径 (例如 "A:/file.txt") 转换为 FatFs 路径 (例如 "0:/file.txt")
static const char *lv_path_to_fatfs_path(lv_fs_drv_t *drv, const char *lv_path)
{
    // 使用静态缓冲区来构建新路径
    static char fatfs_path[256];

    // 定义我们期望剥离的 VFS 挂载点前缀
    const char *vfs_prefix = "/sdcard";
    size_t vfs_prefix_len = strlen(vfs_prefix);

    printf("Input lv_path: %s\n", lv_path);

    // 情况一：路径是标准的 LVGL 格式 "A:..."
    if (lv_path[0] == drv->letter && lv_path[1] == ':') {
        // 转换 "A:/file.bin" 为 "0:/file.bin"
        snprintf(fatfs_path, sizeof(fatfs_path), "0:%s", lv_path + 2);
    }
    // 情况二：路径是以 "/sdcard" 开头的 VFS 格式
    else if (strncmp(lv_path, vfs_prefix, vfs_prefix_len) == 0) {
        // 转换 "/sdcard/file.bin" 为 "0:/file.bin"
        // lv_path + vfs_prefix_len 会指向 "/sdcard" 后面的部分，即 "/file.bin"
        snprintf(fatfs_path, sizeof(fatfs_path), "0:%s", lv_path + vfs_prefix_len);
    }
    // 情况三：其他意外的格式 (例如相对路径 "file.bin")
    else {

    }

    printf("Output fatfs_path: %s\n", fatfs_path);

    return fatfs_path;
}

static void *fs_open(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode)
{
    const char *fatfs_path = lv_path_to_fatfs_path(drv, path);

    BYTE flags = 0;
    if (mode == LV_FS_MODE_WR) {
        flags = FA_WRITE | FA_CREATE_ALWAYS;
    } else if (mode == LV_FS_MODE_RD) {
        flags = FA_READ;
    } else if (mode == (LV_FS_MODE_WR | LV_FS_MODE_RD)) {
        flags = FA_READ | FA_WRITE | FA_OPEN_ALWAYS;
    }

    // FatFs 使用 FIL 结构体，我们需要在堆上为其分配内存
    FIL *f = malloc(sizeof(FIL));
    if (f == NULL) {
        LV_LOG_WARN("fs_open: Failed to allocate memory for FIL");
        return NULL;
    }
    
    // xSemaphoreTake(spi_mutex, portMAX_DELAY);
    FRESULT res = safe_f_open(f, fatfs_path, flags);
    // xSemaphoreGive(spi_mutex);

    if (res != FR_OK) {
        free(f);
        LV_LOG_WARN("fs_open: f_open failed for %s, error %d", fatfs_path, res);
        return NULL;
    }

    return f;
}

static lv_fs_res_t fs_close(lv_fs_drv_t *drv, void *file_p)
{
    FIL *f = (FIL *)file_p;

    // xSemaphoreTake(spi_mutex, portMAX_DELAY);
    FRESULT res = safe_f_close(f);
    // xSemaphoreGive(spi_mutex);

    free(f); // 释放之前分配的内存

    return (res == FR_OK) ? LV_FS_RES_OK : LV_FS_RES_FS_ERR;
}

static lv_fs_res_t fs_read(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br)
{
    FIL *f = (FIL *)file_p;
    *br = 0;

    // xSemaphoreTake(spi_mutex, portMAX_DELAY);
    FRESULT res = safe_f_read(f, buf, btr, (UINT *)br);
    // xSemaphoreGive(spi_mutex);

    if (res != FR_OK) {
        LV_LOG_WARN("fs_read: f_read failed, error %d", res);
        return LV_FS_RES_FS_ERR;
    }
    
    return LV_FS_RES_OK; // 即使 *br < btr (EOF), FatFs 也返回 FR_OK
}

static lv_fs_res_t fs_write(lv_fs_drv_t *drv, void *file_p, const void *buf, uint32_t btw, uint32_t *bw)
{
    FIL *f = (FIL *)file_p;
    *bw = 0;

    // xSemaphoreTake(spi_mutex, portMAX_DELAY);
    FRESULT res = safe_f_write(f, buf, btw, (UINT *)bw);
    // xSemaphoreGive(spi_mutex);

    if (res != FR_OK || *bw < btw) {
        LV_LOG_WARN("fs_write: f_write failed, error %d", res);
        return LV_FS_RES_FS_ERR;
    }

    return LV_FS_RES_OK;
}

static lv_fs_res_t fs_seek(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence)
{
    FIL *f = (FIL *)file_p;
    FRESULT res;

    // xSemaphoreTake(spi_mutex, portMAX_DELAY);
    // f_lseek 的 whence 只有 SEEK_SET, SEEK_CUR, SEEK_END，但 FatFs f_lseek 只接受绝对偏移量
    // 因此需要根据 whence 计算出绝对位置
    FSIZE_t new_pos = pos;
    if (whence == LV_FS_SEEK_CUR) {
        new_pos = safe_f_tell(f) + pos;
    } else if (whence == LV_FS_SEEK_END) {
        new_pos = safe_f_size(f) + pos; // 注意：pos 通常为负数
    } else if (whence != LV_FS_SEEK_SET) {
        // xSemaphoreGive(spi_mutex);
        return LV_FS_RES_INV_PARAM;
    }

    res = safe_f_lseek(f, new_pos);
    // xSemaphoreGive(spi_mutex);

    if (res != FR_OK) {
        LV_LOG_WARN("fs_seek: f_lseek failed, error %d", res);
        return LV_FS_RES_FS_ERR;
    }
    
    return LV_FS_RES_OK;
}

static lv_fs_res_t fs_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p)
{
    FIL *f = (FIL *)file_p;
    
    // xSemaphoreTake(spi_mutex, portMAX_DELAY);
    *pos_p = safe_f_tell(f);
    // xSemaphoreGive(spi_mutex);
    
    // f_tell 返回的是 FSIZE_t，这里我们假设它不会超过 uint32_t 的范围
    return LV_FS_RES_OK;
}

static void *fs_dir_open(lv_fs_drv_t *drv, const char *path)
{
    const char *fatfs_path = lv_path_to_fatfs_path(drv, path);

    // 为目录句柄分配内存
    FF_DIR *d = malloc(sizeof(FF_DIR));
    if (d == NULL) {
        LV_LOG_WARN("fs_dir_open: Failed to allocate memory for FF_DIR");
        return NULL;
    }

    // xSemaphoreTake(spi_mutex, portMAX_DELAY);
    FRESULT res = safe_f_opendir(d, fatfs_path);
    // xSemaphoreGive(spi_mutex);

    if (res != FR_OK) {
        free(d);
        LV_LOG_WARN("fs_dir_open: f_opendir failed for %s, error %d", fatfs_path, res);
        return NULL;
    }

    return d;
}

static lv_fs_res_t fs_dir_read(lv_fs_drv_t *drv, void *rddir_p, char *fn)
{
    FF_DIR *d = (FF_DIR *)rddir_p;
    FILINFO fno;
    FRESULT res;

    fn[0] = '\0'; // 默认为空字符串

    // xSemaphoreTake(spi_mutex, portMAX_DELAY);
    do {
        res = safe_f_readdir(d, &fno);
        // 如果没有更多文件或发生错误，跳出循环
        if (res != FR_OK || fno.fname[0] == 0) {
            break; 
        }
        // FatFs 不会列出 "." 和 ".."，所以不需要手动过滤
    } while (0);
    // xSemaphoreGive(spi_mutex);
    
    if (res != FR_OK) {
        LV_LOG_WARN("fs_dir_read: f_readdir failed, error %d", res);
        return LV_FS_RES_FS_ERR;
    }

    if (fno.fname[0] != 0) { // 如果成功读到一个条目
        if (fno.fattrib & AM_DIR) {
            // 如果是目录，LVGL 期望在名称前加 '/'
            sprintf(fn, "/%s", fno.fname);
        } else {
            strcpy(fn, fno.fname);
        }
    }

    return LV_FS_RES_OK;
}

static lv_fs_res_t fs_dir_close(lv_fs_drv_t *drv, void *rddir_p)
{
    FF_DIR *d = (FF_DIR *)rddir_p;

    // xSemaphoreTake(spi_mutex, portMAX_DELAY);
    FRESULT res = safe_f_closedir(d);
    // xSemaphoreGive(spi_mutex);

    free(d); // 释放目录句柄内存

    return (res == FR_OK) ? LV_FS_RES_OK : LV_FS_RES_FS_ERR;
}

static lv_fs_res_t fs_remove(lv_fs_drv_t *drv, const char *path)
{
    const char *fatfs_path = lv_path_to_fatfs_path(drv, path);

    // xSemaphoreTake(spi_mutex, portMAX_DELAY);
    FRESULT res = safe_f_unlink(fatfs_path);
    // xSemaphoreGive(spi_mutex);

    if (res != FR_OK) {
        LV_LOG_WARN("fs_remove: f_unlink failed for %s, error %d", fatfs_path, res);
        return LV_FS_RES_FS_ERR;
    }

    return LV_FS_RES_OK;
}

static lv_fs_res_t fs_rename(lv_fs_drv_t *drv, const char *oldname, const char *newname)
{
    const char *fatfs_old = lv_path_to_fatfs_path(drv, oldname);
    const char *fatfs_new = lv_path_to_fatfs_path(drv, newname);

    // xSemaphoreTake(spi_mutex, portMAX_DELAY);
    FRESULT res = safe_f_rename(fatfs_old, fatfs_new);
    // xSemaphoreGive(spi_mutex);

    if (res != FR_OK) {
        LV_LOG_WARN("fs_rename: f_rename failed from %s to %s, error %d", fatfs_old, fatfs_new, res);
        return LV_FS_RES_FS_ERR;
    }

    return LV_FS_RES_OK;
}

#else /*Enable this file at the top*/
/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
