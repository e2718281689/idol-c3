/**
 * @file lv_port_fs_templ.c
 *
 */

/*Copy this file as "lv_port_fs.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
// #include "lv_port_fs_template.h"
#include "lvgl.h"
#include <stdio.h>   // For standard file I/O (fopen, fclose, fread, fwrite, fseek, ftell)
#include <string.h>  // For string manipulation (e.g., strcpy, strlen)
#include <dirent.h>  // For directory operations (opendir, readdir, closedir)
#include <errno.h>   // For error handling (errno)

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void fs_init(void);

static void * fs_open(lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode);
static lv_fs_res_t fs_close(lv_fs_drv_t * drv, void * file_p);
static lv_fs_res_t fs_read(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br);
static lv_fs_res_t fs_write(lv_fs_drv_t * drv, void * file_p, const void * buf, uint32_t btw, uint32_t * bw);
static lv_fs_res_t fs_seek(lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence);
static lv_fs_res_t fs_tell(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p);
static lv_fs_res_t fs_remove(lv_fs_drv_t * drv, const char * path);
static lv_fs_res_t fs_rename(lv_fs_drv_t * drv, const char * oldname, const char * newname);
static void * fs_dir_open(lv_fs_drv_t * drv, const char * path);
static lv_fs_res_t fs_dir_read(lv_fs_drv_t * drv, void * rddir_p, char * fn);
static lv_fs_res_t fs_dir_close(lv_fs_drv_t * drv, void * rddir_p);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
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
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your Storage device and File system.*/
static void fs_init(void)
{
    /* Nothing to do for standard C file I/O */
}

/**
 * Open a file
 * @param drv       pointer to a driver where this function belongs
 * @param path      path to the file beginning with the driver letter (e.g. S:/folder/file.txt)
 * @param mode      read: FS_MODE_RD, write: FS_MODE_WR, both: FS_MODE_RD | FS_MODE_WR
 * @return          a file descriptor or NULL on error
 */
static void * fs_open(lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode)
{
    const char * fs_path = path;
    if (fs_path[0] == drv->letter && fs_path[1] == ':') {
        fs_path += 2; /* Skip the drive letter and colon */
    }

    const char * flags = "";
    if (mode == LV_FS_MODE_WR) {
        flags = "wb";
    } else if (mode == LV_FS_MODE_RD) {
        flags = "rb";
    } else if (mode == (LV_FS_MODE_WR | LV_FS_MODE_RD)) {
        flags = "r+b"; /* Open for read/write, create if not exists, don't truncate */
        /* If file doesn't exist, create it with "wb+" */
        FILE * temp_f = fopen(fs_path, "rb");
        if (temp_f == NULL) {
            flags = "wb+";
        } else {
            fclose(temp_f);
        }
    }

    FILE * f = fopen(fs_path, flags);
    if (f == NULL) {
        LV_LOG_WARN("fs_open: Failed to open file %s with mode %s, errno: %d", fs_path, flags, errno);
    }
    return f;
}

/**
 * Close an opened file
 * @param drv       pointer to a driver where this function belongs
 * @param file_p    pointer to a file_t variable. (opened with fs_open)
 * @return          LV_FS_RES_OK: no error or  any error from @lv_fs_res_t enum
 */
static lv_fs_res_t fs_close(lv_fs_drv_t * drv, void * file_p)
{
    if (fclose((FILE *)file_p) == 0) {
        return LV_FS_RES_OK;
    }
    LV_LOG_WARN("fs_close: Failed to close file, errno: %d", errno);
    return LV_FS_RES_FS_ERR;
}

/**
 * Read data from an opened file
 * @param drv       pointer to a driver where this function belongs
 * @param file_p    pointer to a file_t variable.
 * @param buf       pointer to a memory block where to store the read data
 * @param btr       number of Bytes To Read
 * @param br        the real number of read bytes (Byte Read)
 * @return          LV_FS_RES_OK: no error or  any error from @lv_fs_res_t enum
 */
static lv_fs_res_t fs_read(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br)
{
    *br = fread(buf, 1, btr, (FILE *)file_p);
    if (*br == btr) {
        return LV_FS_RES_OK;
    } else if (ferror((FILE *)file_p)) {
        LV_LOG_WARN("fs_read: Failed to read file, errno: %d", errno);
        return LV_FS_RES_FS_ERR;
    }
    return LV_FS_RES_OK; /* EOF reached, read less than btr */
}

/**
 * Write into a file
 * @param drv       pointer to a driver where this function belongs
 * @param file_p    pointer to a file_t variable
 * @param buf       pointer to a buffer with the bytes to write
 * @param btw       Bytes To Write
 * @param bw        the number of real written bytes (Bytes Written). NULL if unused.
 * @return          LV_FS_RES_OK: no error or  any error from @lv_fs_res_t enum
 */
static lv_fs_res_t fs_write(lv_fs_drv_t * drv, void * file_p, const void * buf, uint32_t btw, uint32_t * bw)
{
    *bw = fwrite(buf, 1, btw, (FILE *)file_p);
    if (*bw == btw) {
        return LV_FS_RES_OK;
    } else {
        LV_LOG_WARN("fs_write: Failed to write file, errno: %d", errno);
        return LV_FS_RES_FS_ERR;
    }
}

/**
 * Set the read write pointer. Also expand the file size if necessary.
 * @param drv       pointer to a driver where this function belongs
 * @param file_p    pointer to a file_t variable. (opened with fs_open )
 * @param pos       the new position of read write pointer
 * @param whence    tells from where to interpret the `pos`. See @lv_fs_whence_t
 * @return          LV_FS_RES_OK: no error or  any error from @lv_fs_res_t enum
 */
static lv_fs_res_t fs_seek(lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence)
{
    int fseek_whence;
    if (whence == LV_FS_SEEK_SET) {
        fseek_whence = SEEK_SET;
    } else if (whence == LV_FS_SEEK_CUR) {
        fseek_whence = SEEK_CUR;
    } else if (whence == LV_FS_SEEK_END) {
        fseek_whence = SEEK_END;
    } else {
        return LV_FS_RES_INV_PARAM;
    }

    if (fseek((FILE *)file_p, pos, fseek_whence) == 0) {
        return LV_FS_RES_OK;
    }
    LV_LOG_WARN("fs_seek: Failed to seek file, errno: %d", errno);
    return LV_FS_RES_FS_ERR;
}
/**
 * Give the position of the read write pointer
 * @param drv       pointer to a driver where this function belongs
 * @param file_p    pointer to a file_t variable.
 * @param pos_p     pointer to to store the result
 * @return          LV_FS_RES_OK: no error or  any error from @lv_fs_res_t enum
 */
static lv_fs_res_t fs_tell(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p)
{
    long int pos = ftell((FILE *)file_p);
    if (pos != -1) {
        *pos_p = (uint32_t)pos;
        return LV_FS_RES_OK;
    }
    LV_LOG_WARN("fs_tell: Failed to get file position, errno: %d", errno);
    return LV_FS_RES_FS_ERR;
}

/**
 * Initialize a 'lv_fs_dir_t' variable for directory reading
 * @param drv       pointer to a driver where this function belongs
 * @param path      path to a directory
 * @return          pointer to the directory read descriptor or NULL on error
 */
static void * fs_dir_open(lv_fs_drv_t * drv, const char * path)
{
    const char * fs_path = path;
    if (fs_path[0] == drv->letter && fs_path[1] == ':') {
        fs_path += 2; /* Skip the drive letter and colon */
    }

    DIR * dir = opendir(fs_path);
    if (dir == NULL) {
        LV_LOG_WARN("fs_dir_open: Failed to open directory %s, errno: %d", fs_path, errno);
    }
    return dir;
}

/**
 * Read the next filename form a directory.
 * The name of the directories will begin with '/'
 * @param drv       pointer to a driver where this function belongs
 * @param rddir_p   pointer to an initialized 'lv_fs_dir_t' variable
 * @param fn        pointer to a buffer to store the filename
 * @return          LV_FS_RES_OK: no error or  any error from @lv_fs_res_t enum
 */
static lv_fs_res_t fs_dir_read(lv_fs_drv_t * drv, void * rddir_p, char * fn)
{
    struct dirent * entry;
    do {
        entry = readdir((DIR *)rddir_p);
        if (entry == NULL) {
            fn[0] = '\0'; /* No more entries */
            return LV_FS_RES_OK;
        }
    } while (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0);

    if (entry->d_type == DT_DIR) {
        fn[0] = '/';
        strcpy(&fn[1], entry->d_name);
    } else {
        strcpy(fn, entry->d_name);
    }
    return LV_FS_RES_OK;
}

/**
 * Close the directory reading
 * @param drv       pointer to a driver where this function belongs
 * @param rddir_p   pointer to an initialized 'lv_fs_dir_t' variable
 * @return          LV_FS_RES_OK: no error or  any error from @lv_fs_res_t enum
 */
static lv_fs_res_t fs_dir_close(lv_fs_drv_t * drv, void * rddir_p)
{
    if (closedir((DIR *)rddir_p) == 0) {
        return LV_FS_RES_OK;
    }
    LV_LOG_WARN("fs_dir_close: Failed to close directory, errno: %d", errno);
    return LV_FS_RES_FS_ERR;
}

static lv_fs_res_t fs_remove(lv_fs_drv_t * drv, const char * path)
{
    const char * fs_path = path;
    if (fs_path[0] == drv->letter && fs_path[1] == ':') {
        fs_path += 2; /* Skip the drive letter and colon */
    }

    if (remove(fs_path) == 0) {
        return LV_FS_RES_OK;
    }
    LV_LOG_WARN("fs_remove: Failed to remove file %s, errno: %d", fs_path, errno);
    return LV_FS_RES_FS_ERR;
}

static lv_fs_res_t fs_rename(lv_fs_drv_t * drv, const char * oldname, const char * newname)
{
    const char * fs_oldname = oldname;
    if (fs_oldname[0] == drv->letter && fs_oldname[1] == ':') {
        fs_oldname += 2; /* Skip the drive letter and colon */
    }

    const char * fs_newname = newname;
    if (fs_newname[0] == drv->letter && fs_newname[1] == ':') {
        fs_newname += 2; /* Skip the drive letter and colon */
    }

    if (rename(fs_oldname, fs_newname) == 0) {
        return LV_FS_RES_OK;
    }
    LV_LOG_WARN("fs_rename: Failed to rename file from %s to %s, errno: %d", fs_oldname, fs_newname, errno);
    return LV_FS_RES_FS_ERR;
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
