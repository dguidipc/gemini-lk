#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_pmic.h>
#include <platform/mt_gpt.h>
#include <platform/mt_pmic_wrap_init.h>
#include <printf.h>
#include <platform/upmu_hw.h>
#include <platform/upmu_common.h>
#include <platform/mt_pumpexpress.h>

//==============================================================================
// Global variable
//==============================================================================
int Enable_PMIC_LOG = 1;

CHARGER_TYPE g_ret = CHARGER_UNKNOWN;
int g_charger_in_flag = 0;
int g_first_check=0;

extern int g_R_BAT_SENSE;
extern int g_R_I_SENSE;
extern int g_R_CHARGER_1;
extern int g_R_CHARGER_2;

//==============================================================================
// PMIC-AUXADC related define
//==============================================================================
#define VOLTAGE_FULL_RANGE      1800
#define ADC_PRECISE     32768   // 15 bits

//==============================================================================
// PMIC-AUXADC global variable
//==============================================================================
kal_int32 count_time_out=10000;
void pmic_auxadc_debug(int index);
//==============================================================================
// PMIC access API
//==============================================================================
U32 pmic_read_interface (U32 RegNum, U32 *val, U32 MASK, U32 SHIFT)
{
	U32 return_value = 0;
	U32 pmic_reg = 0;
	U32 rdata;

	//mt_read_byte(RegNum, &pmic_reg);
	return_value= pwrap_wacs2(0, (RegNum), 0, &rdata);
	pmic_reg=rdata;
	if (return_value!=0) {
		dprintf(INFO, "[pmic_read_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		return return_value;
	}
	//dprintf(INFO, "[pmic_read_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg);

	pmic_reg &= (MASK << SHIFT);
	*val = (pmic_reg >> SHIFT);
	//dprintf(INFO, "[pmic_read_interface] val=0x%x\n", *val);

	return return_value;
}

U32 pmic_config_interface (U32 RegNum, U32 val, U32 MASK, U32 SHIFT)
{
	U32 return_value = 0;
	U32 pmic_reg = 0;
	U32 rdata;

	//1. mt_read_byte(RegNum, &pmic_reg);
	return_value= pwrap_wacs2(0, (RegNum), 0, &rdata);
	pmic_reg=rdata;
	if (return_value!=0) {
		dprintf(INFO, "[pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		return return_value;
	}
	//dprintf(INFO, "[pmic_config_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg);

	pmic_reg &= ~(MASK << SHIFT);
	pmic_reg |= (val << SHIFT);

	//2. mt_write_byte(RegNum, pmic_reg);
	return_value= pwrap_wacs2(1, (RegNum), pmic_reg, &rdata);
	if (return_value!=0) {
		dprintf(INFO, "[pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		return return_value;
	}
	//dprintf(INFO, "[pmic_config_interface] write Reg[%x]=0x%x\n", RegNum, pmic_reg);

#if 0
	//3. Double Check
	//mt_read_byte(RegNum, &pmic_reg);
	return_value= pwrap_wacs2(0, (RegNum), 0, &rdata);
	pmic_reg=rdata;
	if (return_value!=0) {
		dprintf(INFO, "[pmic_config_interface] Reg[%x]= pmic_wrap write data fail\n", RegNum);
		return return_value;
	}
	dprintf(INFO, "[pmic_config_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg);
#endif

	return return_value;
}


void upmu_set_reg_value(kal_uint32 reg, kal_uint32 reg_val)
{
	U32 ret=0;

	ret=pmic_config_interface(reg, reg_val, 0xFFFF, 0x0);
}

//==============================================================================
// PMIC Usage APIs
//==============================================================================
U32 get_mt6351_pmic_chip_version (void)
{
	U32 ret=0;
	U32 val=0;

	ret=pmic_read_interface( (U32)(MT6351_SWCID),
	                         (&val),
	                         (U32)(MT6351_PMIC_SWCID_MASK),
	                         (U32)(MT6351_PMIC_SWCID_SHIFT)
	                       );
	if (ret!=0) dprintf(INFO, "%d", ret);
	return val;
}


kal_uint32 mt6351_upmu_get_rgs_chrdet(void)
{
	kal_uint32 ret=0;
	kal_uint32 val=0;


	ret=pmic_read_interface( (kal_uint32)(MT6351_CHR_CON0),
	                         (&val),
	                         (kal_uint32)(MT6351_PMIC_RGS_CHRDET_MASK),
	                         (kal_uint32)(MT6351_PMIC_RGS_CHRDET_SHIFT)
	                       );


	return val;
}

kal_bool upmu_is_chr_det(void)
{
	U32 tmp32=0;
#if defined(MTK_PUMP_EXPRESS_PLUS_SUPPORT)
	static int  rst = 0;
	if (rst == 0) {
		pumpex_reset_adapter();
		#ifdef MTK_RT9466_SUPPORT
		pumpex_reset_adapter_rt();
		#endif		
		
		rst = 1;
	}
	/* reset PE+ adapter back to 5V*/
#endif

#if 0
	tmp32 = 1; // for bring up
#else
	tmp32 = mt6351_upmu_get_rgs_chrdet();
#endif

	dprintf(CRITICAL, "[upmu_is_chr_det] %d\n", tmp32);

	if (tmp32 == 0) {
		return KAL_FALSE;
	} else {
		return KAL_TRUE;
	}
}

kal_bool pmic_chrdet_status(void)
{
	if ( upmu_is_chr_det() == KAL_TRUE ) {
#ifndef USER_BUILD
		dprintf(INFO, "[pmic_chrdet_status] Charger exist\r\n");
#endif

		return KAL_TRUE;
	} else {
#ifndef USER_BUILD
		dprintf(INFO, "[pmic_chrdet_status] No charger\r\n");
#endif

		return KAL_FALSE;
	}
}

int pmic_detect_powerkey(void)
{
	U32 ret=0;
	U32 val=0;

	ret=pmic_read_interface( (U32)(MT6351_TOPSTATUS),
	                         (&val),
	                         (U32)(MT6351_PMIC_PWRKEY_DEB_MASK),
	                         (U32)(MT6351_PMIC_PWRKEY_DEB_SHIFT)
	                       );

	if (Enable_PMIC_LOG>1)
		dprintf(INFO, "%d", ret);

	if (val==1) {
#ifndef USER_BUILD
		dprintf(INFO, "LK pmic powerkey Release\n");
#endif

		return 0;
	} else {
#ifndef USER_BUILD
		dprintf(INFO, "LK pmic powerkey Press\n");
#endif

		return 1;
	}
}

int pmic_detect_homekey(void)
{
	U32 ret=0;
	U32 val=0;

	ret=pmic_read_interface( (U32)(MT6351_TOPSTATUS),
	                         (&val),
	                         (U32)(MT6351_PMIC_HOMEKEY_DEB_MASK),
	                         (U32)(MT6351_PMIC_HOMEKEY_DEB_SHIFT)
	                       );

	if (Enable_PMIC_LOG>1)
		dprintf(INFO, "%d", ret);

	if (val==1) {
#ifndef USER_BUILD
		dprintf(INFO, "LK pmic HOMEKEY Release\n");
#endif

		return 0;
	} else {
#ifndef USER_BUILD
		dprintf(INFO, "LK pmic HOMEKEY Press\n");
#endif

		return 1;
	}
}

kal_uint32 upmu_get_reg_value(kal_uint32 reg)
{
	U32 ret=0;
	U32 temp_val=0;

	ret=pmic_read_interface(reg, &temp_val, 0xFFFF, 0x0);

	if (Enable_PMIC_LOG>1)
		dprintf(INFO, "%d", ret);

	return temp_val;
}

//==============================================================================
// PMIC Init Code
//==============================================================================
void PMIC_INIT_SETTING_V1(void)
{
	//dprintf(INFO, "[LK_PMIC_INIT_SETTING_V1] Done\n");
}

void PMIC_CUSTOM_SETTING_V1(void)
{
	//dprintf(INFO, "[LK_PMIC_CUSTOM_SETTING_V1] Done\n");
}

U32 pmic_init (void)
{
	U32 ret_code = PMIC_TEST_PASS, ret, val;

	dprintf(INFO, "[pmic_init] LK Start..................\n");
	dprintf(INFO, "[pmic_init] MT6351 CHIP Code = 0x%x\n", get_mt6351_pmic_chip_version());

	/*pmic_auxadc_debug(2);*/
	PMIC_INIT_SETTING_V1();
	PMIC_CUSTOM_SETTING_V1();

#if 1
	//mt6311_driver_probe();
#endif
	ret = pmic_read_interface(MT6351_TOP_RST_MISC, &val, 0xFFFF, 0x0);
	dprintf(INFO, "[pmic_init] 0x2b6:0x%x\n",val);

	dprintf(INFO, "[pmic_init] Done\n");
	/*pmic_auxadc_debug(3);*/

	return ret_code;
}

//==============================================================================
// PMIC API for LK : AUXADC
//==============================================================================

void pmic_auxadc_init(void)
{
}

void pmic_auxadc_debug(int index)
{
	int ret_val = 0, val, val1, val2, val3, val4;

	ret_val = pmic_read_interface((U32)(0x0240), (&val), (0xffff), 0);
	ret_val = pmic_read_interface((U32)(0x0282), (&val1), (0xffff), 0);
	ret_val = pmic_read_interface((U32)(0x025e), (&val2), (0xffff), 0);
	ret_val = pmic_read_interface((U32)(0x023a), (&val3), (0xffff), 0);
	ret_val = pmic_read_interface((U32)(0x0ea2), (&val4), (0xffff), 0);
	dprintf(INFO, "[pmic_init]%d 0x0240:0x%x, 0x0282:0x%x, 0x025e:0x%x , 0x023a:0x%x , 0x0ea2:0x%x\n", index, val, val1, val2, val3, val4);
	ret_val = pmic_read_interface(MT6351_TOP_CKPDN_CON2, &val, MT6351_PMIC_TOP_CKPDN_CON2_CLR_MASK, MT6351_PMIC_TOP_CKPDN_CON2_CLR_SHIFT);
	ret_val = pmic_read_interface(MT6351_PMIC_RG_BATON_HT_EN_ADDR, &val1, 0xFFFF, 0x0);
	ret_val = pmic_read_interface(MT6351_LDO_VBIF28_CON0, &val2, 0xFFFF, 0x0);
	ret_val = pmic_read_interface(MT6351_TOP_CKHWEN_CON0, &val3, 0xFFFF, 0x0);
	ret_val = pmic_read_interface(MT6351_TOP_RST_MISC, &val4, 0xFFFF, 0x0);
	dprintf(INFO, "[PMIC_init] _TOP_CKPDN_CON2:0x%x BIF_BAT_CON0:0x%x LDO_VBIF28:0x%x 0x282:0x%x 0x2b6:0x%x\n", val, val1, val2, val3, val4);


}

kal_uint32 PMIC_IMM_GetOneChannelValue(kal_uint8 dwChannel, int deCount, int trimd)
{
	kal_int32 ret=0;
	kal_int32 ret_data;
	kal_int32 r_val_temp=0;
	kal_int32 adc_result=0;
	int count=0;
	kal_uint32 busy;

	/*
	CH0: BATSNS
	CH1: ISENSE
	CH2: VCDT
	CH3: BAT ON
	CH4: PMIC TEMP
	CH5: ACCDET
	CH6:
	CH7: TSX
	CH8:
	CH9:
	CH10:
	CH11:
	CH12:
	CH13:
	CH14:
	CH15:
	BATSNS 3v-4.5v
	ISENSE 1.5-4.5v
	BATON  0-1.8v
	VCDT   4v-14v
	ACCDET 1.8v
	GPS    1.8v

	*/

	if (dwChannel>15)
		return -1;
#if MT6328
	upmu_set_reg_value(0x0a44,0x010a);
	upmu_set_reg_value(0x0cec,0x0000);
	upmu_set_reg_value(0x0d00,0x0010);
	upmu_set_reg_value(0x0f14,0x1290);
#endif
	/*pmic_auxadc_debug(1);*/
	//ret=pmic_config_interface(MT6351_TOP_CLKSQ_SET,(1<<2),0xffff,0);
	ret=pmic_config_interface(MT6351_AUXADC_RQST0_SET,(1<<dwChannel),0xffff,0);

	busy=upmu_get_reg_value(MT6351_AUXADC_STA0);
	udelay(50);

	switch (dwChannel) {
		case 0:
			while (pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_RDY_CH0_BY_AP) != 1 ) {
				mdelay(1);
				if ( (count++) > count_time_out) {
					dprintf(CRITICAL, "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
					break;
				}
			}
			ret_data = pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_OUT_CH0_BY_AP);
			break;
		case 1:
			while (pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_RDY_CH1_BY_AP) != 1 ) {
				mdelay(1);
				if ( (count++) > count_time_out) {
					dprintf(CRITICAL, "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
					break;
				}
			}
			ret_data = pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_OUT_CH1_BY_AP);
			break;
		case 2:
			while (pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_RDY_CH2) != 1 ) {
				mdelay(1);
				if ( (count++) > count_time_out) {
					dprintf(CRITICAL, "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
					break;
				}
			}
			ret_data = pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_OUT_CH2);
			break;
		case 3:
			while (pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_RDY_CH3) != 1 ) {
				mdelay(1);
				if ( (count++) > count_time_out) {
					dprintf(CRITICAL, "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
					break;
				}
			}
			ret_data = pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_OUT_CH3);
			break;
		case 4:
			while (pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_RDY_CH4) != 1 ) {
				mdelay(1);
				if ( (count++) > count_time_out) {
					dprintf(CRITICAL, "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
					break;
				}
			}
			ret_data = pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_OUT_CH4);
			break;
		case 5:
			while (pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_RDY_CH5) != 1 ) {
				mdelay(1);
				if ( (count++) > count_time_out) {
					dprintf(CRITICAL, "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
					break;
				}
			}
			ret_data = pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_OUT_CH5);
			break;
		case 6:
			while (pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_RDY_CH6) != 1 ) {
				mdelay(1);
				if ( (count++) > count_time_out) {
					dprintf(CRITICAL, "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
					break;
				}
			}
			ret_data = pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_OUT_CH6);
			break;
		case 7:
			while (pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_RDY_CH7_BY_AP) != 1 ) {
				mdelay(1);
				if ( (count++) > count_time_out) {
					dprintf(CRITICAL, "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
					break;
				}
			}
			ret_data = pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_OUT_CH7_BY_AP);
			break;
		case 8:
			while (pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_RDY_CH8) != 1 ) {
				mdelay(1);
				if ( (count++) > count_time_out) {
					dprintf(CRITICAL, "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
					break;
				}
			}
			ret_data = pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_OUT_CH8);
			break;
		case 9:
			while (pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_RDY_CH9) != 1 ) {
				mdelay(1);
				if ( (count++) > count_time_out) {
					dprintf(CRITICAL, "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
					break;
				}
			}
			ret_data = pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_OUT_CH9);
			break;
		case 10:
			while (pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_RDY_CH10) != 1 ) {
				mdelay(1);
				if ( (count++) > count_time_out) {
					dprintf(CRITICAL, "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
					break;
				}
			}
			ret_data = pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_OUT_CH10);
			break;
		case 11:
			while (pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_RDY_CH11) != 1 ) {
				mdelay(1);
				if ( (count++) > count_time_out) {
					dprintf(CRITICAL, "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
					break;
				}
			}
			ret_data = pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_OUT_CH11);
			break;
		case 12:
		case 13:
		case 14:
		case 15:
			while (pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_RDY_CH12_15) != 1 ) {
				mdelay(1);
				if ( (count++) > count_time_out) {
					dprintf(CRITICAL, "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
					break;
				}
			}
			ret_data = pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_OUT_CH12_15);
			break;


		default:
			dprintf(CRITICAL, "[AUXADC] Invalid channel value(%d,%d)\n", dwChannel, trimd);

			return -1;
			break;
	}

	switch (dwChannel) {
		case 0:
			r_val_temp = 3;
			adc_result = (ret_data*r_val_temp*VOLTAGE_FULL_RANGE)/32768;
			break;
		case 1:
			r_val_temp = 3;
			adc_result = (ret_data*r_val_temp*VOLTAGE_FULL_RANGE)/32768;
			break;
		case 2:
			r_val_temp = 1;
			adc_result = (ret_data*r_val_temp*VOLTAGE_FULL_RANGE)/4096;
			break;
		case 3:
			r_val_temp = 2;
			adc_result = (ret_data*r_val_temp*VOLTAGE_FULL_RANGE)/4096;
			break;
		case 4:
			r_val_temp = 1;
			adc_result = (ret_data*r_val_temp*VOLTAGE_FULL_RANGE)/4096;
			break;
		case 5:
			r_val_temp = 1;
			adc_result = (ret_data*r_val_temp*VOLTAGE_FULL_RANGE)/4096;
			break;
		case 6:
			r_val_temp = 1;
			adc_result = (ret_data*r_val_temp*VOLTAGE_FULL_RANGE)/4096;
			break;
		case 7:
			r_val_temp = 1;
			adc_result = (ret_data*r_val_temp*VOLTAGE_FULL_RANGE)/32768;
			break;
		case 8:
			r_val_temp = 1;
			adc_result = (ret_data*r_val_temp*VOLTAGE_FULL_RANGE)/4096;
			break;
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
			r_val_temp = 1;
			adc_result = (ret_data*r_val_temp*VOLTAGE_FULL_RANGE)/4096;
			break;
		default:
			dprintf(CRITICAL, "[AUXADC] Invalid channel value(%d,%d)\n", dwChannel, trimd);

			return -1;
			break;
	}



	dprintf(CRITICAL, "[AUXADC] ch=%d raw=%d data=%d \n", dwChannel, ret_data,adc_result);

	//return ret_data;
	return adc_result;

}


//==============================================================================
// PMIC-AUXADC
//==============================================================================



int get_bat_sense_volt(int times)
{
	return PMIC_IMM_GetOneChannelValue(0,times,1);
}

int get_i_sense_volt(int times)
{
	return PMIC_IMM_GetOneChannelValue(1,times,1);
}

#define R_CHARGER_1 330
#define R_CHARGER_2 39


int get_charger_volt(int times)
{
	kal_int32 val;
	val=PMIC_IMM_GetOneChannelValue(2,times,1);
	val = (((R_CHARGER_1+R_CHARGER_2)*100*val)/R_CHARGER_2)/100;
	return val;
}

int get_tbat_volt(int times)
{
	return PMIC_IMM_GetOneChannelValue(3,times,1);
}

#define CUST_R_SENSE         68

int get_charging_current(int times)
{
	int ret;
	kal_int32 ADC_I_SENSE=1;   // 1 measure time
	kal_int32 ADC_BAT_SENSE=1; // 1 measure time
	int ICharging=0;

	ADC_I_SENSE=get_i_sense_volt(1);
	ADC_BAT_SENSE=get_bat_sense_volt(1);

	ICharging = (ADC_I_SENSE - ADC_BAT_SENSE )*1000/CUST_R_SENSE;

	return ICharging;
}

void vibr_Enable_HW(void)
{
	pmic_set_register_value(MT6351_PMIC_RG_VIBR_VOSEL,5);
	//mt6351_upmu_set_rg_vibr_vosel(0x5); // 0x5: 2.8V, 0x6: 3V, 0x7: 3.3V


	//mt6351_upmu_set_rg_vibr_sw_mode(0);
	//mt6351_upmu_set_rg_vibr_fr_ori(1);

	pmic_set_register_value(MT6351_PMIC_RG_VIBR_EN,1);
	//mt6351_upmu_set_rg_vibr_en(1);
}

void vibr_Disable_HW(void)
{
	pmic_set_register_value(MT6351_PMIC_RG_VIBR_EN,0);
	//mt6351_upmu_set_rg_vibr_en(0);
}

void lcm_Enable_HW(int powerVolt)
{
	dprintf(CRITICAL, "[lcm_Enable_HW] powerVolt=%d \n", powerVolt);
	if (powerVolt == 1500)    {pmic_set_register_value(MT6351_PMIC_RG_VCAMA_VOSEL, 0);}
	else if (powerVolt == 1800)    {pmic_set_register_value(MT6351_PMIC_RG_VCAMA_VOSEL, 1);}
	else if (powerVolt == 2500)    {pmic_set_register_value(MT6351_PMIC_RG_VCAMA_VOSEL, 2);}
	else if (powerVolt == 2800)    {pmic_set_register_value(MT6351_PMIC_RG_VCAMA_VOSEL, 3);}
	else {
		dprintf(CRITICAL, "[lcm_Enable_HW] Error Setting %d. DO nothing.\r\n", powerVolt);
		return;
	}

	pmic_set_register_value(MT6351_PMIC_RG_VCAMA_EN, 1);
}

void lcm_Disable_HW(void)
{
	dprintf(CRITICAL, "[lcm_Disable_HW]\n");
	pmic_set_register_value(MT6351_PMIC_RG_VCAMA_EN, 0);
}
