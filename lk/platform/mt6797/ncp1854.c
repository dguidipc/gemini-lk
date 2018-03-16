#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_i2c.h>
#include <platform/mt_gpio.h>
#include <platform/mt_pmic.h>
#include <platform/ncp1854.h>
#include <printf.h>

#if !defined(CONFIG_POWER_EXT)
#include <platform/upmu_common.h>
#endif

int g_ncp1854_log_en=0;

/**********************************************************
  *
  *   [I2C Slave Setting]
  *
  *********************************************************/
#define NCP1854_SLAVE_ADDR_WRITE    0x6C
#define NCP1854_SLAVE_ADDR_Read 0x6D

/**********************************************************
  *
  *   [Global Variable]
  *
  *********************************************************/
#define NCP1854_REG_NUM 18

#ifndef NCP1854_BUSNUM
#define NCP1854_BUSNUM 1
#endif

#define PRECC_BATVOL 2800 //preCC 2.8V

/* #ifndef GPIO_SWCHARGER_EN_PIN
#define GPIO_SWCHARGER_EN_PIN 65
#endif  */
kal_uint8 ncp1854_reg[NCP1854_REG_NUM] = {0};


/**********************************************************
  *
  *   [I2C Function For Read/Write ncp1854]
  *
  *********************************************************/
#ifndef NCP1854_I2C_ID
#define NCP1854_I2C_ID  I2C0
#endif
static struct mt_i2c_t ncp1854_i2c;

kal_uint32 ncp1854_write_byte(kal_uint8 addr, kal_uint8 value)
{
	kal_uint32 ret_code = I2C_OK;
	kal_uint8 write_data[2];
	kal_uint16 len;

	write_data[0]= addr;
	write_data[1] = value;

	ncp1854_i2c.id = NCP1854_I2C_ID;
	/* Since i2c will left shift 1 bit, we need to set NCP1854 I2C address to >>1 */
	ncp1854_i2c.addr = (NCP1854_SLAVE_ADDR_WRITE >> 1);
	ncp1854_i2c.mode = ST_MODE;
	ncp1854_i2c.speed = 100;
	len = 2;

	ret_code = i2c_write(&ncp1854_i2c, write_data, len);

	if (I2C_OK != ret_code)
		dprintf(INFO, "%s: i2c_write: ret_code: %d\n", __func__, ret_code);

	return ret_code;
}

kal_uint32 ncp1854_read_byte (kal_uint8 addr, kal_uint8 *dataBuffer)
{
	kal_uint32 ret_code = I2C_OK;
	kal_uint16 len;
	*dataBuffer = addr;

	ncp1854_i2c.id = NCP1854_I2C_ID;
	/* Since i2c will left shift 1 bit, we need to set NCP1854 I2C address to >>1 */
	ncp1854_i2c.addr = (NCP1854_SLAVE_ADDR_Read >> 1);
	ncp1854_i2c.mode = ST_MODE;
	ncp1854_i2c.speed = 100;
	len = 1;

	ret_code = i2c_write_read(&ncp1854_i2c, dataBuffer, len, len);

	if (I2C_OK != ret_code)
		dprintf(INFO, "%s: i2c_read: ret_code: %d\n", __func__, ret_code);

	return ret_code;
}


/**********************************************************
  *
  *   [Read / Write Function]
  *
  *********************************************************/
kal_uint32 ncp1854_read_interface (kal_uint8 RegNum, kal_uint8 *val, kal_uint8 MASK, kal_uint8 SHIFT)
{
	kal_uint8 ncp1854_reg = 0;
	int ret = 0;

	dprintf(INFO, "--------------------------------------------------\n");

	ret = ncp1854_read_byte(RegNum, &ncp1854_reg);
	dprintf(INFO, "[ncp1854_read_interface] Reg[%x]=0x%x\n", RegNum, ncp1854_reg);

	ncp1854_reg &= (MASK << SHIFT);
	*val = (ncp1854_reg >> SHIFT);
	dprintf(INFO, "[ncp1854_read_interface] Val=0x%x\n", *val);

	return ret;

}

kal_uint32 ncp1854_config_interface (kal_uint8 RegNum, kal_uint8 val, kal_uint8 MASK, kal_uint8 SHIFT)
{
	kal_uint8 ncp1854_reg = 0;
	int ret = 0;

	dprintf(INFO, "--------------------------------------------------\n");

	ret = ncp1854_read_byte(RegNum, &ncp1854_reg);
	//dprintf(INFO, "[ncp1854_config_interface] Reg[%x]=0x%x\n", RegNum, ncp1854_reg);

	ncp1854_reg &= ~(MASK << SHIFT);
	ncp1854_reg |= (val << SHIFT);

	if (RegNum == NCP1854_CON1 && val == 1 && MASK ==CON1_REG_RST_MASK && SHIFT == CON1_REG_RST_SHIFT) {
		// RESET bit
	} else if (RegNum == NCP1854_CON1) {
		ncp1854_reg &= ~0x80;   //RESET bit read returs 1, so clear it
	}

	ret = ncp1854_write_byte(RegNum, ncp1854_reg);
	//dprintf(INFO, "[ncp18546_config_interface] Write Reg[%x]=0x%x\n", RegNum, ncp1854_reg);

	// Check
	//ncp1854_read_byte(RegNum, &ncp1854_reg);
	//dprintf(INFO, "[ncp1854_config_interface] Check Reg[%x]=0x%x\n", RegNum, ncp1854_reg);

	return ret;

}

/**********************************************************
  *
  *   [ncp1854 Function]
  *
  *********************************************************/
//CON0
kal_uint32 ncp1854_get_chip_status(void)
{
	kal_uint32 ret=0;
	kal_uint8 val=0;

	ret=ncp1854_read_interface((kal_uint8)(NCP1854_CON0),
	                           (&val),
	                           (kal_uint8)(CON0_STATE_MASK),
	                           (kal_uint8)(CON0_STATE_SHIFT)
	                          );
	return val;
}

kal_uint32 ncp1854_get_batfet(void)
{
	kal_uint32 ret=0;
	kal_uint8 val=0;

	ret=ncp1854_read_interface((kal_uint8)(NCP1854_CON0),
	                           (&val),
	                           (kal_uint8)(CON0_BATFET_MASK),
	                           (kal_uint8)(CON0_BATFET_SHIFT)
	                          );
	return val;
}

//CON1
void ncp1854_set_reset(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface((kal_uint8)(NCP1854_CON1),
	                             (kal_uint8)(val),
	                             (kal_uint8)(CON1_REG_RST_MASK),
	                             (kal_uint8)(CON1_REG_RST_SHIFT)
	                            );
}

void ncp1854_set_chg_en(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface((kal_uint8)(NCP1854_CON1),
	                             (kal_uint8)(val),
	                             (kal_uint8)(CON1_CHG_EN_MASK),
	                             (kal_uint8)(CON1_CHG_EN_SHIFT)
	                            );
}

void ncp1854_set_otg_en(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface((kal_uint8)(NCP1854_CON1),
	                             (kal_uint8)(val),
	                             (kal_uint8)(CON1_OTG_EN_MASK),
	                             (kal_uint8)(CON1_OTG_EN_SHIFT)
	                            );
	return val;
}

void ncp1854_set_ntc_en(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface((kal_uint8)(NCP1854_CON1),
	                             (kal_uint8)(val),
	                             (kal_uint8)(CON1_NTC_EN_MASK),
	                             (kal_uint8)(CON1_NTC_EN_SHIFT)
	                            );
}

void ncp1854_set_tj_warn_opt(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface((kal_uint8)(NCP1854_CON1),
	                             (kal_uint8)(val),
	                             (kal_uint8)(CON1_TJ_WARN_OPT_MASK),
	                             (kal_uint8)(CON1_TJ_WARN_OPT_SHIFT)
	                            );
	return val;
}

void ncp1854_set_jeita_opt(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface((kal_uint8)(NCP1854_CON1),
	                             (kal_uint8)(val),
	                             (kal_uint8)(CON1_JEITA_OPT_MASK),
	                             (kal_uint8)(CON1_JEITA_OPT_SHIFT)
	                            );
	return val;
}

void ncp1854_set_tchg_rst(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface(   (kal_uint8)(NCP1854_CON1),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON1_TCHG_RST_MASK),
	                                (kal_uint8)(CON1_TCHG_RST_SHIFT)
	                            );
}

void ncp1854_set_int_mask(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface((kal_uint8)(NCP1854_CON1),
	                             (kal_uint8)(val),
	                             (kal_uint8)(CON1_INT_MASK_MASK),
	                             (kal_uint8)(CON1_INT_MASK_SHIFT)
	                            );
	return val;
}

//CON2
void ncp1854_set_wdto_dis(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface((kal_uint8)(NCP1854_CON2),
	                             (kal_uint8)(val),
	                             (kal_uint8)(CON2_WDTO_DIS_MASK),
	                             (kal_uint8)(CON2_WDTO_DIS_SHIFT)
	                            );
}

void ncp1854_set_chgto_dis(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface((kal_uint8)(NCP1854_CON2),
	                             (kal_uint8)(val),
	                             (kal_uint8)(CON2_CHGTO_DIS_MASK),
	                             (kal_uint8)(CON2_CHGTO_DIS_SHIFT)
	                            );
}

void ncp1854_set_pwr_path(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface((kal_uint8)(NCP1854_CON2),
	                             (kal_uint8)(val),
	                             (kal_uint8)(CON2_PWR_PATH_MASK),
	                             (kal_uint8)(CON2_PWR_PATH_SHIFT)
	                            );
}

void ncp1854_set_trans_en(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface((kal_uint8)(NCP1854_CON2),
	                             (kal_uint8)(val),
	                             (kal_uint8)(CON2_TRANS_EN_MASK),
	                             (kal_uint8)(CON2_TRANS_EN_SHIFT)
	                            );
}

void ncp1854_set_factory_mode(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface((kal_uint8)(NCP1854_CON2),
	                             (kal_uint8)(val),
	                             (kal_uint8)(CON2_FCTRY_MOD_MASK),
	                             (kal_uint8)(CON2_FCTRY_MOD_SHIFT)
	                            );
}

void ncp1854_set_iinset_pin_en(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface((kal_uint8)(NCP1854_CON2),
	                             (kal_uint8)(val),
	                             (kal_uint8)(CON2_IINSET_PIN_EN_MASK),
	                             (kal_uint8)(CON2_IINSET_PIN_EN_SHIFT)
	                            );
}

void ncp1854_set_iinlim_en(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface((kal_uint8)(NCP1854_CON2),
	                             (kal_uint8)(val),
	                             (kal_uint8)(CON2_IINLIM_EN_MASK),
	                             (kal_uint8)(CON2_IINLIM_EN_SHIFT)
	                            );
}

void ncp1854_set_aicl_en(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface((kal_uint8)(NCP1854_CON2),
	                             (kal_uint8)(val),
	                             (kal_uint8)(CON2_AICL_EN_MASK),
	                             (kal_uint8)(CON2_AICL_EN_SHIFT)
	                            );
}

//CON8
kal_uint32 ncp1854_get_vfet_ok(void)
{
	kal_uint32 ret=0;
	kal_uint8 val=0;

	ret=ncp1854_read_interface((kal_uint8)(NCP1854_CON8),
	                           (&val),
	                           (kal_uint8)(CON8_VFET_OK_MASK),
	                           (kal_uint8)(CON8_VFET_OK_SHIFT)
	                          );
	return val;
}


//CON14
void ncp1854_set_ctrl_vbat(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface((kal_uint8)(NCP1854_CON14),
	                             (kal_uint8)(val),
	                             (kal_uint8)(CON14_CTRL_VBAT_MASK),
	                             (kal_uint8)(CON14_CTRL_VBAT_SHIFT)
	                            );
}

//CON15
void ncp1854_set_ichg_high(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface((kal_uint8)(NCP1854_CON15),
	                             (kal_uint8)(val),
	                             (kal_uint8)(CON15_ICHG_HIGH_MASK),
	                             (kal_uint8)(CON15_ICHG_HIGH_SHIFT)
	                            );
}

void ncp1854_set_ieoc(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface((kal_uint8)(NCP1854_CON15),
	                             (kal_uint8)(val),
	                             (kal_uint8)(CON15_IEOC_MASK),
	                             (kal_uint8)(CON15_IEOC_SHIFT)
	                            );
}

void ncp1854_set_ichg(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface((kal_uint8)(NCP1854_CON15),
	                             (kal_uint8)(val),
	                             (kal_uint8)(CON15_ICHG_MASK),
	                             (kal_uint8)(CON15_ICHG_SHIFT)
	                            );
}

//CON16
void ncp1854_set_iweak(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface((kal_uint8)(NCP1854_CON16),
	                             (kal_uint8)(val),
	                             (kal_uint8)(CON16_IWEAK_MASK),
	                             (kal_uint8)(CON16_IWEAK_SHIFT)
	                            );
}

void ncp1854_set_ctrl_vfet(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface((kal_uint8)(NCP1854_CON16),
	                             (kal_uint8)(val),
	                             (kal_uint8)(CON16_CTRL_VFET_MASK),
	                             (kal_uint8)(CON16_CTRL_VFET_SHIFT)
	                            );
}

void ncp1854_set_iinlim(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface((kal_uint8)(NCP1854_CON16),
	                             (kal_uint8)(val),
	                             (kal_uint8)(CON16_IINLIM_MASK),
	                             (kal_uint8)(CON16_IINLIM_SHIFT)
	                            );
}

//CON17

void ncp1854_set_iinlim_ta(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=ncp1854_config_interface((kal_uint8)(NCP1854_CON17),
	                             (kal_uint8)(val),
	                             (kal_uint8)(CON17_IINLIM_TA_MASK),
	                             (kal_uint8)(CON17_IINLIM_TA_SHIFT)
	                            );
}


/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/
void ncp1854_dump_register(void)
{
	int i=0;
	for (i=0; i<NCP1854_REG_NUM; i++) {
		if ((i == 3) || (i == 4) || (i == 5) || (i == 6)) //do not dump read clear status register
			continue;
		if ((i == 10) || (i == 11) || (i == 12) || (i == 13)) //do not dump interrupt mask bit register
			continue;
		ncp1854_read_byte(i, &ncp1854_reg[i]);
		dprintf(CRITICAL, "[ncp1854_dump_register] Reg[0x%X]=0x%X\n", i, ncp1854_reg[i]);
	}

}
void ncp1854_hw_init()
{
	dprintf(CRITICAL, "[ncp1854] ChargerHwInit_ncp1854\n" );

	kal_uint32 ncp1854_status;
	ncp1854_status = ncp1854_get_chip_status();

	ncp1854_set_otg_en(0x0);
	ncp1854_set_trans_en(0);
	ncp1854_set_tj_warn_opt(0x1);
//  ncp1854_set_int_mask(0x0); //disable all interrupt
	ncp1854_set_int_mask(0x1); //enable all interrupt for boost mode status monitor
	// ncp1854_set_tchg_rst(0x1); //reset charge timer
#ifdef NCP1854_PWR_PATH
	ncp1854_set_pwr_path(0x1);
#else
	ncp1854_set_pwr_path(0x0);
#endif

	if ((ncp1854_status == 0x8) || (ncp1854_status == 0x9) || (ncp1854_status == 0xA)) //WEAK WAIT, WEAK SAFE, WEAK CHARGE
		ncp1854_set_ctrl_vbat(0x1C); //VCHG = 4.0V

	ncp1854_set_ieoc(0x4); // terminate current = 200mA for ICS optimized suspend power

	ncp1854_set_iweak(0x3); //weak charge current = 300mA

	ncp1854_set_ctrl_vfet(0x3); // VFET = 3.4V
}


static CHARGER_TYPE g_chr_type_num = CHARGER_UNKNOWN;
int hw_charging_get_charger_type(void);
extern int g_std_ac_large_current_en;

void ncp1854_charging_enable(kal_uint32 bEnable)
{
	int temp_CC_value = 0;
	kal_int32 bat_val = 0;

	if (CHARGER_UNKNOWN == g_chr_type_num && KAL_TRUE == upmu_is_chr_det()) {
		hw_charging_get_charger_type();
		dprintf(CRITICAL, "[BATTERY:ncp1854] charger type: %d\n", g_chr_type_num);
	}

	bat_val = get_i_sense_volt(1);
	if (g_chr_type_num == STANDARD_CHARGER) {
		if (g_std_ac_large_current_en==1) {
			temp_CC_value = 2000;
			ncp1854_set_iinlim_ta(0xF); //IN current limit at 2A
			ncp1854_set_iinlim_en(0x1); //IN current limit enable

			if (bat_val < PRECC_BATVOL) {
				ncp1854_set_ichg_high(0x0);  //ICHG
				ncp1854_set_ichg(0x1);  //Pre-Charging Current Limit at 500mA
			} else {
				ncp1854_set_ichg_high(0);  //ICHG
				ncp1854_set_ichg(0xF);  //Fast Charging Current Limit at 1.9A
			}

		} else {
			temp_CC_value = 1000;
			ncp1854_set_iinlim_ta(0xA); //IN current limit at 1.5A
			ncp1854_set_iinlim_en(0x1); //IN current limit enable

			if (bat_val < PRECC_BATVOL) {
				ncp1854_set_ichg_high(0x0);  //ICHG
				ncp1854_set_ichg(0x1);  //Pre-Charging Current Limit at 500mA
			} else {
				ncp1854_set_ichg_high(0);  //ICHG
				ncp1854_set_ichg(0x6);  //Fast Charging Current Limit at 1A
			}
		}
	} else if (g_chr_type_num == STANDARD_HOST \
	           || g_chr_type_num == CHARGING_HOST \
	           || g_chr_type_num == NONSTANDARD_CHARGER) {
		temp_CC_value = 500;
		ncp1854_set_iinlim_ta(0); //IINLIM
		ncp1854_set_iinlim(0x1); //IN current limit at 500mA
		ncp1854_set_iinlim_en(0x1); //IN current limit enable

		ncp1854_set_ichg_high(0x0);    //ICHG
		ncp1854_set_ichg(0x1);  //Pre-Charging Current Limit at 500mA

	} else {
		temp_CC_value = 500;
		ncp1854_set_iinlim_ta(0); //IINLIM
		ncp1854_set_iinlim(0x1); //IN current limit at 500mA
		ncp1854_set_iinlim_en(0x1); //IN current limit enable

		ncp1854_set_ichg_high(0x0);    //ICHG
		ncp1854_set_ichg(0x1);  //Pre-Charging Current Limit at 500mA

	}

	if (KAL_TRUE == bEnable)
		ncp1854_set_chg_en(0x1); // charger enable
	else
		ncp1854_set_chg_en(0); // charger disable

	ncp1854_set_tchg_rst(0x1);

	dprintf(INFO, "[BATTERY:ncp1854] ncp1854_set_ac_current(), CC value(%dmA) \r\n", temp_CC_value);
	dprintf(INFO, "[BATTERY:ncp1854] charger enable/disable %d !\r\n", bEnable);
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

	if (g_ncp1854_log_en>1) {
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

	if (g_ncp1854_log_en>1) {
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

	if (g_ncp1854_log_en>1) {
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

	if (g_ncp1854_log_en>1) {
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

	if (g_ncp1854_log_en>1) {
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

	if (g_ncp1854_log_en>1) {
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
				dprintf(INFO, "step B2 :	Charging Host!\r\n");
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

