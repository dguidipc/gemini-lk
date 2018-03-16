#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <platform/bq24196.h>
#include <printf.h>

#if !defined(CONFIG_POWER_EXT)
#include <platform/upmu_common.h>
#endif

int g_bq24196_log_en=0;

/**********************************************************
  *
  *   [I2C Slave Setting]
  *
  *********************************************************/
#define BQ24196_SLAVE_ADDR_WRITE   0xD6
#define BQ24196_SLAVE_ADDR_READ    0xD7
#define PRECC_BATVOL 2800 //preCC 2.8V
/**********************************************************
  *
  *   [Global Variable]
  *
  *********************************************************/
kal_uint8 bq24196_reg[bq24196_REG_NUM] = {0};

/**********************************************************
  *
  *   [I2C Function For Read/Write bq24196]
  *
  *********************************************************/
#define BQ24196_I2C_ID  I2C0
static struct mt_i2c_t bq24196_i2c;

kal_uint32 bq24196_write_byte(kal_uint8 addr, kal_uint8 value)
{
	kal_uint32 ret_code = I2C_OK;
	kal_uint8 write_data[2];
	kal_uint16 len;

	write_data[0]= addr;
	write_data[1] = value;

	bq24196_i2c.id = BQ24196_I2C_ID;
	/* Since i2c will left shift 1 bit, we need to set BQ24196 I2C address to >>1 */
	bq24196_i2c.addr = (BQ24196_SLAVE_ADDR_WRITE >> 1);
	bq24196_i2c.mode = ST_MODE;
	bq24196_i2c.speed = 100;
	len = 2;

	ret_code = i2c_write(&bq24196_i2c, write_data, len);

	if (I2C_OK != ret_code)
		dprintf(INFO, "%s: i2c_write: ret_code: %d\n", __func__, ret_code);

	return ret_code;
}

kal_uint32 bq24196_read_byte (kal_uint8 addr, kal_uint8 *dataBuffer)
{
	kal_uint32 ret_code = I2C_OK;
	kal_uint16 len;
	*dataBuffer = addr;

	bq24196_i2c.id = BQ24196_I2C_ID;
	/* Since i2c will left shift 1 bit, we need to set BQ24196 I2C address to >>1 */
	bq24196_i2c.addr = (BQ24196_SLAVE_ADDR_READ >> 1);
	bq24196_i2c.mode = ST_MODE;
	bq24196_i2c.speed = 100;
	len = 1;

	ret_code = i2c_write_read(&bq24196_i2c, dataBuffer, len, len);

	if (I2C_OK != ret_code)
		dprintf(INFO, "%s: i2c_read: ret_code: %d\n", __func__, ret_code);

	return ret_code;
}

/**********************************************************
  *
  *   [Read / Write Function]
  *
  *********************************************************/
kal_uint32 bq24196_read_interface (kal_uint8 RegNum, kal_uint8 *val, kal_uint8 MASK, kal_uint8 SHIFT)
{
	kal_uint8 bq24196_reg = 0;
	int ret = 0;

	dprintf(INFO, "--------------------------------------------------LK\n");

	ret = bq24196_read_byte(RegNum, &bq24196_reg);
	dprintf(INFO, "[bq24196_read_interface] Reg[%x]=0x%x\n", RegNum, bq24196_reg);

	bq24196_reg &= (MASK << SHIFT);
	*val = (bq24196_reg >> SHIFT);

	if (g_bq24196_log_en>1)
		dprintf(INFO, "%d\n", ret);

	return ret;
}

kal_uint32 bq24196_config_interface (kal_uint8 RegNum, kal_uint8 val, kal_uint8 MASK, kal_uint8 SHIFT)
{
	kal_uint8 bq24196_reg = 0;
	kal_uint32 ret = 0;

	dprintf(INFO, "--------------------------------------------------LK\n");

	ret = bq24196_read_byte(RegNum, &bq24196_reg);

	bq24196_reg &= ~(MASK << SHIFT);
	bq24196_reg |= (val << SHIFT);

	ret = bq24196_write_byte(RegNum, bq24196_reg);

	dprintf(INFO, "[bq24196_config_interface] write Reg[%x]=0x%x\n", RegNum, bq24196_reg);

	// Check
	//bq24196_read_byte(RegNum, &bq24196_reg);
	//dprintf(INFO, "[bq24196_config_interface] Check Reg[%x]=0x%x\n", RegNum, bq24196_reg);

	if (g_bq24196_log_en>1)
		dprintf(INFO, "%d\n", ret);

	return ret;
}

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/
//CON0----------------------------------------------------

void bq24196_set_en_hiz(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON0),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON0_EN_HIZ_MASK),
	                                (kal_uint8)(CON0_EN_HIZ_SHIFT)
	                            );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24196_set_vindpm(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON0),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON0_VINDPM_MASK),
	                                (kal_uint8)(CON0_VINDPM_SHIFT)
	                            );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24196_set_iinlim(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON0),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON0_IINLIM_MASK),
	                                (kal_uint8)(CON0_IINLIM_SHIFT)
	                            );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

//CON1----------------------------------------------------

void bq24196_set_reg_rst(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON1),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON1_REG_RST_MASK),
	                                (kal_uint8)(CON1_REG_RST_SHIFT)
	                            );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24196_set_wdt_rst(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON1),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON1_WDT_RST_MASK),
	                                (kal_uint8)(CON1_WDT_RST_SHIFT)
	                            );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24196_set_chg_config(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON1),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON1_CHG_CONFIG_MASK),
	                                (kal_uint8)(CON1_CHG_CONFIG_SHIFT)
	                            );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24196_set_sys_min(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON1),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON1_SYS_MIN_MASK),
	                                (kal_uint8)(CON1_SYS_MIN_SHIFT)
	                            );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24196_set_boost_lim(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON1),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON1_BOOST_LIM_MASK),
	                                (kal_uint8)(CON1_BOOST_LIM_SHIFT)
	                            );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

//CON2----------------------------------------------------

void bq24196_set_ichg(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON2),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON2_ICHG_MASK),
	                                (kal_uint8)(CON2_ICHG_SHIFT)
	                            );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

//CON3----------------------------------------------------

void bq24196_set_iprechg(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON3),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON3_IPRECHG_MASK),
	                                (kal_uint8)(CON3_IPRECHG_SHIFT)
	                            );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24196_set_iterm(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON3),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON3_ITERM_MASK),
	                                (kal_uint8)(CON3_ITERM_SHIFT)
	                            );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

//CON4----------------------------------------------------

void bq24196_set_vreg(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON4),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON4_VREG_MASK),
	                                (kal_uint8)(CON4_VREG_SHIFT)
	                            );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24196_set_batlowv(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON4),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON4_BATLOWV_MASK),
	                                (kal_uint8)(CON4_BATLOWV_SHIFT)
	                            );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24196_set_vrechg(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON4),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON4_VRECHG_MASK),
	                                (kal_uint8)(CON4_VRECHG_SHIFT)
	                            );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

//CON5----------------------------------------------------

void bq24196_set_en_term(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON5),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON5_EN_TERM_MASK),
	                                (kal_uint8)(CON5_EN_TERM_SHIFT)
	                            );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24196_set_term_stat(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON5),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON5_TERM_STAT_MASK),
	                                (kal_uint8)(CON5_TERM_STAT_SHIFT)
	                            );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24196_set_watchdog(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON5),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON5_WATCHDOG_MASK),
	                                (kal_uint8)(CON5_WATCHDOG_SHIFT)
	                            );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24196_set_en_timer(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON5),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON5_EN_TIMER_MASK),
	                                (kal_uint8)(CON5_EN_TIMER_SHIFT)
	                            );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24196_set_chg_timer(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON5),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON5_CHG_TIMER_MASK),
	                                (kal_uint8)(CON5_CHG_TIMER_SHIFT)
	                            );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

//CON6----------------------------------------------------

void bq24196_set_treg(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON6),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON6_TREG_MASK),
	                                (kal_uint8)(CON6_TREG_SHIFT)
	                            );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

//CON7----------------------------------------------------

void bq24196_set_tmr2x_en(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON7),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON7_TMR2X_EN_MASK),
	                                (kal_uint8)(CON7_TMR2X_EN_SHIFT)
	                            );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24196_set_batfet_disable(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON7),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON7_BATFET_Disable_MASK),
	                                (kal_uint8)(CON7_BATFET_Disable_SHIFT)
	                            );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

void bq24196_set_int_mask(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq24196_config_interface(   (kal_uint8)(bq24196_CON7),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON7_INT_MASK_MASK),
	                                (kal_uint8)(CON7_INT_MASK_SHIFT)
	                            );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);
}

//CON8----------------------------------------------------

kal_uint32 bq24196_get_system_status(void)
{
	kal_uint32 ret=0;
	kal_uint8 val=0;

	ret=bq24196_read_interface(     (kal_uint8)(bq24196_CON8),
	                                (&val),
	                                (kal_uint8)(0xFF),
	                                (kal_uint8)(0x0)
	                          );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);

	return val;
}

kal_uint32 bq24196_get_vbus_stat(void)
{
	kal_uint32 ret=0;
	kal_uint8 val=0;

	ret=bq24196_read_interface(     (kal_uint8)(bq24196_CON8),
	                                (&val),
	                                (kal_uint8)(CON8_VBUS_STAT_MASK),
	                                (kal_uint8)(CON8_VBUS_STAT_SHIFT)
	                          );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);

	return val;
}

kal_uint32 bq24196_get_chrg_stat(void)
{
	kal_uint32 ret=0;
	kal_uint8 val=0;

	ret=bq24196_read_interface(     (kal_uint8)(bq24196_CON8),
	                                (&val),
	                                (kal_uint8)(CON8_CHRG_STAT_MASK),
	                                (kal_uint8)(CON8_CHRG_STAT_SHIFT)
	                          );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);

	return val;
}

kal_uint32 bq24196_get_vsys_stat(void)
{
	kal_uint32 ret=0;
	kal_uint8 val=0;

	ret=bq24196_read_interface(     (kal_uint8)(bq24196_CON8),
	                                (&val),
	                                (kal_uint8)(CON8_VSYS_STAT_MASK),
	                                (kal_uint8)(CON8_VSYS_STAT_SHIFT)
	                          );
	if (g_bq24196_log_en>1)
		dprintf(INFO, "[%s]ret=%d\n", __func__, ret);

	return val;
}

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/
void bq24196_dump_register(void)
{
	int i=0;

	dprintf(CRITICAL, "bq24196_dump_register\r\n");
	for (i=0; i<bq24196_REG_NUM; i++) {
		bq24196_read_byte(i, &bq24196_reg[i]);
		dprintf(CRITICAL, "[0x%x]=0x%x\r\n", i, bq24196_reg[i]);
	}
}
void bq24196_hw_init(void)
{
	bq24196_set_en_hiz(0x0);
	bq24196_set_vindpm(0xA); //VIN DPM check 4.68V
	bq24196_set_reg_rst(0x0);
	bq24196_set_wdt_rst(0x1); //Kick watchdog

	bq24196_set_sys_min(0x5); //Minimum system voltage 3.5V
//	bq24196_set_iprechg(0x3); //Precharge current 512mA   //move to bq24196_charging_enable
	bq24196_set_iterm(0x0); //Termination current 128mA

	bq24196_set_batlowv(0x1); //BATLOWV 3.0V
	bq24196_set_vrechg(0x0); //VRECHG 0.1V (4.108V)
	bq24196_set_en_term(0x1); //Enable termination
	bq24196_set_watchdog(0x1); //WDT 40s
	bq24196_set_en_timer(0x0); //Disable charge timer
	bq24196_set_int_mask(0x0); //Disable fault interrupt

	if (get_mt6325_pmic_chip_version() < PMIC6325_E3_CID_CODE)
		bq24196_set_vreg(0x1F);     //4.0v
	else
		bq24196_set_vreg(0x2C);     //4.2v
}

static CHARGER_TYPE g_chr_type_num = CHARGER_UNKNOWN;
int hw_charging_get_charger_type(void);
extern int g_std_ac_large_current_en;

void bq24196_charging_enable(kal_uint32 bEnable)
{
	int temp_CC_value = 0;
	kal_int32 bat_val = 0;

	if (CHARGER_UNKNOWN == g_chr_type_num && KAL_TRUE == upmu_is_chr_det()) {
		hw_charging_get_charger_type();
		dprintf(CRITICAL, "[BATTERY:bq24196] charger type: %d\n", g_chr_type_num);
	}

	bat_val = get_i_sense_volt(1);
	if (g_chr_type_num == STANDARD_CHARGER) {
		if (g_std_ac_large_current_en==1) {
			temp_CC_value = 2000;
			bq24196_set_iinlim(0x6);  //IN current limit at 2A
			if (bat_val < PRECC_BATVOL)
				bq24196_set_ichg(0x0);  //Pre-Charging Current Limit at 500ma
			else
				bq24196_set_ichg(0x18);  //Fast Charging Current Limit at 2A
		} else {
			temp_CC_value = 1200;
			bq24196_set_iinlim(0x4);  //IN current limit at 1200mA
			if (bat_val < PRECC_BATVOL)
				bq24196_set_ichg(0x0);  //Pre-Charging Current Limit at 500ma
			else
				bq24196_set_ichg(0x8);  //Fast Charging Current Limit at 1A
		}
	} else if (g_chr_type_num == STANDARD_HOST \
	           || g_chr_type_num == CHARGING_HOST \
	           || g_chr_type_num == NONSTANDARD_CHARGER) {
		temp_CC_value = 500;
		bq24196_set_iinlim(0x2); //IN current limit at 500mA
		bq24196_set_ichg(0);  //Fast Charging Current Limit at 500mA
	} else {
		temp_CC_value = 500;
		bq24196_set_iinlim(0x2); //IN current limit at 500mA
		bq24196_set_ichg(0);  //Fast Charging Current Limit at 500mA
	}

	if (KAL_TRUE == bEnable)
		bq24196_set_chg_config(0x1); // charger enable
	else
		bq24196_set_chg_config(0);  // charger disable

	bq24196_set_wdt_rst(1);

	dprintf(INFO, "[BATTERY:bq24196] bq24196_set_ac_current(), CC value(%dmA) \r\n", temp_CC_value);
	dprintf(INFO, "[BATTERY:bq24196] charger enable/disable %d !\r\n", bEnable);
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
	        MT6325_CHR_CON20, upmu_get_reg_value(MT6325_CHR_CON20),
	        MT6325_CHR_CON21, upmu_get_reg_value(MT6325_CHR_CON21)
	       );
}

static void hw_bc11_init(void)
{
	mdelay(300);
	Charger_Detect_Init();

	//RG_bc11_BIAS_EN=1
	mt6325_upmu_set_rg_bc11_bias_en(0x1);
	//RG_bc11_VSRC_EN[1:0]=00
	mt6325_upmu_set_rg_bc11_vsrc_en(0x0);
	//RG_bc11_VREF_VTH = [1:0]=00
	mt6325_upmu_set_rg_bc11_vref_vth(0x0);
	//RG_bc11_CMP_EN[1.0] = 00
	mt6325_upmu_set_rg_bc11_cmp_en(0x0);
	//RG_bc11_IPU_EN[1.0] = 00
	mt6325_upmu_set_rg_bc11_ipu_en(0x0);
	//RG_bc11_IPD_EN[1.0] = 00
	mt6325_upmu_set_rg_bc11_ipd_en(0x0);
	//bc11_RST=1
	mt6325_upmu_set_rg_bc11_rst(0x1);
	//bc11_BB_CTRL=1
	mt6325_upmu_set_rg_bc11_bb_ctrl(0x1);

	mdelay(50);

	if (g_bq24196_log_en>1) {
		dprintf(INFO, "hw_bc11_init() \r\n");
		hw_bc11_dump_register();
	}

}


static U32 hw_bc11_DCD(void)
{
	U32 wChargerAvail = 0;

	//RG_bc11_IPU_EN[1.0] = 10
	mt6325_upmu_set_rg_bc11_ipu_en(0x2);
	//RG_bc11_IPD_EN[1.0] = 01
	mt6325_upmu_set_rg_bc11_ipd_en(0x1);
	//RG_bc11_VREF_VTH = [1:0]=01
	mt6325_upmu_set_rg_bc11_vref_vth(0x1);
	//RG_bc11_CMP_EN[1.0] = 10
	mt6325_upmu_set_rg_bc11_cmp_en(0x2);

	mdelay(80);

	wChargerAvail = mt6325_upmu_get_rgs_bc11_cmp_out();

	if (g_bq24196_log_en>1) {
		dprintf(INFO, "hw_bc11_DCD() \r\n");
		hw_bc11_dump_register();
	}

	//RG_bc11_IPU_EN[1.0] = 00
	mt6325_upmu_set_rg_bc11_ipu_en(0x0);
	//RG_bc11_IPD_EN[1.0] = 00
	mt6325_upmu_set_rg_bc11_ipd_en(0x0);
	//RG_bc11_CMP_EN[1.0] = 00
	mt6325_upmu_set_rg_bc11_cmp_en(0x0);
	//RG_bc11_VREF_VTH = [1:0]=00
	mt6325_upmu_set_rg_bc11_vref_vth(0x0);

	return wChargerAvail;
}


static U32 hw_bc11_stepA1(void)
{
	U32 wChargerAvail = 0;

	//RG_bc11_IPD_EN[1.0] = 01
	mt6325_upmu_set_rg_bc11_ipd_en(0x1);
	//RG_bc11_VREF_VTH = [1:0]=00
	mt6325_upmu_set_rg_bc11_vref_vth(0x0);
	//RG_bc11_CMP_EN[1.0] = 01
	mt6325_upmu_set_rg_bc11_cmp_en(0x1);

	mdelay(80);

	wChargerAvail = mt6325_upmu_get_rgs_bc11_cmp_out();

	if (g_bq24196_log_en>1) {
		dprintf(INFO, "hw_bc11_stepA1() \r\n");
		hw_bc11_dump_register();
	}

	//RG_bc11_IPD_EN[1.0] = 00
	mt6325_upmu_set_rg_bc11_ipd_en(0x0);
	//RG_bc11_CMP_EN[1.0] = 00
	mt6325_upmu_set_rg_bc11_cmp_en(0x0);

	return  wChargerAvail;
}


static U32 hw_bc11_stepA2(void)
{
	U32 wChargerAvail = 0;

	//RG_bc11_VSRC_EN[1.0] = 10
	mt6325_upmu_set_rg_bc11_vsrc_en(0x2);
	//RG_bc11_IPD_EN[1:0] = 01
	mt6325_upmu_set_rg_bc11_ipd_en(0x1);
	//RG_bc11_VREF_VTH = [1:0]=00
	mt6325_upmu_set_rg_bc11_vref_vth(0x0);
	//RG_bc11_CMP_EN[1.0] = 01
	mt6325_upmu_set_rg_bc11_cmp_en(0x1);

	mdelay(80);

	wChargerAvail = mt6325_upmu_get_rgs_bc11_cmp_out();

	if (g_bq24196_log_en>1) {
		dprintf(INFO, "hw_bc11_stepA2() \r\n");
		hw_bc11_dump_register();
	}

	//RG_bc11_VSRC_EN[1:0]=00
	mt6325_upmu_set_rg_bc11_vsrc_en(0x0);
	//RG_bc11_IPD_EN[1.0] = 00
	mt6325_upmu_set_rg_bc11_ipd_en(0x0);
	//RG_bc11_CMP_EN[1.0] = 00
	mt6325_upmu_set_rg_bc11_cmp_en(0x0);

	return  wChargerAvail;
}


static U32 hw_bc11_stepB2(void)
{
	U32 wChargerAvail = 0;

	//RG_bc11_IPU_EN[1:0]=10
	mt6325_upmu_set_rg_bc11_ipu_en(0x2);
	//RG_bc11_VREF_VTH = [1:0]=01
	mt6325_upmu_set_rg_bc11_vref_vth(0x1);
	//RG_bc11_CMP_EN[1.0] = 01
	mt6325_upmu_set_rg_bc11_cmp_en(0x1);

	mdelay(80);

	wChargerAvail = mt6325_upmu_get_rgs_bc11_cmp_out();

	if (g_bq24196_log_en>1) {
		dprintf(INFO, "hw_bc11_stepB2() \r\n");
		hw_bc11_dump_register();
	}

	//RG_bc11_IPU_EN[1.0] = 00
	mt6325_upmu_set_rg_bc11_ipu_en(0x0);
	//RG_bc11_CMP_EN[1.0] = 00
	mt6325_upmu_set_rg_bc11_cmp_en(0x0);
	//RG_bc11_VREF_VTH = [1:0]=00
	mt6325_upmu_set_rg_bc11_vref_vth(0x0);

	return  wChargerAvail;
}


static void hw_bc11_done(void)
{
	//RG_bc11_VSRC_EN[1:0]=00
	mt6325_upmu_set_rg_bc11_vsrc_en(0x0);
	//RG_bc11_VREF_VTH = [1:0]=0
	mt6325_upmu_set_rg_bc11_vref_vth(0x0);
	//RG_bc11_CMP_EN[1.0] = 00
	mt6325_upmu_set_rg_bc11_cmp_en(0x0);
	//RG_bc11_IPU_EN[1.0] = 00
	mt6325_upmu_set_rg_bc11_ipu_en(0x0);
	//RG_bc11_IPD_EN[1.0] = 00
	mt6325_upmu_set_rg_bc11_ipd_en(0x0);
	//RG_bc11_BIAS_EN=0
	mt6325_upmu_set_rg_bc11_bias_en(0x0);

	Charger_Detect_Release();

	if (g_bq24196_log_en>1) {
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

