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
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#include <debug.h>
#include <stdlib.h>
#include <string.h>
#include <platform.h>
#include <platform/boot_mode.h>
#include <libfdt.h>
#include <arch/arm/mmu.h>

#ifdef MBLOCK_TEST
static void mblock_test(void)
{
	u64 addr = 0;
	dprintf(CRITICAL,"######################################johnson test###########################\n");
	addr = mblock_reserve(&g_boot_arg->mblock_info, 0x2100000, 0x100000, 0x100000000, RANKMAX);
	dprintf(CRITICAL,"%s:reserved addr = %lx\n",__func__,addr);
	mblock_resize(&g_boot_arg->mblock_info,&g_boot_arg->orig_dram_info,addr, 0x2100000, 0x1f00000);
	dprintf(CRITICAL,"%s: after resize\n",__func__);
	if (addr) {
		mblock_create(&g_boot_arg->mblock_info, &g_boot_arg->orig_dram_info, addr+0x100000,0x100000);
		mblock_create(&g_boot_arg->mblock_info, &g_boot_arg->orig_dram_info, addr+0x200000,0x100000);
		mblock_create(&g_boot_arg->mblock_info, &g_boot_arg->orig_dram_info, addr+0x1100000,0x100000);
		mblock_create(&g_boot_arg->mblock_info, &g_boot_arg->orig_dram_info, addr+0x1400000,0x100000);
		mblock_create(&g_boot_arg->mblock_info, &g_boot_arg->orig_dram_info, addr+0x1800000,0x100000);
		mblock_create(&g_boot_arg->mblock_info, &g_boot_arg->orig_dram_info, addr+0x700000,0x100000);
		mblock_create(&g_boot_arg->mblock_info, &g_boot_arg->orig_dram_info, addr+0x600000,0x100000);
		mblock_create(&g_boot_arg->mblock_info, &g_boot_arg->orig_dram_info, addr+0x500000,0x100000);
		mblock_create(&g_boot_arg->mblock_info, &g_boot_arg->orig_dram_info, addr+0x400000,0x100000);
		mblock_create(&g_boot_arg->mblock_info, &g_boot_arg->orig_dram_info, addr+0x300000,0x100000);

	}
	dprintf(CRITICAL,"################################johnson test done ###########################\n",addr);
	return ;
}
#endif

#ifdef MBLOCK_LIB_SUPPORT
#ifdef NEW_MEMORY_RESERVED_MODEL
#error NEW_MEMORY_RESERVED_MODEL is deprecated , you should not define it and still enable MBLOCK_LIB_SUPPORT Plz choose either
#endif
int get_mblock_num(void)
{
	return g_boot_arg->mblock_info.mblock_num;
}

int setup_mem_property_use_mblock_info(dt_dram_info *property, size_t p_size)
{
	mblock_info_t *mblock_info = &g_boot_arg->mblock_info;
	dt_dram_info *p;
	unsigned int i;

	if (mblock_info->mblock_num > p_size) {
		dprintf(CRITICAL, "mblock_info->mblock_num =%d is bigger than mem_property=%d\n", mblock_info->mblock_num, p_size);
		return 1;
	}

	for (i = 0; i < mblock_info->mblock_num; ++i) {
		p = (property + i);

		p->start_hi = cpu_to_fdt32(mblock_info->mblock[i].start>>32);
		p->start_lo = cpu_to_fdt32(mblock_info->mblock[i].start);
		p->size_hi = cpu_to_fdt32((mblock_info->mblock[i].size)>>32);
		p->size_lo = cpu_to_fdt32(mblock_info->mblock[i].size);
		dprintf(INFO, "mblock[%d].start: 0x%llx, size: 0x%llx\n",
		        i,
		        mblock_info->mblock[i].start,
		        mblock_info->mblock[i].size);

		dprintf(INFO, " mem_reg_property[%d].start_hi = 0x%08X\n", i, p->start_hi);
		dprintf(INFO, " mem_reg_property[%d].start_lo = 0x%08X\n", i, p->start_lo);
		dprintf(INFO, " mem_reg_property[%d].size_hi  = 0x%08X\n", i, p->size_hi);
		dprintf(INFO, " mem_reg_property[%d].size_lo  = 0x%08X\n", i, p->size_lo);
	}

	return 0;
}

void mblock_show_info()
{
	mblock_info_t *mblock_info = &g_boot_arg->mblock_info;
	unsigned int i;
	u64 start, sz;

	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		dprintf(CRITICAL, "mblock[%d].start: 0x%llx, size: 0x%llx\n", i,
				start, sz);
	}
}


/*
 * reserve a memory from mblock
 * @mblock_info: address of mblock_info
 * @reserved_size: size of memory
 * @align: alignment
 * @limit: address limit. Must higher than return address + reserved_size
 * @rank: preferable rank, the returned address is in rank or lower ranks
 * It returns as high rank and high address as possible. (consider rank first)
 */
u64 mblock_reserve(mblock_info_t *mblock_info, u64 reserved_size, u64 align, u64 limit,
                   enum reserve_rank rank)
{
	unsigned int i, max_rank;
	int target = -1;
	u64 start, end, sz, max_addr = 0;
	u64 reserved_addr = 0;
	mblock_t mblock;

	if (mblock_info->mblock_num == 128) {
		/* the mblock[] is full */
		dprintf(CRITICAL,"mblock_reserve error: mblock[] is full\n");
		return 0;
	}

	if (!align)
		align = 0x1000;
	/* must be at least 4k aligned */
	if (align & (0x1000 - 1))
		align &= ~(0x1000 - 1);

	if (rank == RANK0) {
		/* reserve memory from rank 0 */
		max_rank = 0;
	} else {
		/* reserve memory from any possible rank */
		/* mblock_num >= nr_ranks is true */
		max_rank = mblock_info->mblock_num - 1;
	}

	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		end = limit < (start + sz)? limit: (start + sz);
		reserved_addr = (end - reserved_size);
		reserved_addr &= ~(align - 1);
		dprintf(INFO,"mblock[%d].start: 0x%llx, sz: 0x%llx, limit: 0x%llx, "
		        "max_addr: 0x%llx, max_rank: %d, target: %d, "
		        "mblock[].rank: %d, reserved_addr: 0x%llx,"
		        "reserved_size: 0x%llx\n",
		        i, start, sz, limit, max_addr, max_rank,
		        target, mblock_info->mblock[i].rank,
		        reserved_addr, reserved_size);
		dprintf(INFO,"mblock_reserve dbg[%d]: %d, %d, %d, %d, %d\n",
		        i, (reserved_addr + reserved_size < start + sz),
		        (reserved_addr >= start),
		        (mblock_info->mblock[i].rank <= max_rank),
		        (start + sz > max_addr),
		        (reserved_addr + reserved_size <= limit));
		if ((reserved_addr + reserved_size <= start + sz) &&
		        (reserved_addr >= start) &&
		        (mblock_info->mblock[i].rank <= max_rank) &&
		        (start + sz > max_addr) &&
		        (reserved_addr + reserved_size <= limit)) {
			max_addr = start + sz;
			target = i;
		}
	}

	if (target < 0) {
		dprintf(CRITICAL,"mblock_reserve error\n");
		return 0;
	}

	start = mblock_info->mblock[target].start;
	sz = mblock_info->mblock[target].size;
	end = limit < (start + sz)? limit: (start + sz);
	reserved_addr = (end - reserved_size);
	reserved_addr &= ~(align - 1);

	/* split mblock if necessary */
	if (reserved_addr == start) {
		/*
		 * only needs to fixup target mblock
		 * [reserved_addr, reserved_size](reserved) +
		 * [reserved_addr + reserved_size, sz - reserved_size]
		 */
		mblock_info->mblock[target].start = reserved_addr + reserved_size;
		mblock_info->mblock[target].size -= reserved_size;
	} else if ((reserved_addr + reserved_size) == (start + sz)) {
		/*
		 * only needs to fixup target mblock
		 * [start, reserved_addr - start] +
		 * [reserved_addr, reserved_size](reserved)
		 */
		mblock_info->mblock[target].size = reserved_addr - start;
	} else {
		/*
		 * fixup target mblock and create a new mblock
		 * [start, reserved_addr - start] +
		 * [reserved_addr, reserved_size](reserved) +
		 * [reserved_addr + reserved_size, start + sz - reserved_addr - reserved_size]
		 */
		/* fixup original mblock */
		mblock_info->mblock[target].size = reserved_addr - start;

		/* new mblock */
		mblock.rank =  mblock_info->mblock[target].rank;
		mblock.start = reserved_addr + reserved_size;
		mblock.size = start + sz - (reserved_addr + reserved_size);

		/* insert the new node, keep the list sorted */
		memmove(&mblock_info->mblock[target + 2],
		        &mblock_info->mblock[target + 1],
		        sizeof(mblock_t) *
		        (mblock_info->mblock_num - target - 1));
		mblock_info->mblock[target + 1] = mblock;
		mblock_info->mblock_num += 1;
		dprintf(INFO, "mblock[%d]: %llx, %llx from mblock\n"
		        "mblock[%d]: %llx, %llx from mblock\n",
		        target,
		        mblock_info->mblock[target].start,
		        mblock_info->mblock[target].size,
		        target + 1,
		        mblock_info->mblock[target + 1].start,
		        mblock_info->mblock[target + 1].size);
	}

	dprintf(CRITICAL,"mblock_reserve: %llx - %llx from mblock %d\n",
	        reserved_addr, reserved_addr + reserved_size,
	        target);
#ifdef MTK_3LEVEL_PAGETABLE
	{
		u64 m_start = (u64)reserved_addr;
		u32 m_size = (u32)reserved_size;
		if (start <= 0xffffffff) {
			arch_mmu_map((uint64_t)m_start, (uint32_t)m_start,
						 MMU_MEMORY_TYPE_NORMAL_WRITE_BACK | MMU_MEMORY_AP_P_RW_U_NA, ROUNDUP(m_size, PAGE_SIZE));
		}
	}
#endif

	/* print debug info */
	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		dprintf(INFO,"mblock-debug[%d].start: 0x%llx, sz: 0x%llx\n",
		        i, start, sz);
	}

	return reserved_addr;
}

/*
 * mblock_resize - resize mblock started at addr from oldsize to newsize,
 * current implementation only consider oldsize >= newsize.
 *
 * @mblock_info: mblock information
 * @orig_dram_info: original dram information
 * @addr: start address of a mblock
 * @oldsize: origianl size of the mblock
 * @newsize: new size of the given block
 * return 0 on success, otherwise 1
 */
int mblock_resize(mblock_info_t *mblock_info, dram_info_t *orig_dram_info,
                  u64 addr, u64 oldsize, u64 newsize)
{
	int err = 1;
	unsigned int i;
	u64 start, sz;
	mblock_t mblock;

	/* check size, oldsize must larger than newsize */
	if (oldsize <= newsize) {
		dprintf(CRITICAL,"mblock_resize error: mblock %llx oldsize(%llx) <= newsize(%llx)",
		        addr, oldsize, newsize);
		goto error;
	}

	/* check alignment, at least 4k aligned */
	if ((oldsize & (0x1000 - 1)) || (newsize & (0x1000 - 1))) {
		dprintf(CRITICAL,"mblock_resize alignment error: oldsize(%llx) or newsize(%llx)\n",
		        oldsize, newsize);
		goto error;
	}

	/* check mblock */
	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		/* invalid mblock */
		if ((addr >= start) && ((addr + oldsize) <= (start + sz))) {
			dprintf(CRITICAL,"mblock_resize error: mblock %llx, size: %llx is free\n",
			        addr, oldsize);
			goto error;
		}
	}

	/*
	 * ok, the mblock is valid and oldsize > newsize, let's
	 * shrink this mblock
	 */
	/* setup a new mblock */
	mblock.start = addr + newsize;
	mblock.size = oldsize - newsize;
	dprintf(CRITICAL,"mblock_resize putback mblock %llx size: %llx\n",
	        mblock.start, mblock.size);
	/* setup rank */
	for (i = 0; i < orig_dram_info->rank_num; i++) {
		start = orig_dram_info->rank_info[i].start;
		sz = orig_dram_info->rank_info[i].size;
		if ((mblock.start >= start) && ((mblock.start + mblock.size) <= (start + sz))) {
			mblock.rank = i;
			break;
		}
	}
	if (i >= orig_dram_info->rank_num) {
		dprintf(CRITICAL,"mblock_resize error: mblock not in orig_dram_info: %llx, size(%llx)\n",
		        mblock.start, mblock.size);
		goto error;
	}

	/* put the mblock back to mblock_info */
	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		if (mblock.rank == mblock_info->mblock[i].rank) {
			if (mblock.start == start + sz) {
				/*
				 * the new mblock can be merged to this mblock
				 * [start, start + sz] +
				 * [mblock.start, mblock.start + mblock.size](new)
				 */
				mblock_info->mblock[i].size += mblock.size;
				/* destroy block */
				mblock.size = 0;
			} else if (start == mblock.start + mblock.size) {
				/*
				 * the new mblock can be merged to this mblock
				 * [mblock.start, mblock.start + * mblock.size](new) +
				 * [start, start + sz]
				 */
				mblock_info->mblock[i].start = mblock.start;
				mblock_info->mblock[i].size += mblock.size;
				/* destroy block */
				mblock.size = 0;
			}
		}
	}

	/*
	 * mblock cannot be merge info mblock_info, insert it into mblock_info
	 */
	if (mblock.size) {
		for (i = 0; i < mblock_info->mblock_num; i++) {
			if (mblock.start < mblock_info->mblock[i].start)
				break;
		}
		memmove(&mblock_info->mblock[i + 1],
		        &mblock_info->mblock[i],
		        sizeof(mblock_t) *
		        (mblock_info->mblock_num - i));
		mblock_info->mblock[i] = mblock;
		mblock_info->mblock_num += 1;
	}

	/* print debug info */
	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		dprintf(INFO,"mblock-resize-debug[%d].start: 0x%llx, sz: 0x%llx\n",
		        i, start, sz);
	}

	return 0;
error:
	return err;
}

/*
 * mblock_create - create mblock started at addr or merge with existing mblock
 *
 * @mblock_info: mblock information
 * @orig_dram_info: original dram information
 * @addr: start address of a mblock, must be 4k align
 * @size: size of the given block, must be 4K align
 * return 0 on success, otherwise 1
 */
int mblock_create(mblock_info_t *mblock_info, dram_info_t *orig_dram_info
                  , u64 addr, u64 size)
{
	int err = -1;
	unsigned i,valid;
	u64 start, sz;
	mblock_t mblock;
	mblock_t *mblock_candidate_left = NULL, *mblock_candidate_right = NULL;

	/* check size, addr valid and align with 4K*/
	if (!size || size&(0x1000 - 1) || addr&(0x1000 - 1) ) {
		dprintf(CRITICAL,"mblock_create size invalid size=%llx\n",size);
		goto error;
	}
	/* for lca check*/
	if (g_boot_arg->lca_reserved_mem.start && g_boot_arg->lca_reserved_mem.size) {
		if ((addr >= g_boot_arg->lca_reserved_mem.start && addr < g_boot_arg->lca_reserved_mem.start \
			+ g_boot_arg->lca_reserved_mem.size)|| \
				(addr + size > g_boot_arg->lca_reserved_mem.start \
					&& (addr + size) < g_boot_arg->lca_reserved_mem.start + g_boot_arg->lca_reserved_mem.size) ) {
			dprintf(CRITICAL,"mblock_create ERROR , overlap with LCA addr and size invalid addr = %llx size=%llx\n", addr, size);
			goto error;
		}
	}

	/* for tee check*/
	if (g_boot_arg->tee_reserved_mem.start && g_boot_arg->tee_reserved_mem.size) {
		if ((addr >= g_boot_arg->tee_reserved_mem.start && addr < g_boot_arg->tee_reserved_mem.start \
			+ g_boot_arg->tee_reserved_mem.size)|| \
				(addr + size > g_boot_arg->tee_reserved_mem.start \
					&& (addr + size) < g_boot_arg->tee_reserved_mem.start + g_boot_arg->tee_reserved_mem.size) ) {
			dprintf(CRITICAL,"mblock_create ERROR , overlap with tee addr and size invalid addr = %llx size=%llx\n", addr, size);
			goto error;
		}
	}

	/*it's not allow to create mblock which is cross rank
	 * and mblock should not exceed rank size */
	for (i = 0, valid = 0; i < orig_dram_info->rank_num; i++) {
		start = orig_dram_info->rank_info[i].start;
		sz = orig_dram_info->rank_info[i].size;
		if (addr >= start && addr < start + sz && addr + size <= start + sz) {
			valid =1;
			break;
		}
	}
	if (!valid) {
		dprintf(CRITICAL,"mblock_create addr and size invalid addr=%llx size=%llx\n",
		        addr,size);
		goto error;
	}

	/* check every mblock the addr and size should not be within any existing mblock */
	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		/*addr should start from reserved memory space and addr + size should not overlap with mblock
		 * when addr is smaller than start*/
		if (((addr >= start) && (addr < start + sz)) || (addr < start && addr + size > start)) {
			dprintf(CRITICAL,"mblock_create error: addr %llx overlap with mblock %llx, size: %llx \n",
			        addr, start, sz);
			goto error;
		}
	}

	/*
	 * ok, the mblock is valid let's create the mblock
	 * and try to merge it with the same bank and choose the bigger size one
	 */
	/* setup a new mblock */
	mblock.start = addr;
	mblock.size = size;
	dprintf(CRITICAL,"mblock_create mblock start %llx size: %llx\n",
	        mblock.start, mblock.size);
	/* setup rank */
	for (i = 0; i < orig_dram_info->rank_num; i++) {
		start = orig_dram_info->rank_info[i].start;
		sz = orig_dram_info->rank_info[i].size;
		if ((mblock.start >= start) && ((mblock.start + mblock.size) <= (start + sz))) {
			mblock.rank = i;
			break;
		}
	}
	if (i >= orig_dram_info->rank_num) {
		dprintf(CRITICAL,"mblock_create error: mblock not in orig_dram_info: %llx, size(%llx)\n",
		        mblock.start, mblock.size);
		goto error;
	}

	/* put the mblock back to mblock_info */
	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		if (mblock.rank == mblock_info->mblock[i].rank) {
			if (mblock.start + mblock.size == start) {
				/*
				 * the new mblock could be merged to this mblock
				 */
				mblock_candidate_right = &mblock_info->mblock[i];
			} else if (start + sz == mblock.start) {
				/*
				 * the new mblock can be merged to this mblock
				 */
				mblock_candidate_left =  &mblock_info->mblock[i];
			}
		}
	}
	/*we can merge either left or right , choose the bigger one */
	if (mblock_candidate_right && mblock_candidate_left) {
		if (mblock_candidate_right->size >= mblock_candidate_left->size) {
			dprintf(CRITICAL,"mblock_candidate_right->size = %llx \
				mblock_candidate_left->size = %llx \n",mblock_candidate_right->size, mblock_candidate_left->size);
			mblock_candidate_right->start = mblock.start;
			mblock_candidate_right->size += mblock.size;
		} else { /*left bigger*/
			dprintf(CRITICAL,"mblock_candidate_right->size = %llx \
				mblock_candidate_left->size = %llx \n",mblock_candidate_right->size, mblock_candidate_left->size);
			mblock_candidate_left->size += mblock.size;
		}
		/* destroy block */
		mblock.size = 0;
	} else {
		if (mblock_candidate_right) {
			mblock_candidate_right->start = mblock.start;
			mblock_candidate_right->size += mblock.size;
			/* destroy block */
			mblock.size = 0;
		}

		if (mblock_candidate_left) {
			mblock_candidate_left->size += mblock.size;
			/* destroy block */
			mblock.size = 0;
		}
	}

	/*
	 * mblock cannot be merge into mblock_info, insert it into mblock_info
	 */
	if (mblock.size) {
		for (i = 0; i < mblock_info->mblock_num; i++) {
			if (mblock.start < mblock_info->mblock[i].start)
				break;
		}
		/* insert the new node, keep the list sorted */
		if (i != mblock_info->mblock_num) {
			memmove(&mblock_info->mblock[i + 1],
			        &mblock_info->mblock[i],
			        sizeof(mblock_t) *
			        (mblock_info->mblock_num - i));
		}
		mblock_info->mblock[i] = mblock;
		mblock_info->mblock_num += 1;
		dprintf(INFO, "create mblock[%d]: %llx, %llx \n",
		        i,
		        mblock_info->mblock[i].start,
		        mblock_info->mblock[i].size);
	}

	/* print debug info */
	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		dprintf(INFO,"mblock-create-debug[%d].start: 0x%llx, sz: 0x%llx\n",
		        i, start, sz);
	}

	return 0;
error:
	return err;
}

/*
 * memory_lowpower_fixup
 *
 * To support various DRAM size with single logic, we fixup
 * the reserved memory used by memory-lowpower feature according
 * chip DRAM size.
 * It's fine to put the function in common part since it searches
 * for specified node and only fixup the node.
 *
 * input:
 * @fdt: pointer to fdt
 *
 * output:
 * return 0 on success, otherwise 1
 */
int memory_lowpwer_fixup(void *fdt)
{
	int offset, nodeoffset, len, errline, ret;
	unsigned int i;
	mblock_info_t *mblock_info = &g_boot_arg->mblock_info;
	u32 *psize, *prange, *palign, node_size[2];
	u64 size, start, alignment, newstart, end = 0;

	offset = fdt_path_offset(fdt, "/reserved-memory");

	if (!offset) {
		errline = __LINE__;
		goto error;
	}
	nodeoffset = fdt_subnode_offset(fdt, offset,
			"memory-lowpower-reserved-memory");
	if (nodeoffset < 0) {
		nodeoffset = fdt_subnode_offset(fdt, offset,
				"zone-movable-cma-memory");
		if (nodeoffset < 0)
			goto exit;
	}

	/* get size */
	psize = (u32 *)fdt_getprop(fdt, nodeoffset, "size", &len);
	if (!psize) {
		errline = __LINE__;
		goto error;
	}
	size = ((u64)fdt32_to_cpu(*psize) << 32) + fdt32_to_cpu(*(psize + 1));

	/* get alignment */
	palign = (u32 *)fdt_getprop(fdt, nodeoffset, "alignment", &len);
	if (palign) {
		alignment = ((u64)fdt32_to_cpu(*palign) << 32) + fdt32_to_cpu(*(palign + 1));
	} else {
		/* fallback to default alignment */
		alignment = 256 * 1024 * 1024;
	}

	/* get alloc_rage */
	prange = (u32 *)fdt_getprop(fdt, nodeoffset, "alloc-ranges", &len);
	if (!prange) {
		errline = __LINE__;
		goto error;
	}
	start = ((u64)fdt32_to_cpu(*prange) << 32) + fdt32_to_cpu(*(prange + 1));

	/* search for max available DRAM address */
	for (i = 0; i < mblock_info->mblock_num; i++) {
		if ((mblock_info->mblock[i].start +
			mblock_info->mblock[i].size) > end)
			end = mblock_info->mblock[i].start +
				mblock_info->mblock[i].size;
	}

	/* fix size according to DRAM size */
	if (size > (end - start)) {
		newstart = start + size - (end - start);
		newstart = ((newstart + alignment - 1) / alignment) * alignment;
		dprintf(CRITICAL,"%s: newstart: %llx, size: 0x%llx => 0x%llx)\n",
				__func__, newstart, size, (start + size - newstart));
		if (newstart >= start + size) {
			errline = __LINE__;
			goto error;
		}
		size = start + size - newstart;
		node_size[0] = (u32)cpu_to_fdt32(size >> 32);
		node_size[1] = (u32)cpu_to_fdt32(size);
		ret = fdt_setprop(fdt, nodeoffset, "size",  node_size,
				sizeof(u32) * 2);
		if (ret) {
			errline = __LINE__;
			goto error;
		}
	}

exit:
	return 0;
error:
	dprintf(CRITICAL, "%s: errline: %d\n", __func__, errline);
	return 1;
}

int fdt_memory_append(void *fdt)
{
	char *ptr;
	int offset;
	int ret = 0;

	offset = fdt_path_offset(fdt, "/memory");

	if (offset < 0 ) {
		dprintf(CRITICAL,"%s:[%d] get fdt_path_offset of memory failed \n",__func__,__LINE__);
		ret = offset;
		goto exit;
	}

	ptr = (char *)&g_boot_arg->orig_dram_info;
	ret = fdt_setprop(fdt, offset, "orig_dram_info", ptr, sizeof(dram_info_t));
	if (ret) goto exit;

	ptr = (char *)&g_boot_arg->mblock_info;
	ret = fdt_setprop(fdt, offset, "mblock_info", ptr, sizeof(mblock_info_t));
	if (ret) goto exit;

	ptr = (char *)&g_boot_arg->lca_reserved_mem;
	ret = fdt_setprop(fdt, offset, "lca_reserved_mem", ptr, sizeof(mem_desc_t));
	if (ret) goto exit;

	ptr = (char *)&g_boot_arg->tee_reserved_mem;
	ret = fdt_setprop(fdt, offset, "tee_reserved_mem", ptr, sizeof(mem_desc_t));
	if (ret) goto exit;

	ret = memory_lowpwer_fixup(fdt);
	if (ret) goto exit;
exit:

	if (ret)
		return 1;

	return 0;
}
#endif
