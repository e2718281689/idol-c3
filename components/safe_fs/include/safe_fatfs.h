#pragma once

#include "esp_err.h"
#include "ff.h" // 包含原始 FatFS 的头文件，以便使用 FIL, FRESULT 等类型

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t safe_fs_init(const char* base_path);


FRESULT safe_f_open(FIL* fp, const TCHAR* path, BYTE mode);

FRESULT safe_f_close(FIL* fp);

FRESULT safe_f_read(FIL* fp, void* buff, UINT btr, UINT* br);

FRESULT safe_f_write(FIL* fp, const void* buff, UINT btw, UINT* bw);

FRESULT safe_f_tell(FIL* fp);

FRESULT safe_f_size(FIL* fp);

FRESULT safe_f_lseek(FIL* fp, FSIZE_t ofs);

FRESULT safe_f_mkdir(const TCHAR* path);

FRESULT safe_f_stat(const TCHAR* path, FILINFO* fno);

FRESULT safe_f_unlink(const TCHAR* path);

FRESULT safe_f_rename(const TCHAR* path_old, const TCHAR* path_new);

FRESULT safe_f_opendir(FF_DIR* dp, const TCHAR* path);

FRESULT safe_f_readdir(FF_DIR* dp, FILINFO* fno);

FRESULT safe_f_closedir(FF_DIR* dp);

#ifdef __cplusplus
}
#endif
