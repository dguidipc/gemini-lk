/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2015. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#include "ufs_aio_cfg.h"
#include "ufs_aio_types.h"
#include "ufs_aio.h"
#include "ufs_aio_utils.h"

#if defined(MTK_UFS_DRV_LK)
#include <debug.h>
#include <string.h> /* for memcpy, memset */
#include <mt_gpt.h>
#include <malloc.h>
#endif

#if defined(MTK_UFS_DRV_CTP)
#include <common.h> /* for size_t */
#endif

struct ufs_ocs_error g_ufs_ocs_error_str[] = {
	{ "ABORT", 0 },
	{ "ABORT_TM", 1 },
	{ "FATAL_ERROR_TRANSACTION_TYPE_TM", 2 },
	{ "FATAL_ERROR_COMMAND_TYPE_NOT_ONE", 3 },
	{ "FATAL_ERROR_RUO_LESS_THAN_EIGHT", 4 },
	{ "FATAL_ERROR_RUL_LESS_THAN_EIGHT", 5 },
	{ "FATAL_ERROR_RTT_UPIU_MISMATCH_SIZE", 6 },
	{ "FATAL_ERROR_DATAIN_UPIU_MISMATCH_SIZE", 7 },
	{ "FATAL_ERROR_NOPI_UPIU_MISMATCH_SIZE", 8 },
	{ "FATAL_ERROR_REJECT_UPIU_MISMATCH_SIZE", 9 },
	{ "FATAL_ERROR_WRONG_RECEIVED_UPIU", 10 },
	{ "FATAL_ERROR_LOSE_TRANSFER_REQUEST", 11 },
	{ "FATAL_ERROR_LOSE_TM_REQUEST", 12 },
	{ "INVALID_PRDT_ATTRIBUTES", 13 },
	{ "INVALID_TASK_MANAGEMENT_FUNCTION_ATTRIBUTES_DSL_NOT_ZERO", 14 },
	{ "INVALID_TASK_MANAGEMENT_FUNCTION_ATTRIBUTES_FUNC_ERROR", 15 },
	{ "INVALID_CMD_TABLE_ATTRIBUTES_TRANSACTION_TYPE_NOT_ONE", 16 },
	{ "INVALID_CMD_TABLE_ATTRIBUTES_COMMAND_SET_TYPE_NOT_ZERO", 17 },
	{ "INVALID_CMD_TABLE_ATTRIBUTES_DSL_NOT_ZERO", 18 },
	{ "INVALID_CMD_TABLE_ATTRIBUTES_PRDT_OFFSET_LESS_THAN_8", 19 },
	{ "INVALID_CMD_TABLE_ATTRIBUTES_TRANSACTION_TYPE", 20 },
	{ "PEER_COMMUNICATION_FAILURE_TM", 21 },
	{ "PEER_COMMUNICATION_FAILURE", 22 },
	{ "MISMATCH_TASK_MANAGEMENT_REQUEST_SIZE", 23 },
	{ "MISMATCH_DATA_BUFFER_SIZE", 24 },
	{ "MISMATCH_RESPONSE_UPIU_SIZE", 25 },
	{ "MISMATCH_TASK_MANAGEMENT_RESPONSE_SIZE_UPIU", 26 },
	{ "MISMATCH_TASK_MANAGEMENT_RESPONSE_SIZE_DSL_NOT_ZERO", 27 },
	{ NULL, 0xFF },
};

int ufs_mtk_dump_asc_ascq(struct ufs_hba * hba, u8 asc, u8 ascq)
{
	int skip_err = 0;

	if (0x21 == asc) {
		if (0x00 == ascq)
			UFS_DBG_LOGE("[UFS] err: LBA out of range!\n");
	} else if (0x25 == asc) {
		if (0x00 == ascq)
			UFS_DBG_LOGE("[UFS] err: Logical unit not supported!\n");
	} else if (0x29 == asc) {
		if (0x00 == ascq) {
			if (0 == (hba->drv_status & UFS_DRV_STS_TEST_UNIT_READY_ALL_DEVICE))
				UFS_DBG_LOGE("[UFS] err: Power on, reset, or bus device reset occupied\n");
			else
				skip_err = 1;
		}
	}

	if (!skip_err)
		UFS_DBG_LOGE("[UFS] err: Sense Data: ASC=%x, ASCQ=%x\n", asc, ascq);

	return skip_err;
}

#ifndef UFS_CFG_SINGLE_COMMAND
unsigned long find_last_bit(const unsigned long *addr, unsigned long size)
{
	unsigned long target = *addr;
	unsigned long ret;

	for (ret = 0; ret < size; ret++, target = target >> 1) {
		if (0x1 & target)
			return ret;
	}

	return ret;
}

int test_and_set_bit(int nr, volatile unsigned long *addr)
{
	unsigned long mask = (1UL << ((nr) % (sizeof(unsigned long) * 8)));
	unsigned long old;

	old = *addr;
	*addr = old | mask;

	return (old & mask) != 0;
}
#endif

#if 0
void clear_bit(int nr, volatile unsigned long *addr)
{
	unsigned long mask = (1UL << ((nr) % (sizeof(unsigned long) * 8)));
	unsigned long *p = ((unsigned long *)addr) + ((nr) / (sizeof(unsigned long) * 8));

	*p &= ~mask;
}

void set_bit(int nr, volatile unsigned long *addr)
{
	unsigned long mask = (1UL << ((nr) % (sizeof(unsigned long) * 8)));
	unsigned long *p = ((unsigned long *)addr) + ((nr) / (sizeof(unsigned long) * 8));

	*p  |= mask;
}
#endif

static inline unsigned long get_utf16(unsigned c, enum utf16_endian endian)
{
	switch (endian) {
		default:
			return c;
		case UTF16_LITTLE_ENDIAN:
			return le16_to_cpu(c);
		case UTF16_BIG_ENDIAN:
			return be16_to_cpu(c);
	}
}

int utf16s_to_utf8s(const wchar_t *pwcs, int inlen, enum utf16_endian endian, u8 *s, int maxout)
{
	u8 *op;
	unsigned long u;

	op = s;

	while (inlen > 0 && maxout > 0) {

		u = get_utf16(*pwcs, endian);

		if (!u)
			break;

		pwcs++;
		inlen--;

		if (u > 0x7f)
			return 0;   // not support non-ascii code in UFS AIO driver.
		else {
			*op++ = (u8) u;
			maxout--;
		}
	}
	return op - s;
}

#if !defined(MTK_UFS_DRV_DA) && !defined(MTK_UFS_DRV_LK)
unsigned long strlcpy(char *dest, const char *src, unsigned long size)
{
	unsigned long ret = strlen(src);

	if (size) {
		unsigned long len = (ret >= size) ? size - 1 : ret;
		memcpy(dest, src, len);
		dest[len] = '\0';
	}

	return ret;
}
#endif

#if defined(MTK_UFS_DRV_DA)
int strcmp(const char *cs, const char *ct)
{
	signed char __res;
	while (1) {
		if ((__res = *cs - *ct++) != 0 || !*cs++)
			break;
	}
	return __res;
}
#endif

#if defined(MTK_UFS_DRV_CTP)
int strncmp(const char *s1, const char *s2, size_t n)
{
    for ( ; n > 0; s1++, s2++, --n)
	if (*s1 != *s2)
	    return ((*(unsigned char *)s1 < *(unsigned char *)s2) ? -1 : +1);
	else if (*s1 == '\0')
	    return 0;
    return 0;
}
#endif

int ufs_util_sanitize_inquiry_string(unsigned char *s, int len)
{
	int processed_char = 0;

	for (; len > 0; (--len, ++s, ++processed_char)) {
		if (*s == 0)
			return processed_char; // return string length

		if (*s < 0x20 || *s > 0x7e)
			*s = ' ';
	}

	*s = 0; // ensure end of string is 0

	return processed_char;
}


