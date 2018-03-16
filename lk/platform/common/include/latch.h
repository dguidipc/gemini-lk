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

#ifndef __LATCH_H__
#define __LATCH_H__

/* a 4KB buffer is sufficient for lastpc, lastbus */
#define LATCH_BUF_LENGTH    4096
#define MTK_SIP_LK_LASTBUS  0x82000107
#define LASTBSU_SMC_INIT    0
#define LASTBSU_SMC_CHECK_HANG  1
#define LASTBSU_SMC_MONITOR_GET 2

/* platform-dependent configs for lastpc */
struct plt_cfg_pc_latch {
	unsigned int nr_max_core;
	unsigned int nr_max_big_core;
	unsigned int mp0_dbg_ctrl;
	unsigned int mp0_dbg_flag;
	unsigned int mp1_dbg_ctrl;
	unsigned int mp1_dbg_flag;
	unsigned int spm_pwr_sts;
	unsigned long plat_sram_flag0;
};

/* platform-dependent configs for lastbus */
struct lastbus_mcusys_offsets {
	unsigned int bus_mcu_m0;
	unsigned int bus_mcu_s1;
	unsigned int bus_mcu_m0_m;
	unsigned int bus_mcu_s1_m;
};

struct lastbus_perisys_offsets {
	unsigned int bus_peri_r0;
	unsigned int bus_peri_r1;
	unsigned int bus_peri_mon;
};

struct lastbus_infrasys_offsets {
	unsigned int bus_infra_ctrl;
	unsigned int bus_infra_mask_l;
	unsigned int bus_infra_mask_h;
	unsigned int bus_infra_snapshot;
};

struct plt_cfg_bus_latch {
	unsigned int supported;
	unsigned int num_master_port;
	unsigned int num_slave_port;
	unsigned int num_perisys_mon;
	unsigned int num_infrasys_mon;
	unsigned int secure_perisys;
	unsigned int perisys_enable;
	unsigned int perisys_timeout;
	unsigned int infrasys_enable;
	unsigned int infrasys_config;
	struct lastbus_mcusys_offsets mcusys_offsets;
	struct lastbus_perisys_offsets perisys_offsets;
	struct lastbus_infrasys_offsets infrasys_offsets;
};

struct plt_cfg_l2_parity_latch {
	unsigned int supported;
	unsigned int mp0_l2_cache_parity1_rdata;
	unsigned int mp0_l2_cache_parity2_rdata;
	unsigned int mp1_l2_cache_parity1_rdata;
	unsigned int mp1_l2_cache_parity2_rdata;
};

struct plt_cfg_big_core {
	unsigned int cpuid;
	unsigned int nr_circular_buffer_entry;
	unsigned long circular_buffer_addr;
};

struct plt_circular_buffer_op {
	void (*unlock)(void);
	void (*lock)(void);
};

extern int plt_get_cluster_id(unsigned int cpu_id, unsigned int *core_id_in_cluster);
extern unsigned long plt_get_cpu_power_status_at_wdt(void);
extern const struct plt_cfg_pc_latch cfg_pc_latch;
extern const struct plt_cfg_bus_latch cfg_bus_latch;
extern const struct plt_cfg_l2_parity_latch cfg_l2_parity_latch;
extern const struct plt_cfg_big_core cfg_big_core[];

extern struct plt_circular_buffer_op circular_buffer_op;
extern int g_is_64bit_kernel;

void latch_lastbus_init(void);
#endif
