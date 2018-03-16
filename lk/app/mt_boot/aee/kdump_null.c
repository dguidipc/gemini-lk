/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2016. All rights reserved.
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

#include <string.h>
#include <malloc.h>
#ifdef MTK_GPT_SCHEME_SUPPORT
#include <platform/partition.h>
#else
#include <mt_partition.h>
#endif
#include <stdint.h>
#include <string.h>
#include <video.h>
#include <platform/mtk_key.h>
#include <platform/mtk_wdt.h>

#include "aee.h"
#include "kdump.h"

static int null_write_cb(void *handle, void *buf, int size)
{
	return size;
}

int kdump_null_output(const struct mrdump_control_block *mrdump_cb, uint64_t total_dump_size)
{
	const struct mrdump_machdesc *kparams = &mrdump_cb->machdesc;

	voprintf_info("null dumping(address %x, size:%lluM)\n", kparams->phys_offset, total_dump_size / 0x100000UL);
	mtk_wdt_restart();

	bool ok = true;
	void *bufp = kdump_core_header_init(mrdump_cb, kparams->phys_offset, total_dump_size);
	if (bufp != NULL) {
		mtk_wdt_restart();
		struct kzip_file *zf = kzip_open(NULL, null_write_cb);
		if (zf != NULL) {
			struct kzip_memlist memlist[3];
			memlist[0].address = (uint64_t)(uintptr_t)bufp;
			memlist[0].size = KDUMP_CORE_HEADER_SIZE;
			memlist[0].need2map = false;
			memlist[1].address = kparams->phys_offset;
			memlist[1].size = total_dump_size;
			memlist[1].need2map = true;
			memlist[2].address = 0;
			memlist[2].size = 0;
			memlist[2].need2map = false;
			kzip_add_file(zf, memlist, "SYS_COREDUMP");
			kzip_close(zf);
			zf = NULL;
		} else {
			ok = false;
		}
		free(bufp);
	}

	mtk_wdt_restart();
	if (ok) {
		voprintf_info("%s: dump finished, dumped.\n", __func__);
		mrdump_status_ok("NULL-OUTPUT\n");
	}
	return 0;
}
