#ifndef __DOWNLOAD_COMMANDS_H
#define __DOWNLOAD_COMMANDS_H
#include <platform/mt_typedefs.h>


void cmd_install_sig(const char *arg, void *data, unsigned sz);
void cmd_flash_mmc_img(const char *arg, void *data, unsigned sz);
void cmd_flash_mmc_sparse_img(const char *arg, void *data, unsigned sz);
void cmd_flash_mmc(const char *arg, void *data, unsigned sz);
void cmd_erase_mmc(const char *arg, void *data, unsigned sz);

#endif
