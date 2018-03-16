/*****************************************************************************
*
* Filename:
* ---------
*   bq24261.h
*
* Project:
* --------
*   Android
*
* Description:
* ------------
*   bq24261 header file
*
* Author:
* -------
*
****************************************************************************/

#ifndef _bq24261_SW_H_
#define _bq24261_SW_H_

#define bq24261_CON0      0x00
#define bq24261_CON1      0x01
#define bq24261_CON2      0x02
#define bq24261_CON3      0x03
#define bq24261_CON4      0x04
#define bq24261_CON5      0x05
#define bq24261_CON6      0x06
#define bq24261_REG_NUM 7


/**********************************************************
  *
  *   [MASK/SHIFT]
  *
  *********************************************************/
//CON0
#define CON0_TMR_RST_MASK   0x1
#define CON0_TMR_RST_SHIFT  7

#define CON0_EN_BOOST_MASK   0x1
#define CON0_EN_BOOST_SHIFT  6

#define CON0_STAT_MASK   0x3
#define CON0_STAT_SHIFT  4

#define CON0_EN_SHIPMODE_MASK   0x1
#define CON0_EN_SHIPMODE_SHIFT  3

#define CON0_FAULT_MASK   0x7
#define CON0_FAULT_SHIFT  0

//CON1
#define CON1_RESET_MASK   0x1
#define CON1_RESET_SHIFT  7

#define CON1_IN_LIMIT_MASK   0x7
#define CON1_IN_LIMIT_SHIFT  4

#define CON1_EN_STAT_MASK   0x1
#define CON1_EN_STAT_SHIFT  3

#define CON1_TE_MASK   0x1
#define CON1_TE_SHIFT  2

#define CON1_DIS_CE_MASK   0x1
#define CON1_DIS_CE_SHIFT  1

#define CON1_HZ_MODE_MASK   0x1
#define CON1_HZ_MODE_SHIFT  0

//CON2
#define CON2_VBREG_MASK   0x3F
#define CON2_VBREG_SHIFT  2

#define CON2_MOD_FREQ_MASK   0x3
#define CON2_MOD_FREQ_SHIFT  0

//CON3
#define CON3_VENDER_CODE_MASK   0x7
#define CON3_VENDER_CODE_SHIFT  5

#define CON3_PN_MASK   0x3
#define CON3_PN_SHIFT  3

//CON4
#define CON4_ICHRG_MASK   0x1F
#define CON4_ICHRG_SHIFT  3

#define CON4_ITERM_MASK   0x7
#define CON4_ITERM_SHIFT  0

//CON5
#define CON5_MINSYS_STATUS_MASK   0x1
#define CON5_MINSYS_STATUS_SHIFT  7

#define CON5_VINDPM_STATUS_MASK   0x1
#define CON5_VINDPM_STATUS_SHIFT  6

#define CON5_LOW_CHG_MASK   0x1
#define CON5_LOW_CHG_SHIFT  5

#define CON5_DPDM_EN_MASK   0x1
#define CON5_DPDM_EN_SHIFT  4

#define CON5_CD_STATUS_MASK   0x1
#define CON5_CD_STATUS_SHIFT  3

#define CON5_VINDPM_MASK   0x7
#define CON5_VINDPM_SHIFT  0

//CON6
#define CON6_2XTMR_EN_MASK   0x1
#define CON6_2XTMR_EN_SHIFT  7

#define CON6_TMR_MASK   0x3
#define CON6_TMR_SHIFT  5

#define CON6_BOOST_ILIM_MASK   0x1
#define CON6_BOOST_ILIM_SHIFT  4

#define CON6_TS_EN_MASK   0x1
#define CON6_TS_EN_SHIFT  3

#define CON6_TS_FAULT_MASK   0x3
#define CON6_TS_FAULT_SHIFT  1

#define CON6_VINDPM_OFF_MASK   0x1
#define CON6_VINDPM_OFF_SHIFT  0

/**********************************************************
  *
  *   [Extern Function]
  *
  *********************************************************/
//CON0----------------------------------------------------
extern void bq24261_set_tmr_rst(kal_uint32 val);
extern void bq24261_set_en_boost(kal_uint32 val);
extern kal_uint32 bq24261_get_stat(void);
extern void bq24261_set_en_shipmode(kal_uint32 val);
extern kal_uint32 bq24261_get_fault(void);
//CON1----------------------------------------------------
extern void bq24261_set_reset(kal_uint32 val);
extern void bq24261_set_in_limit(kal_uint32 val);
extern void bq24261_set_en_stat(kal_uint32 val);
extern void bq24261_set_te(kal_uint32 val);
extern void bq24261_set_dis_ce(kal_uint32 val);
extern void bq24261_set_hz_mode(kal_uint32 val);
//CON2----------------------------------------------------
extern void bq24261_set_vbreg(kal_uint32 val);
extern void bq24261_set_mod_freq(kal_uint32 val);
//CON3----------------------------------------------------
extern kal_uint32 bq24261_get_vender_code(void);
extern kal_uint32 bq24261_get_pn(void);
//CON4----------------------------------------------------
extern void bq24261_set_ichg(kal_uint32 val);
extern void bq24261_set_iterm(kal_uint32 val);
//CON5----------------------------------------------------
extern kal_uint32 bq24261_get_minsys_status(void);
extern kal_uint32 bq24261_get_vindpm_status(void);
extern void bq24261_set_low_chg(kal_uint32 val);
extern void bq24261_set_dpdm_en(kal_uint32 val);
extern kal_uint32 bq24261_get_cd_status(void);
extern void bq24261_set_vindpm(kal_uint32 val);
//CON6----------------------------------------------------
extern void bq24261_set_2xtmr_en(kal_uint32 val);
extern void bq24261_set_tmr(kal_uint32 val);
extern void bq24261_set_boost_ilim(kal_uint32 val);
extern void bq24261_set_ts_en(kal_uint32 val);
extern kal_uint32 bq24261_get_ts_fault(void);
extern void bq24261_set_vindpm_off(kal_uint32 val);

//---------------------------------------------------------
extern void bq24261_hw_init(void);
extern void bq24261_charging_enable(kal_uint32 bEnable);
extern void bq24261_dump_register(void);
extern kal_uint32 bq24261_get_chrg_stat(void);

#endif // _bq24261_SW_H_

