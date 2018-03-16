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
#include <malloc.h>
#include <spm_common.h>
#include <platform/mt_typedefs.h>
#include <debug.h>
#include <platform/errno.h>
#include <platform/partition.h>

#ifdef SPM_FW_USE_PARTITION
void get_spmfw_version(char *part_name, char *img_name, char *buf, int *wp)
{
	long len;
	part_t *part;
	part_dev_t *dev;
	void *ptr;
	part_hdr_t part_hdr_buf;
	part_hdr_t *part_hdr = &part_hdr_buf;
	u64 offset, offset_ver, part_size;
	long blksz;
	struct spm_fw_header *header;
	int idx = 0;

	dev = mt_part_get_device();
	if (!dev) {
		return;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		return;
	}

	blksz = dev->blkdev->blksz;
	ptr = malloc(blksz * 2);

	if (!ptr) {
		return;
	}

	part_size = part->nr_sects;
	part_size *= dev->blkdev->blksz;

	offset = 0;

	len = partition_read(part_name, offset, (uchar*)part_hdr, sizeof(part_hdr_t));

	if (len < 0) {
		dprintf(CRITICAL, "%s partition read error. LINE: %d\n", part_name, __LINE__);
		goto err;
	}

	part_hdr->info.name[31]='\0'; //append end char
	dprintf(CRITICAL, "\n=========================================\n");
	dprintf(CRITICAL, "%s magic number : 0x%x\n",part_name,part_hdr->info.magic);
	dprintf(CRITICAL, "%s name         : %s\n",part_name,part_hdr->info.name);
	dprintf(CRITICAL, "%s size         : %d\n",part_name,part_hdr->info.dsize);
	dprintf(CRITICAL, "=========================================\n");

	//***************
	//* check partition magic
	//*
	if (part_hdr->info.magic != PART_MAGIC) {
		dprintf(CRITICAL, "%s partition magic error\n", part_name);
		goto err;
	}

	offset += sizeof(part_hdr_t);

	while (1) {
		unsigned int arg_size;
		unsigned short size;

		if (offset + blksz * 2 > part_size) {
			dprintf(CRITICAL, "%s partition read over size. LINE: %d\n", part_name, __LINE__);
			break;
		}

		len = partition_read(part_name, align_sz(offset, blksz), (uchar*)ptr, blksz * 2);

		if (len < 0) {
			dprintf(CRITICAL, "%s partition read error. LINE: %d\n", part_name, __LINE__);
			break;
		}

		header = (struct spm_fw_header *)(ptr + offset % blksz);

		if (header->magic != SPM_FW_MAGIC) {
			break;
		}
		dprintf(CRITICAL, "header->name %s\n", header->name);

		offset_ver = offset;
		offset += (sizeof(*header) + header->size);

		size = DRV_Reg16(ptr + offset_ver % blksz + sizeof(*header)) * SPM_FW_UNIT;
		arg_size = sizeof(struct pcm_desc) - offsetof(struct pcm_desc, size);
		offset_ver += (sizeof(*header) + 2 + size + arg_size);

		if (offset_ver + blksz * 2 > part_size) {
			dprintf(CRITICAL, "%s partition read over size. LINE: %d\n", part_name, __LINE__);
			break;
		}

		len = partition_read(part_name, align_sz(offset_ver, blksz), (uchar*)ptr, blksz * 2);

		if (len < 0) {
			dprintf(CRITICAL, "%s partition read error. LINE: %d\n", part_name, __LINE__);
			break;
		}

		idx++;
		*wp += sprintf(buf + *wp, "SPM firmware version(0x%x) = %s\n",
				idx, (char *)(ptr + offset_ver % blksz));
		dprintf(CRITICAL, "SPM firmware version(0x%x) = %s\n",
				idx, (char *)(ptr + offset_ver % blksz));
	}

err:
	if (ptr)
		free(ptr);
}
#endif /* SPM_FW_USE_PARTITION */

