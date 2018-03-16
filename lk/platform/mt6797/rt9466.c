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

#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/errno.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <platform/rt9466.h>
#include <platform/mtk_charger_intf.h>
#include <printf.h>

#if !defined(CONFIG_POWER_EXT)
#include <platform/upmu_common.h>
#endif

#define RT9466_LK_DRV_VERSION "1.0.4_MTK"

/* ================= */
/* Internal variable */
/* ================= */

struct rt9466_info {
	struct mtk_charger_info mchr_info;
	struct mt_i2c_t i2c;
	int i2c_log_level;
	unsigned int hidden_mode_cnt;
};

/*
 * Not to unmask the following IRQs
 * PWR_RDYM/CHG_MIVRM/CHG_AICRM
 * VBUSOVM
 * TS_BAT_HOTM/TS_BAT_WARMM/TS_BAT_COOLM/TS_BAT_COLDM
 * CHG_OTPM/CHG_RVPM/CHG_ADPBADM/CHG_STATCM/CHG_FAULTM/TS_STATCM
 * IEOCM/TERMM/SSFINISHM/SSFINISHM/AICLMeasM
 * BST_BATUVM/PUMPX_DONEM/ADC_DONEM
 */
static const u8 rt9466_irq_init_data[] = {
	RT9466_MASK_PWR_RDYM | RT9466_MASK_CHG_MIVRM | RT9466_MASK_CHG_AICRM,
	RT9466_MASK_VBUSOVM,
	RT9466_MASK_TS_BAT_HOTM | RT9466_MASK_TS_BAT_WARMM |
	RT9466_MASK_TS_BAT_COOLM | RT9466_MASK_TS_BAT_COLDM |
	RT9466_MASK_TS_STATC_RESERVED,
	RT9466_MASK_CHG_OTPM | RT9466_MASK_CHG_RVPM | RT9466_MASK_CHG_ADPBADM |
	RT9466_MASK_CHG_STATCM | RT9466_MASK_CHG_FAULTM | RT9466_MASK_TS_STATCM,
	RT9466_MASK_IEOCM | RT9466_MASK_TERMM | RT9466_MASK_SSFINISHM |
	RT9466_MASK_CHG_AICLMEASM | RT9466_MASK_IRQ2_RESERVED,
	RT9466_MASK_BST_BATUVM | RT9466_MASK_PUMPX_DONEM |
	RT9466_MASK_ADC_DONEM | RT9466_MASK_IRQ3_RESERVED,
};

static const u8 rt9466_irq_maskall_data[] = {
	0xF0, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF
};

static const u8 rt9466_reg_en_hidden_mode[] = {
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
};

static const u8 rt9466_val_en_hidden_mode[] = {
	0x49, 0x32, 0xB6, 0x27, 0x48, 0x18, 0x03, 0xE2,
};

enum rt9466_charging_status {
	RT9466_CHG_STATUS_READY = 0,
	RT9466_CHG_STATUS_PROGRESS,
	RT9466_CHG_STATUS_DONE,
	RT9466_CHG_STATUS_FAULT,
	RT9466_CHG_STATUS_MAX,
};

/* Charging status name */
static const char *rt9466_chg_status_name[RT9466_CHG_STATUS_MAX] = {
	"ready", "progress", "done", "fault",
};

/* ======================= */
/* Address & Default value */
/* ======================= */


static const unsigned char rt9466_reg_addr[] = {
	RT9466_REG_CORE_CTRL0,
	RT9466_REG_CHG_CTRL1,
	RT9466_REG_CHG_CTRL2,
	RT9466_REG_CHG_CTRL3,
	RT9466_REG_CHG_CTRL4,
	RT9466_REG_CHG_CTRL5,
	RT9466_REG_CHG_CTRL6,
	RT9466_REG_CHG_CTRL7,
	RT9466_REG_CHG_CTRL8,
	RT9466_REG_CHG_CTRL9,
	RT9466_REG_CHG_CTRL10,
	RT9466_REG_CHG_CTRL11,
	RT9466_REG_CHG_CTRL12,
	RT9466_REG_CHG_CTRL13,
	RT9466_REG_CHG_CTRL14,
	RT9466_REG_CHG_CTRL15,
	RT9466_REG_CHG_CTRL16,
	RT9466_REG_CHG_ADC,
	RT9466_REG_CHG_CTRL17,
	RT9466_REG_CHG_CTRL18,
	RT9466_REG_DEVICE_ID,
	RT9466_REG_CHG_STAT,
	RT9466_REG_CHG_NTC,
	RT9466_REG_ADC_DATA_H,
	RT9466_REG_ADC_DATA_L,
	RT9466_REG_CHG_STATC,
	RT9466_REG_CHG_FAULT,
	RT9466_REG_TS_STATC,
	RT9466_REG_CHG_IRQ1,
	RT9466_REG_CHG_IRQ2,
	RT9466_REG_CHG_IRQ3,
	RT9466_REG_CHG_STATC_CTRL,
	RT9466_REG_CHG_FAULT_CTRL,
	RT9466_REG_TS_STATC_CTRL,
	RT9466_REG_CHG_IRQ1_CTRL,
	RT9466_REG_CHG_IRQ2_CTRL,
	RT9466_REG_CHG_IRQ3_CTRL,
};


enum rt9466_iin_limit_sel {
	RT9466_IIMLMTSEL_PSEL_OTG,
	RT9466_IINLMTSEL_AICR = 2,
	RT9466_IINLMTSEL_LOWER_LEVEL, /* lower of above two */
};


/* ========================= */
/* I2C operations */
/* ========================= */

static int rt9466_i2c_write_byte(struct rt9466_info *info, u8 cmd, u8 data)
{
	unsigned int ret = I2C_OK;
	unsigned char write_buf[2] = {cmd, data};
	struct mt_i2c_t *i2c = &info->i2c;

	ret = i2c_write(i2c, write_buf, 2);

	if (ret != I2C_OK)
		dprintf(CRITICAL,
			"%s: I2CW[0x%02X] = 0x%02X failed, code = %d\n",
			__func__, cmd, data, ret);
	else
		dprintf(info->i2c_log_level, "%s: I2CW[0x%02X] = 0x%02X\n",
			__func__, cmd, data);

	return ret;
}

static int rt9466_i2c_read_byte(struct rt9466_info *info, u8 cmd, u8 *data)
{
	int ret = I2C_OK;
	u8 ret_data = cmd;
	struct mt_i2c_t *i2c = &info->i2c;

	ret = i2c_write_read(i2c, &ret_data, 1, 1);

	if (ret != I2C_OK)
		dprintf(CRITICAL, "%s: I2CR[0x%02X] failed, code = %d\n",
			__func__, cmd, ret);
	else {
		dprintf(info->i2c_log_level, "%s: I2CR[0x%02X] = 0x%02X\n",
			__func__, cmd, ret_data);
		*data = ret_data;
	}

	return ret;
}

static int rt9466_i2c_block_write(struct rt9466_info *info, u8 cmd, int len,
	const u8 *data)
{
	unsigned char write_buf[len + 1];
	struct mt_i2c_t *i2c = &info->i2c;

	write_buf[0] = cmd;
	memcpy(&write_buf[1], data, len);

	return i2c_write(i2c, write_buf, len + 1);
}

static int rt9466_i2c_test_bit(struct rt9466_info *info, u8 cmd, u8 shift)
{
	int ret = 0;
	u8 data = 0;

	ret = rt9466_i2c_read_byte(info, cmd, &data);
	if (ret != I2C_OK)
		return ret;

	ret = data & (1 << shift);

	return ret;
}

static int rt9466_i2c_update_bits(struct rt9466_info *info, u8 cmd, u8 data,
	u8 mask)
{
	int ret = 0;
	u8 reg_data = 0;

	ret = rt9466_i2c_read_byte(info, cmd, &reg_data);
	if (ret != I2C_OK)
		return ret;

	reg_data = reg_data & 0xFF;
	reg_data &= ~mask;
	reg_data |= (data & mask);

	return rt9466_i2c_write_byte(info, cmd, reg_data);
}

static inline int rt9466_set_bit(struct rt9466_info *info, u8 reg, u8 mask)
{
    return rt9466_i2c_update_bits(info, reg, mask, mask);
}

static inline int rt9466_clr_bit(struct rt9466_info *info, u8 reg, u8 mask)
{
    return rt9466_i2c_update_bits(info, reg, 0x00, mask);
}

/* ================== */
/* internal functions */
/* ================== */
static int rt_charger_set_ichg(struct mtk_charger_info *mchr_info, u32 ichg);
static int rt_charger_set_aicr(struct mtk_charger_info *mchr_info, u32 aicr);
static int rt_charger_set_mivr(struct mtk_charger_info *mchr_info, u32 mivr);
static int rt_charger_get_ichg(struct mtk_charger_info *mchr_info, u32 *ichg);
static int rt_charger_get_aicr(struct mtk_charger_info *mchr_info, u32 *aicr);

static u8 rt9466_find_closest_reg_value(const u32 min, const u32 max,
	const u32 step, const u32 num, const u32 target)
{
	u32 i = 0, cur_val = 0, next_val = 0;

	/* Smaller than minimum supported value, use minimum one */
	if (target < min)
		return 0;

	for (i = 0; i < num - 1; i++) {
		cur_val = min + i * step;
		next_val = cur_val + step;

		if (cur_val > max)
			cur_val = max;

		if (next_val > max)
			next_val = max;

		if (target >= cur_val && target < next_val)
			return i;
	}

	/* Greater than maximum supported value, use maximum one */
	return num - 1;
}

static u8 rt9466_find_closest_reg_value_via_table(const u32 *value_table,
	const u32 table_size, const u32 target_value)
{
	u32 i = 0;

	/* Smaller than minimum supported value, use minimum one */
	if (target_value < value_table[0])
		return 0;

	for (i = 0; i < table_size - 1; i++) {
		if (target_value >= value_table[i] && target_value < value_table[i + 1])
			return i;
	}

	/* Greater than maximum supported value, use maximum one */
	return table_size - 1;
}

static u32 rt9466_find_closest_real_value(const u32 min, const u32 max,
	const u32 step, const u8 reg_val)
{
	u32 ret_val = 0;

	ret_val = min + reg_val * step;
	if (ret_val > max)
		ret_val = max;

	return ret_val;
}

static int rt9466_enable_hidden_mode(struct rt9466_info *info, bool en)
{
	int ret = 0;

	if (en) {
		if (info->hidden_mode_cnt == 0) {
			/*
			 * For platform before MT6755
			 * len of block write cannnot over 8 byte
			 */
			ret = rt9466_i2c_block_write(info,
				rt9466_reg_en_hidden_mode[0],
				ARRAY_SIZE(rt9466_val_en_hidden_mode) / 4,
				rt9466_val_en_hidden_mode);
			if (ret < 0)
				goto err;

			ret = rt9466_i2c_block_write(info,
				rt9466_reg_en_hidden_mode[4],
				ARRAY_SIZE(rt9466_val_en_hidden_mode) / 4,
				&(rt9466_val_en_hidden_mode[4]));
			if (ret < 0)
				goto err;
		}
		info->hidden_mode_cnt++;
	} else {
		if (info->hidden_mode_cnt == 1) /* last one */
			ret = rt9466_i2c_write_byte(info, 0x70, 0x00);
		info->hidden_mode_cnt--;
		if (ret < 0)
			goto err;
	}
	dprintf(CRITICAL, "%s: en = %d\n", __func__, en);
	goto out;

err:
	dprintf(CRITICAL, "%s: en = %d fail(%d)\n", __func__, en, ret);
out:
	return ret;
}

/* Hardware pin current limit */
static int rt9466_enable_ilim(struct rt9466_info *info, bool enable)
{
	int ret = 0;

	dprintf(CRITICAL, "%s: enable ilim = %d\n", __func__, enable);

	ret = (enable ? rt9466_set_bit : rt9466_clr_bit)
		(info, RT9466_REG_CHG_CTRL3, RT9466_MASK_ILIM_EN);

	return ret;
}

/* Select IINLMTSEL to use AICR */
static int rt9466_select_input_current_limit(struct rt9466_info *info,
	enum rt9466_iin_limit_sel sel)
{
	int ret = 0;

	dprintf(CRITICAL, "%s: select input current limit = %d\n",
		__func__, sel);

	ret = rt9466_i2c_update_bits(
		info,
		RT9466_REG_CHG_CTRL2,
		sel << RT9466_SHIFT_IINLMTSEL,
		RT9466_MASK_IINLMTSEL);

	return ret;
}

static bool rt9466_is_hw_exist(struct rt9466_info *info)
{
	int ret = 0;
	u8 device_id = 0, vendor_id = 0, chip_rev = 0;

	ret = rt9466_i2c_read_byte(info, RT9466_REG_DEVICE_ID, &device_id);
	if (ret != I2C_OK)
		return false;

	vendor_id = device_id & 0xF0;
	chip_rev = device_id & 0x0F;
	if (vendor_id != RT9466_VENDOR_ID) {
		dprintf(CRITICAL, "%s: vendor id is incorrect (0x%02X)\n",
			__func__, vendor_id);
		return false;
	}
	dprintf(CRITICAL, "%s: chip rev(E%d,0x%02X)\n", __func__, chip_rev,
		chip_rev);


	info->mchr_info.device_id = chip_rev;
	return true;
}

/* Set register's value to default */
static int rt9466_reset_chip(struct rt9466_info *info)
{
	int ret = 0;

	dprintf(CRITICAL, "%s: starts\n", __func__);

	ret = rt9466_set_bit(info, RT9466_REG_CORE_CTRL0, RT9466_MASK_RST);

	return ret;
}

static int rt9466_set_battery_voreg(struct rt9466_info *info, u32 voreg)
{
	int ret = 0;
	u8 reg_voreg = 0;

	reg_voreg = rt9466_find_closest_reg_value(RT9466_BAT_VOREG_MIN,
		RT9466_BAT_VOREG_MAX, RT9466_BAT_VOREG_STEP,
		RT9466_BAT_VOREG_NUM, voreg);

	dprintf(CRITICAL, "%s: bat voreg = %d\n", __func__, voreg);

	ret = rt9466_i2c_update_bits(
		info,
		RT9466_REG_CHG_CTRL4,
		reg_voreg << RT9466_SHIFT_BAT_VOREG,
		RT9466_MASK_BAT_VOREG
	);

	return ret;
}

static int rt9466_mask_all_irq(struct rt9466_info *info)
{
	int ret = 0;
	u32 i = 0;

	for (i = 0; i < ARRAY_SIZE(rt9466_irq_maskall_data); i++) {
		ret = rt9466_i2c_write_byte(info, RT9466_REG_CHG_STATC_CTRL + i,
			rt9466_irq_maskall_data[i]);
		if (ret != I2C_OK)
			dprintf(CRITICAL,
				"%s: mask irq 0x%02X failed, ret = %d\n",
				__func__, i, ret);
	}

	return ret;
}

static int rt9466_enable_watchdog_timer(struct rt9466_info *info, bool enable)
{
	int ret = 0;

	ret = (enable ? rt9466_set_bit : rt9466_clr_bit)
		(info, RT9466_REG_CHG_CTRL13, RT9466_MASK_WDT_EN);

	return ret;
}


static int rt9466_init_setting(struct rt9466_info *info)
{
	int ret = 0;

	dprintf(CRITICAL, "%s: starts\n", __func__);

	/* Mask all IRQs */
	ret = rt9466_mask_all_irq(info);
	if (ret < 0)
		dprintf(CRITICAL, "%s: mask all irq failed\n", __func__);

	/* Disable HW iinlimit, use SW */
	ret = rt9466_enable_ilim(info, false);
	if (ret < 0)
		dprintf(CRITICAL, "%s: disable ilim failed\n", __func__);

	/* Select input current limit to referenced from AICR */
	ret = rt9466_select_input_current_limit(info, RT9466_IINLMTSEL_AICR);
	if (ret < 0)
		dprintf(CRITICAL, "%s: select input current limit failed\n",
			__func__);

	ret = rt_charger_set_ichg(&info->mchr_info, 2000);
	if (ret < 0)
		dprintf(CRITICAL, "%s: set ichg failed\n", __func__);

	ret = rt_charger_set_aicr(&info->mchr_info, 500);
	if (ret < 0)
		dprintf(CRITICAL, "%s: set aicr failed\n", __func__);

	ret = rt_charger_set_mivr(&info->mchr_info, 4500);
	if (ret < 0)
		dprintf(CRITICAL, "%s: set mivr failed\n", __func__);

	ret = rt9466_set_battery_voreg(info, 4350);
	if (ret < 0)
		dprintf(CRITICAL, "%s: set cv failed\n", __func__);

	ret = rt9466_enable_watchdog_timer(info, true);
	if (ret < 0)
		dprintf(CRITICAL, "%s: enable wdt failed\n",__func__);

	return ret;
}


static int rt9466_get_charging_status(struct rt9466_info *info,
	enum rt9466_charging_status *chg_stat)
{
	int ret = 0;
	u8 data = 0;

	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_STAT, &data);
	if (ret != I2C_OK)
		return ret;

	*chg_stat = (data & RT9466_MASK_CHG_STAT) >> RT9466_SHIFT_CHG_STAT;

	return ret;
}

static int rt9466_get_mivr(struct rt9466_info *info, u32 *mivr)
{
	int ret = 0;
	u8 reg_mivr = 0;
	u8 data = 0;

	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_CTRL6, &data);
	if (ret != I2C_OK)
		return ret;
	reg_mivr = ((data & RT9466_MASK_MIVR) >> RT9466_SHIFT_MIVR) & 0xFF;

	*mivr = rt9466_find_closest_real_value(RT9466_MIVR_MIN, RT9466_MIVR_MAX,
		RT9466_MIVR_STEP, reg_mivr);


	return ret;
}

static int rt9466_is_charging_enable(struct rt9466_info *info, bool *enable)
{
	int ret = 0;
	u8 data = 0;

	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_CTRL2, &data);
	if (ret != I2C_OK)
		return ret;

	if (((data & RT9466_MASK_CHG_EN) >> RT9466_SHIFT_CHG_EN) & 0xFF)
		*enable = true;
	else
		*enable = false;

	return ret;
}

static int rt9466_get_ieoc(struct rt9466_info *info, u32 *ieoc)
{
	int ret = 0;
	u8 reg_ieoc = 0;
	u8 data = 0;

	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_CTRL9, &data);
	if (ret != I2C_OK)
		return ret;

	reg_ieoc = (data & RT9466_MASK_IEOC) >> RT9466_SHIFT_IEOC;
	*ieoc = rt9466_find_closest_real_value(RT9466_IEOC_MIN, RT9466_IEOC_MAX,
		RT9466_IEOC_STEP, reg_ieoc);

	return ret;
}


/* =========================================================== */
/* The following is implementation for interface of rt_charger */
/* =========================================================== */


static int rt_charger_dump_register(struct mtk_charger_info *mchr_info)
{
	int ret = 0;
	u32 i = 0;
	u32 ichg = 0, aicr = 0, mivr = 0, ieoc = 0;
	bool chg_enable = 0;
	enum rt9466_charging_status chg_status = RT9466_CHG_STATUS_READY;
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;
	u8 data = 0;

	ret = rt9466_get_charging_status(info, &chg_status);
	if (chg_status == RT9466_CHG_STATUS_FAULT) {
		info->i2c_log_level = CRITICAL;
		for (i = 0; i < ARRAY_SIZE(rt9466_reg_addr); i++) {
			ret = rt9466_i2c_read_byte(info, rt9466_reg_addr[i],
				&data);
			if (ret != I2C_OK)
				return ret;
		}
	} else
		info->i2c_log_level = INFO;

	ret = rt_charger_get_ichg(mchr_info, &ichg);
	ret = rt9466_get_mivr(info, &mivr);
	ret = rt_charger_get_aicr(mchr_info, &aicr);
	ret = rt9466_get_ieoc(info, &ieoc);
	ret = rt9466_is_charging_enable(info, &chg_enable);

	dprintf(CRITICAL,
		"%s: ICHG = %dmA, AICR = %dmA, MIVR = %dmV, IEOC = %dmA\n",
		__func__, ichg, aicr, mivr, ieoc);

	dprintf(CRITICAL, "%s: CHG_EN = %d, CHG_STATUS = %s\n",
		__func__, chg_enable, rt9466_chg_status_name[chg_status]);

	return ret;
}

static int rt_charger_enable_charging(struct mtk_charger_info *mchr_info,
	bool enable)
{
	int ret = 0;
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	dprintf(CRITICAL, "%s: enable = %d\n", __func__, enable);
	ret = (enable ? rt9466_set_bit : rt9466_clr_bit)
		(info, RT9466_REG_CHG_CTRL2, RT9466_MASK_CHG_EN);

	return ret;
}

static int rt_charger_enable_power_path(struct mtk_charger_info *mchr_info,
	bool enable)
{
	int ret = 0;
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;
	u32 mivr = enable ? 4500 : RT9466_MIVR_MAX;

	dprintf(CRITICAL, "%s: enable = %d\n", __func__, enable);
	ret = rt_charger_set_mivr(&info->mchr_info, mivr);

	return ret;
}

static int rt_charger_set_ichg(struct mtk_charger_info *mchr_info, u32 ichg)
{
	int ret = 0;
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	/* Find corresponding reg value */
	u8 reg_ichg = rt9466_find_closest_reg_value(RT9466_ICHG_MIN,
		RT9466_ICHG_MAX, RT9466_ICHG_STEP, RT9466_ICHG_NUM, ichg);

	dprintf(CRITICAL, "%s: ichg = %d\n", __func__, ichg);

	ret = rt9466_i2c_update_bits(
		info,
		RT9466_REG_CHG_CTRL7,
		reg_ichg << RT9466_SHIFT_ICHG,
		RT9466_MASK_ICHG
	);

	return ret;
}

static int rt_charger_set_aicr(struct mtk_charger_info *mchr_info, u32 aicr)
{
	int ret = 0;
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	/* Find corresponding reg value */
	u8 reg_aicr = rt9466_find_closest_reg_value(RT9466_AICR_MIN,
		RT9466_AICR_MAX, RT9466_AICR_STEP, RT9466_AICR_NUM, aicr);

	dprintf(CRITICAL, "%s: aicr = %d\n", __func__, aicr);

	ret = rt9466_i2c_update_bits(
		info,
		RT9466_REG_CHG_CTRL3,
		reg_aicr << RT9466_SHIFT_AICR,
		RT9466_MASK_AICR
	);

	return ret;
}


static int rt_charger_set_mivr(struct mtk_charger_info *mchr_info, u32 mivr)
{
	int ret = 0;
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	/* Find corresponding reg value */
	u8 reg_mivr = rt9466_find_closest_reg_value(RT9466_MIVR_MIN,
		RT9466_MIVR_MAX, RT9466_MIVR_STEP, RT9466_MIVR_NUM, mivr);

	dprintf(CRITICAL, "%s: mivr = %d\n", __func__, mivr);

	ret = rt9466_i2c_update_bits(
		info,
		RT9466_REG_CHG_CTRL6,
		reg_mivr << RT9466_SHIFT_MIVR,
		RT9466_MASK_MIVR
	);

	return ret;
}


static int rt_charger_get_ichg(struct mtk_charger_info *mchr_info, u32 *ichg)
{
	int ret = 0;
	u8 reg_ichg = 0;
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;
	u8 data = 0;

	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_CTRL7, &data);
	if (ret != I2C_OK)
		return ret;

	reg_ichg = (data & RT9466_MASK_ICHG) >> RT9466_SHIFT_ICHG;
	*ichg = rt9466_find_closest_real_value(RT9466_ICHG_MIN, RT9466_ICHG_MAX,
		RT9466_ICHG_STEP, reg_ichg);

	return ret;
}

static int rt_charger_get_aicr(struct mtk_charger_info *mchr_info, u32 *aicr)
{
	int ret = 0;
	u8 reg_aicr = 0;
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;
	u8 data = 0;

	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_CTRL3, &data);
	if (ret != I2C_OK)
		return ret;

	reg_aicr = (data & RT9466_MASK_AICR) >> RT9466_SHIFT_AICR;
	*aicr = rt9466_find_closest_real_value(RT9466_AICR_MIN, RT9466_AICR_MAX,
		RT9466_AICR_STEP, reg_aicr);

	return ret;
}

static int rt_charger_reset_pumpx(struct mtk_charger_info *mchr_info,
	bool reset)
{
	int ret = 0;
	u32 aicr = 0;
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	rt9466_enable_hidden_mode(info, true);

	ret = (reset ? rt9466_clr_bit : rt9466_set_bit)(info, 0x28, 0x80);
	aicr = (reset ? 100 : 500);
	ret = rt_charger_set_aicr(mchr_info, aicr);

	rt9466_enable_hidden_mode(info, false);
	return ret;
}

/*
 * Workaround to disable IRQ and WDT
 * In case that calling this function before charger is init,
 * use global info
 */
static int rt_charger_sw_reset(struct mtk_charger_info *mchr_info)
{
	int ret = 0, i = 0;
	u8 data = 0;
	struct rt9466_info *info = (struct rt9466_info *)mchr_info;

	dprintf(CRITICAL, "%s: starts\n", __func__);

	/* Mask all IRQs */
	for (i = RT9466_REG_CHG_STATC_CTRL; i <= RT9466_REG_CHG_IRQ3_CTRL; i++) {
		ret = rt9466_i2c_write_byte(info, i, 0xFF);
		if (ret != I2C_OK) {
			dprintf(CRITICAL, "%s: mask irq%d failed\n", __func__, i);
		}
	}

	/* Read all IRQs */
	for (i = RT9466_REG_CHG_STATC; i <= RT9466_REG_CHG_IRQ3; i++) {
		ret = rt9466_i2c_read_byte(info, i, &data);
		if (ret != I2C_OK) {
			dprintf(CRITICAL, "%s: read irq%d failed\n", __func__, i);
		}
	}

	/* Disable WDT */
	ret = rt9466_enable_watchdog_timer(info, false);
	if (ret != I2C_OK)
		dprintf(CRITICAL, "%s: disable wdt failed\n", __func__);

	return ret;
}

static struct mtk_charger_ops rt9466_mchr_ops = {
	.dump_register = rt_charger_dump_register,
	.enable_charging = rt_charger_enable_charging,
	.get_ichg = rt_charger_get_ichg,
	.set_ichg = rt_charger_set_ichg,
	.set_aicr = rt_charger_set_aicr,
	.set_mivr = rt_charger_set_mivr,
	.enable_power_path = rt_charger_enable_power_path,
	.get_aicr = rt_charger_get_aicr,
	.reset_pumpx = rt_charger_reset_pumpx,
	.sw_reset = rt_charger_sw_reset,
};


/* Info of primary charger */
static struct rt9466_info g_rt9466_info = {
	.mchr_info = {
		.name = "primary_charger",
		.alias_name = "rt9466",
		.device_id = -1,
		.mchr_ops = &rt9466_mchr_ops,
	},
	.i2c = {
		.id = I2C0,
		.addr = RT9466_SLAVE_ADDR,
		.mode = ST_MODE,
		.speed = 100,
	},
	.i2c_log_level = INFO,
	.hidden_mode_cnt = 0,
};

int rt9466_probe(void)
{
	int ret = 0;

	/* Check primary charger */
	if (rt9466_is_hw_exist(&g_rt9466_info)) {
		ret = rt9466_reset_chip(&g_rt9466_info);
		ret = rt9466_init_setting(&g_rt9466_info);
		mtk_charger_set_info(&(g_rt9466_info.mchr_info));
		dprintf(CRITICAL, "%s: %s\n", __func__, RT9466_LK_DRV_VERSION);
	}

	return ret;
}

/*
 * Revision Note
 * 1.0.4
 * (1) Enable/Disable spk mode for reset PE+
 *
 * 1.0.3
 * (1) Modify rt9466_is_hw_exist, check vendor id and revision id separately
 * (2) Set MIVR to highest value as disabling power path
 * (3) Update irq mask data
 *
 * 1.0.2
 * (1) Support E4 chip
 * (2) Release sw reset interface
 */
