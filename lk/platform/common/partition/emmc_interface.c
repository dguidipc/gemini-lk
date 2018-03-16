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
#include <part_interface.h>
#include <part_status.h>

/* dummy parameter is for prameter alignment */
ssize_t partition_read(char *part_name, off_t offset, u8* data, size_t size)
{
	part_dev_t *dev;
	ssize_t len;
	u64 part_addr, part_size, read_addr;
	int index;

	index = partition_get_index_by_name(part_name);
	if (index == -1)
		ASSERT(0);

	/* does not check part_offset here since if it's wrong, assertion fails */
	part_addr = partition_get_offset(index);
	part_size = partition_get_size(index);

	if ((u64) (offset + size) > part_size) {
		dprintf(CRITICAL, "[%s]%s: Incorrect range, part: %s, total: %lld, start: %lld, size: %zu\n", PART_COMMON_TAG, __FUNCTION__, part_name, part_size, offset, size);
		return -PART_ACCESS_OVERFLOW;
	}
	read_addr = part_addr + offset;

	dev = mt_part_get_device();
	if (!dev) {
		dprintf(CRITICAL, "[%s]%s: get part_dev fail\n", PART_COMMON_TAG, __FUNCTION__);
		return -PART_GET_DEV_FAIL;
	}

	len = dev->read(dev, read_addr, (uchar *) data, (int)size);
	if (len != (int)size) {
		dprintf(CRITICAL, "[%s]%s: read '%s' fail, expect: %zu, ret: %ld\n", PART_COMMON_TAG, __FUNCTION__, part_name, size, len);
		return -PART_READ_FAIL;
	}

	return len;
}

/* dummy parameter is for prameter alignment */
ssize_t partition_write(char *part_name, off_t offset, u8* data, size_t size)
{
	part_dev_t *dev;
	ssize_t len;
	u64 part_addr, part_size, write_addr;
	int index;

	index = partition_get_index_by_name(part_name);
	if (index == -1)
		ASSERT(0);

	/* does not check part_offset here since if it's wrong, assertion fails */
	part_addr = partition_get_offset(index);
	part_size = partition_get_size(index);
	write_addr = part_addr + offset;
	if ((u64)(offset + size) > part_size) {
		dprintf(CRITICAL, "[%s]%s: Incorrect range, total: %lld, start: %lld, size: %zu\n", PART_COMMON_TAG, __FUNCTION__, part_size, offset, size);
		return -PART_ACCESS_OVERFLOW;
	}
	dev = mt_part_get_device();
	if (!dev) {
		dprintf(CRITICAL, "[%s]%s: get part_dev fail\n", PART_COMMON_TAG, __FUNCTION__);
		return -PART_GET_DEV_FAIL;
	}

	len = dev->write(dev, (uchar *) data, write_addr, (int)size);
	if (len != (int)size) {
		dprintf(CRITICAL, "[%s]%s: write fail, expect: %zu, real: %ld\n", PART_COMMON_TAG, __FUNCTION__, size, len);
		return -PART_WRITE_FAIL;
	}

	return len;
}

int partition_erase(char *part_name)
{
	int ret = PART_OK;

	int index;
	unsigned long long ptn;
	unsigned long long size;

	index = partition_get_index_by_name(part_name);
	if (index == -1) {
		ret = PART_NOT_EXIST;
		return ret;
	}

	ptn = partition_get_offset(index);
	size = partition_get_size(index);

	ret = emmc_erase(ptn, size);
	return ret;
}
