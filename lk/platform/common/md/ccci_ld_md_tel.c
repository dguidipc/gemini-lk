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

#include <sys/types.h>
#include <stdint.h>
#include <platform/partition.h>
#include <platform/mt_typedefs.h>
#include <platform/boot_mode.h>
#include <platform/mt_reg_base.h>
#include <platform/errno.h>
#include <printf.h>
#include <string.h>
#include <malloc.h>
#include <libfdt.h>
#include <platform/mt_gpt.h>
#include <debug.h>
#include "ccci_ld_md_core.h"
#include "ccci_ld_md_errno.h"

#define MODULE_NAME "LK_LD_MD"

/***************************************************************************************************
** Sub section:
**   Telephony operation parsing and prepare part
***************************************************************************************************/
/* For the following option setting, please check config.h after build, or $project.mk */
#ifdef MTK_MD1_SUPPORT
#define MD1_SUPPORT (MTK_MD1_SUPPORT)
#else
#define MD1_SUPPORT (0)
#endif

#ifdef MTK_MD3_SUPPORT
#define MD3_SUPPORT (MTK_MD3_SUPPORT)
#else
#define MD3_SUPPORT (0)
#endif

#ifdef MTK_C2K_LTE_MODE
#define C2K_LTE_MODE    (MTK_C2K_LTE_MODE)
#else
#define C2K_LTE_MODE    (0)
#endif

#ifdef MTK_IRAT_SUPPORT
#define IRAT_SUPPORT    (1)
#else
#define IRAT_SUPPORT    (0)
#endif

#ifdef RAT_CONFIG_LTE_SUPPORT
#define LTE_SUPPORT (1)
#else
#define LTE_SUPPORT (0)
#endif

#ifdef MTK_PROTOCOL1_RAT_CONFIG
#define PS1_RAT_DEFAULT MTK_PROTOCOL1_RAT_CONFIG
#else
#define PS1_RAT_DEFAULT ""
#endif

static char *get_lk_ro_env_str(char *name)
{
	char *(*NULL_FP)(char *) = 0;

	if (NULL_FP == get_ro_env) {
		ALWAYS_LOG("dummy get_ro_env\n");
		return NULL;
	}

	return get_ro_env(name);
}

/* Legacy capability table */
#define LEGACY_UBIN_START_ID	(8)
static const unsigned int legacy_ubin_rat_map[] = {
	(MD_CAP_FDD_LTE|MD_CAP_TDD_LTE|MD_CAP_TDS_CDMA|MD_CAP_GSM), /* ultg */
	(MD_CAP_FDD_LTE|MD_CAP_TDD_LTE|MD_CAP_WCDMA|MD_CAP_GSM), /* ulwg */
	(MD_CAP_FDD_LTE|MD_CAP_TDD_LTE|MD_CAP_WCDMA|MD_CAP_TDS_CDMA|MD_CAP_GSM), /* ulwtg */
	(MD_CAP_FDD_LTE|MD_CAP_TDD_LTE|MD_CAP_WCDMA|MD_CAP_CDMA2000|MD_CAP_GSM), /* ulwcg */
	(MD_CAP_FDD_LTE|MD_CAP_TDD_LTE|MD_CAP_WCDMA|MD_CAP_CDMA2000|MD_CAP_TDS_CDMA|MD_CAP_GSM), /* ulwctg */
	(MD_CAP_TDD_LTE|MD_CAP_TDS_CDMA|MD_CAP_GSM), /* ulttg */
	(MD_CAP_FDD_LTE|MD_CAP_WCDMA|MD_CAP_GSM), /* ulfwg */
	(MD_CAP_FDD_LTE|MD_CAP_WCDMA|MD_CAP_CDMA2000|MD_CAP_GSM), /* ulfwcg */
	(MD_CAP_FDD_LTE|MD_CAP_CDMA2000|MD_CAP_TDS_CDMA|MD_CAP_GSM), /* ulctg */
	(MD_CAP_TDD_LTE|MD_CAP_CDMA2000|MD_CAP_TDS_CDMA|MD_CAP_GSM), /* ultctg */
	(MD_CAP_TDD_LTE|MD_CAP_WCDMA|MD_CAP_GSM), /*ultwg */
	(MD_CAP_TDD_LTE|MD_CAP_WCDMA|MD_CAP_CDMA2000|MD_CAP_GSM), /* ultwcg */
	(MD_CAP_FDD_LTE|MD_CAP_TDS_CDMA|MD_CAP_GSM), /* ulftg */
	(MD_CAP_FDD_LTE|MD_CAP_CDMA2000|MD_CAP_TDS_CDMA|MD_CAP_GSM)/* ulfctg */
};

/* Specail case convert map table source */
static const unsigned int ubin_convert_table_src[] = {
	(MD_CAP_GSM|MD_CAP_TDD_LTE|MD_CAP_FDD_LTE|MD_CAP_CDMA2000),
	(MD_CAP_GSM|MD_CAP_WCDMA|MD_CAP_CDMA2000)
};

/* Specail case convert map table destination */
static const unsigned int ubin_convert_table_des[] = {
	(MD_CAP_GSM|MD_CAP_WCDMA|MD_CAP_TDD_LTE|MD_CAP_FDD_LTE|MD_CAP_CDMA2000),
	(MD_CAP_GSM|MD_CAP_WCDMA|MD_CAP_TDD_LTE|MD_CAP_FDD_LTE|MD_CAP_CDMA2000)
};

typedef enum {
	md_type_2g = 1,
	md_type_3g,
	md_type_wg,
	md_type_tg,
	md_type_lwg,
	md_type_ltg,
};

/**************************************************************************************************************/
/* Global variable or table at this file                                                                      */
/**************************************************************************************************************/
#ifdef DUMMY_AP_MODE
static struct opt_cfg default_option_list[] = { /* string lenght should less then 32 */
	{"opt_md1_support", 12},/* if MD1_SUPPORT > 0, MD1_EN true  */
	{"opt_md3_support", 2},/* if MD3_SUPPORT > 0, MD3_EN true, ECCCI_C2K true, C2K_SUPPORT true */
};
#else
#ifdef LK_OPT_TO_KERNEL_CCCI
static struct opt_cfg default_option_list[] = { /* string lenght should less then 32 */
	{"opt_md1_support", MD1_SUPPORT},/* if MD1_SUPPORT > 0, MD1_EN true  */
	{"opt_md3_support", MD3_SUPPORT},/* if MD3_SUPPORT > 0, MD3_EN true, ECCCI_C2K true, C2K_SUPPORT true */
	{"opt_c2k_lte_mode", C2K_LTE_MODE},/* mode 0:none; 1: SVLTE; 2: SRLTE */
	{"opt_irat_support", IRAT_SUPPORT},
	{"opt_lte_support", LTE_SUPPORT},
	{"opt_c2k_support", 0},
};
#else
/* Platform relate option not included in this table */
static struct opt_cfg default_option_list[] = { /* string lenght should less then 32 */
	{"opt_md1_support", 12},/* if MD1_SUPPORT > 0, MD1_EN true  */
	{"opt_md3_support", 2},/* if MD3_SUPPORT > 0, MD3_EN true, ECCCI_C2K true, C2K_SUPPORT true */
};
#endif
#endif /* End of DUMMY_AP_MODE */
static char *s_g_local_cmd_line_buf;
static int s_g_local_cmd_line_buf_size;
static int s_g_curr_opt_buf_offset;

/**************************************************************************************************/
/* Local function for telephony                                                                   */
/**************************************************************************************************/
static unsigned int get_capability_bit(char cap_str[])
{
	if (cap_str == NULL)
		return 0;
	if ((strcmp(cap_str, "LF") == 0) || (strcmp(cap_str, "Lf") == 0) || (strcmp(cap_str, "lf") == 0))
		return MD_CAP_FDD_LTE;
	if ((strcmp(cap_str, "LT") == 0) || (strcmp(cap_str, "Lt") == 0) || (strcmp(cap_str, "lt") == 0))
		return MD_CAP_TDD_LTE;
	if ((strcmp(cap_str, "W") == 0) || (strcmp(cap_str, "w") == 0))
		return MD_CAP_WCDMA;
	if ((strcmp(cap_str, "C") == 0) || (strcmp(cap_str, "c") == 0))
		return MD_CAP_CDMA2000;
	if ((strcmp(cap_str, "T") == 0) || (strcmp(cap_str, "t") == 0))
		return MD_CAP_TDS_CDMA;
	if ((strcmp(cap_str, "G") == 0) || (strcmp(cap_str, "g") == 0))
		return MD_CAP_GSM;

	return 0;
}

#define MAX_CAP_STR_LENGTH	16
static unsigned int get_capablity_bit_map(char str[])
{
	char tmp_str[MAX_CAP_STR_LENGTH];
	int tmp_str_curr_pos = 0;
	unsigned int capability_bit_map = 0;
	int str_len;
	int i;

	if (str == NULL)
		return 0;

	str_len = strlen(str);
	for (i = 0; i < str_len; i++) {
		if (str[i] == ' ')
			continue;
		if (str[i] == '\t')
			continue;
		if ((str[i] == '/') || (str[i] == '_')) {
			if (tmp_str_curr_pos) {
				tmp_str[tmp_str_curr_pos] = 0;
				capability_bit_map |= get_capability_bit(tmp_str);
			}
			tmp_str_curr_pos = 0;
			continue;
		}
		if (tmp_str_curr_pos < (MAX_CAP_STR_LENGTH-1)) {
			tmp_str[tmp_str_curr_pos] = str[i];
			tmp_str_curr_pos++;
		} else
			break;
	}
	if (tmp_str_curr_pos) {
		tmp_str[tmp_str_curr_pos] = 0;
		capability_bit_map |= get_capability_bit(tmp_str);
	}

	return capability_bit_map;
}

static unsigned int compatible_convert(unsigned int src_rat)
{
	unsigned int i;

	for (i = 0; i < (sizeof(ubin_convert_table_src)/sizeof(unsigned int)); i++) {
		if (ubin_convert_table_src[i] == src_rat)
			return ubin_convert_table_des[i];
	}
	return src_rat;
}

static unsigned int get_ubin_val_by_bitmap(unsigned int rat_bitmap)
{
	unsigned int val;
	unsigned int i;

	val = rat_bitmap & MD_CAP_MASK;
	val = compatible_convert(val);
	for (i = 0; i < (sizeof(legacy_ubin_rat_map)/sizeof(unsigned int)); i++) {
		if (val == legacy_ubin_rat_map[i])
			return LEGACY_UBIN_START_ID + i;
	}
	return rat_bitmap|MD_CAP_KEY;
}

static int cal_md3_type_by_rat_bitmap(unsigned int rat_bitmap)
{
	int rat_ver;
	long long plat_ret;

	plat_ret = ccci_hal_get_ld_md_plat_setting("rat_plat_ver");
	if (plat_ret > 0)
		rat_ver = (int)plat_ret;
	else
		rat_ver = RAT_VER_DEFAULT;

	switch (rat_ver) {
	case RAT_VER_93:
		return 0;

	case RAT_VER_DEFAULT:
		/* go through */
	default: /* Include case 0, using univeral bin rat version */
		if (rat_bitmap & MD_CAP_CDMA2000)
			return md_type_3g;
		return 0;
	}
}

static int cal_md1_type_by_rat_bitmap(unsigned int rat_bitmap)
{
	int rat_ver;
	long long plat_ret;

	plat_ret = ccci_hal_get_ld_md_plat_setting("rat_plat_ver");
	if (plat_ret > 0)
		rat_ver = (int)plat_ret;
	else
		rat_ver = RAT_VER_DEFAULT;

	switch (rat_ver) {
	case RAT_VER_R8:
		if (rat_bitmap & MD_CAP_WCDMA)
			return md_type_wg;
		if (rat_bitmap & MD_CAP_TDS_CDMA)
			return md_type_tg;
		if (rat_bitmap & MD_CAP_GSM)
			return md_type_2g;
		return 0;

	case RAT_VER_90:
		/* 5, 6, 3, 4, 1 */
		if (rat_bitmap & (MD_CAP_FDD_LTE | MD_CAP_TDD_LTE)) {
			if ((rat_bitmap & (MD_CAP_TDS_CDMA | MD_CAP_WCDMA)) == (MD_CAP_TDS_CDMA | MD_CAP_WCDMA))
				return md_type_ltg;
			if (rat_bitmap & MD_CAP_TDS_CDMA)
				return md_type_ltg;
			if (rat_bitmap & MD_CAP_WCDMA)
				return md_type_lwg;
			return md_type_lwg; /* Using lwg as default */
		}
		if ((rat_bitmap & MD_CAP_MASK) == (MD_CAP_WCDMA | MD_CAP_GSM | MD_CAP_CDMA2000))
			return md_type_lwg; /* Special setting for wcg with non-lk load modem */
		if (rat_bitmap & MD_CAP_WCDMA)
			return md_type_wg;
		if (rat_bitmap & MD_CAP_TDS_CDMA)
			return md_type_tg;
		if (rat_bitmap & MD_CAP_GSM)
			return md_type_2g;
		return 0;

	case RAT_VER_DEFAULT:
		/* go through */
	default: /* Include case 0, using univeral bin rat version */
		return (int)get_ubin_val_by_bitmap(rat_bitmap);
	}
}

static int append_rat_str(char buf[], int size, char append_str[])
{
	int buf_used;
	int append_size;
	int left_size;

	buf_used = strlen(buf);
	append_size = strlen(append_str) + 1;

	left_size = size - buf_used;
	if (left_size > append_size) {
		if (buf_used > 0)
			return snprintf(&buf[buf_used], left_size, "/%s", append_str);
		return snprintf(&buf[buf_used], left_size, "%s", append_str);
	}

	return -1;
}

static void rat_str_convert(char rat_str_src[], char rat_str_des[], int des_size)
{
	unsigned int bit_map = 0;

	bit_map = get_capablity_bit_map(rat_str_src);

	if (des_size > 0) {
		rat_str_des[des_size - 1] = 0;
		if (bit_map == 0)
			return;
	} else
		return;

	/* Gen string */
	if (bit_map & MD_CAP_CDMA2000)
		append_rat_str(rat_str_des, des_size, "C");
	if (bit_map & MD_CAP_FDD_LTE)
		append_rat_str(rat_str_des, des_size, "Lf");
	if (bit_map & MD_CAP_TDD_LTE)
		append_rat_str(rat_str_des, des_size, "Lt");
	if (bit_map & MD_CAP_WCDMA)
		append_rat_str(rat_str_des, des_size, "W");
	if (bit_map & MD_CAP_TDS_CDMA)
		append_rat_str(rat_str_des, des_size, "T");
	if (bit_map & MD_CAP_GSM)
		append_rat_str(rat_str_des, des_size, "G");
}

static int get_md_support_value_by_rat_bitmap(int md_id, unsigned int rat_bitmap)
{
	if (md_id == MD_SYS1)
		return cal_md1_type_by_rat_bitmap(rat_bitmap);

	if (md_id == MD_SYS3)
		return cal_md3_type_by_rat_bitmap(rat_bitmap);

	return 0;
}

static void adjust_setting_by_rat_bitmap(unsigned int bit_map, struct opt_cfg option_list[], int num)
{
	int i;
	for (i = 0; i < num; i++) {
		if (strcmp(option_list[i].name, "opt_md1_support") == 0) {
			option_list[i].val = get_md_support_value_by_rat_bitmap(MD_SYS1, bit_map);
			continue;
		}
		if (strcmp(option_list[i].name, "opt_md3_support") == 0) {
			option_list[i].val = get_md_support_value_by_rat_bitmap(MD_SYS3, bit_map);
			continue;
		}
		if (strcmp(option_list[i].name, "opt_c2k_support") == 0) {
			if (bit_map & MD_CAP_CDMA2000)
				option_list[i].val = 1;
			else
				option_list[i].val = 0;
			continue;
		}
		if (strcmp(option_list[i].name, "opt_lte_support") == 0) {
			if (bit_map & (MD_CAP_FDD_LTE | MD_CAP_TDD_LTE))
				option_list[i].val = 1;
			else
				option_list[i].val = 0;
			continue;
		}
	}
}

static int option_validation_check(unsigned int bit_map, struct opt_cfg option_list[], int num)
{
	int i;
	int md1_support = 0, md3_support = 0, lte_support = 0;
	int val;

	for (i = 0; i < num; i++) {
		if (strcmp(option_list[i].name, "opt_md1_support") == 0) {
			md1_support = option_list[i].val;
			continue;
		}
		if (strcmp(option_list[i].name, "opt_md3_support") == 0) {
			md3_support = option_list[i].val;
			continue;
		}
		if (strcmp(option_list[i].name, "opt_lte_support") == 0) {
			lte_support = option_list[i].val;
			continue;
		}
	}

	/* Check lte_support */
	if ((!!(bit_map & (MD_CAP_FDD_LTE | MD_CAP_TDD_LTE))) != (!!lte_support))
		return -1;

	/* Check opt_md1_support */
	val = get_md_support_value_by_rat_bitmap(MD_SYS1, bit_map);
	if ((val > 0) && (md1_support == 0))
		return -1;

	if ((val = 0) && (md1_support > 0))
		return -1;

	/* Check opt_md3_support */
	val = get_md_support_value_by_rat_bitmap(MD_SYS3, bit_map);
	if ((val > 0) && (md3_support == 0))
		return -1;

	if ((val = 0) && (md3_support > 0))
		return -1;

	/* All check OK */
	return 0;
}

/***********************************************************************************************/
/*  Functions that will be called by exteranl                                                  */
/***********************************************************************************************/
/* Using command line buffer to store opt value */
static int add_opt_setting_to_buf(char name[], char val[])
{
	int name_len;
	int value_len;

	if ((name == NULL) || (val == NULL)) {
		ALWAYS_LOG("dt args invalid\n");
		return -1;
	}

	name_len = strlen(name) + 1;
	value_len = strlen(val) + 1;

	if ((s_g_local_cmd_line_buf_size - s_g_curr_opt_buf_offset) < (value_len + name_len)) {
		ALWAYS_LOG("dt buf free size not enough\n");
		return -1;
	}

	/* copy name */
	memcpy(&s_g_local_cmd_line_buf[s_g_curr_opt_buf_offset], name, name_len);
	s_g_curr_opt_buf_offset += name_len;
	/* copy value */
	memcpy(&s_g_local_cmd_line_buf[s_g_curr_opt_buf_offset], val, value_len);
	s_g_curr_opt_buf_offset += value_len;

	return 0;
}


int update_md_opt_to_fdt_firmware(void *fdt)
{
#ifdef LK_OPT_TO_KERNEL_CCCI
	int nodeoffset;
	char *name, *value;
	int name_len, val_len;
	int i = 0;

	if (s_g_curr_opt_buf_offset == 0) {
		ALWAYS_LOG("no need update md_opt_cfg\n");
		return 0;
	}

	nodeoffset = fdt_path_offset(fdt, "/firmware/android");
	if (nodeoffset < 0) {
		ALWAYS_LOG("/firmware/android not found\n");
		return -1;
	}

	while (i < s_g_curr_opt_buf_offset) {
		name_len = strlen(&s_g_local_cmd_line_buf[i]) + 1;
		name = &s_g_local_cmd_line_buf[i];
		i += name_len;
		val_len = strlen(&s_g_local_cmd_line_buf[i]) + 1;
		value = &s_g_local_cmd_line_buf[i];
		i += val_len;
		fdt_setprop_string(fdt, nodeoffset, name, value);
	}
#endif

	return 0;
}

int prepare_tel_fo_setting(void)
{
	unsigned int i;
	char val_str[8];
	char def_rat_str[32];
	int value;
	char *ptr;
	struct opt_cfg *new_opt_list;
	int using_default = 1; /* 1: Using default, 0: Using new setting, -1: New setting abnormal */
	unsigned int default_cap_bit_map, lk_env_cap_bit_map = 0;
	unsigned int cfg_item_num = sizeof(default_option_list)/sizeof(struct opt_cfg);

	/* Prepare default option setting */
	default_cap_bit_map = get_capablity_bit_map(PS1_RAT_DEFAULT);
	if (default_cap_bit_map)
		adjust_setting_by_rat_bitmap(default_cap_bit_map, default_option_list, cfg_item_num);

	/* Get env rat setting bitmap */
#ifdef ENABLE_PARSING_LK_ENV
	ptr = get_lk_ro_env_str("opt_ps1_rat");
	lk_env_cap_bit_map = get_capablity_bit_map(ptr);
#else
	ptr = NULL;
#endif

	/* Prepare env setting table */
	new_opt_list = (struct opt_cfg *)malloc(sizeof(default_option_list));
	if (new_opt_list) {/* copy default or lk_env value to new opt list */
		for (i = 0; i < cfg_item_num; i++) {

#ifdef ENABLE_PARSING_LK_ENV
			ptr = get_lk_ro_env_str(default_option_list[i].name);
#else
			ptr = NULL;
#endif

			if (ptr == NULL)
				value = default_option_list[i].val;
			else {
				snprintf(val_str, sizeof(val_str), "%s", ptr);
				value = str2uint(val_str);
				using_default = 0;
			}
			new_opt_list[i].name = default_option_list[i].name;
			new_opt_list[i].val = value;
		}
	}

	/* Check env setting validation if has env setting */
	if (using_default != 1) {
		if (option_validation_check(lk_env_cap_bit_map, new_opt_list, cfg_item_num) < 0) {
			ALWAYS_LOG("dynamic option check fail\n");
			using_default = -1;
		} else {
			adjust_setting_by_rat_bitmap(lk_env_cap_bit_map, new_opt_list, cfg_item_num);
			/* Copy back to default */
			for (i = 0; i < (sizeof(default_option_list)/sizeof(struct opt_cfg)); i++)
				default_option_list[i].val = new_opt_list[i].val;
		}
	} else
		ALWAYS_LOG("using default option setting\n");

#ifdef RF_DTB_DETECT
	if (!ccci_detect_rf_board()) {
		for (i = 0; i < cfg_item_num; i++) {
			if (strcmp(default_option_list[i].name, "opt_md1_support") == 0) {
				ALWAYS_LOG("no RF, disable MD1!\n");
				default_option_list[i].val = 0;
			}
			if (strcmp(default_option_list[i].name, "opt_md3_support") == 0) {
				ALWAYS_LOG("no RF, disable MD3!\n");
				default_option_list[i].val = 0;
			}
		}
	}
#endif

	/* Update to tag buffer for kernel */
	for (i = 0; i < cfg_item_num; i++) {
		snprintf(val_str, sizeof(val_str), "%d", default_option_list[i].val);
		add_opt_setting_to_buf(default_option_list[i].name, val_str);
		insert_ccci_tag_inf(default_option_list[i].name, (char*)&default_option_list[i].val, sizeof(int));
	}

	/* update modem capability setting */
	if (using_default != 0) { /* no env setting or in-valid env setting */
		insert_ccci_tag_inf("opt_ps1_rat", (char*)&default_cap_bit_map, sizeof(int));
		if (default_cap_bit_map) {
			rat_str_convert(PS1_RAT_DEFAULT, def_rat_str, sizeof(def_rat_str));
			add_opt_setting_to_buf("opt_ps1_rat", def_rat_str);
		}
	} else {
		ptr = get_lk_ro_env_str("opt_ps1_rat");
		add_opt_setting_to_buf("opt_ps1_rat", ptr);
		insert_ccci_tag_inf("opt_ps1_rat", (char*)&lk_env_cap_bit_map, sizeof(int));
	}
	ALWAYS_LOG("default rat:%s[0x%x][0x%x]\n", PS1_RAT_DEFAULT, default_cap_bit_map, lk_env_cap_bit_map);

	/* update using default or not */
	insert_ccci_tag_inf("opt_using_default", (char*)&using_default, sizeof(int));
	snprintf(val_str, sizeof(val_str), "%d", using_default);
	add_opt_setting_to_buf("opt_using_default", val_str);

	/* update platform option */
	value = (int)ccci_hal_get_ld_md_plat_setting("rat_plat_ver");
	if ((value == RAT_VER_R8) || (value == RAT_VER_90))
		value = 0;
	else if (value == RAT_VER_91_92)
		value = 1;
	else if (value == RAT_VER_93)
		value = 1;
	else
		value = 1;

	snprintf(val_str, sizeof(val_str), "%d", value);
	add_opt_setting_to_buf("opt_eccci_c2k", val_str);
	insert_ccci_tag_inf("opt_eccci_c2k", (char*)&value, sizeof(int));

	/* Notify Kernel using LK setting */
#ifdef LK_OPT_TO_KERNEL_CCCI
	value = 1;
	insert_ccci_tag_inf("opt_using_lk_val", (char*)&value, sizeof(int)); /* Notify Kernel using kernel setting */
#endif

	if (new_opt_list)
		free(new_opt_list);

	return 0;
}

int ccci_get_opt_val(char opt[])
{
	int i;

	for (i = 0; i < (int)(sizeof(default_option_list)/sizeof(struct opt_cfg)); i++) {
		if (strcmp(default_option_list[i].name, opt) == 0)
			return default_option_list[i].val;
	}

	return -LD_ERR_OPT_NOT_FOUND;
}

void ccci_append_tel_fo_setting(char *cmdline)
{
	/* Phase out, change to using device tree */
}

int ccci_alloc_local_cmd_line_buf(int size)
{
	s_g_local_cmd_line_buf = malloc(size);
	if (s_g_local_cmd_line_buf == NULL)
		return -LD_ERR_OPT_CMD_BUF_ALLOC_FAIL;
	s_g_local_cmd_line_buf_size = size;
	return 0;
}

void ccci_free_local_cmd_line_buf(void)
{
	if (s_g_local_cmd_line_buf)
		free(s_g_local_cmd_line_buf);

	s_g_local_cmd_line_buf_size = 0;
}