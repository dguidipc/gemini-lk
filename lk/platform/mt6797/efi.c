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

#include "printf.h"
#include "malloc.h"
#include "string.h"
#include "lib/crc.h"
#include "platform/mt_typedefs.h"
#include "platform/partition.h"
#include "platform/efi.h"

#ifndef min
#define min(x, y)   (x < y ? x : y)
#endif

extern bool cmdline_append(const char *append_string);
static part_t *part_ptr = NULL;
u32 sgpt_partition_lba_size;

/*
 ********** Definition of Debug Macro **********
 */
#define TAG "[GPT_LK]"

#define LEVEL_ERR   (0x0001)
#define LEVEL_INFO  (0x0004)

#define DEBUG_LEVEL (LEVEL_ERR | LEVEL_INFO)

#define efi_err(fmt, args...)   \
do {    \
    if (DEBUG_LEVEL & LEVEL_ERR) {  \
        dprintf(CRITICAL, fmt, ##args); \
    }   \
} while (0)

#define efi_info(fmt, args...)  \
do {    \
    if (DEBUG_LEVEL & LEVEL_INFO) {  \
        dprintf(CRITICAL, fmt, ##args);    \
    }   \
} while (0)


/*
 ********** Definition of GPT buffer **********
 */

/*
 ********** Definition of CRC32 Calculation **********
 */
#if 0
static int crc32_table_init = 0;
static u32 crc32_table[256];

static void init_crc32_table(void)
{
	int i, j;
	u32 crc;

	if (crc32_table_init) {
		return;
	}

	for (i = 0; i < 256; i++) {
		crc = i;
		for (j = 0; j < 8; j++) {
			if (crc & 1) {
				crc = (crc >> 1) ^ 0xEDB88320;
			} else {
				crc >>= 1;
			}
		}
		crc32_table[i] = crc;
	}
	crc32_table_init = 1;
}

static u32 crc32(u32 crc, u8 *p, u32 len)
{
	init_crc32_table();

	while (len--) {
		crc ^= *p++;
		crc = (crc >> 8) ^ crc32_table[crc & 255];
	}

	return crc;
}
#endif

static u32 efi_crc32(u8 *p, u32 len)
{
	//return (crc32(~0L, p, len) ^ ~0L);
	//return crc32(0, p, len);  /* from zlib */
	return (crc32_no_comp(~0L, p, len) ^ ~0L);  /* from zlib */
}

static void w2s(u8 *dst, int dst_max, u16 *src, int src_max)
{
	int i = 0;
	int len = min(src_max, dst_max - 1);

	while (i < len) {
		if (!src[i]) {
			break;
		}
		dst[i] = src[i] & 0xFF;
		i++;
	}

	dst[i] = 0;

	return;
}

extern u64 g_emmc_user_size;

static u64 last_lba(u32 part_id)
{
	part_dev_t *dev;
	dev = mt_part_get_device();
	/* Only support USER region now */
	return (g_emmc_user_size / dev->blkdev->blksz - 1);
}


static u64 get_gpt_header_last_usable_lba()
{
	part_dev_t *dev = mt_part_get_device();
	return (last_lba(dev->blkdev->part_user)-((dev->blkdev->blksz+PART_MAX_COUNT*sizeof(gpt_entry))/dev->blkdev->blksz));

}

static void set_spgt_partition_lba_size(u32 size)
{
	sgpt_partition_lba_size = size;

}

static u32 get_spgt_partition_lba_size()
{
	return sgpt_partition_lba_size;

}

static u32 entries_crc32;
static void set_entries_crc32(u32 crc32)
{
	entries_crc32 = crc32;
}
static u32 get_entries_crc32(void)
{
	return entries_crc32;
}

static int read_data(u8 *buf, u32 part_id, u64 lba, u64 size)
{
	int err;
	part_dev_t *dev;

	dev = mt_part_get_device();
	if (!dev) {
		efi_err("%sread data, err(no dev)\n", TAG);
		return 1;
	}

	//err = dev->read(dev, lba, size / 512, buf, part_id);
	err = dev->read(dev, lba * dev->blkdev->blksz, buf, (int)size, part_id);
	if (err != (int)size) {
		efi_err("%sread data, err(%d)\n", TAG, err);
		return err;
	}

	return 0;
}

static int write_data(u64 lba, u8 *buf, u64 size,  u32 part_id)
{
	int err;
	part_dev_t *dev;

	dev = mt_part_get_device();
	if (!dev) {
		efi_err("%swrite data, err(no dev)\n", TAG);
		return 1;
	}
	err = dev->write(dev, buf, lba * dev->blkdev->blksz, (int)size, part_id);
	if (err != (int)size) {
		efi_err("%s write data, err(%d)\n", TAG, err);
		return err;
	}

	return 0;
}

static int parse_gpt_header(u32 part_id, u64 header_lba, u8 *header_buf, u8 *entries_buf)
{
	int i;

	int err;
	u32 calc_crc, orig_header_crc;
	u64 entries_real_size, entries_read_size;
	part_dev_t *dev = mt_part_get_device();

	gpt_header *header = (gpt_header *)header_buf;
	gpt_entry *entries = (gpt_entry *)entries_buf;

	struct part_meta_info *info;

	err = read_data(header_buf, part_id, header_lba, dev->blkdev->blksz);
	if (err) {
		efi_err("%sread header(part_id=%d,lba=%llx), err(%d)\n",
		        TAG, part_id, header_lba, err);
		return err;
	}

	if (header->signature != GPT_HEADER_SIGNATURE) {
		efi_err("%scheck header, err(signature 0x%llx!=0x%llx)\n",
		        TAG, header->signature, GPT_HEADER_SIGNATURE);
		return 1;
	}

	orig_header_crc = header->header_crc32;
	header->header_crc32 = 0;
	calc_crc = efi_crc32((u8 *)header, header->header_size);

	if (orig_header_crc != calc_crc) {
		efi_err("%scheck header, err(crc 0x%x!=0x%x(calc))\n",
		        TAG, orig_header_crc, calc_crc);
		return 1;
	}

	header->header_crc32 = orig_header_crc;

	if (header->my_lba != header_lba) {
		efi_err("%scheck header, err(my_lba 0x%llx!=0x%llx)\n",
		        TAG, header->my_lba, header_lba);
		return 1;
	}

	entries_real_size = (u64)header->num_partition_entries * header->sizeof_partition_entry;
	entries_read_size = (u64)((header->num_partition_entries + 3) / 4) * dev->blkdev->blksz;

	err = read_data(entries_buf, part_id, header->partition_entry_lba, entries_read_size);
	if (err) {
		efi_err("%sread entries(part_id=%d,lba=%llx), err(%d)\n",
		        TAG, part_id, header->partition_entry_lba, err);
		return err;
	}

	calc_crc = efi_crc32((u8 *)entries, (u32)entries_real_size);

	if (header->partition_entry_array_crc32 != calc_crc) {
		efi_err("%scheck header, err(entries crc 0x%x!=0x%x(calc))\n",
		        TAG, header->partition_entry_array_crc32, calc_crc);
		return 1;
	}

	for (i = 0; (u32)i < header->num_partition_entries; i++) {
		/* break if the partition entry is empty */
		if (!entries[i].starting_lba) {
			break;
		}
		part_ptr[i].start_sect = (unsigned long)entries[i].starting_lba;
		part_ptr[i].nr_sects = (unsigned long)(entries[i].ending_lba - entries[i].starting_lba + 1);
		part_ptr[i].part_attr = (unsigned long)entries[i].attributes;
		part_ptr[i].part_id = dev->blkdev->part_user;
		info = malloc(sizeof(*info));
		if (!info) {
			continue;
		}
		part_ptr[i].info = info;
		if ((entries[i].partition_name[0] & 0xFF00) == 0) {
			w2s(part_ptr[i].info->name, PART_META_INFO_NAMELEN, entries[i].partition_name, GPT_ENTRY_NAME_LEN);
		} else {
			memcpy(part_ptr[i].info->name, entries[i].partition_name, 64);
		}
		efi_info("%s[%d]name=%s, part_id=%d, start_sect=0x%lx, nr_sects=0x%lx\n", TAG, i,
		         part_ptr[i].info ? (char *)part_ptr[i].info->name : "unknown",
		         part_ptr[i].part_id, part_ptr[i].start_sect, part_ptr[i].nr_sects);
	}

	return 0;
}


int read_gpt(part_t *part)
{
	int err;
	u64 lba;
	u8 *pgpt_header, *pgpt_entries;
	u8 *sgpt_header, *sgpt_entries;
	part_dev_t *dev = mt_part_get_device();
	u32 part_id = dev->blkdev->part_user;

	part_ptr = part;

	efi_info("%sParsing Primary GPT now...\n", TAG);

	pgpt_header = (u8 *)malloc(dev->blkdev->blksz);
	if (!pgpt_header) {
		efi_err("%smalloc memory(pgpt header), err\n", TAG);
		goto next_try;
	}
	memset(pgpt_header, 0, dev->blkdev->blksz);

	pgpt_entries = (u8 *)malloc(16384);
	if (!pgpt_entries) {
		efi_err("%smalloc memory(pgpt entries), err\n", TAG);
		goto next_try;
	}
	memset(pgpt_entries, 0, 16384);

	err = parse_gpt_header(part_id, 1, pgpt_header, pgpt_entries);
	if (!err) {
		goto find;
	}

next_try:
	efi_info("%sParsing Secondary GPT now...\n", TAG);

	sgpt_header = (u8 *)malloc(dev->blkdev->blksz);
	if (!sgpt_header) {
		efi_err("%smalloc memory(sgpt header), err\n", TAG);
		goto next_try;
	}
	memset(sgpt_header, 0, dev->blkdev->blksz);

	sgpt_entries = (u8 *)malloc(16384);
	if (!sgpt_entries) {
		efi_err("%smalloc memory(sgpt entries), err\n", TAG);
		goto next_try;
	}
	memset(sgpt_entries, 0, 16384);

	lba = last_lba(part_id);
	err = parse_gpt_header(part_id, lba, sgpt_header, sgpt_entries);
	if (!err) {
		goto find;
	}

	efi_err("%sFailure to find valid GPT.\n", TAG);
	return err;

find:
	efi_info("%sSuccess to find valid GPT.\n", TAG);
	return 0;
}

static void pack_pmbr_data(void *ptr)
{
	pmbr *mbr = (pmbr *)ptr;
	u64 nr_sects;
	u16 nr_sects_split[2];
	u16 *nr_sects_split_ptr;
	part_dev_t *dev = mt_part_get_device();

	nr_sects = last_lba(dev->blkdev->part_user) + 1;
	mbr->partition_record[0].sys_ind = 0xEE;
	mbr->partition_record[0].start_sector = 0x1;
	/* Suffer from mbr->partition_record[0].nr_sects address not 4-byte aligned(data abort will occur if directly write u32 data), split it into two 2-byte */
	nr_sects_split[0] =     ((nr_sects < 0xFFFFFFFF) ? (nr_sects - 1) : 0xFFFFFFFF) & 0xFFFF;
	nr_sects_split[1] =     (((nr_sects < 0xFFFFFFFF) ? (nr_sects - 1) : 0xFFFFFFFF) >> 16) & 0xFFFF;
	nr_sects_split_ptr = (u16 *)(&((mbr->partition_record[0].nr_sects)));
	*nr_sects_split_ptr = nr_sects_split[0];
	*(nr_sects_split_ptr +1) = nr_sects_split[1];
	mbr->signature = 0xAA55;
}

static void pack_pheader_data(void *data)
{
	part_dev_t *dev = mt_part_get_device();

	gpt_header *header = (gpt_header *)data;
	header->reserved = 0;
	header->alternate_lba = last_lba(dev->blkdev->part_user);
	header->last_usable_lba = get_gpt_header_last_usable_lba();
	header->partition_entry_array_crc32 = get_entries_crc32();
	header->header_crc32 = efi_crc32(data, sizeof(gpt_header));
}

static void pack_entries_data(void *data)
{
	int i;
	int nr_parts = 0;
	part_dev_t *dev = mt_part_get_device();

	gpt_entry *entries = (gpt_entry *)data;

	//calculate how many partitions
	for (i = 0; i < PART_MAX_COUNT; i++) {
		if (!entries[i].partition_name[0]) {
			nr_parts = i;
			break;
		}
	}
	efi_err("%s number of partitions: %d.\n", TAG, nr_parts);

	/* Update gpt entries */
	for (i = nr_parts-1; i >= 0; i--) {
		/* it's last partition (do not have reserved partition ex: no flashinfo, otp), only need to update last partition */
		/* may not go to here unless customer remove flashinfo partition */
		if (i == nr_parts-1 && !entries[i].starting_lba && !entries[i].ending_lba) {
			entries[i].ending_lba = last_lba(dev->blkdev->part_user) - get_spgt_partition_lba_size() ;
			entries[i].starting_lba = entries[i-1].ending_lba + 1;
			break;
		}

		/* Process reserved partitions(flashinfo, otp) and last partition before reserved partition(ex: userdata or intsd) */
		/* Reserved partition size is stored in ending_lba, sgpt partition size can be retrived from  get_spgt_partition_size()(stored in header->reserved)*/
		if (!entries[i].starting_lba) {
			/* it's a reserved partition and it's last partition, entries[i].ending_lba not empty(partition size is here)  */
			if (i == nr_parts-1 && entries[i].ending_lba) {
				entries[i].starting_lba = last_lba(dev->blkdev->part_user) - get_spgt_partition_lba_size() - entries[i].ending_lba + 1;
				entries[i].ending_lba = last_lba(dev->blkdev->part_user) - get_spgt_partition_lba_size() ;
			}
			/* reserved parttiion but not last one  */
			/* may not go to here uless there exists more than one reserved partitions (ex: otp + flashinfo)*/
			else if (entries[i].ending_lba) {
				entries[i].starting_lba = entries[i+1].starting_lba - entries[i].ending_lba;
				entries[i].ending_lba = entries[i+1].starting_lba - 1;
			}
			/* userdata or intsed */
			else {
				entries[i].starting_lba = entries[i-1].ending_lba + 1;
				entries[i].ending_lba = entries[i+1].starting_lba - 1;
			}

		}
	}


	set_entries_crc32(efi_crc32(data, PART_MAX_COUNT * sizeof(gpt_entry)));
}

static void pack_sheader_data(void *data)
{
	gpt_header *header = (gpt_header *)data;

	header->header_crc32 = 0;
	header->my_lba = header->alternate_lba;
	header->alternate_lba = 1;
	header->partition_entry_lba = get_gpt_header_last_usable_lba() + 1;
	header->partition_entry_array_crc32 = get_entries_crc32();
	header->header_crc32 = efi_crc32(data, sizeof(gpt_header));
}

int write_primary_gpt(void *data, unsigned sz)
{
	int err;
	u64 pentries_write_size;
	gpt_header *header = (gpt_header *)data;
	u8 *pmbr;
	part_dev_t *dev = mt_part_get_device();

	/* fastboot PGPT image check */
	if (sz != (PART_MAX_COUNT*sizeof(gpt_entry)+dev->blkdev->blksz)) {
		err = -1;
		efi_err("[GPT_Update]PGPT size not correct, err(%d), expect: 0x%x read: 0x%x\n", err, (PART_MAX_COUNT*sizeof(gpt_entry)+dev->blkdev->blksz), sz);
		return err;
	}
	if (header->signature != GPT_HEADER_SIGNATURE) {
		err = -1;
		efi_err("[GPT_Update] %scheck header, err(signature 0x%llx!=0x%llx)\n",
		        TAG, header->signature, GPT_HEADER_SIGNATURE);
		return err;
	}

	pmbr = (u8 *)malloc(dev->blkdev->blksz);
	if (!pmbr) {
		err = -1;
		efi_err("%s malloc memory(pmbr header), err\n", TAG);
		return err;
	}
	memset(pmbr, 0, dev->blkdev->blksz);

	set_spgt_partition_lba_size(header->alternate_lba); //sgpt partition size is hrere

	pack_entries_data((u8 *)(data+dev->blkdev->blksz)); //move to gpt entries
	pack_pheader_data(data); //update pheader
	pack_pmbr_data(pmbr);

	/* Write pmbr */
	err = write_data(0, (u8 *)pmbr, (u64)dev->blkdev->blksz, dev->blkdev->part_user);
	if (err) {
		efi_err("[GPT_Update]write pmbr, err(%d)\n", err);
		return err;
	}
	/* Write pheader */
	err = write_data(1, (u8 *)data, (u64)dev->blkdev->blksz, dev->blkdev->part_user);
	if (err) {
		efi_err("[GPT_Update]write pheader, err(%d)\n", err);
		return err;
	}
	/* Write gpt entries */
	pentries_write_size = (u64)((PART_MAX_COUNT + 3) / 4) * dev->blkdev->blksz;
	err = write_data(2, (u8 *)(data+dev->blkdev->blksz), pentries_write_size, dev->blkdev->part_user);
	if (err) {
		efi_err("[GPT_Update]write pentries, err(%d)\n", err);
		return err;
	}
	return 0;
}

int write_secondary_gpt(void *data, unsigned sz)
{
	int err;
	u64 pentries_write_size;
	u64 sentries_start_lba;
	gpt_header *header = (gpt_header *)data;
	part_dev_t *dev = mt_part_get_device();

	/* fastboot PGPT image check */
	if (sz != (PART_MAX_COUNT*sizeof(gpt_entry)+dev->blkdev->blksz)) {
		err = -1;
		efi_err("[GPT_Update]SGPT size not correct, err(%d), expect: 0x%x read: 0x%x\n", err, (PART_MAX_COUNT*sizeof(gpt_entry)+dev->blkdev->blksz), sz);
		return err;
	}
	if (header->signature != GPT_HEADER_SIGNATURE) {
		err = -1;
		efi_err("[GPT_Update] %s check header, err(signature 0x%llx!=0x%llx)\n",
		        TAG, header->signature, GPT_HEADER_SIGNATURE);
		return err;
	}

	pack_sheader_data(data); //update sheader

	/* Write sheader */
	err = write_data(last_lba(dev->blkdev->part_user), (u8 *)data, (u64)dev->blkdev->blksz, dev->blkdev->part_user);
	if (err) {
		efi_err("[GPT_Update]write sheader, err(%d)\n", err);
		return err;
	}
	/* Write gpt entries */
	sentries_start_lba = get_gpt_header_last_usable_lba() + 1;
	pentries_write_size = (u64)((PART_MAX_COUNT + 3) / 4) * dev->blkdev->blksz;
	err = write_data(sentries_start_lba, (u8 *)(data+dev->blkdev->blksz), pentries_write_size, dev->blkdev->part_user);
	if (err) {
		efi_err("[GPT_Update]write pentries, err(%d)\n", err);
		return err;
	}
	return 0;
}

unsigned int *target_commandline_force_gpt(char *cmd)
{
	cmdline_append("gpt=1");
	return 0;
}
