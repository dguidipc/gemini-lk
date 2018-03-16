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

#include <stdint.h>
#include <string.h>
#include <debug.h>
#include <platform/boot_mode.h>
#include <platform/partition.h>
#include <platform/env.h>
#ifdef DEVICE_TREE_SUPPORT
#include <libfdt.h>
#endif
#include <platform/emi_mpu.h>


#define SCP_DRAM_SIZE       0x00200000  // mblock_reserve requires 2MB, SCP only requires 512KB
#define DRAM_ADDR_MAX       0xBFFFFFFF  // max address can SCP remap
#define DRAM_4GB_MAX        0xFFFFFFFF
#define DRAM_4GB_OFFSET     0x40000000
#define SCP_EMI_REGION      2
#define LOADER_NAME         "tinysys-loader-CM4_A"
#define FIRMWARE_NAME       "tinysys-scp-CM4_A"

unsigned int get_scp_status(void);
extern u64 physical_memory_size(void);
extern u64 mblock_reserve(mblock_info_t *mblock_info, u64 size, u64 align, u64 limit, enum reserve_rank rank);
extern int mboot_common_load_part(char *part_name, char *img_name, unsigned long addr);
/* return 0: success */
extern uint32_t sec_img_auth_init(uint8_t *part_name, uint8_t *img_name) ;
/* return 0: success */
extern uint32_t sec_img_auth(uint8_t *img_buf, uint32_t img_buf_sz);
extern int verify_load_scp_image(char *part_name, char *loader_name, char *firmware_name, void *addr, unsigned int offset);


extern BOOT_ARGUMENT *g_boot_arg;

static int load_scp_status = 0;
static void *dram_addr;
static char *scp_part_name[] = {"scp1", "scp2"};


int load_scp_image(char *part_name, char *img_name, void *addr)
{
	uint32_t sec_ret;
	uint32_t scp_vfy_time;
	int ret;
#ifdef MTK_SECURITY_SW_SUPPORT
	unsigned int policy_entry_idx = 0;
	unsigned int img_auth_required = 0;

	policy_entry_idx = get_policy_entry_idx(part_name);
	img_auth_required = get_vfy_policy(policy_entry_idx);
	/* verify cert chain of boot img */
	if (img_auth_required) {
		scp_vfy_time = get_timer(0);
		sec_ret = sec_img_auth_init(part_name, img_name);
		if (sec_ret)
			return -1;
		dprintf(DEBUG, "[SBC] scp cert vfy pass(%d ms)\n", (unsigned int)get_timer(scp_vfy_time));
	}
#endif

	ret = mboot_common_load_part(part_name, img_name, addr);

	dprintf(INFO, "%s(): ret=%d\n", __func__, ret);

	if (ret <= 0)
		return -1;

#ifdef MTK_SECURITY_SW_SUPPORT
	if (img_auth_required) {
		scp_vfy_time = get_timer(0);
		sec_ret = sec_img_auth(addr, ret);
		if (sec_ret)
			return -1;
		dprintf(DEBUG, "[SBC] scp vfy pass(%d ms)\n", (unsigned int)get_timer(scp_vfy_time));
	}
#endif

	return ret;
}

static char *scp_partition_name(void)
{
	int i;
	part_t *part;

	for (i = 0; i < (sizeof(scp_part_name) / sizeof(*scp_part_name)); i++) {
		part = get_part(scp_part_name[i]);
		if (part && mt_part_get_part_active_bit(part))
			return scp_part_name[i];
	}

	dprintf(CRITICAL, "no scp partition with active bit marked, load %s\n", scp_part_name[0]);

	return scp_part_name[0];
}

int load_scp(void)
{
	int ret;
	unsigned int dram_ofs, perm;
	u64 dram_size;
	char *part_name;

	/* check if scp is turned off manually*/
	if (get_scp_status() == 0)
	{
		ret = -1;
		goto error;
	}

	dram_addr = (void *) mblock_reserve(&g_boot_arg->mblock_info, SCP_DRAM_SIZE, 0x10000, DRAM_ADDR_MAX, RANKMAX);

	dprintf(INFO, "%s(): dram_addr=%p\n", __func__, dram_addr);

	if (dram_addr == ((void *) 0))
		goto error;

	dram_size = physical_memory_size();

	if (dram_size > DRAM_4GB_MAX)
		dram_ofs = DRAM_4GB_OFFSET;
	else
		dram_ofs = 0;

	part_name = scp_partition_name();

	if (part_name) {
		ret = verify_load_scp_image(part_name, LOADER_NAME, FIRMWARE_NAME, dram_addr, dram_ofs);

		if (ret < 0) {
			dprintf(CRITICAL, "scp verify %s failed, code=%d\n", part_name, ret);
			goto error;
		}
	} else {
		ret = -1;
		dprintf(CRITICAL, "get scp partition failed\n");
		goto error;
	}

	/*clean dcache & icache before set up EMI MPU*/
	arch_sync_cache_range((addr_t)dram_addr, SCP_DRAM_SIZE);

	/*
	 * setup EMI MPU
	 * domain 0: AP
	 * domain 3: SCP
	 */
	perm = SET_ACCESS_PERMISSON(UNLOCK, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN, SEC_RW);
	emi_mpu_set_region_protection(dram_addr, dram_addr + SCP_DRAM_SIZE - 1, SCP_EMI_REGION, perm);

	dprintf(INFO, "%s(): done\n", __func__);

	load_scp_status = 1;

	return 0;

error:
	/*
	 * @ret = 0, malloc() error
	 * @ret < 0, error code from load_scp_image()
	 */
	load_scp_status = ret;

	return -1;
}

#ifdef DEVICE_TREE_SUPPORT
int platform_fdt_scp(void *fdt)
{
	int nodeoffset;
	char *ret;

	dprintf(CRITICAL, "%s()\n", __func__);
	nodeoffset = fdt_node_offset_by_compatible(fdt, -1, "mediatek,scp");

	if (nodeoffset >= 0) {
		if (load_scp_status <= 0)
			ret = "fail";
		else
			ret = "okay";

		dprintf(CRITICAL, "status=%s\n", ret);

		fdt_setprop(fdt, nodeoffset, "status", ret, strlen(ret));

		return 0;
	}

	return 1;
}
#endif

#ifdef MTK_SECURITY_SW_SUPPORT
#define SCP_ONE_TIME_LOCK 0x100A00DC
#define SCP_SECURE_CRTL 0x100A00E0
#define SCP_SECURE_JTAG_MASK (0x1UL << 21)

void scp_jtag_enable()
{
	*(volatile unsigned int*)(SCP_SECURE_CRTL) &= ~SCP_SECURE_JTAG_MASK;
}

void scp_jtag_disable()
{
	*(volatile unsigned int*)(SCP_SECURE_CRTL) |= SCP_SECURE_JTAG_MASK;
}

void scp_jtag_sync()
{
	if (sec_dbgport_is_disabled()) {
		scp_jtag_disable();
	} else {
		scp_jtag_enable();
	}

	*(volatile unsigned int*)(SCP_ONE_TIME_LOCK) |= SCP_SECURE_JTAG_MASK;

	dprintf(SPEW, "SW SECURE JTAG setting is %s, SCP_SECURE_CRTL is 0x%08x \n",
	        (sec_dbgport_is_disabled())? "disable" : "enable", *(volatile unsigned int*)(SCP_SECURE_CRTL) );
}
#endif
unsigned int get_scp_status(void)
{
	unsigned int scp_status;

	/*if there is no env['scp], scp should be still enabled*/
	scp_status = (get_env("scp") == NULL) ? 1 : atoi(get_env("scp"));
	dprintf(CRITICAL,"[SCP] current setting is %d.\n", scp_status);
	return scp_status;
}

unsigned int set_scp_status(unsigned int en)
{
	char *SCP_STATUS[2] = {"0","1"};

	get_scp_status();
	if (set_env("scp", SCP_STATUS[en]) == 0) {
		dprintf(CRITICAL,"[SCP]set SCP %s success. Plz reboot to make it applied.\n",SCP_STATUS[en]);
		return 0;
	} else {
		dprintf(CRITICAL,"[SCP]set SCP %s fail.\n",SCP_STATUS[en]);
		return 1;
	}
}
