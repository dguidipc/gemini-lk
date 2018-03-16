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

#ifndef __PARTITION_H__
#define __PARTITION_H__

#include <platform/mt_typedefs.h>
#include <platform/mmc_core.h>
#include <part_dev.h>

#define BIMG_HEADER_SZ              (0x800)
#define MKIMG_HEADER_SZ             (0x200)

#define PART_HDR_DATA_SIZE	512

#define PART_KERNEL     "KERNEL"
#define PART_ROOTFS     "ROOTFS"

#define PART_MAGIC          0x58881688
#define PART_EXT_MAGIC      0x58891689
#define PART_MAX_COUNT      128

#define FRP_NAME        "frp"

typedef union {
	struct {
		unsigned int magic;        /* partition magic */
		unsigned int dsize;        /* partition data size */
		char         name[32];     /* partition name */
		unsigned int maddr;        /* partition memory address */
		unsigned int mode;      /* maddr is counted from the beginning or end of RAM */
		/* extension */
		unsigned int ext_magic;    /* always EXT_MAGIC */
		unsigned int hdr_size;     /* header size is 512 bytes currently, but may extend in the future */
		unsigned int hdr_version;  /* see HDR_VERSION */
		unsigned int img_type;     /* please refer to #define beginning with IMG_TYPE_ */
		unsigned int img_list_end; /* end of image list? 0: this image is followed by another image 1: end */
		unsigned int align_size;   /* image size alignment setting in bytes, 16 by default for AES encryption */
		unsigned int dsize_extend; /* high word of image size for 64 bit address support */
		unsigned int maddr_extend; /* high word of image load address in RAM for 64 bit address support */
	} info;
	unsigned char data[PART_HDR_DATA_SIZE];
} part_hdr_t;


#define PART_META_INFO_NAMELEN  64
#define PART_META_INFO_UUIDLEN  16
#define PART_ATTR_LEGACY_BIOS_BOOTABLE  (0x00000004UL) /* bit2 = active bit for OTA */

struct part_meta_info {
	u8 name[PART_META_INFO_NAMELEN];
	u8 uuid[PART_META_INFO_UUIDLEN];
};

typedef struct {
	unsigned long  start_sect;
	unsigned long  nr_sects;
	unsigned long  part_attr;
	unsigned int part_id;
	char *name;
	struct part_meta_info *info;
} part_t;

struct part_name_map {
	char fb_name[32];   /*partition name used by fastboot*/
	char r_name[32];    /*real partition name*/
	char *partition_type;   /*partition_type*/
	int partition_idx;  /*partition index*/
	int is_support_erase;   /*partition support erase in fastboot*/
	int is_support_dl;  /*partition support download in fastboot*/
};

enum {
	RAW_DATA_IMG,
	YFFS2_IMG,
	UBIFS_IMG,
	EXT4_IMG,
	FAT_IMG,
	UNKOWN_IMG,
};

extern part_t *get_part(char *name);
extern void put_part(part_t *part);

extern part_t* mt_part_get_partition(char *name);
extern struct part_name_map g_part_name_map[PART_MAX_COUNT];
extern int part_init(part_dev_t *dev);
extern void mt_part_init(unsigned long totalblks);
extern void mt_part_dump(void);
extern int partition_get_index(const char * name);
extern u32 partition_get_region(int index);
extern u64 partition_get_offset(int index);
extern u64 partition_get_size(int index);
extern int partition_get_type(int index, char **p_type);
extern int partition_get_name(int index, char **p_name);
extern int is_support_erase(int index);
extern int is_support_flash(int index);
extern u64 emmc_write(u32 region, u64 offset, void *data, u64 size);
extern u64 emmc_read(u32 region, u64 offset, void *data, u64 size);
extern int emmc_erase(u32 region, u64 offset, u64 size);
extern unsigned long mt_part_get_part_active_bit(part_t *part);

#endif /* __PARTITION_H__ */

