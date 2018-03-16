#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <platform/bq25890.h>
#include <printf.h>
#include <platform/mt_pumpexpress.h>


#if !defined(CONFIG_POWER_EXT)
#include <platform/upmu_common.h>
#endif

int g_log_en=0;

/**********************************************************
  *
  *   [I2C Slave Setting]
  *
  *********************************************************/
#define BQ25890_SLAVE_ADDR_WRITE   0xD6
#define BQ25890_SLAVE_ADDR_READ    0xD7
#define PRECC_BATVOL 2800 //preCC 2.8V
/**********************************************************
  *
  *   [Global Variable]
  *
  *********************************************************/
kal_uint8 bq25890_reg[bq25890_REG_NUM] = {0};
int g_bq25890_hw_exist=0;

/**********************************************************
  *
  *   [I2C Function For Read/Write bq25890]
  *
  *********************************************************/
#define BQ25890_I2C_ID  I2C0
static struct mt_i2c_t bq25890_i2c;

kal_uint32 bq25890_write_byte(kal_uint8 cmd, kal_uint8 writeData)
{
	kal_uint32 ret_code = I2C_OK;
	kal_uint8 write_data[2];
	kal_uint16 len;

	write_data[0]= cmd;
	write_data[1] = writeData;

	bq25890_i2c.id = BQ25890_I2C_ID;
	/* Since i2c will left shift 1 bit, we need to set I2C address to >>1 */
	bq25890_i2c.addr = (BQ25890_SLAVE_ADDR_WRITE >> 1);
	bq25890_i2c.mode = ST_MODE;
	bq25890_i2c.speed = 100;
	len = 2;

	ret_code = i2c_write(&bq25890_i2c, write_data, len);

	if (I2C_OK != ret_code)
		dprintf(INFO, "%s: i2c_write: ret_code: %d\n", __func__, ret_code);

	return ret_code;
}

kal_uint32 bq25890_read_byte(kal_uint8 cmd, kal_uint8 *returnData)
{
	kal_uint32 ret_code = I2C_OK;
	kal_uint16 len;
	*returnData = cmd;

	bq25890_i2c.id = BQ25890_I2C_ID;
	/* Since i2c will left shift 1 bit, we need to set I2C address to >>1 */
	bq25890_i2c.addr = (BQ25890_SLAVE_ADDR_READ >> 1);
	bq25890_i2c.mode = ST_MODE;
	bq25890_i2c.speed = 100;
	len = 1;

	ret_code = i2c_write_read(&bq25890_i2c, returnData, len, len);

	if (I2C_OK != ret_code)
		dprintf(INFO, "%s: i2c_read: ret_code: %d\n", __func__, ret_code);

	return ret_code;
}

/**********************************************************
  *
  *   [Read / Write Function]
  *
  *********************************************************/
kal_uint32 bq25890_read_interface (kal_uint8 RegNum, kal_uint8 *val, kal_uint8 MASK, kal_uint8 SHIFT)
{
	kal_uint8 bq25890_reg = 0;
	kal_uint32 ret = 0;

	dprintf(INFO, "--------------------------------------------------LK\n");

	ret = bq25890_read_byte(RegNum, &bq25890_reg);
	dprintf(INFO, "[bq25890_read_interface] Reg[%x]=0x%x\n", RegNum, bq25890_reg);

	bq25890_reg &= (MASK << SHIFT);
	*val = (bq25890_reg >> SHIFT);

	dprintf(INFO, "[bq25890_read_interface] val=0x%x\n", *val);

	return ret;
}

kal_uint32 bq25890_config_interface (kal_uint8 RegNum, kal_uint8 val, kal_uint8 MASK, kal_uint8 SHIFT)
{
	kal_uint8 bq25890_reg = 0;
	kal_uint32 ret = 0;

	dprintf(INFO, "--------------------------------------------------\n");

	ret = bq25890_read_byte(RegNum, &bq25890_reg);
	dprintf(INFO, "[bq25890_config_interface] Reg[%x]=0x%x\n", RegNum, bq25890_reg);

	bq25890_reg &= ~(MASK << SHIFT);
	bq25890_reg |= (val << SHIFT);

	if (RegNum == bq25890_CON1 && val == 1 && MASK ==CON1_RESET_MASK && SHIFT == CON1_RESET_SHIFT) {
		// RESET bit
	} else if (RegNum == bq25890_CON1) {
		bq25890_reg &= ~0x80;   //RESET bit read returs 1, so clear it
	}


	ret = bq25890_write_byte(RegNum, bq25890_reg);
	dprintf(INFO, "[bq25890_config_interface] write Reg[%x]=0x%x\n", RegNum, bq25890_reg);

	// Check
	//bq25890_read_byte(RegNum, &bq25890_reg);
	//printk("[bq25890_config_interface] Check Reg[%x]=0x%x\n", RegNum, bq25890_reg);

	return ret;
}




//write one register directly
kal_uint32 bq25890_reg_config_interface (kal_uint8 RegNum, kal_uint8 val)
{
	kal_uint32 ret = 0;

	ret = bq25890_write_byte(RegNum, val);

	return ret;
}

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/
//CON0----------------------------------------------------

void bq25890_set_en_hiz(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq25890_config_interface(   (kal_uint8)(bq25890_CON0),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON0_EN_HIZ_MASK),
	                                (kal_uint8)(CON0_EN_HIZ_SHIFT)
	                            );
}

void bq25890_set_en_ilim(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq25890_config_interface(   (kal_uint8)(bq25890_CON0),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON0_EN_ILIM_MASK),
	                                (kal_uint8)(CON0_EN_ILIM_SHIFT)
	                            );
}

void  bq25890_set_iinlim(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq25890_config_interface(     (kal_uint8)(bq25890_CON0),
	                                  (val),
	                                  (kal_uint8)(CON0_IINLIM_MASK),
	                                  (kal_uint8)(CON0_IINLIM_SHIFT)
	                            );

}

//CON1----------------------------------------------------

void bq25890_ADC_start(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq25890_config_interface(   (kal_uint8)(bq25890_CON2),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON2_CONV_START_MASK),
	                                (kal_uint8)(CON2_CONV_START_SHIFT)
	                            );
}

void bq25890_set_ADC_rate(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq25890_config_interface(   (kal_uint8)(bq25890_CON2),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON2_CONV_RATE_MASK),
	                                (kal_uint8)(CON2_CONV_RATE_SHIFT)
	                            );
}

void bq25890_set_vindpm_os(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq25890_config_interface(   (kal_uint8)(bq25890_CON1),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON1_VINDPM_OS_MASK),
	                                (kal_uint8)(CON1_VINDPM_OS_SHIFT)
	                            );
}

//CON2----------------------------------------------------

void bq25890_set_ico_en_start(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq25890_config_interface(   (kal_uint8)(bq25890_CON2),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON2_ICO_EN_MASK),
	                                (kal_uint8)(CON2_ICO_EN_RATE_SHIFT)
	                            );
}



//CON3----------------------------------------------------

void  bq25890_wd_reset(kal_uint32 val)
{
	kal_uint32 ret=0;


	ret=bq25890_config_interface(     (kal_uint8)(bq25890_CON3),
	                                  (val),
	                                  (kal_uint8)(CON3_WD_MASK),
	                                  (kal_uint8)(CON3_WD_SHIFT)
	                            );

}

void  bq25890_otg_en(kal_uint32 val)
{
	kal_uint32 ret=0;


	ret=bq25890_config_interface(     (kal_uint8)(bq25890_CON3),
	                                  (val),
	                                  (kal_uint8)(CON3_OTG_CONFIG_MASK),
	                                  (kal_uint8)(CON3_OTG_CONFIG_SHIFT)
	                            );

}

void  bq25890_chg_en(kal_uint32 val)
{
	kal_uint32 ret=0;


	ret=bq25890_config_interface(     (kal_uint8)(bq25890_CON3),
	                                  (val),
	                                  (kal_uint8)(CON3_CHG_CONFIG_MASK),
	                                  (kal_uint8)(CON3_CHG_CONFIG_SHIFT)
	                            );

}


void  bq25890_set_sys_min(kal_uint32 val)
{
	kal_uint32 ret=0;


	ret=bq25890_config_interface(     (kal_uint8)(bq25890_CON3),
	                                  (val),
	                                  (kal_uint8)(CON3_SYS_V_LIMIT_MASK),
	                                  (kal_uint8)(CON3_SYS_V_LIMIT_SHIFT)
	                            );

}

//CON4----------------------------------------------------

void bq25890_en_pumpx(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq25890_config_interface(   (kal_uint8)(bq25890_CON4),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON4_EN_PUMPX_MASK),
	                                (kal_uint8)(CON4_EN_PUMPX_SHIFT)
	                            );
}


void bq25890_set_ichg(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq25890_config_interface(   (kal_uint8)(bq25890_CON4),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON4_ICHG_MASK),
	                                (kal_uint8)(CON4_ICHG_SHIFT)
	                            );
}



//CON5----------------------------------------------------

void  bq25890_set_iprechg(kal_uint32 val)
{
	kal_uint32 ret=0;


	ret=bq25890_config_interface(     (kal_uint8)(bq25890_CON5),
	                                  (val),
	                                  (kal_uint8)(CON5_IPRECHG_MASK),
	                                  (kal_uint8)(CON5_IPRECHG_SHIFT)
	                            );

}

void  bq25890_set_iterml(kal_uint32 val)
{
	kal_uint32 ret=0;


	ret=bq25890_config_interface(     (kal_uint8)(bq25890_CON5),
	                                  (val),
	                                  (kal_uint8)(CON5_ITERM_MASK),
	                                  (kal_uint8)(CON5_ITERM_SHIFT)
	                            );

}



//CON6----------------------------------------------------

void bq25890_set_vreg(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq25890_config_interface(   (kal_uint8)(bq25890_CON6),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON6_2XTMR_EN_MASK),
	                                (kal_uint8)(CON6_2XTMR_EN_SHIFT)
	                            );
}

void bq25890_set_batlowv(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq25890_config_interface(   (kal_uint8)(bq25890_CON6),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON6_BATLOWV_MASK),
	                                (kal_uint8)(CON6_BATLOWV_SHIFT)
	                            );
}

void bq25890_set_vrechg(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq25890_config_interface(   (kal_uint8)(bq25890_CON6),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON6_VRECHG_MASK),
	                                (kal_uint8)(CON6_VRECHG_SHIFT)
	                            );
}

//CON7----------------------------------------------------


void bq25890_en_term_chg(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq25890_config_interface(   (kal_uint8)(bq25890_CON7),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON7_EN_TERM_CHG_MASK),
	                                (kal_uint8)(CON7_EN_TERM_CHG_SHIFT)
	                            );
}

void bq25890_en_state_dis(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq25890_config_interface(   (kal_uint8)(bq25890_CON7),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON7_STAT_DIS_MASK),
	                                (kal_uint8)(CON7_STAT_DIS_SHIFT)
	                            );
}

void bq25890_set_wd_timer(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq25890_config_interface(   (kal_uint8)(bq25890_CON7),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON7_WTG_TIM_SET_MASK),
	                                (kal_uint8)(CON7_WTG_TIM_SET_SHIFT)
	                            );
}

void bq25890_en_chg_timer(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq25890_config_interface(   (kal_uint8)(bq25890_CON7),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON7_EN_TIM_MASK),
	                                (kal_uint8)(CON7_EN_TIMG_SHIFT)
	                            );
}

void bq25890_set_chg_timer(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq25890_config_interface(   (kal_uint8)(bq25890_CON7),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON7_SET_CHG_TIM_MASK),
	                                (kal_uint8)(CON7_SET_CHG_TIM_SHIFT)
	                            );
}
//CON8---------------------------------------------------
void bq25890_set_thermal_regulation(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq25890_config_interface(   (kal_uint8)(bq25890_CON8),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON8_TREG_MASK),
	                                (kal_uint8)(CON8_TREG_SHIFT)
	                            );
}
void bq25890_set_VBAT_clamp(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq25890_config_interface(   (kal_uint8)(bq25890_CON8),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON8_VCLAMP_MASK),
	                                (kal_uint8)(CON8_VCLAMP_SHIFT)
	                            );
}

void bq25890_set_VBAT_IR_compensation(kal_uint32 val)
{
	kal_uint32 ret=0;

	ret=bq25890_config_interface(   (kal_uint8)(bq25890_CON8),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CON8_BAT_COMP_MASK),
	                                (kal_uint8)(CON8_BAT_COMP_SHIFT)
	                            );

}

//CON9----------------------------------------------------
void bq25890_set_BATFET_DLY(kal_uint32 val)
{

	bq25890_config_interface(   (kal_uint8)(bq25890_CON9),
	                            (kal_uint8)(val),
	                            (kal_uint8)(CON9_BATFET_DLY_MASK),
	                            (kal_uint8)(CON9_BATFET_DLY_SHIFT)
	                        );

}

void bq25890_set_BATFET_RST_EN(kal_uint32 val)
{

	bq25890_config_interface(   (kal_uint8)(bq25890_CON9),
	                            (kal_uint8)(val),
	                            (kal_uint8)(CON9_BATFET_RST_EN_MASK),
	                            (kal_uint8)(CON9_BATFET_RST_EN_SHIFT)
	                        );

}

#if PUMPEXP_IN_LK
void bq25890_pumpx_up(kal_uint32 val)
{
	kal_uint32 ret=0;
	bq25890_en_pumpx(1);
	if (val==1) {
		ret=bq25890_config_interface(   (kal_uint8)(bq25890_CON9),
		                                (kal_uint8)(1),
		                                (kal_uint8)(CON9_PUMPX_UP),
		                                (kal_uint8)(CON9_PUMPX_UP_SHIFT)
		                            );
	} else {
		ret=bq25890_config_interface(   (kal_uint8)(bq25890_CON9),
		                                (kal_uint8)(1),
		                                (kal_uint8)(CON9_PUMPX_DN),
		                                (kal_uint8)(CON9_PUMPX_DN_SHIFT)
		                            );
	}
	/* Input current limit = 400 mA, changes after port detection*/
	bq25890_set_iinlim(8);
	/* CC mode current = 2048 mA*/
	bq25890_set_ichg(0x20);
	msleep(3000);
}
#endif
//CONA----------------------------------------------------
void bq25890_set_boost_ilim(kal_uint32 val)
{
	kal_uint32 ret=0;
	ret=bq25890_config_interface(   (kal_uint8)(bq25890_CONA),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CONA_BOOST_ILIM_MASK),
	                                (kal_uint8)(CONA_BOOST_ILIM_SHIFT)
	                            );
}
void bq25890_set_boost_vlim(kal_uint32 val)
{
	kal_uint32 ret=0;
	ret=bq25890_config_interface(   (kal_uint8)(bq25890_CONA),
	                                (kal_uint8)(val),
	                                (kal_uint8)(CONA_BOOST_VLIM_MASK),
	                                (kal_uint8)(CONA_BOOST_VLIM_SHIFT)
	                            );
}


//CONB----------------------------------------------------


kal_uint32 bq25890_get_vbus_state(void )
{
	kal_uint32 ret=0;
	kal_uint8 val=0;
	ret=bq25890_read_interface(     (kal_uint8)(bq25890_CONB),
	                                (&val),
	                                (kal_uint8)(CONB_VBUS_STAT_MASK),
	                                (kal_uint8)(CONB_VBUS_STAT_SHIFT)
	                          );
	return val;
}


kal_uint32 bq25890_get_chrg_state(void )
{
	kal_uint32 ret=0;
	kal_uint8 val=0;
	ret=bq25890_read_interface(     (kal_uint8)(bq25890_CONB),
	                                (&val),
	                                (kal_uint8)(CONB_CHRG_STAT_MASK),
	                                (kal_uint8)(CONB_CHRG_STAT_SHIFT)
	                          );
	return val;
}

kal_uint32 bq25890_get_pg_state(void )
{
	kal_uint32 ret=0;
	kal_uint8 val=0;
	ret=bq25890_read_interface(     (kal_uint8)(bq25890_CONB),
	                                (&val),
	                                (kal_uint8)(CONB_PG_STAT_MASK),
	                                (kal_uint8)(CONB_PG_STAT_SHIFT)
	                          );
	return val;
}



kal_uint32 bq25890_get_sdp_state(void )
{
	kal_uint32 ret=0;
	kal_uint8 val=0;
	ret=bq25890_read_interface(     (kal_uint8)(bq25890_CONB),
	                                (&val),
	                                (kal_uint8)(CONB_SDP_STAT_MASK),
	                                (kal_uint8)(CONB_SDP_STAT_SHIFT)
	                          );
	return val;
}

kal_uint32 bq25890_get_vsys_state(void )
{
	kal_uint32 ret=0;
	kal_uint8 val=0;
	ret=bq25890_read_interface(     (kal_uint8)(bq25890_CONB),
	                                (&val),
	                                (kal_uint8)(CONB_VSYS_STAT_MASK),

	                                (kal_uint8)(CONB_VSYS_STAT_SHIFT)
	                          );
	return val;
}

//CON0C----------------------------------------------------
kal_uint32 bq25890_get_wdt_state(void )
{
	kal_uint32 ret=0;
	kal_uint8 val=0;
	ret=bq25890_read_interface(     (kal_uint8)(bq25890_CONC),
	                                (&val),
	                                (kal_uint8)(CONB_WATG_STAT_MASK),
	                                (kal_uint8)(CONB_WATG_STAT_SHIFT)
	                          );
	return val;
}

kal_uint32 bq25890_get_boost_state(void )
{
	kal_uint32 ret=0;
	kal_uint8 val=0;
	ret=bq25890_read_interface(     (kal_uint8)(bq25890_CONC),
	                                (&val),
	                                (kal_uint8)(CONB_BOOST_STAT_MASK),
	                                (kal_uint8)(CONB_BOOST_STAT_SHIFT)
	                          );
	return val;
}
kal_uint32 bq25890_get_chrg_fault_state(void)
{
	kal_uint32 ret=0;
	kal_uint8 val=0;
	ret=bq25890_read_interface(     (kal_uint8)(bq25890_CONC),
	                                (&val),
	                                (kal_uint8)(CONB_CHRG_STAT_MASK),
	                                (kal_uint8)(CONB_CHRG_STAT_SHIFT)
	                          );
	return val;
}

kal_uint32 bq25890_get_bat_state(void)
{
	kal_uint32 ret=0;
	kal_uint8 val=0;
	ret=bq25890_read_interface(     (kal_uint8)(bq25890_CONC),
	                                (&val),
	                                (kal_uint8)(CONB_BAT_STAT_MASK),
	                                (kal_uint8)(CONB_BAT_STAT_SHIFT)
	                          );
	return val;
}


//COND
void bq25890_set_FORCE_VINDPM(kal_uint32 val)
{
	kal_uint32 ret=0;
	ret=bq25890_config_interface(     (kal_uint8)(bq25890_COND),
	                                  (kal_uint8)(val),
	                                  (kal_uint8)(COND_FORCE_VINDPM_MASK),
	                                  (kal_uint8)(COND_FORCE_VINDPM_SHIFT)
	                            );
	return val;
}

void bq25890_set_VINDPM(kal_uint32 val)
{
	kal_uint32 ret=0;
	ret=bq25890_config_interface(     (kal_uint8)(bq25890_COND),
	                                  (kal_uint8)(val),
	                                  (kal_uint8)(COND_VINDPM_MASK),
	                                  (kal_uint8)(COND_VINDPM_SHIFT)
	                            );
	return val;
}

//CON12
kal_uint32 bq25890_get_ichg(void )
{
	kal_uint32 ret=0;
	kal_uint8 val=0;
	ret=bq25890_read_interface(     (kal_uint8)(bq25890_CON12),
	                                (&val),
	                                (kal_uint8)(CONB_ICHG_STAT_MASK),
	                                (kal_uint8)(CONB_ICHG_STAT_SHIFT)
	                          );
	return val;
}



//CON13 ///


kal_uint32 bq25890_get_idpm_state(void )
{
	kal_uint32 ret=0;
	kal_uint8 val=0;
	ret=bq25890_read_interface(     (kal_uint8)(bq25890_CON13),
	                                (&val),
	                                (kal_uint8)(CONB_IDPM_STAT_MASK),
	                                (kal_uint8)(CONB_IDPM_STAT_SHIFT)
	                          );
	return val;
}

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/
void bq25890_dump_register(void)
{
	kal_uint8 i=0;
	dprintf(CRITICAL, "[bq25890] ");
	bq25890_ADC_start(0x1);
	for (i=0; i<bq25890_REG_NUM; i++) {
		bq25890_read_byte(i, &bq25890_reg[i]);
		dprintf(CRITICAL, "[0x%x]=0x%x\n", i, bq25890_reg[i]);
	}
	dprintf(CRITICAL, "\n");
}

void bq25890_hw_init(void)
{
	bq25890_wd_reset(1); // wdt reset
	bq25890_otg_en(0);  // Disable OTG boost
	bq25890_en_state_dis(1);
	bq25890_en_chg_timer(1);     // Disable Safty timer

	bq25890_set_iterml(0x2);   // ITERM = TBD 0x02=128mAi
	/*Set VBAT = 4.352 V*/
	bq25890_set_vreg(0x20);
	/*VBAT IR compensation*/
	bq25890_set_VBAT_clamp(0x7);/*4.352V + 0.224 V*/
	bq25890_set_VBAT_IR_compensation(0x4); /*80mohm*/

	/*precharge2cc voltage,BATLOWV, 3.0V*/
	bq25890_set_batlowv(0x1);
	/*PreCC mode*/
	/* precharge current default 0x1:128mA 0xF:1088mA
	 * Note: 1A is forbidden for dead battery
	 */
	bq25890_set_iprechg(0x1);
	/* ICHG 2048mA*/
	bq25890_set_ichg(0x20);
	/*Iinlim = 3.25mA*/
	bq25890_set_iinlim(0x3F);

	//bq25890_set_vindpm_off(0); // VINDPM_OFFSET = 0 : 4.2V ; 1 for 10.1 V
	//bq25890_set_vindpm_os(0x4);   // VINDPM_OFFSET + CODE x 2%
	bq25890_set_FORCE_VINDPM(1); //1 or 0 for absolute or relative vth
	bq25890_set_VINDPM(0x13);    //4.5V
	/*Thermal regulation : TBD*/
	//bq25890_set_ts_en(0);

	bq25890_set_en_hiz(0x0);

	//if(get_mt6351_pmic_chip_version() < PMIC6351_E3_CID_CODE)
	//bq25890_set_vbreg(0x19);  //TBD battery regulation
	//else
	//bq25890_set_vbreg(0x23);  //TBD battery regulation
}

static CHARGER_TYPE g_chr_type_num = CHARGER_UNKNOWN;
int hw_charging_get_charger_type(void);
extern int g_std_ac_large_current_en;

void bq25890_charging_enable(kal_uint32 bEnable)
{
	if (KAL_TRUE == bEnable)
		bq25890_chg_en(0x1); // charger enable
	else
		bq25890_chg_en(0);  // charger disable

	dprintf(CRITICAL, "[BATTERY:bq25890] charger enable/disable %d !\r\n", bEnable);
#if 0
	int temp_CC_value = 0;
	kal_int32 bat_val = 0;

	if (CHARGER_UNKNOWN == g_chr_type_num && KAL_TRUE == upmu_is_chr_det()) {
		hw_charging_get_charger_type();
		dprintf(CRITICAL, "[BATTERY:bq25896] charger type: %d\n", g_chr_type_num);
	}

	bat_val = get_i_sense_volt(1);
	if (g_chr_type_num == STANDARD_CHARGER) {
		if (g_std_ac_large_current_en==1) {
			temp_CC_value = 2000;
			bq25890_set_iinlim(0x28);  //IN current limit at 2A
			if (bat_val < PRECC_BATVOL)
				bq25890_set_ichg(0x0);  //Pre-Charging Current Limit at 500ma
			else
				bq25890_set_ichg(0x18);  //Fast Charging Current Limit at 2A

		} else {
			temp_CC_value = 1200;
			bq25890_set_iinlim(0x4);  //IN current limit at 1200mA or 1.5mA for bq24196
			if (bat_val < PRECC_BATVOL)
				bq25890_set_ichg(0x0);  //Pre-Charging Current Limit at 500ma
			else
				bq25890_set_ichg(0x8);  //Fast Charging Current Limit at 1A
		}
	} else if (g_chr_type_num == STANDARD_HOST \
	           || g_chr_type_num == CHARGING_HOST \
	           || g_chr_type_num == NONSTANDARD_CHARGER) {
		temp_CC_value = 500;
		bq25890_set_iinlim(0x2); //IN current limit at 500mA
		bq25890_set_ichg(0);  //Fast Charging Current Limit at 500mA
	} else {
		temp_CC_value = 500;
		bq25890_set_iinlim(0x2); //IN current limit at 500mA
		bq25890_set_ichg(0);  //Fast Charging Current Limit at 500mA
	}

	if (KAL_TRUE == bEnable)
		bq25890_chg_en(0x1); // charger enable
	else
		bq25890_chg_en(0);  // charger disable

	bq25890_wd_reset(1);

	dprintf(INFO, "[BATTERY:bq24196] bq24196_set_ac_current(), CC value(%dmA) \r\n", temp_CC_value);
	dprintf(INFO, "[BATTERY:bq24196] charger enable/disable %d !\r\n", bEnable);
#endif
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
extern void pmic_set_register_value(PMU_FLAGS_LIST_ENUM flagname,kal_uint32 val);
extern kal_uint16 pmic_get_register_value(PMU_FLAGS_LIST_ENUM flagname);
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
#if CHARGER_DETECTION
	Charger_Detect_Init();
#endif
	//RG_bc11_BIAS_EN=1
	pmic_set_register_value(MT6351_PMIC_RG_BC11_BIAS_EN, 0x1);//mt6351_upmu_set_rg_bc11_bias_en(0x1);
	//RG_bc11_VSRC_EN[1:0]=00
	pmic_set_register_value(MT6351_PMIC_RG_BC11_VSRC_EN, 0x0);//mt6351_upmu_set_rg_bc11_vsrc_en(0x0);
	//RG_bc11_VREF_VTH = [1:0]=00
	pmic_set_register_value(MT6351_PMIC_RG_BC11_VREF_VTH, 0x0);//mt6351_upmu_set_rg_bc11_vref_vth(0x0);
	//RG_bc11_CMP_EN[1.0] = 00
	pmic_set_register_value(MT6351_PMIC_RG_BC11_CMP_EN, 0x0);//mt6351_upmu_set_rg_bc11_cmp_en(0x0);
	//RG_bc11_IPU_EN[1.0] = 00
	pmic_set_register_value(MT6351_PMIC_RG_BC11_IPU_EN, 0x0);//mt6351_upmu_set_rg_bc11_ipu_en(0x0);
	//RG_bc11_IPD_EN[1.0] = 00
	pmic_set_register_value(MT6351_PMIC_RG_BC11_IPD_EN, 0x0);//mt6351_upmu_set_rg_bc11_ipd_en(0x0);
	//bc11_RST=1
	pmic_set_register_value(MT6351_PMIC_RG_BC11_RST, 0x1);//mt6351_upmu_set_rg_bc11_rst(0x1);
	//bc11_BB_CTRL=1
	pmic_set_register_value(MT6351_PMIC_RG_BC11_BB_CTRL, 0x1);//mt6351_upmu_set_rg_bc11_bb_ctrl(0x1);

	mdelay(50);

	if (g_log_en>1) {
		dprintf(INFO, "hw_bc11_init() \r\n");
		hw_bc11_dump_register();
	}

}


static U32 hw_bc11_DCD(void)
{
	U32 wChargerAvail = 0;

	//RG_bc11_IPU_EN[1.0] = 10
	pmic_set_register_value(MT6351_PMIC_RG_BC11_IPU_EN, 0x2);
	//RG_bc11_IPD_EN[1.0] = 01
	pmic_set_register_value(MT6351_PMIC_RG_BC11_IPD_EN, 0x1);//mt6351_upmu_set_rg_bc11_ipd_en(0x1);
	//RG_bc11_VREF_VTH = [1:0]=01
	pmic_set_register_value(MT6351_PMIC_RG_BC11_VREF_VTH, 0x1);//mt6351_upmu_set_rg_bc11_vref_vth(0x1);
	//RG_bc11_CMP_EN[1.0] = 10
	pmic_set_register_value(MT6351_PMIC_RG_BC11_CMP_EN, 0x2);//mt6351_upmu_set_rg_bc11_cmp_en(0x2);

	mdelay(80);

	wChargerAvail = pmic_get_register_value(MT6351_PMIC_RGS_BC11_CMP_OUT);//mt6351_upmu_get_rgs_bc11_cmp_out();

	if (g_log_en>1) {
		dprintf(INFO, "hw_bc11_DCD() \r\n");
		hw_bc11_dump_register();
	}

	//RG_bc11_IPU_EN[1.0] = 00
	pmic_set_register_value(MT6351_PMIC_RG_BC11_IPU_EN, 0x0);//mt6351_upmu_set_rg_bc11_ipu_en(0x0);
	//RG_bc11_IPD_EN[1.0] = 00
	pmic_set_register_value(MT6351_PMIC_RG_BC11_IPD_EN, 0x0);//mt6351_upmu_set_rg_bc11_ipd_en(0x0);
	//RG_bc11_CMP_EN[1.0] = 00
	pmic_set_register_value(MT6351_PMIC_RG_BC11_CMP_EN, 0x0);//mt6351_upmu_set_rg_bc11_cmp_en(0x0);
	//RG_bc11_VREF_VTH = [1:0]=00
	pmic_set_register_value(MT6351_PMIC_RG_BC11_VREF_VTH, 0x0);//mt6351_upmu_set_rg_bc11_vref_vth(0x0);

	return wChargerAvail;
}


static U32 hw_bc11_stepA1(void)
{
	U32 wChargerAvail = 0;

	//RG_bc11_IPD_EN[1.0] = 01
	pmic_set_register_value(MT6351_PMIC_RG_BC11_IPD_EN, 0x1);//mt6351_upmu_set_rg_bc11_ipd_en(0x1);
	//RG_bc11_VREF_VTH = [1:0]=00
	pmic_set_register_value(MT6351_PMIC_RG_BC11_VREF_VTH, 0x0);//mt6351_upmu_set_rg_bc11_vref_vth(0x0);
	//RG_bc11_CMP_EN[1.0] = 01
	pmic_set_register_value(MT6351_PMIC_RG_BC11_CMP_EN, 0x1);//mt6351_upmu_set_rg_bc11_cmp_en(0x1);

	mdelay(80);

	wChargerAvail = pmic_get_register_value(MT6351_PMIC_RGS_BC11_CMP_OUT);//mt6351_upmu_get_rgs_bc11_cmp_out();

	if (g_log_en>1) {
		dprintf(INFO, "hw_bc11_stepA1() \r\n");
		hw_bc11_dump_register();
	}

	//RG_bc11_IPD_EN[1.0] = 00
	pmic_set_register_value(MT6351_PMIC_RG_BC11_IPD_EN, 0x0);//mt6351_upmu_set_rg_bc11_ipd_en(0x0);
	//RG_bc11_CMP_EN[1.0] = 00
	pmic_set_register_value(MT6351_PMIC_RG_BC11_CMP_EN, 0x0);//mt6351_upmu_set_rg_bc11_cmp_en(0x0);

	return  wChargerAvail;
}


static U32 hw_bc11_stepA2(void)
{
	U32 wChargerAvail = 0;

	//RG_bc11_VSRC_EN[1.0] = 10
	pmic_set_register_value(MT6351_PMIC_RG_BC11_VSRC_EN, 0x2);//mt6351_upmu_set_rg_bc11_vsrc_en(0x2);
	//RG_bc11_IPD_EN[1:0] = 01
	pmic_set_register_value(MT6351_PMIC_RG_BC11_IPD_EN, 0x1);//mt6351_upmu_set_rg_bc11_ipd_en(0x1);
	//RG_bc11_VREF_VTH = [1:0]=00
	pmic_set_register_value(MT6351_PMIC_RG_BC11_VREF_VTH, 0x0);//mt6351_upmu_set_rg_bc11_vref_vth(0x0);
	//RG_bc11_CMP_EN[1.0] = 01
	pmic_set_register_value(MT6351_PMIC_RG_BC11_CMP_EN, 0x1);//mt6351_upmu_set_rg_bc11_cmp_en(0x1);

	mdelay(80);

	wChargerAvail = pmic_get_register_value(MT6351_PMIC_RGS_BC11_CMP_OUT);//mt6351_upmu_get_rgs_bc11_cmp_out();

	if (g_log_en>1) {
		dprintf(INFO, "hw_bc11_stepA2() \r\n");
		hw_bc11_dump_register();
	}

	//RG_bc11_VSRC_EN[1:0]=00
	pmic_set_register_value(MT6351_PMIC_RG_BC11_VSRC_EN, 0x2);//mt6351_upmu_set_rg_bc11_vsrc_en(0x0);
	//RG_bc11_IPD_EN[1.0] = 00
	pmic_set_register_value(MT6351_PMIC_RG_BC11_IPD_EN, 0x0);//mt6351_upmu_set_rg_bc11_ipd_en(0x0);
	//RG_bc11_CMP_EN[1.0] = 00
	pmic_set_register_value(MT6351_PMIC_RG_BC11_CMP_EN, 0x0);//mt6351_upmu_set_rg_bc11_cmp_en(0x0);

	return  wChargerAvail;
}


static U32 hw_bc11_stepB2(void)
{
	U32 wChargerAvail = 0;

	//RG_bc11_IPU_EN[1:0]=10
	pmic_set_register_value(MT6351_PMIC_RG_BC11_IPU_EN, 0x2);//mt6351_upmu_set_rg_bc11_ipu_en(0x2);
	//RG_bc11_VREF_VTH = [1:0]=01
	pmic_set_register_value(MT6351_PMIC_RG_BC11_VREF_VTH, 0x1);//mt6351_upmu_set_rg_bc11_vref_vth(0x1);
	//RG_bc11_CMP_EN[1.0] = 01
	pmic_set_register_value(MT6351_PMIC_RG_BC11_CMP_EN, 0x1);//mt6351_upmu_set_rg_bc11_cmp_en(0x1);

	mdelay(80);

	wChargerAvail = pmic_get_register_value(MT6351_PMIC_RGS_BC11_CMP_OUT);//mt6351_upmu_get_rgs_bc11_cmp_out();

	if (g_log_en>1) {
		dprintf(INFO, "hw_bc11_stepB2() \r\n");
		hw_bc11_dump_register();
	}

	//RG_bc11_IPU_EN[1.0] = 00
	pmic_set_register_value(MT6351_PMIC_RG_BC11_IPU_EN, 0x0);//mt6351_upmu_set_rg_bc11_ipu_en(0x0);
	//RG_bc11_CMP_EN[1.0] = 00
	pmic_set_register_value(MT6351_PMIC_RG_BC11_CMP_EN, 0x0);//mt6351_upmu_set_rg_bc11_cmp_en(0x0);
	//RG_bc11_VREF_VTH = [1:0]=00
	pmic_set_register_value(MT6351_PMIC_RG_BC11_VREF_VTH, 0x0); //mt6351_upmu_set_rg_bc11_vref_vth(0x0);

	return  wChargerAvail;
}


static void hw_bc11_done(void)
{
	//RG_bc11_VSRC_EN[1:0]=00
	pmic_set_register_value(MT6351_PMIC_RG_BC11_VSRC_EN, 0x0);//mt6351_upmu_set_rg_bc11_vsrc_en(0x0);
	//RG_bc11_VREF_VTH = [1:0]=0
	pmic_set_register_value(MT6351_PMIC_RG_BC11_VREF_VTH, 0x0);//mt6351_upmu_set_rg_bc11_vref_vth(0x0);
	//RG_bc11_CMP_EN[1.0] = 00
	pmic_set_register_value(MT6351_PMIC_RG_BC11_CMP_EN, 0x0);//mt6351_upmu_set_rg_bc11_cmp_en(0x0);
	//RG_bc11_IPU_EN[1.0] = 00
	pmic_set_register_value(MT6351_PMIC_RG_BC11_IPU_EN, 0x0);//mt6351_upmu_set_rg_bc11_ipu_en(0x0);
	//RG_bc11_IPD_EN[1.0] = 00
	pmic_set_register_value(MT6351_PMIC_RG_BC11_IPD_EN, 0x0);//mt6351_upmu_set_rg_bc11_ipd_en(0x0);
	//RG_bc11_BIAS_EN=0
	pmic_set_register_value(MT6351_PMIC_RG_BC11_BIAS_EN, 0x0);//mt6351_upmu_set_rg_bc11_bias_en(0x0);

#if CHARGER_DETECTION
	Charger_Detect_Release();
#endif
	if (g_log_en>1) {
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

void pumpex_reset_adapter(void)
{
	bq25890_set_iinlim(0x00);
	mdelay(250);
	dprintf(CRITICAL, "PE+ adapter reset back to 5v.\r\n");
}

void pumpex_reset_adapter_enble(int en)
{
	if(en == 1)
		bq25890_set_iinlim(0x00);
	else
		bq25890_set_iinlim(0x3F);
}
#endif

