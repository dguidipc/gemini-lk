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
#include <reg.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/sync_write.h>
#include <plat_debug_interface.h>
#include <latch.h>
#include "utils.h"
#include "dfd.h"

#include <debug.h>

static int lastpc_dump(char *buf, int *wp)
{
	unsigned int i, cpu_in_cluster = 0, cluster_id;
	unsigned int lastpc_valid_before_reboot = 1;
	unsigned long long pc_value_h, fp_value_h, sp_value_h;
	unsigned long long pc_value, fp_value, sp_value;
	unsigned long dbg_ctrl_base, dbg_flag_base;
	unsigned long cpu_power_status = 0;
	unsigned long plat_sram_flag0;

	if (buf == NULL || wp == NULL)
		return -1;

	*wp += sprintf(buf + *wp, "\n*************************** lastpc ***************************\n");

	/* mcusys registers would be corrupted by DFD */
	if (dfd_internal_dump_before_reboot()) {
		*wp += dfd_internal_dump_get_decoded_lastpc(buf + *wp);
		return 1;
	}

	if (cfg_pc_latch.plat_sram_flag0) {
		/* must check lastpc_valid_before_reboot */
		plat_sram_flag0 = readl(cfg_pc_latch.plat_sram_flag0);
		lastpc_valid_before_reboot = extract_n2mbits(plat_sram_flag0, 1, 1);
		*wp += sprintf(buf + *wp,
		               "plat_sram_flag0 = 0x%lx\n(lastpc_valid=%x, lastpc_valid_before_reboot=%x)\n\n",
		               plat_sram_flag0, extract_n2mbits(plat_sram_flag0, 0, 0),
		               lastpc_valid_before_reboot);
	}

	/* get the power status information */
	cpu_power_status = plt_get_cpu_power_status_at_wdt();
	for (i = 0; i <= cfg_pc_latch.nr_max_core-1; ++i) {
		/* if lastpc_valid_before_reboot is not 1 --> only dump CPU0, skip others */
		if (lastpc_valid_before_reboot != 1 && i >= 1) {
			*wp += sprintf(buf + *wp,
			               "[LAST PC] CORE_%d PC = 0x0, FP = 0x0, SP = 0x0\n", i);
			continue;
		}

		/* if CPUX is not powered on before reboot --> skip */
		if (extract_n2mbits(cpu_power_status, i, i) == 0) {
			*wp += sprintf(buf + *wp,
			               "[LAST PC] CORE_%d PC = 0x0, FP = 0x0, SP = 0x0\n", i);
			continue;
		}

		cluster_id = plt_get_cluster_id(i, &cpu_in_cluster);
		if (cluster_id == 0) {
			/* MP0 */
			dbg_ctrl_base = MCUCFG_BASE + cfg_pc_latch.mp0_dbg_ctrl;
			dbg_flag_base = MCUCFG_BASE + cfg_pc_latch.mp0_dbg_flag;
		} else if (cluster_id == 1) {
			/* MP1 */
			dbg_ctrl_base = MCUCFG_BASE + cfg_pc_latch.mp1_dbg_ctrl;
			dbg_flag_base = MCUCFG_BASE + cfg_pc_latch.mp1_dbg_flag;
		} else
			continue;

		writel((cpu_in_cluster << 4) | 1, dbg_ctrl_base);
		pc_value_h = readl(dbg_flag_base);
		writel((cpu_in_cluster << 4) | 0, dbg_ctrl_base);
		pc_value = (pc_value_h << 32) | readl(dbg_flag_base);

		/* TODO: query kernel symbol */
		/* get the 64bit/32bit kernel information from bootopt */
		if (g_is_64bit_kernel) {
			writel((cpu_in_cluster << 4) | 5, dbg_ctrl_base);
			fp_value_h = readl(dbg_flag_base);
			writel((cpu_in_cluster << 4) | 4, dbg_ctrl_base);
			fp_value = (fp_value_h << 32) | readl(dbg_flag_base);

			writel((cpu_in_cluster << 4) | 7, dbg_ctrl_base);
			sp_value_h = readl(dbg_flag_base);
			writel((cpu_in_cluster << 4) | 6, dbg_ctrl_base);
			sp_value = (sp_value_h << 32) | readl(dbg_flag_base);

			*wp += sprintf(buf + *wp,
			               "[LAST PC] CORE_%d PC = 0x%016llx, FP = 0x%016llx, SP = 0x%016llx\n",
			               i, pc_value, fp_value, sp_value);
		} else {
			writel((cpu_in_cluster << 4) | 2, dbg_ctrl_base);
			fp_value = readl(dbg_flag_base);

			writel((cpu_in_cluster << 4) | 3, dbg_ctrl_base);
			sp_value = readl(dbg_flag_base);

			*wp += sprintf(buf + *wp,
			               "[LAST PC] CORE_%d PC = 0x%016llx, FP = 0x%08llx, SP = 0x%08llx\n",
			               i, pc_value, fp_value, sp_value);
		}
	}

	*wp += sprintf(buf + *wp, "\n");
	return 1;
}

static int circular_buffer_dump(char *buf, int *wp)
{
	unsigned int i, j, cpuid;
	unsigned int lastpc_valid_before_reboot = 1;
	unsigned long addr;
	unsigned long plat_sram_flag0;
	unsigned long cpu_power_status = 0;
	unsigned long long pc, pc_h;

	if (buf == NULL || wp == NULL)
		return -1;

	if (cfg_pc_latch.nr_max_big_core == 0)
		return 0;

	*wp += sprintf(buf + *wp,
	               "\n*************************** circular buffer ***************************\n");

	if (cfg_pc_latch.plat_sram_flag0) {
		/* if lastpc_valid_before_reboot is 0 => circular buffer is invalid */
		plat_sram_flag0 = readl(cfg_pc_latch.plat_sram_flag0);
		lastpc_valid_before_reboot = extract_n2mbits(plat_sram_flag0, 1, 1);
		*wp += sprintf(buf + *wp,
		               "plat_sram_flag0 = 0x%lx\n(lastpc_valid=%x, lastpc_valid_before_reboot=%x)\n\n",
		               plat_sram_flag0, extract_n2mbits(plat_sram_flag0, 0, 0),
		               lastpc_valid_before_reboot);
	}

	if (lastpc_valid_before_reboot == 0) {
		*wp += sprintf(buf + *wp, "lastpc_valid_before_reboot is 0 => circular buffer is invalid\n\n");
		return 1;
	}

	/* get the power status information */
	cpu_power_status = plt_get_cpu_power_status_at_wdt();

	if (circular_buffer_op.unlock)
		circular_buffer_op.unlock();

	for (i = 0; i < cfg_pc_latch.nr_max_big_core; ++i) {
		cpuid = cfg_big_core[i].cpuid;
		if (extract_n2mbits(cpu_power_status, cpuid, cpuid) == 0)
			continue;

		addr = cfg_big_core[i].circular_buffer_addr;
		*wp += sprintf(buf + *wp, "[CIRCULAR BUFFER: CORE_%d]\n", cpuid);
		for (j = 0; j <= cfg_big_core[i].nr_circular_buffer_entry-1; ++j) {
			pc_h = readl(addr + (j*8) + 4);
			pc = (pc_h << 32) | readl(addr + (j*8));
			*wp += sprintf(buf + *wp, "0x%016llx\n", pc);
		}
	}

	if (circular_buffer_op.lock)
		circular_buffer_op.lock();

	*wp += sprintf(buf + *wp, "\n");
	return 1;
}

static int lastbus_dump(char *buf, int *wp)
{
	unsigned int i;
	unsigned long mcu_base = MCUCFG_BASE;
	unsigned long peri_base = PERICFG_BASE;
	unsigned long infra_base = INFRACFG_AO_BASE;
	unsigned long meter;
	unsigned long debug_raw;
	unsigned long w_counter, r_counter, c_counter;

	if (buf == NULL || wp == NULL)
		return -1;

	if (cfg_bus_latch.supported == 0)
		return 0;

	*wp += sprintf(buf + *wp, "\n*************************** lastbus ***************************\n");

	/* mcusys registers would be corrupted by DFD */
	if (!dfd_internal_dump_before_reboot()) {
		for (i = 0; i <= cfg_bus_latch.num_master_port-1; ++i) {
			debug_raw = readl(mcu_base + cfg_bus_latch.mcusys_offsets.bus_mcu_m0 + 4 * i);
			meter = readl(mcu_base + cfg_bus_latch.mcusys_offsets.bus_mcu_m0_m + 4 * i);
			w_counter = meter & 0x3f;
			r_counter = (meter >> 8) & 0x3f;
			if ((w_counter != 0) || (r_counter != 0)) {

				*wp += sprintf(buf + *wp, "[LAST BUS] Master %d: ", i);
				*wp += sprintf(buf + *wp,
				               "aw_pending_counter = 0x%02lx, ar_pending_counter = 0x%02lx\n",
				               w_counter, r_counter);
				*wp += sprintf(buf + *wp, "STATUS = %03lx\n", debug_raw & 0x3ff);
			}
		}

		for (i = 1; i <= cfg_bus_latch.num_slave_port-1; ++i) {
			debug_raw = readl(mcu_base + cfg_bus_latch.mcusys_offsets.bus_mcu_s1 + 4 * (i-1));
			meter = readl(mcu_base + cfg_bus_latch.mcusys_offsets.bus_mcu_s1_m + 4 * (i-1));

			w_counter = meter & 0x3f;
			r_counter = (meter >> 8) & 0x3f;
			c_counter = (meter >> 16) & 0x3f;

			if ((w_counter != 0) || (r_counter != 0) || (c_counter != 0)) {
				*wp += sprintf(buf + *wp, "[LAST BUS] Slave %d: ", i);

				*wp += sprintf(buf + *wp,
				               "aw_pending_counter = 0x%02lx, ar_pending_counter = 0x%02lx,",
				               w_counter, r_counter);
				*wp += sprintf(buf + *wp, " ac_pending_counter = 0x%02lx\n", c_counter);
				if (i <= 2)
					*wp += sprintf(buf + *wp, "STATUS = %04lx\n", debug_raw & 0x3fff);
				else
					*wp += sprintf(buf + *wp, "STATUS = %04lx\n", debug_raw & 0xffff);
			}
		}
	}

	/* always check: not be corrupted by DFD */
	if (cfg_bus_latch.secure_perisys == 1) {
		if (mt_secure_call(MTK_SIP_LK_LASTBUS, LASTBSU_SMC_CHECK_HANG, 0, 0) == 0x1) {
			*wp += sprintf(buf + *wp, "[LAST BUS] PERISYS TIMEOUT:\n");
			for (i = 0; i <= cfg_bus_latch.num_perisys_mon-1; ++i)
				*wp += sprintf(buf + *wp, "PERI MON%d = %04lx\n",
				               i, mt_secure_call(MTK_SIP_LK_LASTBUS, LASTBSU_SMC_MONITOR_GET, i, 0));
		}
	} else {
		if (readl(peri_base + cfg_bus_latch.perisys_offsets.bus_peri_r1) & 0x1) {
			*wp += sprintf(buf + *wp, "[LAST BUS] PERISYS TIMEOUT:\n");
			for (i = 0; i <= cfg_bus_latch.num_perisys_mon-1; ++i)
				*wp += sprintf(buf + *wp, "PERI MON%d = %04x\n",
				               i, readl(peri_base + cfg_bus_latch.perisys_offsets.bus_peri_mon + 4*i));
		}
	}

	if (cfg_bus_latch.num_infrasys_mon != 0) {
		if (readl(infra_base + cfg_bus_latch.infrasys_offsets.bus_infra_ctrl) & 0xFF000000) {
			*wp += sprintf(buf + *wp, "[LAST BUS] INFRASYS TIMEOUT:\n");
			for (i = 0; i <= cfg_bus_latch.num_infrasys_mon-1; ++i)
				*wp += sprintf(buf + *wp, "INFRA SNAPSHOT%d = %04x\n",
					       i, readl(infra_base + cfg_bus_latch.infrasys_offsets.bus_infra_snapshot + 4*i));
		}
	}

	*wp += sprintf(buf + *wp, "\n");
	return 1;
}

static int l2_parity_dump(char *buf, int *wp)
{
	unsigned long ret;
	unsigned int err_found = 0;

	if (buf == NULL || wp == NULL)
		return -1;

	if (cfg_l2_parity_latch.supported != 1)
		return 0;

	*wp += sprintf(buf + *wp, "\n*************************** l2c parity ***************************\n");

	/* mcusys registers would be corrupted by DFD */
	if (dfd_internal_dump_before_reboot()) {
		*wp += sprintf(buf + *wp, "DFD triggered\nPlease refer to dfd post-processing result for L2C parity\n");
		return 1;
	}

	ret = readl(MCUCFG_BASE + cfg_l2_parity_latch.mp0_l2_cache_parity1_rdata);
	if (ret & 0x1) {
		/* get parity error in mp0 */
		*wp += sprintf(buf + *wp, "[L2C parity] get parity error in mp0\n");
		*wp += sprintf(buf + *wp, "error count = 0x%x\n", extract_n2mbits(ret, 8, 15));
		ret = readl(MCUCFG_BASE + cfg_l2_parity_latch.mp0_l2_cache_parity2_rdata);
		*wp += sprintf(buf + *wp, "index = 0x%x\n", extract_n2mbits(ret, 0, 14));
		*wp += sprintf(buf + *wp, "bank = 0x%x\n", extract_n2mbits(ret, 16, 31));

		/* clear mcusys parity check registers */
		writel(0x0, MCUCFG_BASE + cfg_l2_parity_latch.mp0_l2_cache_parity1_rdata);

		err_found = 1;
	}

	ret = readl(MCUCFG_BASE + cfg_l2_parity_latch.mp1_l2_cache_parity1_rdata);
	if (ret & 0x1) {
		/* get parity error in mp1 */
		*wp += sprintf(buf + *wp, "[L2C parity] get parity error in mp1\n");
		*wp += sprintf(buf + *wp, "error count = 0x%x\n", extract_n2mbits(ret, 8, 15));
		ret = readl(MCUCFG_BASE + cfg_l2_parity_latch.mp1_l2_cache_parity2_rdata);
		*wp += sprintf(buf + *wp, "index = 0x%x\n", extract_n2mbits(ret, 0, 14));
		*wp += sprintf(buf + *wp, "bank = 0x%x\n", extract_n2mbits(ret, 16, 31));

		/* clear mcusys parity check registers */
		writel(0x0, MCUCFG_BASE + cfg_l2_parity_latch.mp1_l2_cache_parity1_rdata);

		err_found = 1;
	}

	if (err_found == 0)
		*wp += sprintf(buf + *wp, "[L2C parity] no parity error found\n");

	*wp += sprintf(buf + *wp, "\n");

	return 1;
}

int latch_get(void **data, int *len)
{
	int ret;

	*len = 0;
	*data = malloc(LATCH_BUF_LENGTH);
	if (*data == NULL)
		return 0;

	ret = lastpc_dump(*data, len);
	if (ret < 0 || *len > LATCH_BUF_LENGTH) {
		*len = (*len > LATCH_BUF_LENGTH) ? LATCH_BUF_LENGTH : *len;
		return ret;
	}

	ret = circular_buffer_dump(*data, len);
	if (ret < 0 || *len > LATCH_BUF_LENGTH) {
		*len = (*len > LATCH_BUF_LENGTH) ? LATCH_BUF_LENGTH : *len;
		return ret;
	}

	ret = lastbus_dump(*data, len);
	if (ret < 0 || *len > LATCH_BUF_LENGTH) {
		*len = (*len > LATCH_BUF_LENGTH) ? LATCH_BUF_LENGTH : *len;
		return ret;
	}

	ret = l2_parity_dump(*data, len);
	if (ret < 0 || *len > LATCH_BUF_LENGTH) {
		*len = (*len > LATCH_BUF_LENGTH) ? LATCH_BUF_LENGTH : *len;
		return ret;
	}

	return 1;
}

void latch_put(void **data)
{
	free(*data);
}

void latch_lastbus_init(void)
{
	unsigned long addr = 0;

	if (cfg_bus_latch.supported == 0)
		return;

	if (cfg_bus_latch.secure_perisys == 1)
		mt_secure_call(MTK_SIP_LK_LASTBUS, LASTBSU_SMC_INIT,
		               cfg_bus_latch.perisys_timeout, cfg_bus_latch.perisys_enable);
	else {
		writel(cfg_bus_latch.perisys_timeout, PERICFG_BASE + cfg_bus_latch.perisys_offsets.bus_peri_r0);
		writel(cfg_bus_latch.perisys_enable, PERICFG_BASE + cfg_bus_latch.perisys_offsets.bus_peri_r1);
	}

	if (cfg_bus_latch.num_infrasys_mon != 0) {
		addr = INFRACFG_AO_BASE + cfg_bus_latch.infrasys_offsets.bus_infra_ctrl;
		writel(cfg_bus_latch.infrasys_config, addr);
		writel(readl(addr)|cfg_bus_latch.infrasys_enable, addr);
	}
}
