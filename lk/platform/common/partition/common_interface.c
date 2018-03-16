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
#include <platform/mt_typedefs.h>
#ifdef MTK_GPT_SCHEME_SUPPORT
#include <platform/partition.h>
#else
#include <mt_partition.h>
#endif
#include <platform/errno.h>
#include <debug.h>
#include <string.h>
#include <part_interface.h>
#include <part_status.h>

typedef struct {
	char official_name[12];
	char alt_name1[12];
	char alt_name2[12];
	char alt_name3[12];
} PART_TRANS_TBL_ENTRY;

const PART_TRANS_TBL_ENTRY g_part_name_trans_tbl[] = {
	{"preloader", "PRELOADER", "NULL",    "NULL"      },
	{"seccfg",    "SECCFG",    "SECCNFG", "NULL"      },
	{"uboot",     "UBOOT",     "lk",      "LK"        },
	{"boot",      "BOOT",      "bootimg", "BOOTIMG"   },
	{"recovery",  "RECOVERY",  "NULL",    "NULL"      },
	{"secro",     "SECRO",     "sec_ro",  "SEC_RO"    },
	{"logo",      "LOGO",      "NULL",    "NULL"      },
	{"system",    "SYSTEM",    "android", "ANDROID"   },
	{"userdata",  "USERDATA",  "usrdata", "USRDATA"   },
	{"frp",       "FRP",       "NULL",    "NULL"      },
	{"scp1",      "SCP1",      "NULL",    "NULL"      },
	{"scp2",      "SCP2",      "NULL",    "NULL"      },
	{"odmdtbo",   "ODMDTBO",   "NULL",    "NULL"      },
	{"NULL",      "NULL",      "NULL",    "NULL"      },
};

/* name: should be translated to official name first, used for backward compatible */
int partition_get_index_by_name(char *part_name)
{
	int index = -1;
	int found = 0;
	const PART_TRANS_TBL_ENTRY *entry = g_part_name_trans_tbl;

	/* if name matches one of the official names, use translation table */
	for (; 0 != strcmp(entry->official_name, "NULL"); entry++) {
		if (0 == strcmp(part_name, entry->official_name)) {
			found = 1;
			break;
		}
	}

	if (found) {
		index = partition_get_index(entry->official_name);
		if (index != -1)
			return index;

		/* try alt_name1 */
		index = partition_get_index(entry->alt_name1);
		if (index != -1)
			return index;

		/* try alt_name2 */
		index = partition_get_index(entry->alt_name2);
		if (index != -1)
			return index;

		/* try alt_name3 */
		index = partition_get_index(entry->alt_name3);
		if (index != -1)
			return index;
	} else {
		/* don't do name translation if name is not found in table */
		index = partition_get_index(part_name);
		if (index != -1)
			return index;
	}
	return index;
}

char* partition_get_name_by_index(int index) {
	int part_name_map_len = sizeof(g_part_name_map) / sizeof(struct part_name_map);
	if (index < part_name_map_len) {
		return g_part_name_map[index].r_name;
	} else {
		dprintf(CRITICAL, "[%s]%s: index %d is larger than part_name_map size %d\n", PART_COMMON_TAG, __FUNCTION__, index, part_name_map_len);
		return NULL;
	}
}

u64 partition_get_size_by_name(char *part_name)
{
	int index;
	index = partition_get_index_by_name(part_name);
	if (index == -1)
		return PART_NOT_EXIST;

	return partition_get_size(index);
}

u64 partition_get_offset_by_name(char *part_name)
{
	int index;
	index = partition_get_index_by_name(part_name);
	if (index == -1)
		return PART_NOT_EXIST;

	return partition_get_offset(index);
}


char* partition_get_real_name(char *part_name)
{
	int index;
	index = partition_get_index_by_name(part_name);
	if (index == -1)
		ASSERT(0);

	return partition_get_name_by_index(index);
}

int partition_exists(char *part_name)
{
	if (partition_get_index_by_name(part_name) == -1)
		return PART_NOT_EXIST;
	else
		return PART_OK;
}

int partition_support_erase(char *part_name)
{
	int index;
	index = partition_get_index_by_name(part_name);
	if (index == -1)
		return PART_NOT_EXIST;
	return is_support_erase(index);
}

int partition_support_flash(char *part_name)
{
	int index;
	index = partition_get_index_by_name(part_name);
	if (index == -1)
		return PART_NOT_EXIST;
	return is_support_flash(index);
}


