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
#include <reg.h>
#include <debug.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <platform.h>
#include <platform/mt_typedefs.h>
#include <platform/boot_mode.h>
#include <platform/mt_reg_base.h>
#include <platform/sec_devinfo.h>
#include <platform/env.h>
#include <platform/sec_export.h>
//#include <dfo_boot_default.h>
extern int g_nr_bank;
extern BOOT_ARGUMENT *g_boot_arg;
extern BI_DRAM bi_dram[MAX_NR_BANK];
extern int get_meta_port_id(void);


struct tag_header {
	u32 size;
	u32 tag;
};

#define tag_size(type)  ((sizeof(struct tag_header) + sizeof(struct type)) >> 2)

#define SIZE_1M             (1024 * 1024)
#define SIZE_2M             (2 * SIZE_1M)
#define SIZE_256M           (256 * SIZE_1M)
#define SIZE_512M           (512 * SIZE_1M)

/* The list must start with an ATAG_CORE node */
#define ATAG_CORE   0x54410001
struct tag_core {
	u32 flags;      /* bit 0 = read-only */
	u32 pagesize;
	u32 rootdev;
};

/* it is allowed to have multiple ATAG_MEM nodes */
#define ATAG_MEM  0x54410002
typedef struct {
	uint32_t size;
	uint32_t start_addr;
} mem_info;

#define ATAG_MEM64  0x54420002
typedef struct {
	uint64_t size;
	uint64_t start_addr;
} mem64_info;

#define ATAG_EXT_MEM64  0x54430002
typedef struct {
	uint64_t size;
	uint64_t start_addr;
} ext_mem64_info;

/* command line: \0 terminated string */
#define ATAG_CMDLINE    0x54410009
struct tag_cmdline {
	char    cmdline[1]; /* this is the minimum size */
};

/* describes where the compressed ramdisk image lives (physical address) */
#define ATAG_INITRD2    0x54420005
struct tag_initrd {
	u32 start;  /* physical start address */
	u32 size;   /* size of compressed ramdisk image in bytes */
};

#define ATAG_VIDEOLFB   0x54410008
struct tag_videolfb {
	u64 fb_base;
	u32 islcmfound;
	u32 fps;
	u32 vram;
	char lcmname[1]; /* this is the minimum size */
};

/* boot information */
#define ATAG_BOOT   0x41000802
struct tag_boot {
	u32 bootmode;
};

/*META com port information*/
#define ATAG_META_COM 0x41000803
struct tag_meta_com {
	u32 meta_com_type; /* identify meta via uart or usb */
	u32 meta_com_id;  /* multiple meta need to know com port id */
	u32 meta_uart_port; /* identify meta uart port */
};

/*device information*/
#define ATAG_DEVINFO_DATA         0x41000804
#define ATAG_DEVINFO_DATA_SIZE    100
struct tag_devinfo_data {
	u32 devinfo_data[ATAG_DEVINFO_DATA_SIZE];
	u32 devinfo_data_size;
};

#define ATAG_MDINFO_DATA 0x41000806
struct tag_mdinfo_data {
	u8 md_type[4];
};

/* extbuck fan53526 information */
#define ATAG_EXTBUCK_FAN53526 0x41000807
struct tag_extbuck_fan53526 {
	u32 exist;
};

#define ATAG_VCORE_DVFS_INFO 0x54410007
struct tag_vcore_dvfs_info {
	u32 pllgrpreg_size;
	u32 freqreg_size;
	u32* low_freq_pll_val;
	u32* low_freq_cha_val;
	u32* low_freq_chb_val;
	u32* high_freq_pll_val;
	u32* high_freq_cha_val;
	u32* high_freq_chb_val;
};

#define ATAG_PTP_INFO 0x54410008
struct tag_ptp_info {
	u32 first_volt;
	u32 second_volt;
	u32 third_volt;
	u32 have_550;
};


/* The list ends with an ATAG_NONE node. */
#define ATAG_NONE   0x00000000

unsigned *target_atag_nand_data(unsigned *ptr)
{
	return ptr;
}


unsigned *target_atag_partition_data(unsigned *ptr)
{
	return ptr;
}

unsigned *target_atag_boot(unsigned *ptr)
{
	*ptr++ = tag_size(tag_boot);
	*ptr++ = ATAG_BOOT;
	*ptr++ = g_boot_mode;

	return ptr;
}


extern int imix_r;
unsigned *target_atag_imix_r(unsigned *ptr)
{
//    *ptr++ = tag_size(tag_imix_r);
//    *ptr++ = ATAG_IMIX;
	*ptr++ = imix_r;

	dprintf(CRITICAL, "target_atag_imix_r:%d\n",imix_r);
	return ptr;
}

unsigned *target_atag_devinfo_data(unsigned *ptr)
{
	int i = 0;
	*ptr++ = tag_size(tag_devinfo_data);
	*ptr++ = ATAG_DEVINFO_DATA;
	for (i=0; i<ATAG_DEVINFO_DATA_SIZE; i++) {
		*ptr++ = get_devinfo_with_index(i);
	}
	*ptr++ = ATAG_DEVINFO_DATA_SIZE;

	dprintf(INFO, "SSSS:0x%x\n", get_devinfo_with_index(1));
	dprintf(INFO, "SSSS:0x%x\n", get_devinfo_with_index(2));
	dprintf(INFO, "SSSS:0x%x\n", get_devinfo_with_index(3));
	dprintf(INFO, "SSSS:0x%x\n", get_devinfo_with_index(4));
	dprintf(INFO, "SSSS:0x%x\n", get_devinfo_with_index(20));
	dprintf(INFO, "SSSS:0x%x\n", get_devinfo_with_index(21));

	return ptr;
}

unsigned *target_atag_masp_data(unsigned *ptr)
{
	#ifdef MTK_SECURITY_SW_SUPPORT
	/*tag size*/
	*ptr++ = tag_size(tag_masp_data);
	/*tag name*/
	*ptr++ = ATAG_MASP_DATA;
	ptr = fill_atag_masp_data(ptr);
    #endif

	return ptr;
}

unsigned *target_atag_mem(unsigned *ptr)
{
	int i;

	for (i = 0; i < g_nr_bank; i++) {
#ifndef MTK_LM_MODE
		*ptr++ = 4; //tag size
		*ptr++ = ATAG_MEM; //tag name
		*ptr++ = bi_dram[i].size;
		*ptr++ = bi_dram[i].start;
#else
		*ptr++ = 6; //tag size
		*ptr++ = ATAG_MEM64; //tag name
		//*((unsigned long long*)ptr)++ = bi_dram[i].size;
		//*((unsigned long long*)ptr)++ = bi_dram[i].start;
		unsigned long long *ptr64 = (unsigned long long *)ptr;
		*ptr64++ = bi_dram[i].size;
		*ptr64++ = bi_dram[i].start;
		ptr = (unsigned int *)ptr64;
#endif
	}
	return ptr;
}

unsigned *target_atag_meta(unsigned *ptr)
{
	*ptr++ = tag_size(tag_meta_com);
	*ptr++ = ATAG_META_COM;
	*ptr++ = g_boot_arg->meta_com_type;
	*ptr++ = g_boot_arg->meta_com_id;
	*ptr++ = get_meta_port_id();
	dprintf(CRITICAL, "meta com type = %d\n", g_boot_arg->meta_com_type);
	dprintf(CRITICAL, "meta com id = %d\n", g_boot_arg->meta_com_id);
	dprintf(CRITICAL, "meta uart port = %d\n", get_meta_port_id());
	return ptr;
}

/* todo: give lk strtoul and nuke this */
static unsigned hex2unsigned(const char *x)
{
	unsigned n = 0;

	while (*x) {
		switch (*x) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				n = (n << 4) | (*x - '0');
				break;
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
				n = (n << 4) | (*x - 'a' + 10);
				break;
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
				n = (n << 4) | (*x - 'A' + 10);
				break;
			default:
				return n;
		}
		x++;
	}

	return n;
}
#if 0
#define ATAG_DFO_DATA 0x41000805
unsigned *target_atag_dfo(unsigned *ptr)
{
	int i, j;
	dfo_boot_info *dfo_p;
	char tmp[11];
	unsigned  char *buffer;

	*ptr++ = ((sizeof(struct tag_header) + DFO_BOOT_COUNT * sizeof(dfo_boot_info)) >> 2);
	*ptr++ = ATAG_DFO_DATA;

	memcpy((void *)ptr, (void *)dfo_boot_default, DFO_BOOT_COUNT * sizeof(dfo_boot_info));
	dfo_p = (dfo_boot_info *)ptr;

	ptr += DFO_BOOT_COUNT * sizeof(dfo_boot_info) >> 2;

	buffer = (unsigned char *)get_env("DFO");

	if (buffer != NULL) {

		for (i = 0; i < DFO_BOOT_COUNT; i++) {
			j = 0;
			do {
				dfo_p[i].name[j] = *buffer;
				j++;
			} while (*buffer++ != ',' && j < 31);

			dfo_p[i].name[j-1] = '\0';
			j = 0;

			do {
				tmp[j] = *buffer;
				j++;
			} while (*buffer++ != ',' && j < 10);

			tmp[j] = '\0';

			if ((strncmp("0x", tmp, 2) == 0) || (strncmp("0X", tmp, 2) == 0))
				dfo_p[i].value = hex2unsigned(&tmp[2]);
			else
				dfo_p[i].value = atoi(tmp);
		}

		for (i = 0; i < DFO_BOOT_COUNT; i++)
			dprintf(INFO, "[DFO-%d] NAME:%s, Value:%lu\n",i , dfo_p[i].name, dfo_p[i].value);

	} else
		dprintf(INFO, "No DFO. Use default values.\n");

	return ptr;
}
#endif

unsigned *target_atag_commmandline(unsigned *ptr, char *commandline)
{
	char *p;

	if (!commandline)
		return NULL;

	for (p = commandline; *p == ' '; p++);

	if (*p == '\0')
		return NULL;

	*ptr++ = (sizeof (struct tag_header) + strlen (p) + 1 + 4) >> 2;; //size
	*ptr++ = ATAG_CMDLINE;
	strcpy((char *)ptr, p);  //copy to atags memory region
	ptr += (strlen (p) + 1 + 4) >> 2;
	return ptr;
}

unsigned *target_atag_initrd(unsigned *ptr, ulong initrd_start, ulong initrd_size)
{
	*ptr++ = tag_size(tag_initrd);
	*ptr++ = ATAG_INITRD2;
//TMP for bring up testing
//  *ptr++ = CFG_RAMDISK_LOAD_ADDR;
	// *ptr++ = 0x1072F9;

	*ptr++ = initrd_start;
	*ptr++ = initrd_size;
	return ptr;
}

#include <platform/mt_disp_drv.h>
#include <platform/disp_drv.h>
unsigned *target_atag_videolfb(unsigned *ptr)
{
	extern unsigned long long fb_addr_pa_k;
	const char   *lcmname = mt_disp_get_lcm_id();
	unsigned int *p       = NULL;
	unsigned long long *phy_p   = (unsigned long long *)ptr;
	u32 fps = mt_disp_get_lcd_time();

	*phy_p = fb_addr_pa_k;
	p = (unsigned int*)(phy_p + 1);
	*p++ = DISP_IsLcmFound();
	*p++ = fps;
	*p++ = DISP_GetVRamSize();
	strcpy((char *)p,lcmname);
	p += (strlen(lcmname) + 1 + 4) >> 2;

	dprintf(CRITICAL, "videolfb - fb_base    = 0x%llx\n",fb_addr_pa_k);
	dprintf(CRITICAL, "videolfb - islcmfound = %d\n",DISP_IsLcmFound());
	dprintf(CRITICAL, "videolfb - fps        = %d\n",mt_disp_get_lcd_time());
	dprintf(CRITICAL, "videolfb - vram       = %d\n",DISP_GetVRamSize());
	dprintf(CRITICAL, "videolfb - lcmname    = %s\n",lcmname);

	return (unsigned *)p;
}


unsigned *target_atag_mdinfo(unsigned *ptr)
{
	unsigned char *p;
	*ptr++=tag_size(tag_mdinfo_data);
	*ptr++=ATAG_MDINFO_DATA;
	p=(unsigned char *)ptr;
	*p++=g_boot_arg->md_type[0];
	*p++=g_boot_arg->md_type[1];
	*p++=g_boot_arg->md_type[2];
	*p++=g_boot_arg->md_type[3];
	return (unsigned *)p;
}

unsigned *target_atag_isram(unsigned *ptr)
{
	unsigned char *p;
	p=(unsigned char *)ptr;
	memcpy(p, (unsigned char*)&g_boot_arg->non_secure_sram_addr, sizeof(u32));
	p=p+sizeof(u32);
	memcpy(p, (unsigned char*)&g_boot_arg->non_secure_sram_size, sizeof(u32));
	p=p+sizeof(u32);
	dprintf(CRITICAL,"[LK] non_secure_sram (0x%x, 0x%x)\n", g_boot_arg->non_secure_sram_addr, g_boot_arg->non_secure_sram_size);
	return (unsigned *)p;
}

unsigned *target_atag_ptp(unsigned *ptr)
{
	ptp_info_t* ptp_info = &g_boot_arg->ptp_volt_info;
	u32 first_volt = ptp_info->first_volt;
	u32 second_volt = ptp_info->second_volt;
	u32 third_volt = ptp_info->third_volt;
	u32 have_550 = ptp_info->have_550;

	*ptr++=tag_size(tag_ptp_info);
	*ptr++=ATAG_PTP_INFO;

	*ptr++=first_volt;
	*ptr++=second_volt;
	*ptr++=third_volt;
	*ptr++=have_550;

	return ptr;
}

unsigned *target_atag_extbuck_fan53526(unsigned *ptr)
{
	*ptr++=tag_size(tag_extbuck_fan53526);
	*ptr++=ATAG_EXTBUCK_FAN53526;

	*ptr++=g_boot_arg->extbuck_fan53526_exist;
	return ptr;
}

void *target_get_scratch_address(void)
{
	return ((void *)SCRATCH_ADDR);
}


#ifdef DEVICE_TREE_SUPPORT
#include <libfdt.h>

#define NUM_CLUSTERS  3
#define NUM_CORES_CLUSTER0  4
#define NUM_CORES_CLUSTER1  4
#define NUM_CORES_CLUSTER2  2
#define MAX_CLK_FREQ  2000000000


/*
 * 0000: Qual core
 * 1000: Triple core
 * 1100: Dual core
 * 1110: Single core
 * 1111: All disable
 */
#define DEVINFO_4CPU_QUAL_CORE   0x0
#define DEVINFO_4CPU_TRIPLE_CORE 0x8
#define DEVINFO_4CPU_DUAL_CORE   0xC
#define DEVINFO_4CPU_SINGLE_CORE 0xE
#define DEVINFO_4CPU_ZERO_CORE   0xF

/*
 * 00: Dual core
 * 10: Single core
 * 11: All disable
 */
#define DEVINFO_2CPU_DUAL_CORE   0x0
#define DEVINFO_2CPU_SINGLE_CORE 0x2
#define DEVINFO_2CPU_ZERO_CORE   0x3


struct cpu_dev_info {
	unsigned int cluster0 : 4;
	unsigned int cluster1 : 4;
	unsigned int cluster2 : 2;
	unsigned int reserve  : 22;
};

struct speed_dev_info {
	unsigned int big      : 3;
	unsigned int reserve1 : 1;
	unsigned int small    : 3;
	unsigned int reserve2 : 25;
};

unsigned int dev_info_max_clk_freq(unsigned int cluster_idx)
{
	unsigned int devinfo = get_devinfo_with_index(8);
	struct speed_dev_info *info = (struct speed_dev_info *)&devinfo;
	unsigned int big, max_clk_freq, decrease;

	decrease = 0;

	big = (cluster_idx == 2) ? 1 : 0;

	if (big) {
		/*
		  000:Free
		  001:2.5 GHz
		  010:2.4 GHz
		  011:2.3 GHz
		  100:2.2 GHz
		  101:2.1 GHz
		  110:2.0 GHz
		  111:1.9 GHz
		*/
		max_clk_freq = 2500;

		if (info->big > 0)
			decrease = (info->big - 1) * 100;
	} else {
		/*
		  000:Free
		  001:2.2 GHz
		  010:2.1 GHz
		  011:2.0 GHz
		  100:1.9 GHz
		  111:1.9 GHz
		*/
		max_clk_freq = 2200;

		if (info->small > 0) {
			if (info->small >= 0x4)
				decrease = (0x4 - 1) * 100;
			else
				decrease = (info->small - 1) * 100;
		}
	}

	max_clk_freq -= decrease;
	max_clk_freq *= 1000 * 1000;

	return max_clk_freq;
}

int dev_info_nr_cpu(void)
{
	unsigned int devinfo = get_devinfo_with_index(3);
	unsigned int mp0_disable = ((get_devinfo_with_index(8) >> 7) & 0x1);
	struct cpu_dev_info *info = (struct cpu_dev_info *)&devinfo;
	int cluster[NUM_CLUSTERS];
	int cluster_idx;
	int nr_cpu = 0;

	memset(cluster, 0, sizeof(cluster));

	if (mp0_disable)
		info->cluster0 = DEVINFO_4CPU_ZERO_CORE;

	cluster[0] = info->cluster0;
	cluster[1] = info->cluster1;
	cluster[2] = info->cluster2;

	for (cluster_idx = 0; cluster_idx < (NUM_CLUSTERS - 1); cluster_idx++) {
		switch (cluster[cluster_idx]) {
			case DEVINFO_4CPU_QUAL_CORE:
				nr_cpu += 4;
				break;
			case DEVINFO_4CPU_TRIPLE_CORE:
				nr_cpu += 3;
				break;
			case DEVINFO_4CPU_DUAL_CORE:
				nr_cpu += 2;
				break;
			case DEVINFO_4CPU_SINGLE_CORE:
				nr_cpu += 1;
				break;
			case DEVINFO_4CPU_ZERO_CORE:
				break;
			default:
				nr_cpu += 4;
				break;
		}
	}

	switch (cluster[2]) {
		case DEVINFO_2CPU_DUAL_CORE:
			nr_cpu += 2;
			break;
		case DEVINFO_2CPU_SINGLE_CORE:
			nr_cpu += 1;
			break;
		case DEVINFO_2CPU_ZERO_CORE:
			break;
		default:
			nr_cpu += 2;
			break;
	}

	return nr_cpu;
}

#define PROJ_CODE   0x1A8D

int target_fdt_model(void *fdt)
{
	unsigned int segment = get_devinfo_with_index(22) & 0xF;
	unsigned int proj_code = get_devinfo_with_index(15) & 0x3FFF;
	int code, len;
	int nodeoffset;
	const struct fdt_property *prop;
	char *prop_name = "model";
	const char *model_name[] = {
		"MT6797",
		"MT6797T",
		"MT6797M",
		"MT6797",
		"MT6797X",
		NULL,
		"MT6797T",
		"MT6797M",
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		"MT6797"
	};
	const char *default_model_name = "MT6797";
	const char *str;
	int res = 0;

	ptp_info_t* ptp_info = &g_boot_arg->ptp_volt_info;
	u32 first_volt = ptp_info->first_volt;
	u32 second_volt = ptp_info->second_volt;
	u32 third_volt = ptp_info->third_volt;
	u32 have_550 = ptp_info->have_550;
	/*
	dprintf(CRITICAL, "[xxxx1][LK] first_volt = 0x%X\n", first_volt);
	dprintf(CRITICAL, "[xxxx1][LK] second_volt = 0x%X\n", second_volt);
	dprintf(CRITICAL, "[xxxx1][LK] third_volt = 0x%X\n", third_volt);
	dprintf(CRITICAL, "[xxxx1][LK] have_550 = 0x%X\n", have_550);
	*/
	/* Becuase the model is at the begin of device tree.
	 * use nodeoffset=0
	 */
	code = -1;

	if (proj_code == PROJ_CODE) {
		if (segment < (sizeof(model_name) / sizeof(*model_name)))
			code = segment;
	}


	if ((code == -1) || (model_name[code] == NULL)) {
		// unknown segment code, use default value
		str = default_model_name;
		res = -1;
	}
	else {
		str = model_name[code];
	}

	nodeoffset = 0;
	prop = fdt_get_property(fdt, nodeoffset, prop_name, &len);

	if (prop) {
		int namestroff;
		//dprintf(INFO, "prop->data=0x%8x\n", (uint32_t *)prop->data);
		fdt_setprop_string(fdt, nodeoffset, prop_name, str);
		prop = fdt_get_property(fdt, nodeoffset, prop_name, &len);
		namestroff = fdt32_to_cpu(prop->nameoff);
		dprintf(CRITICAL, "%s=%s\n", fdt_string(fdt, namestroff), (char *)prop->data);
	}
	return res;
}


int target_fdt_cpus(void *fdt)
{
	int cpus_offset, cpu_node, cpumap_offset, last_node = -1;
	int len;
	const struct fdt_property *prop;
	unsigned int *data;
	unsigned int reg, clk_freq;

	unsigned int cluster_idx;
	unsigned int core_num;

	unsigned int activated_cores[NUM_CLUSTERS] = {0};
	unsigned int available_cores[NUM_CLUSTERS] = {NUM_CORES_CLUSTER0, NUM_CORES_CLUSTER1, NUM_CORES_CLUSTER2};
	unsigned int max_clk_freq[NUM_CLUSTERS] = {0};
	unsigned int mp0_disable = ((get_devinfo_with_index(8) >> 7) & 0x1);
	unsigned int devinfo = get_devinfo_with_index(3);
	struct cpu_dev_info *info = (struct cpu_dev_info *)&devinfo;
	char path[32];

	dprintf(INFO, "info->cluster0=0x%x\n", info->cluster0);
	dprintf(INFO, "info->cluster1=0x%x\n", info->cluster1);
	dprintf(INFO, "info->cluster2=0x%x\n", info->cluster2);

	if (mp0_disable)
		info->cluster0 = DEVINFO_4CPU_ZERO_CORE;

	switch ((unsigned int)info->cluster0) {
		case DEVINFO_4CPU_QUAL_CORE:
			available_cores[0] = 4;
			break;
		case DEVINFO_4CPU_TRIPLE_CORE:
			available_cores[0] = 3;
			break;
		case DEVINFO_4CPU_DUAL_CORE:
			available_cores[0] = 2;
			break;
		case DEVINFO_4CPU_SINGLE_CORE:
			available_cores[0] = 1;
			break;
		case DEVINFO_4CPU_ZERO_CORE:
			available_cores[0] = 0;
			break;
		default:
			available_cores[0] = 4;
			break;
	}

	switch ((unsigned int)info->cluster1) {
		case DEVINFO_4CPU_QUAL_CORE:
			available_cores[1] = 4;
			break;
		case DEVINFO_4CPU_TRIPLE_CORE:
			available_cores[1] = 3;
			break;
		case DEVINFO_4CPU_DUAL_CORE:
			available_cores[1] = 2;
			break;
		case DEVINFO_4CPU_SINGLE_CORE:
			available_cores[1] = 1;
			break;
		case DEVINFO_4CPU_ZERO_CORE:
			available_cores[1] = 0;
			break;
		default:
			available_cores[1] = 4;
			break;
	}

	switch ((unsigned int)info->cluster2) {
		case DEVINFO_2CPU_DUAL_CORE:
			available_cores[2] = 2;
			break;
		case DEVINFO_2CPU_SINGLE_CORE:
			available_cores[2] = 1;
			break;
		case DEVINFO_2CPU_ZERO_CORE:
			available_cores[2] = 0;
			break;
		default:
			available_cores[2] = 2;
			break;
	}

#ifdef MTK_NO_BIGCORES
	available_cores[2] = 0;
#endif

	cpus_offset = fdt_path_offset(fdt, "/cpus");
	if (cpus_offset < 0) {
		dprintf(CRITICAL, "couldn't find /cpus\n");
		return cpus_offset;
	}

	for (cluster_idx = 0; cluster_idx < NUM_CLUSTERS; cluster_idx++)
		max_clk_freq[cluster_idx] = dev_info_max_clk_freq(cluster_idx);

	for (cpu_node = fdt_first_subnode(fdt, cpus_offset); cpu_node >= 0;
	        cpu_node = ((last_node >= 0) ? fdt_next_subnode(fdt, last_node) : fdt_first_subnode(fdt, cpus_offset))) {
		prop = fdt_get_property(fdt, cpu_node, "device_type", &len);
		if ((!prop) || (len < 4) || (strcmp(prop->data, "cpu"))) {
			last_node = cpu_node;
			continue;
		}

		prop = fdt_get_property(fdt, cpu_node, "reg", &len);

		data = (uint32_t *)prop->data;
		reg = fdt32_to_cpu(*data);

		dprintf(INFO, "reg = 0x%x\n", reg);
		core_num = reg & 0xFF;
		cluster_idx = (reg & 0xF00) >> 8;
		dprintf(INFO, "cluster_idx=%d, core_num=%d\n", cluster_idx, core_num);

		if (core_num >= available_cores[cluster_idx]) {
			dprintf(INFO, "delete: cluster = %d, core = %d\n", cluster_idx, core_num);
			snprintf(path, sizeof(path), "/cpus/cpu-map/cluster%d/core%d", cluster_idx, core_num);
			fdt_del_node(fdt, cpu_node);
			cpumap_offset = fdt_path_offset(fdt, path);

			if (cpumap_offset > 0)
				fdt_del_node(fdt, cpumap_offset);
		}
		/* ========== */
		else {
			activated_cores[cluster_idx]++;

			prop = fdt_get_property(fdt, cpu_node, "clock-frequency", &len);
			data = (uint32_t *)prop->data;
			clk_freq = fdt32_to_cpu(*data);

			dprintf(INFO, "cluster_idx=%d, core_num=%d, clock-frequency = %u => %u\n",
			        cluster_idx, core_num, clk_freq, max_clk_freq[cluster_idx]);

			if (clk_freq > max_clk_freq[cluster_idx]) {
				dprintf(INFO, "setprop: clock-frequency = %u => %u\n", clk_freq, max_clk_freq[cluster_idx]);
				fdt_setprop_cell(fdt, cpu_node, "clock-frequency", max_clk_freq[cluster_idx]);
			}

			last_node = cpu_node;
		}

		if (cluster_idx == NUM_CLUSTERS) {
			dprintf(CRITICAL, "Warning: unknown cpu type in device tree\n");
			last_node = cpu_node;
		}
	}

	for (cluster_idx = 0; cluster_idx < NUM_CLUSTERS; cluster_idx++) {
		if (activated_cores[cluster_idx] > available_cores[cluster_idx])
			dprintf(CRITICAL, "Warning: unexpected reg value in device tree\n");

		dprintf(CRITICAL, "cluster-%d: %d core\n", cluster_idx, available_cores[cluster_idx]);
	}

	return 0;
}

int target_fdt_firmware(void *fdt, char *serialno)
{
	int nodeoffset, namestroff, len;
	char *name, *value;
	const struct fdt_property *prop;

	nodeoffset = fdt_add_subnode(fdt, 0, "firmware");

	if (nodeoffset < 0) {
		dprintf(CRITICAL, "Warning: can't add firmware node in device tree\n");
		return -1;
	}

	nodeoffset = fdt_add_subnode(fdt, nodeoffset, "android");

	if (nodeoffset < 0) {
		dprintf(CRITICAL, "Warning: can't add firmware/android node in device tree\n");
		return -1;
	}

	name = "compatible";
	value = "android,firmware";
	fdt_setprop_string(fdt, nodeoffset, name, value);

	name = "hardware";
	value = PLATFORM;
	{
		char *tmp_str = value;
		for ( ; *tmp_str; tmp_str++ ) {
			*tmp_str = tolower(*tmp_str);
		}
	}
	fdt_setprop_string(fdt, nodeoffset, name, value);

	name = "serialno";
	value = serialno;
	fdt_setprop_string(fdt, nodeoffset, name, value);
	name = "mode";

	switch (g_boot_mode) {
		case META_BOOT:
		case ADVMETA_BOOT:
			value = "meta";
			break;
#if defined (MTK_KERNEL_POWER_OFF_CHARGING)
		case KERNEL_POWER_OFF_CHARGING_BOOT:
		case LOW_POWER_OFF_CHARGING_BOOT:
			value = "charger";
			break;
#endif
		case FACTORY_BOOT:
		case ATE_FACTORY_BOOT:
			value = "factory";
			break;
		case RECOVERY_BOOT:
			value = "recovery";
			break;
		default:
			value = "normal";
			break;
	}

	fdt_setprop_string(fdt, nodeoffset, name, value);

	return 0;
}


int platform_atag_append(void *fdt)
{
	char *ptr;
	int offset;
	int ret = 0;


#ifdef MTK_TINYSYS_SCP_SUPPORT
	ret = platform_fdt_scp(fdt);
#endif
	extern void get_atf_ramdump_memory(void *fdt)__attribute__((weak));
	if (get_atf_ramdump_memory){
	        get_atf_ramdump_memory(fdt);
	}else {
	        dprintf(CRITICAL,"[atf] ATF does not support ram dump\n");
	}
exit:

	if (ret)
		return 1;

	return 0;
}

#endif //DEVICE_TREE_SUPPORT
