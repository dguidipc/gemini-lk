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

#include <debug.h>
#include <stdlib.h>
#include <string.h>
#include <video.h>
#include <dev/uart.h>
#include <arch/arm.h>
#include <arch/arm/mmu.h>
#include <arch/ops.h>
#include <target/board.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_disp_drv.h>
#include <platform/disp_drv.h>
#include <platform/boot_mode.h>
#include <platform/mt_logo.h>
#include <platform/partition.h>
#include <platform/env.h>
#include <platform/mt_gpio.h>
#include <platform/mt_pmic.h>
#include <platform/mt_pmic_wrap_init.h>
#include <platform/mt_i2c.h>
#include <platform/mtk_key.h>
#include <platform/mt_rtc.h>
#include <platform/mt_leds.h>
#include <platform/upmu_common.h>
#include <platform/mtk_wdt.h>
#include <platform/disp_drv_platform.h>
#include <libfdt.h>
#if defined(MTK_SECURITY_SW_SUPPORT)
#include "oemkey.h"
#endif

#ifdef MTK_SECURITY_SW_SUPPORT
extern u8 g_oemkey[OEM_PUBK_SZ];
#endif

#ifdef LK_DL_CHECK
/*block if check dl fail*/
#undef LK_DL_CHECK_BLOCK_LEVEL
#endif
//#define MT_SRAM_REPAIR_SUPPORT

extern void platform_early_init_timer();
extern void jump_da(u32 addr, u32 arg1, u32 arg2);
extern int i2c_hw_init(void);
extern int mboot_common_load_logo(unsigned long logo_addr, char* filename);
extern int sec_func_init(u64 pl_start_addr);
extern int sec_usbdl_enabled (void);
extern void mtk_wdt_disable(void);
extern void platform_deinit_interrupts(void);
extern int mmc_get_dl_info(void);
extern int mmc_legacy_init(int);
#ifdef MT_SRAM_REPAIR_SUPPORT
extern int repair_sram(void);
#endif

#ifdef MTK_BATLOWV_NO_PANEL_ON_EARLY
extern kal_bool is_low_battery(kal_int32 val);
extern int hw_charging_get_charger_type(void);
#endif

#ifdef MTK_LK_REGISTER_WDT
extern void lk_register_wdt_callback(void);
#endif

void platform_uninit(void);
void config_shared_SRAM_size(void);
extern int dev_info_nr_cpu(void);
extern void mt_pll_turn_off(void);

struct mmu_initial_mapping mmu_initial_mappings[] = {

	{
		.phys = (uint64_t)0,
		.virt = (uint32_t)0,
		.size = 0x40000000,
		.flags = MMU_MEMORY_TYPE_STRONGLY_ORDERED | MMU_MEMORY_AP_P_RW_U_NA,
		.name = "mcusys"
	},
	{
		.phys = (uint64_t)MEMBASE,
		.virt = (uint32_t)MEMBASE,
		.size = ROUNDUP(MEMSIZE, SECTION_SIZE),
		.flags = MMU_MEMORY_TYPE_NORMAL_WRITE_BACK | MMU_MEMORY_AP_P_RW_U_NA,
		.name = "ram"
	},
	{
		.phys = (uint64_t)CFG_BOOTIMG_LOAD_ADDR,
		.virt = (uint32_t)CFG_BOOTIMG_LOAD_ADDR,
		.size = 64*MB,
		.flags = MMU_MEMORY_TYPE_NORMAL_WRITE_BACK | MMU_MEMORY_AP_P_RW_U_NA,
		.name = "bootimg"
	},
	{
		.phys = (uint64_t)SCRATCH_ADDR,
		.virt = (uint32_t)SCRATCH_ADDR,
		.size = SCRATCH_SIZE + 16*MB,
		.flags = MMU_MEMORY_TYPE_NORMAL_WRITE_BACK | MMU_MEMORY_AP_P_RW_U_NA,
		.name = "download"
	},
	/* null entry to terminate the list */
	{ 0 }
};

BOOT_ARGUMENT *g_boot_arg;
BOOT_ARGUMENT boot_addr;
int g_nr_bank;
BI_DRAM bi_dram[MAX_NR_BANK];
unsigned int g_fb_base;
unsigned int g_fb_size;
#if defined(CFG_DTB_EARLY_LOADER_SUPPORT)
void *g_fdt;
#endif
static int g_dram_init_ret;
static unsigned int bootarg_addr;
static unsigned int bootarg_size;

int dram_init(void)
{
	int i;
	struct boot_tag *tags;

	/* Get parameters from pre-loader. Get as early as possible
	 * The address of BOOT_ARGUMENT_LOCATION will be used by Linux later
	 * So copy the parameters from BOOT_ARGUMENT_LOCATION to LK's memory region
	 */
	g_boot_arg = &boot_addr;
	bootarg_addr = (unsigned int *)BOOT_ARGUMENT_LOCATION;
	bootarg_size = 0;

	if (*(unsigned int *)BOOT_ARGUMENT_LOCATION == BOOT_ARGUMENT_MAGIC) {
		memcpy(g_boot_arg, (void*)BOOT_ARGUMENT_LOCATION, sizeof(BOOT_ARGUMENT));
		bootarg_size = sizeof(BOOT_ARGUMENT);
	} else {
		g_boot_arg->maggic_number = BOOT_ARGUMENT_MAGIC;
		for (tags = (void *)BOOT_ARGUMENT_LOCATION; tags->hdr.size; tags = boot_tag_next(tags)) {
			switch (tags->hdr.tag) {
				case BOOT_TAG_BOOT_REASON:
					g_boot_arg->boot_reason = tags->u.boot_reason.boot_reason;
					break;
				case BOOT_TAG_BOOT_MODE:
					g_boot_arg->boot_mode = tags->u.boot_mode.boot_mode;
					break;
				case BOOT_TAG_META_COM:
					g_boot_arg->meta_com_type = tags->u.meta_com.meta_com_type;
					g_boot_arg->meta_com_id = tags->u.meta_com.meta_com_id;
					g_boot_arg->meta_uart_port = tags->u.meta_com.meta_uart_port;
					break;
				case BOOT_TAG_LOG_COM:
					g_boot_arg->log_port = tags->u.log_com.log_port;
					g_boot_arg->log_baudrate = tags->u.log_com.log_baudrate;
					g_boot_arg->log_enable = tags->u.log_com.log_enable;
					break;
				case BOOT_TAG_MEM:
					g_boot_arg->dram_rank_num = tags->u.mem.dram_rank_num;
					for (i = 0; i < tags->u.mem.dram_rank_num; i++) {
						g_boot_arg->dram_rank_size[i] = tags->u.mem.dram_rank_size[i];
					}
					g_boot_arg->mblock_info = tags->u.mem.mblock_info;
					g_boot_arg->orig_dram_info = tags->u.mem.orig_dram_info;
					g_boot_arg->lca_reserved_mem = tags->u.mem.lca_reserved_mem;
					g_boot_arg->tee_reserved_mem = tags->u.mem.tee_reserved_mem;
					break;
				case BOOT_TAG_MD_INFO:
					for (i = 0; i < 4; i++) {
						g_boot_arg->md_type[i] = tags->u.md_info.md_type[i];
					}
					break;
				case BOOT_TAG_BOOT_TIME:
					g_boot_arg->boot_time = tags->u.boot_time.boot_time;
					break;
				case BOOT_TAG_DA_INFO:
					memcpy(&g_boot_arg->da_info, &tags->u.da_info.da_info, sizeof(da_info_t));
					break;
				case BOOT_TAG_SEC_INFO:
					memcpy(&g_boot_arg->sec_limit, &tags->u.sec_info.sec_limit, sizeof(SEC_LIMIT));
					break;
				case BOOT_TAG_PART_NUM:
					g_boot_arg->part_num = tags->u.part_num.part_num;
					break;
				case BOOT_TAG_PART_INFO:
					g_boot_arg->part_info = tags->u.part_info.part_info;  /* only copy the pointer but the contains*/
					break;
				case BOOT_TAG_EFLAG:
					g_boot_arg->e_flag = tags->u.eflag.e_flag;
					break;
				case BOOT_TAG_DDR_RESERVE:
					g_boot_arg->ddr_reserve_enable = tags->u.ddr_reserve.ddr_reserve_enable;
					g_boot_arg->ddr_reserve_success = tags->u.ddr_reserve.ddr_reserve_success;
					g_boot_arg->ddr_reserve_ready = tags->u.ddr_reserve.ddr_reserve_ready;
					break;
				case BOOT_TAG_DRAM_BUF:
					g_boot_arg->dram_buf_size = tags->u.dram_buf.dram_buf_size;
					break;
				case BOOT_TAG_SRAM_INFO:
					g_boot_arg->non_secure_sram_addr = tags->u.sram_info.non_secure_sram_addr;
					g_boot_arg->non_secure_sram_size = tags->u.sram_info.non_secure_sram_size;
					break;
				case BOOT_TAG_PTP:
					memcpy(&g_boot_arg->ptp_volt_info, &tags->u.ptp_volt.ptp_volt_info, sizeof(ptp_info_t));
					break;
				case BOOT_TAG_EXTBUCK_FAN53526:
					g_boot_arg->extbuck_fan53526_exist = tags->u.extbuck_fan53526.exist;
					break;

				default:
					break;
			}

			bootarg_size += tags->hdr.size;
		}
	}


#ifdef MACH_FPGA
	g_nr_bank = 2;
	bi_dram[0].start = DRAM_PHY_ADDR + RIL_SIZE;
	bi_dram[0].size = (256 * 1024 * 1024) - RIL_SIZE;
	bi_dram[1].start = bi_dram[0].start + bi_dram[0].size;
	bi_dram[1].size = (256 * 1024 * 1024);
#else
	g_nr_bank = g_boot_arg->dram_rank_num;

	if (g_nr_bank == 0 || g_nr_bank > MAX_NR_BANK) {
		g_dram_init_ret = -1;
		//dprintf(CRITICAL, "[LK ERROR] DRAM bank number is not correct!!!");
		//while (1) ;
		return -1;
	}

#ifndef CUSTOM_CONFIG_MAX_DRAM_SIZE
	/* return the actual DRAM info */
	for (i = 0; i < g_boot_arg->mblock_info.mblock_num; i++) {
		bi_dram[i].start = g_boot_arg->mblock_info.mblock[i].start;
		bi_dram[i].size = g_boot_arg->mblock_info.mblock[i].size;
	}
//#elif (CUSTOM_CONFIG_MAX_DRAM_SIZE < 0x10000000)
//#error "DRAM size < 0x10000000" /* DRAM is less than 256MB, trigger build error */
#else
#endif

#endif

	return 0;
}

unsigned int platform_get_bootarg_addr(void)
{
	return bootarg_addr;
}

unsigned int platform_get_bootarg_size(void)
{
	return bootarg_size;
}

/*******************************************************
 * Routine: memory_size
 * Description: return DRAM size to LCM driver
 ******************************************************/
u32 ddr_enable_4gb(void)
{
	u32 status;
#define INFRA_MISC      (INFRACFG_AO_BASE + 0x0F00)
#define INFRA_4GB_EN    (1 << 13)
#define PERI_MISC       (PERICFG_BASE + 0x0208)
#define PERI_4GB_EN     (1 << 15)

	status = ((DRV_Reg32(INFRA_MISC) & INFRA_4GB_EN) && (DRV_Reg32(PERI_MISC) & PERI_4GB_EN)) ? 1 : 0;
	dprintf(CRITICAL, "%s() status=%u\n", __func__, status);

	return status;
}

u64 physical_memory_size(void)
{
	int i;
	unsigned long long size = 0;

	for (i = 0; i < g_boot_arg->orig_dram_info.rank_num; i++) {
		size += g_boot_arg->orig_dram_info.rank_info[i].size;
	}

	return size;
}

u32 memory_size(void)
{
	unsigned long long size = physical_memory_size();

	while (((unsigned long long)DRAM_PHY_ADDR + size) > 0x100000000ULL) {
		size -= (unsigned long long)(1024*1024*1024);
	}

	return (unsigned int)size;
}

void sw_env()
{
#ifdef LK_DL_CHECK
#ifdef MTK_EMMC_SUPPORT
	int dl_status = 0;
	dl_status = mmc_get_dl_info();
	dprintf(INFO, "mt65xx_sw_env--dl_status: %d\n", dl_status);
	if (dl_status != 0) {
		video_printf("=> TOOL DL image Fail!\n");
		dprintf(CRITICAL, "TOOL DL image Fail\n");
#ifdef LK_DL_CHECK_BLOCK_LEVEL
		dprintf(CRITICAL, "uboot is blocking by dl info\n");
		while (1) ;
#endif
	}
#endif
#endif

#ifndef MACH_FPGA_NO_DISPLAY
#ifndef USER_BUILD
	switch (g_boot_mode) {
		case META_BOOT:
			video_printf(" => META MODE\n");
			break;
		case FACTORY_BOOT:
			video_printf(" => FACTORY MODE\n");
			break;
		case RECOVERY_BOOT:
			video_printf(" => RECOVERY MODE\n");
			break;
		case SW_REBOOT:
			//video_printf(" => SW RESET\n");
			break;
		case NORMAL_BOOT:
			//if(g_boot_arg->boot_reason != BR_RTC && get_env("hibboot") != NULL && atoi(get_env("hibboot")) == 1)
			if (get_env("hibboot") != NULL && atoi(get_env("hibboot")) == 1)
				video_printf(" => HIBERNATION BOOT\n");
			else
				video_printf(" => NORMAL BOOT\n");
			break;
		case ADVMETA_BOOT:
			video_printf(" => ADVANCED META MODE\n");
			break;
		case ATE_FACTORY_BOOT:
			video_printf(" => ATE FACTORY MODE\n");
			break;
#ifdef MTK_KERNEL_POWER_OFF_CHARGING
		case KERNEL_POWER_OFF_CHARGING_BOOT:
			video_printf(" => POWER OFF CHARGING MODE\n");
			break;
		case LOW_POWER_OFF_CHARGING_BOOT:
			video_printf(" => LOW POWER OFF CHARGING MODE\n");
			break;
#endif
		case ALARM_BOOT:
			video_printf(" => ALARM BOOT\n");
			break;
		case FASTBOOT:
			video_printf(" => FASTBOOT mode...\n");
			break;
		default:
			video_printf(" => UNKNOWN BOOT\n");
	}
	return;
#endif

#ifdef USER_BUILD
	if (g_boot_mode == FASTBOOT)
		video_printf(" => FASTBOOT mode...\n");
	return;
#endif
#endif
}

extern uint64_t ld_tt_l1[4];
void platform_init_mmu(void)
{
	unsigned long long addr;
	unsigned int vaddr;
	unsigned long long dram_size;

	/* configure available RAM banks */
	dram_init();
	/* Long-descriptor translation with lpae enable */
	arm_mmu_lpae_init();

	struct mmu_initial_mapping *m = mmu_initial_mappings;

	for (uint i = 0; i < countof(mmu_initial_mappings); i++, m++) {
		arch_mmu_map(m->phys, m->virt, m->flags, m->size);
	}

	arch_enable_mmu();  //enable mmu after setup page table to avoid cpu prefetch which may bring on emi violation
}
#ifndef USER_BUILD
extern void disable_BATFET_counter(unsigned long ms);
#endif
void platform_early_init(void)
{
#ifdef MT_SRAM_REPAIR_SUPPORT
	int repair_ret;
#endif
#ifdef LK_PROFILING
#ifdef MT_SRAM_REPAIR_SUPPORT
	unsigned int time_repair_sram;
#endif
	unsigned int time_wdt_early_init;
	unsigned int time_pmic_init;
	unsigned int time_platform_early_init;

	time_platform_early_init = get_timer(0);
#endif

#ifdef MTK_LK_REGISTER_WDT
	lk_register_wdt_callback();
#endif

	platform_init_interrupts();

	platform_early_init_timer();

#ifndef MACH_FPGA
	mt_gpio_set_default();
#endif

	/* initialize the uart */
	uart_init_early();

	dprintf(SPEW, "bootarg_addr: 0x%x, bootarg_size: 0x%x\n", platform_get_bootarg_addr(), platform_get_bootarg_size());

	if (g_dram_init_ret < 0) {
		dprintf(CRITICAL, "[LK ERROR] DRAM bank number is not correct!!!\n");
		while (1) ;
	}

	//i2c_v1_init();

#ifdef LK_PROFILING
	time_wdt_early_init = get_timer(0);
#endif
	mtk_wdt_init();
#ifdef LK_PROFILING
	dprintf(CRITICAL, "[PROFILE] ------- WDT Init  takes %d ms -------- \n", (int)get_timer(time_wdt_early_init));
#endif

#ifdef MT_SRAM_REPAIR_SUPPORT
#ifdef LK_PROFILING
	time_repair_sram = get_timer(0);
#endif
	repair_ret = repair_sram();
	if (repair_ret != 0) {
		dprintf(CRITICAL, "Sram repair failed %d\n", repair_ret);
		while (1);
	}
#ifdef LK_PROFILING
	dprintf(CRITICAL, "[PROFILE] ------- Repair SRAM takes %d ms -------- \n", (int)get_timer(time_repair_sram));
#endif
#endif

	//i2c init
	i2c_hw_init();

#ifdef MACH_FPGA
	mtk_timer_init();  // GPT4 will be initialized at PL after
	mtk_wdt_disable();  // WDT will be triggered when uncompressing linux image on FPGA
#endif



	pwrap_init_lk();

#ifdef LK_PROFILING
	time_pmic_init = get_timer(0);
#endif
	pmic_init();
	/*
	    // Workaround by Weiqi
	    mt6331_upmu_set_rg_vgp1_en(1);
	    mt6331_upmu_set_rg_vcam_io_en(1);
	*/
#ifdef LK_PROFILING
	dprintf(CRITICAL, "[PROFILE] ------- pmic_init takes %d ms -------- \n", (int)get_timer(time_pmic_init));
#endif
#ifndef USER_BUILD
	/*reset charger /QON counter for 5ms to prevent long press reset*/
	disable_BATFET_counter(5);
#endif

#ifdef LK_PROFILING
	dprintf(CRITICAL, "[PROFILE] ------- platform_early_init takes %d ms -------- \n", (int)get_timer(time_platform_early_init));
#endif
}

extern void mt65xx_bat_init(void);
#if defined (MTK_KERNEL_POWER_OFF_CHARGING)

int kernel_charging_boot(void)
{
	if ((g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT || g_boot_mode == LOW_POWER_OFF_CHARGING_BOOT) && upmu_is_chr_det() == KAL_TRUE) {
		dprintf(INFO,"[%s] Kernel Power Off Charging with Charger/Usb \n", __func__);
		return  1;
	} else if ((g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT || g_boot_mode == LOW_POWER_OFF_CHARGING_BOOT) && upmu_is_chr_det() == KAL_FALSE) {
		dprintf(INFO,"[%s] Kernel Power Off Charging without Charger/Usb \n", __func__);
		return -1;
	} else
		return 0;
}
#endif

void disable_scp_hw(void)
{
	/* turn off SCP related module
	 *PDEF_SCP_SRAM_PDN = #0xffff
	 *PDEF_SCP_CLK_SEL = #0x1
	 *PDEF_SCP_CLK_EN = #0x0
	 *PDEF_SCP_CFGREG_SW_RSTN = #0x0
	 *PDEF_SCP_SLP_EN = #0x100
	*/
#define SCP_SRAM_PDN		0x100A402c
#define SCP_CLK_SEL		0x100A4000
#define SCP_CLK_EN		0x100A4004
#define SCP_CFGREG_SW_RSTN	0x100A0000
#define SCP_SLP_EN		0x100A4020
	*(volatile unsigned int*)(SCP_SRAM_PDN)=0xFFFF; /*turn off scp sram*/
	*(volatile unsigned int*)(SCP_CLK_SEL)=0x1;    /*turn off scp clock*/
	*(volatile unsigned int*)(SCP_CLK_EN)=0x0;
	*(volatile unsigned int*)(SCP_CFGREG_SW_RSTN)=0x0;
	*(volatile unsigned int*)(SCP_SLP_EN)=0x100;
	dprintf(CRITICAL, "DISABLE SCP: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
			*(volatile unsigned int*)(SCP_SRAM_PDN),
			*(volatile unsigned int*)(SCP_CLK_SEL),
			*(volatile unsigned int*)(SCP_CLK_EN),
			*(volatile unsigned int*)(SCP_CFGREG_SW_RSTN),
			*(volatile unsigned int*)(SCP_SLP_EN));

}

void platform_init(void)
{
#ifdef LK_PROFILING
	unsigned int time_nand_emmc;
	unsigned int time_led_init;
	unsigned int time_env;
	unsigned int time_disp_init;
	unsigned int time_load_logo;
	unsigned int time_backlight;
	unsigned int time_boot_mode;
	unsigned int time_disp_update;
#ifdef MTK_SECURITY_SW_SUPPORT
	unsigned int time_security_init;
	unsigned int time_logo_vfy;
#endif
	unsigned int time_bat_init;
	unsigned int time_RTC_boot_Check;
	unsigned int time_show_logo;
	unsigned int time_sw_env;
	unsigned int time_scp;
	unsigned int time_platform_init;

	time_platform_init = get_timer(0);
#endif
	u64 pl_start_addr = 0;
	plinfo_get_brom_header_block_size(&pl_start_addr);

	dprintf(CRITICAL, "platform_init()\n");

#ifdef DUMMY_AP
	dummy_ap_entry();
#endif

#ifdef LK_PROFILING
	time_nand_emmc = get_timer(0);
#endif
#ifdef MTK_EMMC_SUPPORT
	mmc_legacy_init(1);
#else
#ifndef MACH_FPGA
	nand_init();
	nand_driver_test();
#endif
#endif
#ifdef LK_PROFILING
	dprintf(CRITICAL, "[PROFILE] ------- NAND/EMMC init takes %d ms -------- \n", (int)get_timer(time_nand_emmc));
#endif
#if defined(CFG_DTB_EARLY_LOADER_SUPPORT)
	if (bldr_load_dtb("boot")<0)
		dprintf(CRITICAL, "bldr_load_dtb fail\n");
#endif
#ifndef MACH_FPGA
#ifdef LK_PROFILING
	time_led_init = get_timer(0);
#endif
	leds_init();
#ifdef LK_PROFILING
	dprintf(CRITICAL, "[PROFILE] ------- led init takes %d ms -------- \n", (int)get_timer(time_led_init));
#endif
#endif
/*guohuajun add */
#if 1   
	if(kernel_charging_boot() != 1 && g_boot_mode != ALARM_BOOT && 
		g_boot_mode != KERNEL_POWER_OFF_CHARGING_BOOT && g_boot_mode != LOW_POWER_OFF_CHARGING_BOOT){ 
			       pmic_set_register_value(PMIC_RG_VIBR_VOSEL,0x5); 
			       pmic_set_register_value(PMIC_RG_VIBR_EN,1); 
			       mdelay(200); 
			       pmic_set_register_value(PMIC_RG_VIBR_EN,0); 
	} 
#endif
#ifdef MTK_KERNEL_POWER_OFF_CHARGING
	if ((g_boot_arg->boot_reason == BR_USB) && (upmu_is_chr_det() == KAL_FALSE)) {
		dprintf(INFO, "[%s] Unplugged Charger/Usb between Pre-loader and Uboot in Kernel Charging Mode, Power Off \n", __func__);
		mt6575_power_off();
	}
#endif

#ifdef LK_PROFILING
	time_env = get_timer(0);
#endif
	env_init();
	print_env();
#ifdef MTK_USB2JTAG_SUPPORT
	extern void usb2jtag_init(void);
	usb2jtag_init();
#endif

#ifdef LK_PROFILING
	dprintf(CRITICAL, "[PROFILE] ------- ENV init takes %d ms -------- \n", (int)get_timer(time_env));
#endif

#ifdef LK_PROFILING
	time_disp_init = get_timer(0);
#endif

	/* initialize the frame buffet information */
	g_fb_size = mt_disp_get_vram_size();
#if 0
	g_fb_base = memory_size() - g_fb_size + DRAM_PHY_ADDR;
#else

#if 0
	if (g_is_64bit_kernel) {
		g_fb_base = mblock_reserve(&g_boot_arg->mblock_info, g_fb_size, 0x200000, 0x100000000, RANKMAX);
		g_fb_base = ALIGN_TO(g_fb_base,0x200000); // size 2MB align
	} else {
		g_fb_base = mblock_reserve(&g_boot_arg->mblock_info, g_fb_size, 0x100000, 0x100000000, RANKMAX);
	}
#else
	g_fb_base = mblock_reserve(&g_boot_arg->mblock_info, g_fb_size, 0x10000, 0xC0000000, RANKMAX);
#endif

	if (!g_fb_base) {
		/* ERROR */
	}
#endif

	dprintf(CRITICAL, "FB base = 0x%x, FB size = 0x%x (%d)\n", g_fb_base, g_fb_size, g_fb_size);

#ifndef MACH_FPGA_NO_DISPLAY
	mt_disp_init((void *)g_fb_base);
	/* show black picture fisrtly in case of  backlight is on before nothing is drawed*/
	mt_disp_fill_rect(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT, 0x0);
	mt_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
#ifdef LK_PROFILING
	dprintf(CRITICAL, "[PROFILE] ------- disp init takes %d ms -------- \n", (int)get_timer(time_disp_init));
#endif
	drv_video_init();
#endif

	/*for kpd pmic mode setting*/
	set_kpd_pmic_mode();

#ifndef MACH_FPGA
#ifdef LK_PROFILING
	time_boot_mode = get_timer(0);
#endif
	boot_mode_select();
#ifdef CFG_DTB_EARLY_LOADER_SUPPORT
	/* reload dtb when boot mode = recovery */
	if ((g_boot_mode == RECOVERY_BOOT) && (bldr_load_dtb("recovery") < 0))
		dprintf(CRITICAL, "bldr_load_dtb fail\n");
#endif
#ifdef LK_PROFILING
	dprintf(CRITICAL, "[PROFILE] ------- boot mode select takes %d ms -------- \n", (int)get_timer(time_boot_mode));
#endif
#endif
#ifndef MACH_FPGA_NO_DISPLAY
#ifdef LK_PROFILING
	time_load_logo = get_timer(0);
#endif
	mboot_common_load_logo((unsigned long)mt_get_logo_db_addr_pa(), "logo");
#ifdef LK_PROFILING
	dprintf(CRITICAL, "[PROFILE] ------- load_logo takes %d ms -------- \n", (int)get_timer(time_load_logo));
#endif
#endif

#ifdef MTK_SECURITY_SW_SUPPORT
#ifdef LK_PROFILING
	time_security_init = get_timer(0);
#endif

	/* initialize security library */
	sec_func_init(pl_start_addr);

#ifdef LK_PROFILING
	dprintf(CRITICAL,"[PROFILE] ------- Security init takes %d ms -------- \n", (int)get_timer(time_security_init));
#endif

#ifdef LK_PROFILING
	time_logo_vfy = get_timer(0);
#endif
	seclib_set_oemkey(g_oemkey, OEM_PUBK_SZ);
	/*Verify logo before use it*/
	if ( 0 != sec_logo_check() ) {
		dprintf(CRITICAL,"<ASSERT> %s:line %d\n",__FILE__,__LINE__);
		while (1);
	}
#ifdef LK_PROFILING
	dprintf(CRITICAL,"[PROFILE] ------- logo verify takes %d ms -------- \n", (int)get_timer(time_logo_vfy));
#endif
#endif



	/*Show download logo & message on screen */
	if (g_boot_arg->boot_mode == DOWNLOAD_BOOT) {
		dprintf(CRITICAL, "[LK] boot mode is DOWNLOAD_BOOT\n");

#ifndef MACH_FPGA_NO_DISPLAY
		mt_disp_show_boot_logo();
#endif
		video_printf(" => Downloading...\n");
		dprintf(CRITICAL, "enable backlight after show bootlogo! \n");
		mt65xx_backlight_on();

		mtk_wdt_disable(); //Disable wdt before jump to DA
		platform_uninit();
#ifdef HAVE_CACHE_PL310
		l2_disable();
#endif
		arch_disable_cache(UCACHE);
		arch_disable_mmu();
#ifdef ENABLE_L2_SHARING
		config_shared_SRAM_size();
#endif

		jump_da(g_boot_arg->da_info.addr, g_boot_arg->da_info.arg1, g_boot_arg->da_info.arg2);
	}

#ifdef LK_PROFILING
	time_bat_init = get_timer(0);
#endif
	mt_pll_turn_off();
	mt_dcm_init();
	md1_power_down();
	mt65xx_bat_init();
#ifdef LK_PROFILING
	dprintf(CRITICAL, "[PROFILE] ------- battery init takes %d ms -------- \n", (int)get_timer(time_bat_init));
#endif

#ifndef CFG_POWER_CHARGING
#ifdef LK_PROFILING
	time_RTC_boot_Check = get_timer(0);
#endif
	/* NOTE: if define CFG_POWER_CHARGING, will rtc_boot_check() in mt65xx_bat_init() */
	rtc_boot_check(false);
#ifdef LK_PROFILING
	dprintf(CRITICAL, "[PROFILE] ------- RTC boot check Init  takes %d ms -------- \n", (int)get_timer(time_RTC_boot_Check));
#endif
#endif

#ifdef LK_PROFILING
	time_show_logo = get_timer(0);
#endif
#ifdef MTK_KERNEL_POWER_OFF_CHARGING
	if (kernel_charging_boot() == 1) {
#ifdef MTK_BATLOWV_NO_PANEL_ON_EARLY
		CHARGER_TYPE CHR_Type_num = CHARGER_UNKNOWN;
		CHR_Type_num = hw_charging_get_charger_type();
		if ((g_boot_mode != LOW_POWER_OFF_CHARGING_BOOT) ||
		        ((CHR_Type_num != STANDARD_HOST) && (CHR_Type_num != NONSTANDARD_CHARGER))) {
#endif
			mt_disp_power(TRUE);
#ifndef MACH_FPGA_NO_DISPLAY
			mt_disp_show_low_battery();
#endif
			mt65xx_leds_brightness_set(6, 110);
#ifdef MTK_BATLOWV_NO_PANEL_ON_EARLY
		}
#endif
	} else if (g_boot_mode != KERNEL_POWER_OFF_CHARGING_BOOT && g_boot_mode != LOW_POWER_OFF_CHARGING_BOOT) {
		if (g_boot_mode != ALARM_BOOT && (g_boot_mode != FASTBOOT)) {
#ifndef MACH_FPGA_NO_DISPLAY
			mt_disp_show_boot_logo();
#endif
		}
	}
#else
	if (g_boot_mode != ALARM_BOOT && (g_boot_mode != FASTBOOT)) {
#ifndef MACH_FPGA_NO_DISPLAY
		mt_disp_show_boot_logo();
#endif
	}
#endif

#ifdef LK_PROFILING
	dprintf(CRITICAL, "[PROFILE] ------- show logo takes %d ms -------- \n", (int)get_timer(time_show_logo));
#endif

#ifdef MTK_BATLOWV_NO_PANEL_ON_EARLY
	if (!is_low_battery(0)) {
#endif
#ifdef LK_PROFILING
		time_backlight = get_timer(0);
#endif
		mt65xx_backlight_on();
#ifdef LK_PROFILING
		dprintf(CRITICAL, "[PROFILE] ------- backlight takes %d ms -------- \n", (int)get_timer(time_backlight));
#endif

		//pwm need display sof
#ifndef MACH_FPGA_NO_DISPLAY
#ifdef LK_PROFILING
		time_disp_update = get_timer(0);
#endif
		mt_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
#ifdef LK_PROFILING
		dprintf(CRITICAL, "[PROFILE] ------- disp_update takes %d ms -------- \n", (int)get_timer(time_disp_update));
#endif
#endif
#ifdef MTK_BATLOWV_NO_PANEL_ON_EARLY
	}
#endif

#ifndef MACH_FPGA
#ifdef LK_PROFILING
	time_sw_env = get_timer(0);
#endif
	sw_env();
#ifdef LK_PROFILING
	dprintf(CRITICAL, "[PROFILE] ------- sw_env takes %d ms -------- \n", (int)get_timer(time_sw_env));
#endif

#ifdef MTK_TINYSYS_SCP_SUPPORT
#ifdef LK_PROFILING
	time_scp = get_timer(0);
#endif

	if (load_scp()<0){
		/* if it load scp fail, turn off SCP hw*/
		disable_scp_hw();
	}

#ifdef LK_PROFILING
	dprintf(CRITICAL, "[PROFILE] ------- load scp takes %d ms -------- \n", (int)get_timer(time_scp));
#endif
#else
	/* if scp is not built in, turn off SCP hw*/
	disable_scp_hw();
#endif

#endif

#ifdef LK_PROFILING
	dprintf(CRITICAL, "[PROFILE] ------- platform_init takes %d ms -------- \n", (int)get_timer(time_platform_init));
#endif
}

void platform_uninit(void)
{
#ifndef MACH_FPGA
	leds_deinit();
#endif
	platform_deinit_interrupts();

#ifdef MTK_SECURITY_SW_SUPPORT
#ifdef MTK_TINYSYS_SCP_SUPPORT
	scp_jtag_sync();
#endif
#endif
	return;
}

#ifdef ENABLE_L2_SHARING
#define ADDR_CA7L_CACHE_CONFIG_MP(x) (CA7MCUCFG_BASE + 0x200 * x)
#define L2C_SIZE_CFG_OFFSET  8
#define L2C_SHARE_EN_OFFSET  12
/* 4'b1111: 2048KB(not support)
 * 4'b0111: 1024KB(not support)
 * 4'b0011: 512KB
 * 4'b0001: 256KB
 * 4'b0000: 128KB (not support)
 */

int is_l2_need_config(void)
{
	volatile unsigned int cache_cfg, addr;

	addr = ADDR_CA7L_CACHE_CONFIG_MP(0);
	cache_cfg = DRV_Reg32(addr);
	cache_cfg = cache_cfg >> L2C_SIZE_CFG_OFFSET;

	/* only read 256KB need to be config.*/
	if ((cache_cfg &(0x7)) == 0x1) {
		return 1;
	}
	return 0;
}

void cluster_l2_share_enable(int cluster)
{
	volatile unsigned int cache_cfg, addr;

	addr = ADDR_CA7L_CACHE_CONFIG_MP(cluster);
	/* set L2C size to 256KB */
	cache_cfg = DRV_Reg32(addr);
	cache_cfg &= (~0x7) << L2C_SIZE_CFG_OFFSET;
	cache_cfg |= 0x1 << L2C_SIZE_CFG_OFFSET;

	/* enable L2C_share_en. Sram only for other to use*/
	cache_cfg |= (0x1 << L2C_SHARE_EN_OFFSET);
	DRV_WriteReg32(addr, cache_cfg);
}

void cluster_l2_share_disable(int cluster)
{
	volatile unsigned int cache_cfg, addr;

	addr = ADDR_CA7L_CACHE_CONFIG_MP(cluster);
	/* set L2C size to 512KB */
	cache_cfg = DRV_Reg32(addr);
	cache_cfg &= (~0x7) << L2C_SIZE_CFG_OFFSET;
	cache_cfg |= 0x3 << L2C_SIZE_CFG_OFFSET;
	DRV_WriteReg32(addr, cache_cfg);

	/* disable L2C_share_en. Sram only for cpu to use*/
	cache_cfg &= ~(0x1 << L2C_SHARE_EN_OFFSET);
	DRV_WriteReg32(addr, cache_cfg);
}

/* config L2 cache and sram to its size */
void config_L2_size(void)
{
	int cluster;

	if (is_l2_need_config()) {
		/*
		 * Becuase mcu config is protected.
		 * only can write in secutity mode
		 */

		if (dev_info_nr_cpu() == 6) {
			cluster_l2_share_disable(0);
			cluster_l2_share_enable(1);
		}

		else {
			for (cluster = 0; cluster < 2; cluster++) {
				cluster_l2_share_disable(cluster);
			}
		}
	}
}

/* config SRAM back from L2 cache for DA relocation */
void config_shared_SRAM_size(void)
{
	int cluster;

	if (is_l2_need_config()) {
		/*
		 * Becuase mcu config is protected.
		 * only can write in secutity mode
		 */

		for (cluster = 0; cluster < 2; cluster++) {
			cluster_l2_share_enable(cluster);
		}
	}
}
#endif


void platform_sec_post_init(void)
{
	/* Lock Device APC in LK */
	dprintf(CRITICAL, "[DEVAPC] sec_post_init\n");

#if 1
	dprintf(CRITICAL, "[DEVAPC] platform_sec_post_init - SMC call to ATF from LK\n");
	mt_secure_call(0x82000101, 0, 0, 0);
#endif

}

u32 get_devinfo_with_index(u32 index)
{
#ifdef MTK_FORCE_N_MODE
	u32 sc = internal_get_devinfo_with_index(22);
#endif

#ifdef MTK_FORCE_MODEL
	if (index == 22) {
		return MTK_FORCE_MODEL;
	} else if (index == 15) {
		return 0x1A8D;
	}
#endif

#ifdef MTK_PDTEST_MODE
        if (index == 22) {
                return MTK_PDTEST_MODE;
        } else if (index == 15) {
                return 0x1A8D;
        }
#endif

#ifdef MTK_FORCE_L_MODE
	if (index == 22) {
		return MTK_FORCE_L_MODE;
	} else if (index == 15) {
		return 0x1A8D;
	}
#endif

#ifdef MTK_FORCE_N_MODE
	if (index == 22) {
		if ((sc == 0x1) || (sc == 0x6)) {
			return MTK_FORCE_N_MODE;
		}
	}
#endif

	return internal_get_devinfo_with_index(index);
}

int platform_skip_hibernation(void)
{
	switch (g_boot_arg->boot_reason) {
#if 0 // let schedule power on to go hiberantion bootup process
		case BR_RTC:
#endif
		case BR_WDT:
		case BR_WDT_BY_PASS_PWK:
		case BR_WDT_SW:
		case BR_WDT_HW:
			return 1;
	}

	return 0;
}
