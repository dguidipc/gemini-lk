/* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein is
* confidential and proprietary to MediaTek Inc. and/or its licensors. Without
* the prior written permission of MediaTek inc. and/or its licensors, any
* reproduction, modification, use or disclosure of MediaTek Software, and
* information contained herein, in whole or in part, shall be strictly
* prohibited.
*
* MediaTek Inc. (C) 2010. All rights reserved.
*
* BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
* THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
* RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
* ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
* WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
* WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
* NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
* RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
* INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
* TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
* RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
* OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
* SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
* RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
* STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
* ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
* RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
* MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
* CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
* The following software/firmware and/or related documentation ("MediaTek
* Software") have been modified by MediaTek Inc. All revisions are subject to
* any receiver's applicable license agreements with MediaTek Inc.
*/
#include <stdlib.h>
#include <arch/arm/mmu.h>
#include "platform/log_store_lk.h"

#define MOD "LK_LOG_STORE"

#define DEBUG_LOG

#ifdef DEBUG_LOG
#define LOG_DEBUG(fmt, ...) \
    log_store_enable = false; \
    printf(fmt, ##__VA_ARGS__); \
    log_store_enable = true
#else
#define LOG_DEBUG(fmt, ...)
#endif


/* !!!!!!! Because log store be called by print, so these function don't use print log to debug.*/

enum {
	LOG_WRITE = 0x1,        /* Log is write to buff */
	LOG_READ_KERNEL = 0x2,  /* Log have readed by kernel */
	LOG_WRITE_EMMC = 0x4,   /* log need save to emmc */
	LOG_EMPTY = 0x8,        /* log is empty */
	LOG_FULL = 0x10,        /* log is full */
	LOG_PL_FINISH = 0X20,   /* pl boot up finish */
	LOG_LK_FINISH = 0X40,   /* lk boot up finish */
	LOG_DEFAULT = LOG_WRITE_EMMC|LOG_EMPTY,
} BLOG_FLAG;

static int log_store_status = BUFF_NOT_READY;
static struct pl_lk_log *buff_header;
static char *pbuff;
static struct dram_buf_header *sram_dram_buff;
static bool log_store_enable = true;

void log_store_init(void)
{
	struct sram_log_header *sram_header = NULL;

	LOG_DEBUG("%s:lk log_store_init start.\n", MOD);

	if (log_store_status != BUFF_NOT_READY) {
		LOG_DEBUG("%s:log_sotore_status is ready!\n", MOD);
		return;
	}

	/* SRAM buff header init */
	sram_header = (struct sram_log_header *)SRAM_LOG_ADDR;
	if (sram_header->sig != SRAM_HEADER_SIG) {
		LOG_DEBUG("%s:sram header 0x%x is not match: %d!\n", MOD, sram_header, sram_header->sig);
		memset(sram_header, 0, sizeof(struct sram_log_header));
		sram_header->sig = SRAM_HEADER_SIG;
		log_store_status = BUFF_ALLOC_ERROR;
		return;
	}

	sram_dram_buff = &(sram_header->dram_buf[LOG_PL_LK]);
	if (sram_dram_buff->sig != DRAM_HEADER_SIG || sram_dram_buff->flag == BUFF_ALLOC_ERROR) {
		log_store_status = BUFF_ALLOC_ERROR;
		LOG_DEBUG("%s:sram_dram_buff 0x%x, sig 0x%x, flag 0x%x.\n", MOD,
		          sram_dram_buff, sram_dram_buff->sig, sram_dram_buff->flag);
		return;
	}

	pbuff = sram_dram_buff->buf_addr;

	buff_header = (struct pl_lk_log *)pbuff;
#ifdef MTK_3LEVEL_PAGETABLE
	uint32_t start = ROUNDDOWN((uint32_t)buff_header, PAGE_SIZE);
	uint32_t logsize = ROUNDUP(((uint32_t)buff_header - start + LOG_STORE_SIZE), PAGE_SIZE);
	LOG_DEBUG("%s:dram pl/lk log buff mapping start addr = 0x%x, size = 0x%x\n", MOD, start, logsize);
	if (start >= DRAM_PHY_ADDR) {
		/*need to use header in DRAZM, we must allocate it first */
		arch_mmu_map((uint64_t) start, start,
		             MMU_MEMORY_TYPE_NORMAL_WRITE_BACK | MMU_MEMORY_AP_P_RW_U_NA, logsize);
	}
#endif

	LOG_DEBUG("%s:sram buff header 0x%x,buff address 0x%x, sig 0x%x, buff_size 0x%x, pl log size 0x%x@0x%x, lk log size 0x%x@0x%x!\n",
	          MOD, sram_header, buff_header, buff_header->sig, buff_header->buff_size,
	          buff_header->sz_pl, buff_header->off_pl, buff_header->sz_lk, buff_header->off_lk);

	if (buff_header->sig != LOG_STORE_SIG || buff_header->buff_size != LOG_STORE_SIZE
	        || buff_header->off_pl != sizeof(struct pl_lk_log)) {
		log_store_status = BUFF_ERROR;
		LOG_DEBUG("%s: BUFF_ERROR, sig 0x%x, buff_size 0x%x, off_pl 0x%x.\n", MOD,
		          buff_header->sig, buff_header->buff_size, buff_header->off_pl);
		return;
	}

	if (buff_header->sz_pl + sizeof(struct pl_lk_log) >= LOG_STORE_SIZE) {
		LOG_DEBUG("%s: buff full pl size 0x%x.\n", MOD, buff_header->sz_pl);
		log_store_status = BUFF_FULL;
		return;
	}

	buff_header->off_lk = sizeof(struct pl_lk_log) + buff_header->sz_pl;
	buff_header->sz_lk = 0;
	buff_header->lk_flag = LOG_DEFAULT;


	log_store_status = BUFF_READY;
	LOG_DEBUG("%s: buff ready.\n", MOD);
}

void lk_log_store(char c)
{
	if (log_store_enable == false)
		return;

	if (log_store_status == BUFF_NOT_READY)
		log_store_init();

	if ((log_store_status != BUFF_READY) || (log_store_status == BUFF_FULL))
		return;

	*(pbuff + buff_header->off_lk + buff_header->sz_lk) = c;
	buff_header->sz_lk++;
	if ((buff_header->off_lk + buff_header->sz_lk) >= LOG_STORE_SIZE) {
		log_store_status = BUFF_FULL;
		LOG_DEBUG("%s: dram buff full", MOD);
	}

	sram_dram_buff->buf_point = buff_header->sz_lk + buff_header->sz_pl;
}