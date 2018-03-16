/*
 * Used for crc32.c
 */

#ifndef CRC_H
#define CRC_H

#include <stdint.h>
#include <sys/types.h>
#include "zconf.h"

uint32_t crc32 (uint32_t, const unsigned char *, uint);
uint32_t crc32_no_comp (uint32_t crc, const Bytef *buf, uInt len);

#endif
