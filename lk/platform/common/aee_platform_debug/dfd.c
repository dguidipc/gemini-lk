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

#include <debug.h>
#include <malloc.h>
#include <mt_typedefs.h>
#include <reg.h>
#include <plat_debug_interface.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <arch/arm/mmu.h>
#include "dfd.h"
#include "latch.h"
#include "utils.h"
#include <platform/lastpc.h>
#include <platform/sec_devinfo.h>

static unsigned int dfd_internal_dump_triggered;
static struct decoded_lastpc *lastpc;

u64 dfd_decode(const u64 *raw, const struct reg_collector *collector)
{
	u64 reg = 0;
	unsigned int i = 0;
	unsigned int raw_offset = 0;
	unsigned int bit_offset = 0;
	unsigned int inv = 0;

	if (raw == NULL || collector == NULL)
		return 0;

	for (i = 0; i < DFD_REG_LENGTH; ++i) {
		raw_offset = collector->bit_pairs[i].raw_offset + cfg_dfd.nr_header_row;
		bit_offset = collector->bit_pairs[i].bit_offset;
		inv = collector->bit_pairs[i].inv & 0x1;

		if (inv == 0)
			reg |= ((raw[raw_offset] & (((u64)1)<<bit_offset))>>bit_offset) << i;
		else
			reg |= ((~raw[raw_offset] & (((u64)1)<<bit_offset))>>bit_offset) << i;
	}

	return reg;
}

static void dfd_internal_dump_decode_lastpc(const u64 *dfd_raw_data)
{
	unsigned int i;
	unsigned long cpu_power_status;

	lastpc = malloc(cfg_pc_latch.nr_max_core * sizeof(struct decoded_lastpc));
	for (i = 0; i <= cfg_pc_latch.nr_max_core-1; ++i) {
		lastpc[i].pc = 0x0;
		lastpc[i].sp_64 = 0x0;
		lastpc[i].fp_64 = 0x0;
		lastpc[i].sp_32 = 0x0;
		lastpc[i].fp_32 = 0x0;
	}

	cpu_power_status = plt_get_cpu_power_status_at_wdt();
	for (i = 0; i <= cfg_pc_latch.nr_max_core - cfg_pc_latch.nr_max_big_core - 1; ++i) {
		/* CPUX is not online on WDT */
		if (extract_n2mbits(cpu_power_status, i, i) == 0)
			continue;

		lastpc[i].pc = dfd_decode(dfd_raw_data, &(little_core[i].pc));
		lastpc[i].sp_32 = dfd_decode(dfd_raw_data, &(little_core[i].sp32));
		/* TODO: select SP by cpsr */
		lastpc[i].sp_64 = dfd_decode(dfd_raw_data, &(little_core[i].sp_EL1));
		lastpc[i].fp_32 = dfd_decode(dfd_raw_data, &(little_core[i].fp32));
		lastpc[i].fp_64 = dfd_decode(dfd_raw_data, &(little_core[i].fp64));
	}

	for (i = 0; i<= cfg_pc_latch.nr_max_big_core - 1; ++i) {
		/* CPUX is not online on WDT */
		if (extract_n2mbits(cpu_power_status, cfg_big_core[i].cpuid, cfg_big_core[i].cpuid) == 0)
			continue;
		lastpc[cfg_big_core[i].cpuid].pc = dfd_decode(dfd_raw_data, &(big_core[i].edpcsr));
	}
}

unsigned int get_efuse_dfd_disabled(void)
{
	return ((get_devinfo_with_index(cfg_dfd.dfd_disable_devinfo_index)
	         & (0x1 << cfg_dfd.dfd_disable_bit)) >> cfg_dfd.dfd_disable_bit);
}

static unsigned int dfd_internal_dump_check_triggered_or_not(void)
{

	if (cfg_dfd.version >= DFD_V3_0)
	{
		/* DFD triggers only if dfd_valid_before_reboot == 1 and efuse_dfd_disbled == 0 */
		if ((readl(cfg_dfd.plat_sram_flag1) & 0x2) && (get_efuse_dfd_disabled() == 0x0))
			dfd_internal_dump_triggered = 1;
		else
			dfd_internal_dump_triggered = 0;
	}
	else
		dfd_internal_dump_triggered = 1;

	return dfd_internal_dump_triggered;
}

/*
 * DFD internal dump before reboot implies that mcusys registers are all corrupted
 */
unsigned int dfd_internal_dump_before_reboot(void)
{
	/* for platform that is not support DFD internal dump */
	if (cfg_dfd.version < DFD_V2_0)
		return 0;

	return dfd_internal_dump_triggered;
}

/*
 * fills lastpc into the buffer, and returns the total length we wrote
 */
unsigned int dfd_internal_dump_get_decoded_lastpc(char* buf)
{
	unsigned int len = 0, i;
	unsigned long plat_sram_flag1 = readl(cfg_dfd.plat_sram_flag1);
	unsigned long plat_sram_flag2 = readl(cfg_dfd.plat_sram_flag2);

	len += sprintf(buf, "DFD triggered\nPlease refer to dfd post-processing result for lastpc\n\n");
	len += sprintf(buf + len, "plat_sram_flag1 = 0x%lx\n(dfd_valid=%x, dfd_valid_before_reboot=%x)\n",
	               plat_sram_flag1, extract_n2mbits(plat_sram_flag1, 0, 0),
	               extract_n2mbits(plat_sram_flag1, 1, 1));
	len += sprintf(buf + len, "plat_sram_flag2 = 0x%lx\n(base address=0x%llx)\n\n",
	               plat_sram_flag2,
	               (plat_sram_flag2 & ~(0x1))|(((uint64_t)plat_sram_flag2 & 0x1) << 32));

	if (!lastpc)
		return len;

	if (g_is_64bit_kernel) {
		for (i = 0; i <= cfg_pc_latch.nr_max_core-1; ++i)
			len += sprintf(buf + len, "[LAST PC] CORE_%d PC = 0x%016llx, FP = 0x%016llx, SP = 0x%016llx\n",
			               i, lastpc[i].pc, lastpc[i].fp_64, lastpc[i].sp_64);
	} else {
		for (i = 0; i <= cfg_pc_latch.nr_max_core-1; ++i)
			len += sprintf(buf + len, "[LAST PC] CORE_%d PC = 0x%016llx, FP = 0x%08lx, SP = 0x%08lx\n",
			               i, lastpc[i].pc, lastpc[i].fp_32, lastpc[i].sp_32);
	}

	free(lastpc);

	return len;
}

int dfd_get(void **data, int *len)
{
	unsigned int i, dfd_dump_type;
	unsigned long ret, nr_bytes_remained = 0, nr_total_words;
	uint64_t paddr;
	vaddr_t vaddr;

	if (len == NULL || data == NULL)
		return -1;

	*len = 0;
	*data = NULL;

	if (cfg_dfd.version < DFD_V2_0)
		return 0;

	if (dfd_internal_dump_check_triggered_or_not()) {
		dfd_dump_type = plt_get_dfd_dump_type();
		if (dfd_dump_type == DFD_DUMP_TO_DRAM) {
			/*
			 * use DRAM: need mapping to scratch memory before accessing
			 * base address[31:1] from AP view => plat_sram_flag2[31:1]
			 * base address[32:32] from AP view => plat_sram_flag2[0:0]
			 */
			vaddr = SCRATCH_ADDR;
			paddr = (uint64_t)readl(cfg_dfd.plat_sram_flag2 & ~(0x1))
			        |((uint64_t)(cfg_dfd.plat_sram_flag2 & 0x1) << 32);
			arch_mmu_map(paddr, vaddr,
			             MMU_MEMORY_TYPE_NORMAL_WRITE_BACK | MMU_MEMORY_AP_READ_ONLY, cfg_dfd.buffer_length);
		} else if (dfd_dump_type == DFD_DUMP_TO_SRAM) {
			/* use internal SRAM: no need to map */
			vaddr = cfg_dfd.buffer_addr;
			paddr = cfg_dfd.buffer_addr;
		} else {
			dprintf(CRITICAL, "[dfd] dfd_dump_type is \"not support\" -> skip\n");
			return 0;
		}

		/* check pa and va */
		if (paddr == 0 || vaddr == 0) {
			dprintf(CRITICAL, "[dfd] pa or va is invalid -> skip\npa = 0x%llx, va = 0x%lx",
			        paddr, vaddr);
			return 0;
		}

		dprintf(CRITICAL, "[dfd] pa = 0x%llx, va = 0x%lx, length = 0x%lx\n",
		        paddr, vaddr, cfg_dfd.buffer_length);

		/* allocate memory for AEE */
		*data = malloc(cfg_dfd.buffer_length);
		if (*data == NULL)
			return 0;

		nr_total_words = cfg_dfd.buffer_length / 4;
		nr_bytes_remained = cfg_dfd.buffer_length % 4;

		if (dfd_op.acquire_ram_control)
			dfd_op.acquire_ram_control();

		/* copy results */
		for (i = 0; i <= nr_total_words-1; i++) {
			ret = readl(vaddr + i*4);
			*(char *)(*data + i*4) = extract_n2mbits(ret, 0, 7);
			*(char *)(*data + i*4 + 1) = extract_n2mbits(ret, 8, 15);
			*(char *)(*data + i*4 + 2) = extract_n2mbits(ret, 16, 23);
			*(char *)(*data + i*4 + 3) = extract_n2mbits(ret, 24, 31);
		}

		/* handle unalgiend case */
		if (nr_bytes_remained != 0) {
			dprintf(CRITICAL, "[dfd] warning: the buffer length is not aligned to 4-byte\n");
			ret = readl(vaddr + nr_total_words*4);
			for (i = 0; i <= nr_bytes_remained-1; ++i)
				*(char *)(*data + nr_total_words*4 + i) =
				    extract_n2mbits(ret, 0 + 8*i, 7 + 8*i);
		}

		if (dfd_op.release_ram_control)
			dfd_op.release_ram_control();

		/* insert chip id */
		for (i = 0; i <= 7; ++i)
			*(char *)(*data + cfg_dfd.chip_id_offset + i) = cfg_dfd.chip_id[i];
		*len = cfg_dfd.buffer_length;

		/* decode lastpc */
		dfd_internal_dump_decode_lastpc((u64 *)*data);
	} else {
		/* not triggered */
		*len = 0;
	}

	return 1;
}

void dfd_put(void **data)
{
	free(*data);
}
