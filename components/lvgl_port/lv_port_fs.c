/**
 * @file lv_port_fs_templ.c
 *
 */

/*Copy this file as "lv_port_fs.c" and set this value to "1" to enable content*/
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

static void *fs_open(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode);
static lv_fs_res_t fs_close(lv_fs_drv_t *drv, void *file_p);
static lv_fs_res_t fs_read(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br);
static lv_fs_res_t fs_write(lv_fs_drv_t *drv, void *file_p, const void *buf, uint32_t btw, uint32_t *bw);
static lv_fs_res_t fs_seek(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence);
static lv_fs_res_t fs_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p);
static lv_fs_res_t fs_remove(lv_fs_drv_t *drv, const char *path);
static lv_fs_res_t fs_rename(lv_fs_drv_t *drv, const char *oldname, const char *newname);
static void *fs_dir_open(lv_fs_drv_t *drv, const char *path);
static lv_fs_res_t fs_dir_read(lv_fs_drv_t *drv, void *rddir_p, char *fn);
static lv_fs_res_t fs_dir_close(lv_fs_drv_t *drv, void *rddir_p);

/**********************
 * STATIC VARIABLES
 **********************/
// 文件系统操作的互斥锁
extern SemaphoreHandle_t fs_mutex;

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
    /*----------------------------------------------------
     * Initialize your storage device and File System
     * -------------------------------------------------*/
    fs_init();

    /*---------------------------------------------------
     * Register the file system interface in LVGL
     *--------------------------------------------------*/

    /*Add a simple drive to open images*/
    static lv_fs_drv_t fs_drv;
    lv_fs_drv_init(&fs_drv);

    /*Set up fields...*/
    fs_drv.letter = 'A';
    fs_drv.open_cb = fs_open;
    fs_drv.close_cb = fs_close;
    fs_drv.read_cb = fs_read;
    fs_drv.write_cb = fs_write;
    fs_drv.seek_cb = fs_seek;
    fs_drv.tell_cb = fs_tell;
    // fs_drv.remove_cb = fs_remove;
    // fs_drv.rename_cb = fs_rename;

    fs_drv.dir_close_cb = fs_dir_close;
    fs_drv.dir_open_cb = fs_dir_open;
    fs_drv.dir_read_cb = fs_dir_read;

    lv_fs_drv_register(&fs_drv);
}

/**********************
 * STATIC FUNCTIONS
 **********************/

/*Initialize your Storage device and File system.*/
static void fs_init(void)
{
    // 创建一个互斥锁来保护文件系统操作
}

/**
 * Open a file
 */
static void *fs_open(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode)
{
    xSemaphoreTake(fs_mutex, portMAX_DELAY); // 加锁

    const char *fs_path = path;
    if (fs_path[0] == drv->letter && fs_path[1] == ':')
    {
        fs_path += 2; /* Skip the drive letter and colon */
    }

    const char *flags = "";
    if (mode == LV_FS_MODE_WR)
    {
        flags = "wb";
    }
    else if (mode == LV_FS_MODE_RD)
    {
        flags = "rb";
    }
    else if (mode == (LV_FS_MODE_WR | LV_FS_MODE_RD))
    {
        flags = "r+b"; /* Open for read/write, create if not exists, don't truncate */
        /* If file doesn't exist, create it with "wb+" */
        FILE *temp_f = fopen(fs_path, "rb");
        if (temp_f == NULL)
        {
            flags = "wb+";
        }
        else
        {
            fclose(temp_f);
        }
    }

    FILE *f = fopen(fs_path, flags);
    if (f == NULL)
    {
        LV_LOG_WARN("fs_open: Failed to open file %s with mode %s, errno: %d", fs_path, flags, errno);
    }

    // ESP_LOGI(TAG, "[%s:%d] ", __FILE__, __LINE__);
    xSemaphoreGive(fs_mutex); // 解锁
    return f;
}

/**
 * Close an opened file
 */
static lv_fs_res_t fs_close(lv_fs_drv_t *drv, void *file_p)
{
    xSemaphoreTake(fs_mutex, portMAX_DELAY); // 加锁

    if (fclose((FILE *)file_p) == 0)
    {
        ESP_LOGI(TAG, "[%s:%d] ", __FILE__, __LINE__);
        xSemaphoreGive(fs_mutex); // 解锁
        return LV_FS_RES_OK;
    }
    LV_LOG_WARN("fs_close: Failed to close file, errno: %d", errno);

    // ESP_LOGI(TAG, "[%s:%d] ", __FILE__, __LINE__);
    xSemaphoreGive(fs_mutex); // 解锁
    return LV_FS_RES_FS_ERR;
}

/**
 * Read data from an opened file
 */
static lv_fs_res_t fs_read(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br)
{
    xSemaphoreTake(fs_mutex, portMAX_DELAY); // 加锁

    *br = fread(buf, 1, btr, (FILE *)file_p);
    if (*br == btr)
    {
        // ESP_LOGI(TAG, "[%s:%d] ", __FILE__, __LINE__);
        xSemaphoreGive(fs_mutex); // 解锁
        return LV_FS_RES_OK;
    }
    else if (ferror((FILE *)file_p))
    {
        LV_LOG_WARN("fs_read: Failed to read file, errno: %d", errno);
        // ESP_LOGI(TAG, "[%s:%d] ", __FILE__, __LINE__);
        xSemaphoreGive(fs_mutex); // 解锁
        return LV_FS_RES_FS_ERR;
    }

    // ESP_LOGI(TAG, "[%s:%d] ", __FILE__, __LINE__);
    xSemaphoreGive(fs_mutex); // 解锁
    return LV_FS_RES_OK;      /* EOF reached, read less than btr */
}

/**
 * Write into a file
 */
static lv_fs_res_t fs_write(lv_fs_drv_t *drv, void *file_p, const void *buf, uint32_t btw, uint32_t *bw)
{
    xSemaphoreTake(fs_mutex, portMAX_DELAY); // 加锁

    *bw = fwrite(buf, 1, btw, (FILE *)file_p);
    if (*bw == btw)
    {
        // ESP_LOGI(TAG, "[%s:%d] ", __FILE__, __LINE__);
        xSemaphoreGive(fs_mutex); // 解锁
        return LV_FS_RES_OK;
    }
    else
    {
        LV_LOG_WARN("fs_write: Failed to write file, errno: %d", errno);
        // ESP_LOGI(TAG, "[%s:%d] ", __FILE__, __LINE__);
        xSemaphoreGive(fs_mutex); // 解锁
        return LV_FS_RES_FS_ERR;
    }
}

/**
 * Set the read write pointer. Also expand the file size if necessary.
 */
static lv_fs_res_t fs_seek(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence)
{
    xSemaphoreTake(fs_mutex, portMAX_DELAY); // 加锁

    int fseek_whence;
    if (whence == LV_FS_SEEK_SET)
    {
        fseek_whence = SEEK_SET;
    }
    else if (whence == LV_FS_SEEK_CUR)
    {
        fseek_whence = SEEK_CUR;
    }
    else if (whence == LV_FS_SEEK_END)
    {
        fseek_whence = SEEK_END;
    }
    else
    {
        // ESP_LOGI(TAG, "[%s:%d] ", __FILE__, __LINE__);
        xSemaphoreGive(fs_mutex); // 解锁
        return LV_FS_RES_INV_PARAM;
    }

    if (fseek((FILE *)file_p, pos, fseek_whence) == 0)
    {
        // ESP_LOGI(TAG, "[%s:%d] ", __FILE__, __LINE__);
        xSemaphoreGive(fs_mutex); // 解锁
        return LV_FS_RES_OK;
    }
    LV_LOG_WARN("fs_seek: Failed to seek file, errno: %d", errno);

    // ESP_LOGI(TAG, "[%s:%d] ", __FILE__, __LINE__);
    xSemaphoreGive(fs_mutex); // 解锁
    return LV_FS_RES_FS_ERR;
}
/**
 * Give the position of the read write pointer
 */
static lv_fs_res_t fs_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p)
{
    xSemaphoreTake(fs_mutex, portMAX_DELAY); // 加锁

    long int pos = ftell((FILE *)file_p);
    if (pos != -1)
    {
        *pos_p = (uint32_t)pos;
        // ESP_LOGI(TAG, "[%s:%d] ", __FILE__, __LINE__);
        xSemaphoreGive(fs_mutex); // 解锁
        return LV_FS_RES_OK;
    }
    LV_LOG_WARN("fs_tell: Failed to get file position, errno: %d", errno);

    // ESP_LOGI(TAG, "[%s:%d] ", __FILE__, __LINE__);
    xSemaphoreGive(fs_mutex); // 解锁
    return LV_FS_RES_FS_ERR;
}

/**
 * Initialize a 'lv_fs_dir_t' variable for directory reading
 */
static void *fs_dir_open(lv_fs_drv_t *drv, const char *path)
{
    xSemaphoreTake(fs_mutex, portMAX_DELAY); // 加锁

    const char *fs_path = path;
    if (fs_path[0] == drv->letter && fs_path[1] == ':')
    {
        fs_path += 2; /* Skip the drive letter and colon */
    }

    DIR *dir = opendir(fs_path);
    if (dir == NULL)
    {
        LV_LOG_WARN("fs_dir_open: Failed to open directory %s, errno: %d", fs_path, errno);
    }

    // ESP_LOGI(TAG, "[%s:%d] ", __FILE__, __LINE__);
    xSemaphoreGive(fs_mutex); // 解锁
    return dir;
}

/**
 * Read the next filename form a directory.
 */
static lv_fs_res_t fs_dir_read(lv_fs_drv_t *drv, void *rddir_p, char *fn)
{
    xSemaphoreTake(fs_mutex, portMAX_DELAY); // 加锁

    struct dirent *entry;
    do
    {
        entry = readdir((DIR *)rddir_p);
        if (entry == NULL)
        {
            fn[0] = '\0'; /* No more entries */
            ESP_LOGI(TAG, "[%s:%d] ", __FILE__, __LINE__);
            xSemaphoreGive(fs_mutex); // 解锁
            return LV_FS_RES_OK;
        }
    } while (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0);

    if (entry->d_type == DT_DIR)
    {
        fn[0] = '/';
        strcpy(&fn[1], entry->d_name);
    }
    else
    {
        strcpy(fn, entry->d_name);
    }

    // ESP_LOGI(TAG, "[%s:%d] ", __FILE__, __LINE__);
    xSemaphoreGive(fs_mutex); // 解锁
    return LV_FS_RES_OK;
}

/**
 * Close the directory reading
 */
static lv_fs_res_t fs_dir_close(lv_fs_drv_t *drv, void *rddir_p)
{
    xSemaphoreTake(fs_mutex, portMAX_DELAY); // 加锁

    if (closedir((DIR *)rddir_p) == 0)
    {
        ESP_LOGI(TAG, "[%s:%d] ", __FILE__, __LINE__);
        xSemaphoreGive(fs_mutex); // 解锁
        return LV_FS_RES_OK;
    }
    LV_LOG_WARN("fs_dir_close: Failed to close directory, errno: %d", errno);

    // ESP_LOGI(TAG, "[%s:%d] ", __FILE__, __LINE__);
    xSemaphoreGive(fs_mutex); // 解锁
    return LV_FS_RES_FS_ERR;
}

static lv_fs_res_t fs_remove(lv_fs_drv_t *drv, const char *path)
{
    xSemaphoreTake(fs_mutex, portMAX_DELAY); // 加锁

    const char *fs_path = path;
    if (fs_path[0] == drv->letter && fs_path[1] == ':')
    {
        fs_path += 2; /* Skip the drive letter and colon */
    }

    if (remove(fs_path) == 0)
    {
        ESP_LOGI(TAG, "[%s:%d] ", __FILE__, __LINE__);
        xSemaphoreGive(fs_mutex); // 解锁
        return LV_FS_RES_OK;
    }
    LV_LOG_WARN("fs_remove: Failed to remove file %s, errno: %d", fs_path, errno);

    // ESP_LOGI(TAG, "[%s:%d] ", __FILE__, __LINE__);
    xSemaphoreGive(fs_mutex); // 解锁
    return LV_FS_RES_FS_ERR;
}

static lv_fs_res_t fs_rename(lv_fs_drv_t *drv, const char *oldname, const char *newname)
{
    xSemaphoreTake(fs_mutex, portMAX_DELAY); // 加锁

    const char *fs_oldname = oldname;
    if (fs_oldname[0] == drv->letter && fs_oldname[1] == ':')
    {
        fs_oldname += 2; /* Skip the drive letter and colon */
    }

    const char *fs_newname = newname;
    if (fs_newname[0] == drv->letter && fs_newname[1] == ':')
    {
        fs_newname += 2; /* Skip the drive letter and colon */
    }

    if (rename(fs_oldname, fs_newname) == 0)
    {
        ESP_LOGI(TAG, "[%s:%d] ", __FILE__, __LINE__);
        xSemaphoreGive(fs_mutex); // 解锁
        return LV_FS_RES_OK;
    }
    LV_LOG_WARN("fs_rename: Failed to rename file from %s to %s, errno: %d", fs_oldname, fs_newname, errno);

    // ESP_LOGI(TAG, "[%s:%d] ", __FILE__, __LINE__);
    xSemaphoreGive(fs_mutex); // 解锁
    return LV_FS_RES_FS_ERR;
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
