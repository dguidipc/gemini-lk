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

#include <etb.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <latch.h>
#include <malloc.h>
#include <reg.h>
#include <platform/sync_write.h>
#include <plat_debug_interface.h>
#include "utils.h"

static unsigned int etb_users[MAX_NR_ETB];
static const char* etb_user_id_to_name[ETB_USER_TYPE] = {
	"big_core",
	"cm4",
	"audio_cm4",
	"bus_tracer",
	"mcsi_b_tracer_normal_mode",
	"reserved",
	"reserved",
	"reserved"
};

static void get_etb_users(void)
{
	unsigned int i, offset = 0;
	unsigned long plat_sram_flag0;

	/* get the user id from plat_sram_flag0 */
	if (cfg_pc_latch.plat_sram_flag0) {
		plat_sram_flag0 = readl(cfg_pc_latch.plat_sram_flag0);
		for (i = 0; i <= cfg_etb.nr_etb-1; ++i) {
			if (cfg_etb.etb[i].support_multi_user == 1 && offset <= 9) {
				/* plat_sram_flag0 is for the first 10 ETBs that supports multi-user */
				etb_users[i] = extract_n2mbits(plat_sram_flag0, 2+offset*3, 4+offset*3);
				offset++;
			} else {
				/* user id 7 stands for "reserved" */
				etb_users[i] = 7;
			}
		}
	}
}

static unsigned int unlock_etb(unsigned long base)
{
	if (base == 0)
		return 0;

	writel(ETB_UNLOCK_MAGIC, base + ETB_LAR);
	dsb();

	if (readl(base + ETB_LSR) != 0x1)
		return 0;

	return 1;
}

static unsigned int lock_etb(unsigned long base)
{
	if (base == 0)
		return 0;

	dsb();
	writel(0, base + ETB_LAR);

	return 1;
}

static int copy_from_etb(unsigned int index, char *buf, int *wp)
{
	unsigned long depth, etb_rp, etb_wp, ret, base;
	unsigned long nr_words, nr_unaligned_bytes;
	unsigned int i;

	if (buf == NULL || wp == NULL || (index+1 > cfg_etb.nr_etb))
		return -1;

	base = cfg_etb.etb[index].base;

	if (base == 0)
		return -1;

	if (unlock_etb(base) != 1) {
		dprintf(CRITICAL, "[ETB%d] unlock etb failed\n", index);
		return 0;
	}

	ret = readl(base + ETB_STATUS);
	etb_rp = readl(base + ETB_READADDR);
	etb_wp = readl(base + ETB_WRITEADDR);

	/* depth is counted in byte */
	if (ret & 0x1)
		depth = readl(base + ETB_DEPTH) << 2;
	else
		depth = CIRC_CNT(etb_wp, etb_rp, readl(base + ETB_DEPTH) << 2);

	if (depth == 0)
		return 0;
	else
		dprintf(INFO, "[ETB%d] depth = 0x%lx bytes (etb status = 0x%lx, rp = 0x%lx, wp = 0x%lx)\n",
		        index, depth, ret, etb_rp, etb_wp);

	if (cfg_etb.etb[index].support_multi_user == 1)
		*wp += sprintf(buf + *wp, "\n*************************** ETB%d (etb_user=%s) @0x%08lx ***************************\n",
		               index, etb_user_id_to_name[etb_users[index] & (ETB_USER_TYPE-1)], base);
	else
		*wp += sprintf(buf + *wp, "\n*************************** ETB%d @0x%08lx ***************************\n",
		               index, base);

	/* disable ETB before dump */
	writel(0x0, base + ETB_CTRL);

	nr_words = depth / 4;
	nr_unaligned_bytes = depth % 4;
	for (i = 0; i < nr_words; ++i)
		*wp += sprintf(buf + *wp, "%08lx\n", (unsigned long) readl(base + ETB_READMEM));

	if (nr_unaligned_bytes != 0)
		/* ETB depth is not word-aligned -> not expected */
		*wp += sprintf(buf + *wp,  "[warning] etb depth is not word-aligned!\n");

	*wp += sprintf(buf + *wp, "\n\ndepth = 0x%lx bytes (etb status = 0x%lx, rp = 0x%lx, wp = 0x%lx)\n",
	               depth, ret, etb_rp, etb_wp);

	writel(0x0, base + ETB_TRIGGERCOUNT);
	writel(0x0, base + ETB_READADDR);
	writel(0x0, base + ETB_WRITEADDR);
	dsb();

	if (lock_etb(base) != 1)
		dprintf(INFO, "lock etb failed -> ignore due to no impact to system\n");

	*wp += sprintf(buf + *wp, "\n");
	return 1;
}

int etb_get(void **data, int *len)
{
	int ret;
	unsigned int i;
	/*
	 * every 4-byte word would be output in 9 characters (e.g. "ffffabcd\n")
	 * -> etb_sz * 3 is sufficient for all the etb content
	 */
	unsigned long total_sz_to_dump = cfg_etb.total_etb_sz * 3;

	if (len == NULL || data == NULL)
		return -1;

	*len = 0;
	*data = NULL;
	if (cfg_etb.nr_etb == 0 || cfg_etb.dem_base == 0)
		return 0;

	*data = malloc(total_sz_to_dump);
	if (*data == NULL)
		return 0;

	/* enable ATB clock */
	writel(0x1, cfg_etb.dem_base + DEM_ATB_CLK);

	/* get the information of ETB users from sram flag */
	get_etb_users();

	for (i = 0; i <= cfg_etb.nr_etb-1; ++i) {
		ret = copy_from_etb(i, *data, len);
		if (ret < 0 || (*len > 0 && ((unsigned long)*len > total_sz_to_dump))) {
			if (*len > 0 && ((unsigned long)*len > total_sz_to_dump))
				*len = total_sz_to_dump;
			return ret;
		}
	}

	return 1;
}

void etb_put(void **data)
{
	free(*data);
}
