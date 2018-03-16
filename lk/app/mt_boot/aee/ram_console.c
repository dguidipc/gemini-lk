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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arch/arm/mmu.h>
#include <platform/ram_console.h>

#define RAM_CONSOLE_SIG (0x43474244) /* DBGC */
#define MOD "RAM_CONSOLE"
typedef unsigned int u32;

struct ram_console_buffer {
	u32 sig;
	/* for size comptible */
	u32 off_pl;
	u32 off_lpl; /* last preloader */
	u32 sz_pl;
	u32 off_lk;
	u32 off_llk; /* last lk */
	u32 sz_lk;
	u32 padding[4]; /* size = 4 * 16 = 64 byte */
	u32 off_linux;
	u32 off_console;
	u32 padding2[3];
};

#define RAM_CONSOLE_LK_SIZE 16
struct reboot_reason_lk {
	u32 last_func[RAM_CONSOLE_LK_SIZE];
};

struct reboot_reason_pl {
	u32 wdt_status;
	u32 last_func[0];
};

struct reboot_reason_kernel {
	u32 fiq_step;
	u32 exp_type; /* 0xaeedeadX: X=1 (HWT), X=2 (KE), X=3 (nested panic) */
	u32 others[0];
};

extern int sLOG(char *fmt, ...);
#define LOG(fmt, ...)           \
    sLOG(fmt, ##__VA_ARGS__);       \
    printf(fmt, ##__VA_ARGS__)

static struct ram_console_buffer *ram_console = NULL;
static int ram_console_size;

#define ALIGN(x, size) ((x + size - 1) & ~(size - 1))
static void ram_console_ptr_init(void)
{
	if (((struct ram_console_buffer *)RAM_CONSOLE_SRAM_ADDR)->sig == RAM_CONSOLE_SIG) {
		ram_console = (struct ram_console_buffer *)RAM_CONSOLE_SRAM_ADDR;
		ram_console_size = RAM_CONSOLE_SRAM_SIZE;
	} else {
		LOG("%s. sram(0x%x) sig %x mismatch\n", MOD, RAM_CONSOLE_SRAM_ADDR, ((struct ram_console_buffer*)RAM_CONSOLE_SRAM_ADDR)->sig);
		ram_console = (struct ram_console_buffer *)RAM_CONSOLE_DRAM_ADDR;
		ram_console_size = RAM_CONSOLE_DRAM_SIZE;
#ifdef MTK_3LEVEL_PAGETABLE
	{
		uint32_t start = ram_console;
		/* ram console: MRDUMP will call ram_console init (when abnormal boot) before heap initial, we must allocate it first */
		arch_mmu_map(ROUNDDOWN((uint64_t)start, SECTION_SIZE), ROUNDDOWN((uint32_t)start, SECTION_SIZE),
		             MMU_MEMORY_TYPE_NORMAL_WRITE_BACK | MMU_MEMORY_AP_P_RW_U_NA, ROUNDUP(ram_console_size, SECTION_SIZE));
	}
#endif
	}
}
void ram_console_init(void)
{
	int i;
	struct reboot_reason_lk *rr_lk;
	ram_console_ptr_init();
	if (ram_console) {
		LOG("%s. start: 0x%x, size: 0x%x\n", MOD, ram_console, ram_console_size);
	} else {
		LOG("%s. sig not match\n", MOD);
		return;
	}

	ram_console->off_lk = ram_console->off_lpl + ALIGN(ram_console->sz_pl, 64);
	ram_console->off_llk = ram_console->off_lk + ALIGN(sizeof(struct reboot_reason_lk), 64);
	if (ram_console->sz_lk == sizeof(struct reboot_reason_lk) && (ram_console->off_lk + ram_console->sz_lk == ram_console->off_llk)) {
		LOG("%s. lk last status: ", MOD);
		rr_lk = (void*)ram_console + ram_console->off_lk;
		for (i = 0; i < RAM_CONSOLE_LK_SIZE; i++) {
			LOG("0x%x ", rr_lk->last_func[i]);
		}
		LOG("\n");
		memcpy((void*)ram_console + ram_console->off_llk, (void*)ram_console + ram_console->off_lk, ram_console->sz_lk);
	} else {
		LOG("%s. lk size mismatch %x + %x != %x\n", MOD, ram_console->sz_lk, ram_console->off_lk, ram_console->off_llk);
		ram_console->sz_lk = sizeof(struct reboot_reason_lk);
	}
	ram_console->off_linux = ram_console->off_llk + ALIGN(ram_console->sz_lk, 64);
}


#define RE_BOOT_BY_WDT_SW 2
#define RE_BOOT_NORMAL_BOOT 0
#define RE_BOOT_BY_EINT 256/*we can find the definition from preloader ,this value should  sync with preloader incase issue happened*/

#ifdef MTK_PMIC_FULL_RESET
#define RE_BOOT_FULL_PMIC 0x800
#endif

bool ram_console_reboot_by_mrdump_key(void)
{
	unsigned int wdt_status;
	wdt_status = ((struct reboot_reason_pl*)((void*)ram_console + ram_console->off_pl))->wdt_status;
	return (wdt_status & RE_BOOT_BY_EINT)?true:false;
}

#ifdef MTK_PMIC_FULL_RESET
bool ram_console_reboot_by_cold_reset(void)
{
	unsigned int wdt_status;
	wdt_status = ((struct reboot_reason_pl*)((void*)ram_console + ram_console->off_pl))->wdt_status;
	return (wdt_status & RE_BOOT_FULL_PMIC) ? true : false;
}
#endif

bool ram_console_is_abnormal_boot(void)
{
	unsigned int fiq_step, wdt_status, exp_type;

	if (!ram_console) {
		ram_console_ptr_init();
	}
	if (ram_console && ram_console->off_linux && ram_console->off_linux < ram_console_size && ram_console->off_pl < ram_console_size) {
		wdt_status = ((struct reboot_reason_pl*)((void*)ram_console + ram_console->off_pl))->wdt_status;
		fiq_step = ((struct reboot_reason_kernel*)((void*)ram_console + ram_console->off_linux))->fiq_step;
		exp_type = ((struct reboot_reason_kernel*)((void*)ram_console + ram_console->off_linux))->exp_type;
		LOG("%s. wdt_status 0x%x, fiq_step 0x%x, exp_type 0x%x\n", MOD, wdt_status, fiq_step, (exp_type ^ 0xaeedead0) < 16 ? exp_type ^ 0xaeedead0 : exp_type);
		if (fiq_step != 0 && (exp_type ^ 0xaeedead0) >= 16) {
			fiq_step = 0;
			((struct reboot_reason_kernel*)((void*)ram_console + ram_console->off_linux))->fiq_step = fiq_step;
		}
#ifdef MTK_PMIC_FULL_RESET
		if ((fiq_step == 0 && (wdt_status == RE_BOOT_BY_WDT_SW || wdt_status == RE_BOOT_FULL_PMIC)) || (wdt_status == RE_BOOT_NORMAL_BOOT))
#else
		if ((fiq_step == 0 && wdt_status == RE_BOOT_BY_WDT_SW) || (wdt_status == RE_BOOT_NORMAL_BOOT))
#endif
			return false;
		else
			return true;
	}
	return false;
}

void ram_console_lk_save(unsigned int val, int index)
{
	struct reboot_reason_lk *rr_lk;
	if (ram_console && ram_console->off_lk < ram_console_size) {
		rr_lk = (void*)ram_console + ram_console->off_lk;
		if (index < RAM_CONSOLE_LK_SIZE)
			rr_lk->last_func[index] = val;
	}
}

void ram_console_addr_size(unsigned long *addr, unsigned long *size)
{
	*addr = (unsigned long)ram_console;
	*size = ram_console_size;
}
