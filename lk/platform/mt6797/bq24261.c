#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <platform/bq24261.h>
#include <printf.h>

#if !defined(CONFIG_POWER_EXT)
#include <platform/upmu_common.h>
#endif

int g_bq24261_log_en=0;

/**********************************************************
  *
  *   [I2C Slave Setting]
  *
  *********************************************************/
#define BQ24261_SLAVE_ADDR_WRITE   0xD6
#define BQ24261_SLAVE_ADDR_READ    0xD7
#define PRECC_BATVOL 2800 //preCC 2.8V
/**********************************************************
  *
  *   [Global Variable]
  *
  *********************************************************/
kal_uint8 bq24261_reg[bq24261_REG_NUM] = {0};

/**********************************************************
  *
  *   [I2C Function For Read/Write bq24261]
  *
  *********************************************************/
#define BQ24261_I2C_ID  I2C0
static struct mt_i2c_t bq24261_i2c;

kal_uint32 bq24261_write_byte(kal_uint8 addr, kal_uint8 value)
{
	kal_uint32 ret_code = I2C_OK;
	kal_uint8 write_data[2];
	kal_uint16 len;

	write_data[0]= addr;
	write_data[1] = value;

	bq24261_i2c.id = BQ24261_I2C_ID;
	/* Since i2c will left shift 1 bit, we need to set BQ24261 I2C address to >>1 */
	bq24261_i2c.addr = (BQ24261_SLAVE_ADDR_WRITE >> 1);
	bq24261_i2c.mode = ST_MODE;
	bq24261_i2c.speed = 100;
	len = 2;

	ret_code = i2c_write(&bq24261_i2c, write_data, len);

	if (I2C_OK != ret_code)
		dprintf(INFO, "%s: i2c_write: ret_code: %d\n", __func__, ret_code);

	return ret_code;
}

kal_uint32 bq24261_read_byte (kal_uint8 addr, kal_uint8 *dataBuffer)
{
	kal_uint32 ret_code = I2C_OK;
	kal_uint16 len;
	*dataBuffer = addr;

	bq24261_i2c.id = BQ24261_I2C_ID;
	/* Since i2c will left shift 1 bit, we need to set BQ24261 I2C address to >>1 */
	bq24261_i2c.addr = (BQ24261_SLAVE_ADDR_READ >> 1);
	bq24261_i2c.mode = ST_MODE;
	bq24261_i2c.speed = 100;
	len = 1;

	ret_code = i2c_write_read(&bq24261_i2c, dataBuffer, len, len);

	if (I2C_OK != ret_code)
		dprintf(INFO, "%s: i2c_read: ret_code: %d\n", __func__, ret_code);

	return ret_code;
}
/**********************************************************
  *
  *   [Read / Write Function]
  *
  *********************************************************/
kal_uint32 bq24261_read_interface (kal_uint8 RegNum, kal_uint8 *val, kal_uint8 MASK, kal_uint8 SHIFT)
{
	kal_uint8 bq24261_reg = 0;
	int ret = 0;

	dprintf(INFO, "--------------------------------------------------LK\n");

	ret = bq24261_read_byte(RegNum, &bq24261_reg);
	dprintf(INFO, "[bq24261_read_interface] Reg[%x]=0x%x\n", RegNum, bq24261_reg);

	bq24261_reg &= (MASK << SHIFT);
	*val = (bq24261_reg >> SHIFT);

	if (g_bq24261_log_en>1)
		dprintf(INFO, "%d\n", ret);

	return ret;
}

kal_uint32 bq24261_config_interface (kal_uint8 RegNum, kal_uint8 val, kal_uint8 MASK, kal_uint8 SHIFT)
{
	kal_uint8 bq24261_reg = 0;
	kal_uint32 ret = 0;

	dprintf(INFO, "--------------------------------------------------LK\n");

	ret = bq24261_read_byte(RegNum, &bq24261_reg);

	bq24261_reg &= ~(MASK << SHIFT);
	bq24261_reg |= (val << SHIFT);

	if (RegNum == bq24261_CON1 && val == 1
	        && MASK ==CON1_RESET_MASK && SHIFT == CON1_RESET_SHIFT) {
		// RESET bit
	} else if (RegNum == bq24261_CON1) {
		bq24261_reg &= ~0x80;   //RESET bit read returs 1, so clear it
	}

	ret = bq24261_write_byte(RegNum, bq24261_reg);

	dprintf(INFO, "[bq24261_config_interface] write Reg[%x]=0x%x\n", RegNum, bq24261_reg);

	// Check
	//bq24261_read_byte(RegNum, &bq24261_reg);
	//dprintf(INFO, "[bq24261_config_interface] Check Reg[%x]=0x%x\n", RegNum, bq24261_reg);

	if (g_bq24261_log_en>1)
		dprintf(INFO, "%d\n", ret);

	return ret;
}

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/

//CON0----------------------------------------------------

void bq24261_set_tmr_rst(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24261_config_interface(   (kal_uint8)(bq24261_CON0),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON0_TMR_RST_MASK),
	                                (kal_uint8)(CON0_TMR_RST_SHIFT)
	                            );
	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24261_set_en_boost(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24261_config_interface(   (kal_uint8)(bq24261_CON0),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON0_EN_BOOST_MASK),
	                                (kal_uint8)(CON0_EN_BOOST_SHIFT)
	                            );
	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

kal_uint32 bq24261_get_stat(void)
{
	kal_uint32 ret=0;
	kal_uint8 val=0;

	ret=bq24261_read_interface(     (kal_uint8)(bq24261_CON0),
	                                (&val),
	                                (kal_uint8)(CON0_STAT_MASK),
	                                (kal_uint8)(CON0_STAT_SHIFT)
	                          );

	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);

	return val;
}

void bq24261_set_en_shipmode(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24261_config_interface(   (kal_uint8)(bq24261_CON0),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON0_EN_SHIPMODE_MASK),
	                                (kal_uint8)(CON0_EN_SHIPMODE_SHIFT)
	                            );
	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

kal_uint32 bq24261_get_fault(void)
{
	kal_uint32 ret=0;
	kal_uint8 val=0;

	ret=bq24261_read_interface(     (kal_uint8)(bq24261_CON0),
	                                (&val),
	                                (kal_uint8)(CON0_FAULT_MASK),
	                                (kal_uint8)(CON0_FAULT_SHIFT)
	                          );

	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);

	return val;
}

//CON1----------------------------------------------------

void bq24261_set_reset(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24261_config_interface(   (kal_uint8)(bq24261_CON1),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON1_RESET_MASK),
	                                (kal_uint8)(CON1_RESET_SHIFT)
	                            );
	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24261_set_in_limit(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24261_config_interface(   (kal_uint8)(bq24261_CON1),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON1_IN_LIMIT_MASK),
	                                (kal_uint8)(CON1_IN_LIMIT_SHIFT)
	                            );
	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24261_set_en_stat(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24261_config_interface(   (kal_uint8)(bq24261_CON1),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON1_EN_STAT_MASK),
	                                (kal_uint8)(CON1_EN_STAT_SHIFT)
	                            );
	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24261_set_te(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24261_config_interface(   (kal_uint8)(bq24261_CON1),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON1_TE_MASK),
	                                (kal_uint8)(CON1_TE_SHIFT)
	                            );
	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24261_set_dis_ce(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24261_config_interface(   (kal_uint8)(bq24261_CON1),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON1_DIS_CE_MASK),
	                                (kal_uint8)(CON1_DIS_CE_SHIFT)
	                            );
	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24261_set_hz_mode(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24261_config_interface(   (kal_uint8)(bq24261_CON1),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON1_HZ_MODE_MASK),
	                                (kal_uint8)(CON1_HZ_MODE_SHIFT)
	                            );
	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

//CON2----------------------------------------------------

void bq24261_set_vbreg(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24261_config_interface(   (kal_uint8)(bq24261_CON2),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON2_VBREG_MASK),
	                                (kal_uint8)(CON2_VBREG_SHIFT)
	                            );
	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24261_set_mod_freq(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24261_config_interface(   (kal_uint8)(bq24261_CON2),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON2_MOD_FREQ_MASK),
	                                (kal_uint8)(CON2_MOD_FREQ_SHIFT)
	                            );
	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

//CON3----------------------------------------------------

kal_uint32 bq24261_get_vender_code(void)
{
	kal_uint32 ret=0;
	kal_uint8 val=0;

	ret=bq24261_read_interface(     (kal_uint8)(bq24261_CON3),
	                                (&val),
	                                (kal_uint8)(CON3_VENDER_CODE_MASK),
	                                (kal_uint8)(CON3_VENDER_CODE_SHIFT)
	                          );

	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);

	return val;
}

kal_uint32 bq24261_get_pn(void)
{
	kal_uint32 ret=0;
	kal_uint8 val=0;

	ret=bq24261_read_interface(     (kal_uint8)(bq24261_CON3),
	                                (&val),
	                                (kal_uint8)(CON3_PN_MASK),
	                                (kal_uint8)(CON3_PN_SHIFT)
	                          );

	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);

	return val;
}

//CON4----------------------------------------------------

void bq24261_set_ichg(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24261_config_interface(   (kal_uint8)(bq24261_CON4),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON4_ICHRG_MASK),
	                                (kal_uint8)(CON4_ICHRG_SHIFT)
	                            );
	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24261_set_iterm(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24261_config_interface(   (kal_uint8)(bq24261_CON4),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON4_ITERM_MASK),
	                                (kal_uint8)(CON4_ITERM_SHIFT)
	                            );
	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

//CON5----------------------------------------------------

kal_uint32 bq24261_get_minsys_status(void)
{
	kal_uint32 ret=0;
	kal_uint8 val=0;

	ret=bq24261_read_interface(     (kal_uint8)(bq24261_CON5),
	                                (&val),
	                                (kal_uint8)(CON5_MINSYS_STATUS_MASK),
	                                (kal_uint8)(CON5_MINSYS_STATUS_SHIFT)
	                          );

	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);

	return val;
}

kal_uint32 bq24261_get_vindpm_status(void)
{
	kal_uint32 ret=0;
	kal_uint8 val=0;

	ret=bq24261_read_interface(     (kal_uint8)(bq24261_CON5),
	                                (&val),
	                                (kal_uint8)(CON5_VINDPM_STATUS_MASK),
	                                (kal_uint8)(CON5_VINDPM_STATUS_SHIFT)
	                          );

	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);

	return val;
}

void bq24261_set_low_chg(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24261_config_interface(   (kal_uint8)(bq24261_CON5),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON5_LOW_CHG_MASK),
	                                (kal_uint8)(CON5_LOW_CHG_SHIFT)
	                            );
	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24261_set_dpdm_en(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24261_config_interface(   (kal_uint8)(bq24261_CON5),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON5_DPDM_EN_MASK),
	                                (kal_uint8)(CON5_DPDM_EN_SHIFT)
	                            );
	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

kal_uint32 bq24261_get_cd_status(void)
{
	kal_uint32 ret=0;
	kal_uint8 val=0;

	ret=bq24261_read_interface(     (kal_uint8)(bq24261_CON5),
	                                (&val),
	                                (kal_uint8)(CON5_CD_STATUS_MASK),
	                                (kal_uint8)(CON5_CD_STATUS_SHIFT)
	                          );

	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);

	return val;
}

void bq24261_set_vindpm(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24261_config_interface(   (kal_uint8)(bq24261_CON5),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON5_VINDPM_MASK),
	                                (kal_uint8)(CON5_VINDPM_SHIFT)
	                            );
	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

//CON6----------------------------------------------------

void bq24261_set_2xtmr_en(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24261_config_interface(   (kal_uint8)(bq24261_CON6),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON6_2XTMR_EN_MASK),
	                                (kal_uint8)(CON6_2XTMR_EN_SHIFT)
	                            );
	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24261_set_tmr(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24261_config_interface(   (kal_uint8)(bq24261_CON6),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON6_TMR_MASK),
	                                (kal_uint8)(CON6_TMR_SHIFT)
	                            );
	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24261_set_boost_ilim(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24261_config_interface(   (kal_uint8)(bq24261_CON6),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON6_BOOST_ILIM_MASK),
	                                (kal_uint8)(CON6_BOOST_ILIM_SHIFT)
	                            );
	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24261_set_ts_en(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24261_config_interface(   (kal_uint8)(bq24261_CON6),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON6_TS_EN_MASK),
	                                (kal_uint8)(CON6_TS_EN_SHIFT)
	                            );
	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

kal_uint32 bq24261_get_ts_fault(void)
{
	kal_uint32 ret=0;
	kal_uint8 val=0;

	ret=bq24261_read_interface(     (kal_uint8)(bq24261_CON6),
	                                (&val),
	                                (kal_uint8)(CON6_TS_FAULT_MASK),
	                                (kal_uint8)(CON6_TS_FAULT_SHIFT)
	                          );

	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);

	return val;
}

void bq24261_set_vindpm_off(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24261_config_interface(   (kal_uint8)(bq24261_CON6),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON6_VINDPM_OFF_MASK),
	                                (kal_uint8)(CON6_VINDPM_OFF_SHIFT)
	                            );
	if (g_bq24261_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/
void bq24261_dump_register(void)
{
	int i=0;

	dprintf(CRITICAL, "bq24261_dump_register\r\n");
	for (i=0; i<bq24261_REG_NUM; i++) {
		bq24261_read_byte(i, &bq24261_reg[i]);
		dprintf(CRITICAL, "[0x%x]=0x%x\r\n", i, bq24261_reg[i]);
	}
}

void bq24261_hw_init(void)
{
	bq24261_set_tmr_rst(0x1); // wdt reset
	bq24261_set_en_boost(0); // Disable OTG boost
	bq24261_set_tmr(0x3); // Disable Safty timer

	bq24261_set_iterm(0x2); // ITERM to BAT

	bq24261_set_vindpm_off(0); // VINDPM_OFF
	bq24261_set_vindpm(0x4); // VINDPM

	bq24261_set_ts_en(0); // Thermal sense

	bq24261_set_hz_mode(0x0);

	if (get_mt6351_pmic_chip_version() < PMIC6351_E3_CID_CODE)
		bq24261_set_vbreg(0x19);
	else
		bq24261_set_vbreg(0x23);
}

static CHARGER_TYPE g_chr_type_num = CHARGER_UNKNOWN;
int hw_charging_get_charger_type(void);
extern int g_std_ac_large_current_en;

void bq24261_charging_enable(kal_uint32 bEnable)
{
	int temp_CC_value = 0;
	kal_int32 bat_val = 0;

	if (CHARGER_UNKNOWN == g_chr_type_num && KAL_TRUE == upmu_is_chr_det()) {
		hw_charging_get_charger_type();
		dprintf(CRITICAL, "[BATTERY:bq24261] charger type: %d\n", g_chr_type_num);
	}

	bat_val = get_i_sense_volt(1);
	if (g_chr_type_num == STANDARD_CHARGER) {
		if (g_std_ac_large_current_en==1) {
			temp_CC_value = 2000;
			bq24261_set_in_limit(0x7); //IN current limit at 2A
			if (bat_val < PRECC_BATVOL)
				bq24261_set_ichg(0x0);  //Pre-Charging Current Limit at 500ma
			else
				bq24261_set_ichg(0xF);  //Fast Charging Current Limit at 2A

		} else {
			temp_CC_value = 1000;
			bq24261_set_in_limit(0x4); //IN current limit at 1.5A
			if (bat_val < PRECC_BATVOL)
				bq24261_set_ichg(0x0);  //Pre-Charging Current Limit at 500ma
			else
				bq24261_set_ichg(0x5);  //Fast Charging Current Limit at 1A
		}
	} else if (g_chr_type_num == STANDARD_HOST \
	           || g_chr_type_num == CHARGING_HOST \
	           || g_chr_type_num == NONSTANDARD_CHARGER) {
		temp_CC_value = 500;
		bq24261_set_in_limit(0x2); //IN current limit at 500mA
		bq24261_set_ichg(0);  //Fast Charging Current Limit at 500mA
	} else {
		temp_CC_value = 500;
		bq24261_set_in_limit(0x2); //IN current limit at 500mA
		bq24261_set_ichg(0);  //Fast Charging Current Limit at 500mA
	}

	if (KAL_TRUE == bEnable)
		bq24261_set_dis_ce(0); // charger enable
	else
		bq24261_set_dis_ce(0x1); // charger disable

	bq24261_set_tmr_rst(0x1);

	dprintf(INFO, "[BATTERY:bq24261] bq24261_set_ac_current(), CC value(%dmA) \r\n", temp_CC_value);
	dprintf(INFO, "[BATTERY:bq24261] charger enable/disable %d !\r\n", bEnable);
}

#if defined(CONFIG_POWER_EXT)

int hw_charging_get_charger_type(void)
{
	g_chr_type_num = STANDARD_HOST;

	return STANDARD_HOST;
}

#else

extern void Charger_Detect_Init(void);
extern void Charger_Detect_Release(void);
extern void mdelay (unsigned long msec);

static void hw_bc11_dump_register(void)
{
	dprintf(INFO, "Reg[0x%x]=0x%x,Reg[0x%x]=0x%x\n",
	        MT6351_CHR_CON20, upmu_get_reg_value(MT6351_CHR_CON20),
	        MT6351_CHR_CON21, upmu_get_reg_value(MT6351_CHR_CON21)
	       );
}

static void hw_bc11_init(void)
{
	mdelay(300);
	Charger_Detect_Init();

	//RG_bc11_BIAS_EN=1
	mt6351_upmu_set_rg_bc11_bias_en(0x1);
	//RG_bc11_VSRC_EN[1:0]=00
	mt6351_upmu_set_rg_bc11_vsrc_en(0x0);
	//RG_bc11_VREF_VTH = [1:0]=00
	mt6351_upmu_set_rg_bc11_vref_vth(0x0);
	//RG_bc11_CMP_EN[1.0] = 00
	mt6351_upmu_set_rg_bc11_cmp_en(0x0);
	//RG_bc11_IPU_EN[1.0] = 00
	mt6351_upmu_set_rg_bc11_ipu_en(0x0);
	//RG_bc11_IPD_EN[1.0] = 00
	mt6351_upmu_set_rg_bc11_ipd_en(0x0);
	//bc11_RST=1
	mt6351_upmu_set_rg_bc11_rst(0x1);
	//bc11_BB_CTRL=1
	mt6351_upmu_set_rg_bc11_bb_ctrl(0x1);

	mdelay(50);

	if (g_bq24261_log_en>1) {
		dprintf(INFO, "hw_bc11_init() \r\n");
		hw_bc11_dump_register();
	}

}


static U32 hw_bc11_DCD(void)
{
	U32 wChargerAvail = 0;

	//RG_bc11_IPU_EN[1.0] = 10
	mt6351_upmu_set_rg_bc11_ipu_en(0x2);
	//RG_bc11_IPD_EN[1.0] = 01
	mt6351_upmu_set_rg_bc11_ipd_en(0x1);
	//RG_bc11_VREF_VTH = [1:0]=01
	mt6351_upmu_set_rg_bc11_vref_vth(0x1);
	//RG_bc11_CMP_EN[1.0] = 10
	mt6351_upmu_set_rg_bc11_cmp_en(0x2);

	mdelay(80);

	wChargerAvail = mt6351_upmu_get_rgs_bc11_cmp_out();

	if (g_bq24261_log_en>1) {
		dprintf(INFO, "hw_bc11_DCD() \r\n");
		hw_bc11_dump_register();
	}

	//RG_bc11_IPU_EN[1.0] = 00
	mt6351_upmu_set_rg_bc11_ipu_en(0x0);
	//RG_bc11_IPD_EN[1.0] = 00
	mt6351_upmu_set_rg_bc11_ipd_en(0x0);
	//RG_bc11_CMP_EN[1.0] = 00
	mt6351_upmu_set_rg_bc11_cmp_en(0x0);
	//RG_bc11_VREF_VTH = [1:0]=00
	mt6351_upmu_set_rg_bc11_vref_vth(0x0);

	return wChargerAvail;
}


static U32 hw_bc11_stepA1(void)
{
	U32 wChargerAvail = 0;

	//RG_bc11_IPD_EN[1.0] = 01
	mt6351_upmu_set_rg_bc11_ipd_en(0x1);
	//RG_bc11_VREF_VTH = [1:0]=00
	mt6351_upmu_set_rg_bc11_vref_vth(0x0);
	//RG_bc11_CMP_EN[1.0] = 01
	mt6351_upmu_set_rg_bc11_cmp_en(0x1);

	mdelay(80);

	wChargerAvail = mt6351_upmu_get_rgs_bc11_cmp_out();

	if (g_bq24261_log_en>1) {
		dprintf(INFO, "hw_bc11_stepA1() \r\n");
		hw_bc11_dump_register();
	}

	//RG_bc11_IPD_EN[1.0] = 00
	mt6351_upmu_set_rg_bc11_ipd_en(0x0);
	//RG_bc11_CMP_EN[1.0] = 00
	mt6351_upmu_set_rg_bc11_cmp_en(0x0);

	return  wChargerAvail;
}


static U32 hw_bc11_stepA2(void)
{
	U32 wChargerAvail = 0;

	//RG_bc11_VSRC_EN[1.0] = 10
	mt6351_upmu_set_rg_bc11_vsrc_en(0x2);
	//RG_bc11_IPD_EN[1:0] = 01
	mt6351_upmu_set_rg_bc11_ipd_en(0x1);
	//RG_bc11_VREF_VTH = [1:0]=00
	mt6351_upmu_set_rg_bc11_vref_vth(0x0);
	//RG_bc11_CMP_EN[1.0] = 01
	mt6351_upmu_set_rg_bc11_cmp_en(0x1);

	mdelay(80);

	wChargerAvail = mt6351_upmu_get_rgs_bc11_cmp_out();

	if (g_bq24261_log_en>1) {
		dprintf(INFO, "hw_bc11_stepA2() \r\n");
		hw_bc11_dump_register();
	}

	//RG_bc11_VSRC_EN[1:0]=00
	mt6351_upmu_set_rg_bc11_vsrc_en(0x0);
	//RG_bc11_IPD_EN[1.0] = 00
	mt6351_upmu_set_rg_bc11_ipd_en(0x0);
	//RG_bc11_CMP_EN[1.0] = 00
	mt6351_upmu_set_rg_bc11_cmp_en(0x0);

	return  wChargerAvail;
}


static U32 hw_bc11_stepB2(void)
{
	U32 wChargerAvail = 0;

	//RG_bc11_IPU_EN[1:0]=10
	mt6351_upmu_set_rg_bc11_ipu_en(0x2);
	//RG_bc11_VREF_VTH = [1:0]=01
	mt6351_upmu_set_rg_bc11_vref_vth(0x1);
	//RG_bc11_CMP_EN[1.0] = 01
	mt6351_upmu_set_rg_bc11_cmp_en(0x1);

	mdelay(80);

	wChargerAvail = mt6351_upmu_get_rgs_bc11_cmp_out();

	if (g_bq24261_log_en>1) {
		dprintf(INFO, "hw_bc11_stepB2() \r\n");
		hw_bc11_dump_register();
	}

	//RG_bc11_IPU_EN[1.0] = 00
	mt6351_upmu_set_rg_bc11_ipu_en(0x0);
	//RG_bc11_CMP_EN[1.0] = 00
	mt6351_upmu_set_rg_bc11_cmp_en(0x0);
	//RG_bc11_VREF_VTH = [1:0]=00
	mt6351_upmu_set_rg_bc11_vref_vth(0x0);

	return  wChargerAvail;
}


static void hw_bc11_done(void)
{
	//RG_bc11_VSRC_EN[1:0]=00
	mt6351_upmu_set_rg_bc11_vsrc_en(0x0);
	//RG_bc11_VREF_VTH = [1:0]=0
	mt6351_upmu_set_rg_bc11_vref_vth(0x0);
	//RG_bc11_CMP_EN[1.0] = 00
	mt6351_upmu_set_rg_bc11_cmp_en(0x0);
	//RG_bc11_IPU_EN[1.0] = 00
	mt6351_upmu_set_rg_bc11_ipu_en(0x0);
	//RG_bc11_IPD_EN[1.0] = 00
	mt6351_upmu_set_rg_bc11_ipd_en(0x0);
	//RG_bc11_BIAS_EN=0
	mt6351_upmu_set_rg_bc11_bias_en(0x0);

	Charger_Detect_Release();

	if (g_bq24261_log_en>1) {
		dprintf(INFO, "hw_bc11_done() \r\n");
		hw_bc11_dump_register();
	}

}

int hw_charging_get_charger_type(void)
{
	if (CHARGER_UNKNOWN != g_chr_type_num)
		return g_chr_type_num;

	/********* Step initial  ***************/
	hw_bc11_init();

	/********* Step DCD ***************/
	if (1 == hw_bc11_DCD()) {
		/********* Step A1 ***************/
		if (1 == hw_bc11_stepA1()) {
			g_chr_type_num = APPLE_2_1A_CHARGER;
			dprintf(INFO, "step A1 : Apple 2.1A CHARGER!\r\n");
		} else {
			g_chr_type_num = NONSTANDARD_CHARGER;
			dprintf(INFO, "step A1 : Non STANDARD CHARGER!\r\n");
		}
	} else {
		/********* Step A2 ***************/
		if (1 == hw_bc11_stepA2()) {
			/********* Step B2 ***************/
			if (1 == hw_bc11_stepB2()) {
				g_chr_type_num = STANDARD_CHARGER;
				dprintf(INFO, "step B2 : STANDARD CHARGER!\r\n");
			} else {
				g_chr_type_num = CHARGING_HOST;
				dprintf(INFO, "step B2 :  Charging Host!\r\n");
			}
		} else {
			g_chr_type_num = STANDARD_HOST;
			dprintf(INFO, "step A2 : Standard USB Host!\r\n");
		}

	}

	/********* Finally setting *******************************/
	hw_bc11_done();

	return g_chr_type_num;
}
#endif
