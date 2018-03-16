#include <target/board.h>
#ifdef MTK_KERNEL_POWER_OFF_CHARGING
#define CFG_POWER_CHARGING
#endif
#ifdef CFG_POWER_CHARGING
#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_pmic.h>
#include <platform/upmu_hw.h>
#include <platform/upmu_common.h>
#include <platform/boot_mode.h>
#include <platform/mt_gpt.h>
#include <platform/mt_rtc.h>
#include <platform/mt_rtc_hw.h>
#include <platform/mt_pmic_wrap_init.h>
#include <platform/mt_pumpexpress.h>
//#include <platform/mt_disp_drv.h>
//#include <platform/mtk_wdt.h>
//#include <platform/mtk_key.h>
//#include <platform/mt_logo.h>
#include <platform/mt_leds.h>
#include <printf.h>
#include <sys/types.h>
#include <target/cust_battery.h>

#if defined(MTK_BQ24261_SUPPORT)
#include <platform/bq24261.h>
#endif

#if defined(MTK_BQ24296_SUPPORT)
#include <platform/bq24296.h>
#endif

#if defined(MTK_NCP1854_SUPPORT)
#include <platform/ncp1854.h>
#endif

#if defined(MTK_BQ25896_SUPPORT)
#include <platform/bq25890.h>
#endif

#if defined(MTK_CHARGER_INTERFACE)
#include <platform/mtk_charger_intf.h>
static struct mtk_charger_info *primary_mchr;
#endif

#define DLPT_FEATURE_SUPPORT

#define V_CHARGER_MAX 6500              // 6.5 V

#undef printf


/*****************************************************************************
 *  Type define
 ****************************************************************************/
#if defined(CUST_BATTERY_LOWVOL_THRESOLD)
#define BATTERY_LOWVOL_THRESOLD CUST_BATTERY_LOWVOL_THRESOLD
#else
#define BATTERY_LOWVOL_THRESOLD             3450
#endif

#define BAT_LV_NO_CHR 3200
/*****************************************************************************
 *  Global Variable
 ****************************************************************************/
bool g_boot_reason_change = false;

#if defined(STD_AC_LARGE_CURRENT)
int g_std_ac_large_current_en=1;
#else
int g_std_ac_large_current_en=0;
#endif

/*****************************************************************************
 *  Externl Variable
 ****************************************************************************/
extern bool g_boot_menu;
extern void mtk_wdt_restart(void);

void kick_charger_wdt(void)
{
	/*
	//mt6325_upmu_set_rg_chrwdt_td(0x0);           // CHRWDT_TD, 4s
	mt6325_upmu_set_rg_chrwdt_td(0x3);           // CHRWDT_TD, 32s for keep charging for lk to kernel
	mt6325_upmu_set_rg_chrwdt_wr(1);             // CHRWDT_WR
	mt6325_upmu_set_rg_chrwdt_int_en(1);         // CHRWDT_INT_EN
	mt6325_upmu_set_rg_chrwdt_en(1);             // CHRWDT_EN
	mt6325_upmu_set_rg_chrwdt_flag_wr(1);        // CHRWDT_WR
	*/

	pmic_set_register_value(MT6351_PMIC_RG_CHRWDT_TD,3);  // CHRWDT_TD, 32s for keep charging for lk to kernel
	pmic_set_register_value(MT6351_PMIC_RG_CHRWDT_WR,1); // CHRWDT_WR
	pmic_set_register_value(MT6351_PMIC_RG_CHRWDT_INT_EN,1);    // CHRWDT_INT_EN
	pmic_set_register_value(MT6351_PMIC_RG_CHRWDT_EN,1);        // CHRWDT_EN
	pmic_set_register_value(MT6351_PMIC_RG_CHRWDT_FLAG_WR,1);// CHRWDT_WR
}

#if defined(MTK_BATLOWV_NO_PANEL_ON_EARLY)
kal_bool is_low_battery(kal_int32  val)
{
	static UINT8 g_bat_low = 0xFF;

	//low battery only justice once in lk
	if (0xFF != g_bat_low)
		return g_bat_low;
	else
		g_bat_low = FALSE;

#if defined(SWCHR_POWER_PATH)
	if (0 == val)
		val = get_i_sense_volt(1);
#else
	if (0 == val)
		val = get_bat_sense_volt(1);
#endif

	if (val < BATTERY_LOWVOL_THRESOLD) {
		dprintf(INFO, "%s, TRUE\n", __FUNCTION__);
		g_bat_low = 0x1;
	}

	if (FALSE == g_bat_low)
		dprintf(INFO, "%s, FALSE\n", __FUNCTION__);

	return g_bat_low;
}
#endif

void pchr_turn_on_charging(kal_bool bEnable)
{
#ifdef MTK_CHARGER_INTERFACE
	int ret = 0;
	bool enable = bEnable ? true : false;
#endif
	pmic_set_register_value(MT6351_PMIC_RG_USBDL_RST,1);//force leave USBDL mode
	//mt6325_upmu_set_rg_usbdl_rst(1);       //force leave USBDL mode
	pmic_set_register_value(MT6351_PMIC_RG_BC11_RST,1);//BC11_RST
	kick_charger_wdt();

	pmic_set_register_value(MT6351_PMIC_RG_NORM_CS_VTH,0xC);    // CS_VTH, 450mA
	//mt6325_upmu_set_rg_cs_vth(0xC);             // CS_VTH, 450mA
	pmic_set_register_value(MT6351_PMIC_RG_CSDAC_EN,bEnable);
	//mt6325_upmu_set_rg_csdac_en(1);               // CSDAC_EN
	pmic_set_register_value(MT6351_PMIC_RG_NORM_CHR_EN,bEnable);
	//mt6325_upmu_set_rg_chr_en(1);             // CHR_EN

	pmic_set_register_value(MT6351_PMIC_RG_CSDAC_MODE,1);//CSDAC_MODE
	pmic_set_register_value(MT6351_PMIC_RG_CSDAC_EN,1);

#if defined(MTK_BQ24261_SUPPORT)
	bq24261_hw_init();
	bq24261_charging_enable(bEnable);
	bq24261_dump_register();
#endif

#if defined(MTK_BQ24296_SUPPORT)
	bq24296_hw_init();
	bq24296_charging_enable(bEnable);
	bq24296_dump_register();
#endif

#if defined(MTK_NCP1854_SUPPORT)
	ncp1854_hw_init();
	ncp1854_charging_enable(bEnable);
	ncp1854_dump_register();
#endif

#if defined(MTK_BQ25896_SUPPORT)
	bq25890_hw_init();
	bq25890_charging_enable(bEnable);
	bq25890_dump_register();
#endif

#ifdef MTK_CHARGER_INTERFACE
	ret = mtk_charger_enable_charging(primary_mchr, enable);
	if (ret < 0)
		dprintf(CRITICAL, "%s: %s charging failed, ret = %d\n",
			__func__, (enable ? "enable" : "disable"), ret);
	mtk_charger_dump_register(primary_mchr);
	if (ret < 0)
		dprintf(CRITICAL, "%s: dump register failed, ret = %d\n",
			__func__, ret);
#endif

}

void pchr_turn_off_charging(void)
{
	pmic_set_register_value(MT6351_PMIC_RG_CHRWDT_INT_EN,0);// CHRWDT_INT_EN
	pmic_set_register_value(MT6351_PMIC_RG_CHRWDT_EN,0);// CHRWDT_EN
	pmic_set_register_value(MT6351_PMIC_RG_CHRWDT_FLAG_WR,0);// CHRWDT_FLAG
	pmic_set_register_value(MT6351_PMIC_RG_CSDAC_EN,0);// CSDAC_EN
	pmic_set_register_value(MT6351_PMIC_RG_NORM_CHR_EN,0);// CHR_EN
	pmic_set_register_value(MT6351_PMIC_RG_HWCV_EN,0);// RG_HWCV_EN
}

/*
*   Switch Charger Power Path switch
*/
void switch_charger_power_path_enable(kal_bool enable)
{
#if defined(MTK_BQ25896_SUPPORT)
	if (enable == KAL_TRUE) {
		bq25890_set_FORCE_VINDPM(1);
		bq25890_set_VINDPM(0x14);
	} else {
		bq25890_set_FORCE_VINDPM(1);
		bq25890_set_VINDPM(0x7F);
	}
#elif defined(MTK_CHARGER_INTERFACE)
	int ret = 0;
	bool _enable = enable ? true : false;

	ret = mtk_charger_enable_power_path(primary_mchr, _enable);
	if (ret < 0)
		dprintf(CRITICAL, "%s: %s power path failed, ret = %d\n",
			__func__, (_enable ? "enable" : "disable"), ret);
#endif
}

#define rtc_busy_wait()                 \
do {                            \
    while (RTC_Read(RTC_BBPU) & RTC_BBPU_CBUSY);    \
} while (0)

static U16 RTC_Read(U16 addr)
{
	U32 rdata=0;
	pwrap_read((U32)addr, &rdata);
	return (U16)rdata;
}

static void RTC_Write(U16 addr, U16 data)
{
	pwrap_write((U32)addr, (U32)data);
}

static void rtc_write_trigger(void)
{
	RTC_Write(RTC_WRTGR, 1);
	rtc_busy_wait();
}

void lk_set_rtc_to_zero()
{
	int tmp_val1, tmp_val2;
	tmp_val1 = RTC_Read(RTC_AL_HOU);
	RTC_Write(RTC_AL_HOU, (RTC_Read(RTC_AL_HOU)&RTC_AL_HOU_MASK));
	rtc_write_trigger();
	tmp_val2 = RTC_Read(RTC_AL_HOU);
	dprintf(CRITICAL, "LK reset FG_RTC %d => %d\n", tmp_val1>>8, tmp_val2>>8);
}

/*
* enter this function when low battery with charger
* For BQ25896, power path support can provide current and voltage for cell phone to boot directly to kernel.
*/
void check_bat_protect_status()
{
	kal_int32 bat_val = 0;
	int current,chr_volt,cnt=0,i;

#if defined(SWCHR_POWER_PATH)
	bat_val = get_i_sense_volt(5);
#else
	bat_val = get_bat_sense_volt(5);
#endif

	dprintf(INFO, "[%s]: check VBAT=%d mV with %d mV, start charging... \n", __FUNCTION__, bat_val, BATTERY_LOWVOL_THRESOLD);

	while (bat_val < BATTERY_LOWVOL_THRESOLD) {
		mtk_wdt_restart();
		if (upmu_is_chr_det() == KAL_FALSE) {
			if (bat_val < BAT_LV_NO_CHR) {
				dprintf(CRITICAL, "[BATTERY] No Charger, Power OFF !\n");
				mt6575_power_off();
				while (1);
			} else
				break;
		}


		chr_volt= get_charger_volt(1);

		if (chr_volt>V_CHARGER_MAX) {
			dprintf(CRITICAL, "[BATTERY] charger voltage is too high :%d , threshold is %d !\n",chr_volt,V_CHARGER_MAX);
#if defined(SWCHR_POWER_PATH)
			mt6575_power_off();
#endif
			break;
		}

		pmic_set_register_value(PMIC_BATON_TDET_EN, 1);
		pmic_set_register_value(PMIC_RG_BATON_EN, 1);
		if (pmic_get_register_value(PMIC_RGS_BATON_UNDET) == 1) {
			dprintf(CRITICAL, "[BATTERY] No battry plug-in. Power Off.");
			mt6575_power_off();
			break;
		}

		dprintf(CRITICAL, "[%s]: check VBAT=%d mV with %d mV, start charging... \n", __FUNCTION__, bat_val, BATTERY_LOWVOL_THRESOLD);
		pchr_turn_on_charging(KAL_TRUE);
		lk_set_rtc_to_zero();

#if defined(SWCHR_POWER_PATH)
		thread_sleep(10000);
#else
		cnt=0;
		for (i=0; i<10; i++) {
			current=get_charging_current(1);
			chr_volt=get_charger_volt(1);
			if (current<100 && chr_volt<4400) {
				cnt++;
				dprintf(CRITICAL, "[BATTERY] charging current=%d charger volt=%d\n\r",current,chr_volt);
			} else {
				dprintf(CRITICAL, "[BATTERY] charging current=%d charger volt=%d\n\r",current,chr_volt);
				cnt=0;
			}
		}

		if (cnt>=8) {

			dprintf(CRITICAL, "[BATTERY] charging current and charger volt too low !! \n\r",cnt);

			pchr_turn_off_charging();
#ifndef NO_POWER_OFF
			mt6575_power_off();
#endif
			while (1) {
				dprintf(CRITICAL, "If you see the log, please check with RTC power off API\n\r");
			}
		}
		mdelay(50);
#endif



#if defined(SWCHR_POWER_PATH)
#ifndef MTK_NCP1854_SUPPORT /* NCP1854 needs enable charging to have power path */
		pchr_turn_on_charging(KAL_FALSE);
		mdelay(100);
#endif
		bat_val = get_i_sense_volt(5);
#else
		bat_val = get_bat_sense_volt(5);
#endif
		dprintf(CRITICAL, "[%s]: check VBAT=%d mV  \n", __FUNCTION__, bat_val);
	}

	dprintf(CRITICAL, "[%s]: check VBAT=%d mV with %d mV, stop charging... \n", __FUNCTION__, bat_val, BATTERY_LOWVOL_THRESOLD);
}

void mt65xx_bat_init(void)
{
	kal_int32 bat_vol;
#if defined(SWCHR_POWER_PATH) || defined(MTK_PUMP_EXPRESS_PLUS_SUPPORT)
	kal_int32 chr_volt;
	kal_int32 rc = 0;
#endif
	// Low Battery Safety Booting

#if defined(SWCHR_POWER_PATH)
	bat_vol = get_i_sense_volt(1);
#else
	bat_vol = get_bat_sense_volt(1);
#endif

	pchr_turn_on_charging(KAL_TRUE);
	pchr_turn_off_charging();
	dprintf(INFO, "[mt65xx_bat_init] check VBAT=%d mV with %d mV\n", bat_vol, BATTERY_LOWVOL_THRESOLD);

	/* Get charger interface */
#ifdef MTK_CHARGER_INTERFACE
	mtk_charger_init();
	primary_mchr = mtk_charger_get_by_name("primary_charger");
	if (!primary_mchr)
		dprintf(CRITICAL, "%s: get primary charger failed\n", __func__);
#endif


#if defined(MTK_PUMP_EXPRESS_PLUS_SUPPORT)
	/*Try to reset PE+ adapter once abnormal voltage is found*/
	chr_volt = get_charger_volt(1);
	while (chr_volt > V_CHARGER_MAX) {
		dprintf(CRITICAL, "[mt65xx_bat_init] PE+ adpater should be reset to 5V now\n");

		pumpex_reset_adapter_enble(1);
		mdelay(250);
		pumpex_reset_adapter_enble(0);

		#ifdef MTK_RT9466_SUPPORT
		pumpex_reset_adapter_enble_rt(1);
		mdelay(250);
		pumpex_reset_adapter_enble_rt(0);
		#endif
		
		chr_volt = get_charger_volt(1);
		rc++;
		if (rc == 3)
			break;
	}
#endif

	if (g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT && (pmic_get_register_value(MT6351_PMIC_PWRKEY_DEB)==0) ) {
		dprintf(CRITICAL, "[mt65xx_bat_init] KPOC+PWRKEY => change boot mode\n");
		g_boot_reason_change = true;
	}
	rtc_boot_check(false);

#ifndef MTK_DISABLE_POWER_ON_OFF_VOLTAGE_LIMITATION
#ifndef MTK_BATLOWV_NO_PANEL_ON_EARLY
	if (bat_vol < BATTERY_LOWVOL_THRESOLD)
#else
	if (is_low_battery(bat_vol))
#endif
	{
		if (g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT && upmu_is_chr_det() == KAL_TRUE) {
			dprintf(CRITICAL, "[%s] Kernel Low Battery Power Off Charging Mode\n", __func__);
			g_boot_mode = LOW_POWER_OFF_CHARGING_BOOT;

			check_bat_protect_status();

		} else {
			if (bat_vol < BAT_LV_NO_CHR) {
				dprintf(CRITICAL, "[BATTERY] battery voltage(%dmV) <= CLV ! Can not Boot Linux Kernel !! \n\r",bat_vol);
#ifndef NO_POWER_OFF
				mt6575_power_off();
#endif
				while (1) {
					dprintf(CRITICAL, "If you see the log, please check with RTC power off API\n\r");
				}
			}
		}
	}
#endif

#if defined(SWCHR_POWER_PATH)
	chr_volt = get_charger_volt(1);
	if (chr_volt > V_CHARGER_MAX) {
		dprintf(CRITICAL, "Charger Over Voltage:%d\n, power off...", chr_volt);
		mt6575_power_off();
	}
#endif

#if defined(DLPT_FEATURE_SUPPORT)
	fgauge_initialization(NULL);

	/*if (g_boot_mode != META_BOOT && g_boot_mode != FACTORY_BOOT && g_boot_mode != ATE_FACTORY_BOOT) {
		pmic_set_register_value(PMIC_BATON_TDET_EN, 1);
		pmic_set_register_value(PMIC_RG_BATON_EN, 1);
		if (pmic_get_register_value(PMIC_RGS_BATON_UNDET) == 1) {
			dprintf(CRITICAL, "[BATTERY] No battry plug-in. Power Off.");
			mt6575_power_off();
		}
	}*/
	pchr_turn_on_charging(KAL_FALSE);
	/* disable SW charger power path */
	switch_charger_power_path_enable(KAL_FALSE);
	mdelay(50);
	get_dlpt_imix_r();
	/* after get imix, re-enable SW charger power path */
	switch_charger_power_path_enable(KAL_TRUE);
	mdelay(50);
	check_bat_protect_status();
#endif //#if defined(DLPT_FEATURE_SUPPORT)

	return;
}



#if defined(DLPT_FEATURE_SUPPORT)
int imix_r=170;
kal_int32 chip_diff_trim_value_4_0 = 0;
kal_int32 chip_diff_trim_value = 0; // unit = 0.1
#define UNIT_FGCURRENT     (158122)     // 158.122 uA

/* battery meter parameter */
#define CHANGE_TRACKING_POINT
#define CUST_TRACKING_POINT  1
#define CUST_R_SENSE         68
#define CUST_HW_CC          0
#define AGING_TUNING_VALUE   103
#define CUST_R_FG_OFFSET    0

#define OCV_BOARD_COMPESATE 0 //mV
#define R_FG_BOARD_BASE     1000
#define R_FG_BOARD_SLOPE    1000 //slope
#define CAR_TUNE_VALUE      86 //1.00


/* HW Fuel gague  */
#define CURRENT_DETECT_R_FG 10  //1mA
#define MinErrorOffset       1000
#define FG_VBAT_AVERAGE_SIZE 18
#define R_FG_VALUE          10 // mOhm, base is 20

kal_bool g_fg_is_charging = 0;

kal_uint32 ptim_bat_vol=0;
kal_int32  ptim_R_curr=0;


extern kal_uint32 upmu_get_reg_value(kal_uint32 reg);

void get_hw_chip_diff_trim_value(void)
{
#if 1
	kal_int32 reg_val = 0;

	reg_val = upmu_get_reg_value(0xCB8);
	chip_diff_trim_value_4_0 = (reg_val>>7)&0x001F;//chip_diff_trim_value_4_0 = (reg_val>>10)&0x001F;

	dprintf(CRITICAL,"[Chip_Trim] Reg[0xCB8]=0x%x, chip_diff_trim_value_4_0=%d\n", reg_val, chip_diff_trim_value_4_0);
#else
	dprintf(CRITICAL,"[Chip_Trim] need check reg number\n");
#endif

	switch (chip_diff_trim_value_4_0) {
		case 0:
			chip_diff_trim_value = 1000;
			break;
		case 1:
			chip_diff_trim_value = 1005;
			break;
		case 2:
			chip_diff_trim_value = 1010;
			break;
		case 3:
			chip_diff_trim_value = 1015;
			break;
		case 4:
			chip_diff_trim_value = 1020;
			break;
		case 5:
			chip_diff_trim_value = 1025;
			break;
		case 6:
			chip_diff_trim_value = 1030;
			break;
		case 7:
			chip_diff_trim_value = 1036;
			break;
		case 8:
			chip_diff_trim_value = 1041;
			break;
		case 9:
			chip_diff_trim_value = 1047;
			break;
		case 10:
			chip_diff_trim_value = 1052;
			break;
		case 11:
			chip_diff_trim_value = 1058;
			break;
		case 12:
			chip_diff_trim_value = 1063;
			break;
		case 13:
			chip_diff_trim_value = 1069;
			break;
		case 14:
			chip_diff_trim_value = 1075;
			break;
		case 15:
			chip_diff_trim_value = 1081;
			break;
		case 31:
			chip_diff_trim_value = 995;
			break;
		case 30:
			chip_diff_trim_value = 990;
			break;
		case 29:
			chip_diff_trim_value = 985;
			break;
		case 28:
			chip_diff_trim_value = 980;
			break;
		case 27:
			chip_diff_trim_value = 975;
			break;
		case 26:
			chip_diff_trim_value = 970;
			break;
		case 25:
			chip_diff_trim_value = 966;
			break;
		case 24:
			chip_diff_trim_value = 961;
			break;
		case 23:
			chip_diff_trim_value = 956;
			break;
		case 22:
			chip_diff_trim_value = 952;
			break;
		case 21:
			chip_diff_trim_value = 947;
			break;
		case 20:
			chip_diff_trim_value = 943;
			break;
		case 19:
			chip_diff_trim_value = 938;
			break;
		case 18:
			chip_diff_trim_value = 934;
			break;
		case 17:
			chip_diff_trim_value = 930;
			break;
		default:
			dprintf(CRITICAL, "[Chip_Trim] Invalid value(%d)\n", chip_diff_trim_value_4_0);
			break;
	}

	dprintf(CRITICAL, "[Chip_Trim] chip_diff_trim_value=%d\n", chip_diff_trim_value);
}

static kal_uint32 fg_get_data_ready_status(void)
{
	kal_uint32 ret=0;
	kal_uint32 temp_val=0;

	ret=pmic_read_interface(MT6351_FGADC_CON0, &temp_val, 0xFFFF, 0x0);

	dprintf(CRITICAL, "[fg_get_data_ready_status] Reg[0x%x]=0x%x\r\n", MT6351_FGADC_CON0, temp_val);

	temp_val = (temp_val & (MT6351_PMIC_FG_LATCHDATA_ST_MASK << MT6351_PMIC_FG_LATCHDATA_ST_SHIFT)) >> MT6351_PMIC_FG_LATCHDATA_ST_SHIFT;

	return temp_val;
}

kal_int32 use_chip_trim_value(kal_int32 not_trim_val)
{

	kal_int32 ret_val=0;

	ret_val=((not_trim_val*chip_diff_trim_value)/1000);

	dprintf(CRITICAL, "[use_chip_trim_value] %d -> %d\n", not_trim_val, ret_val);

	return ret_val;

}


void fgauge_read_current(void *data)
{

	kal_uint16 uvalue16 = 0;
	kal_int32 dvalue = 0;
	int m = 0;
	uint64_t Temp_Value = 0;
	kal_int32 Current_Compensate_Value=0;
	kal_uint32 ret = 0;

// HW Init
	//(1)    i2c_write (0x60, 0xC8, 0x01); // Enable VA2
	//(2)    i2c_write (0x61, 0x15, 0x00); // Enable FGADC clock for digital
	//(3)    i2c_write (0x61, 0x69, 0x28); // Set current mode, auto-calibration mode and 32KHz clock source
	//(4)    i2c_write (0x61, 0x69, 0x29); // Enable FGADC

//Read HW Raw Data
	//(1)    Set READ command
	ret=pmic_config_interface(MT6351_FGADC_CON0, 0x0200, 0xFF00, 0x0);
	//(2)     Keep i2c read when status = 1 (0x06)
	m=0;
	while ( fg_get_data_ready_status() == 0 ) {
		m++;
		if (m>1000) {
			dprintf(CRITICAL, "[fgauge_read_current] fg_get_data_ready_status timeout 1 !\r\n");
			break;
		}
	}
	//(3)    Read FG_CURRENT_OUT[15:08]
	//(4)    Read FG_CURRENT_OUT[07:00]
	uvalue16 = pmic_get_register_value(MT6351_PMIC_FG_CURRENT_OUT); //mt6325_upmu_get_fg_current_out();
	dprintf(CRITICAL, "[fgauge_read_current] : FG_CURRENT = %x\r\n", uvalue16);
	//(5)    (Read other data)
	//(6)    Clear status to 0
	ret=pmic_config_interface(MT6351_FGADC_CON0, 0x0800, 0xFF00, 0x0);
	//(7)    Keep i2c read when status = 0 (0x08)
	//while ( fg_get_sw_clear_status() != 0 )
	m=0;
	while ( fg_get_data_ready_status() != 0 ) {
		m++;
		if (m>1000) {
			dprintf(CRITICAL, "[fgauge_read_current] fg_get_data_ready_status timeout 2 !\r\n");
			break;
		}
	}
	//(8)    Recover original settings
	ret=pmic_config_interface(MT6351_FGADC_CON0, 0x0000, 0xFF00, 0x0);

//calculate the real world data
	dvalue = (kal_uint32) uvalue16;
	if ( dvalue == 0 ) {
		Temp_Value = (uint64_t) dvalue;
		g_fg_is_charging = KAL_FALSE;
	} else if ( dvalue > 32767 ) { // > 0x8000
		Temp_Value = (uint64_t)(dvalue - 65535);
		Temp_Value = Temp_Value - (Temp_Value*2);
		g_fg_is_charging = KAL_FALSE;
	} else {
		Temp_Value = (uint64_t) dvalue;
		g_fg_is_charging = KAL_TRUE;
	}

	Temp_Value = Temp_Value * UNIT_FGCURRENT;
	//do_div(Temp_Value, 100000);
	Temp_Value=Temp_Value/100000;
	dvalue = (kal_uint32)Temp_Value;

	if ( g_fg_is_charging == KAL_TRUE ) {
		dprintf(CRITICAL, "[fgauge_read_current] current(charging) = %d mA\r\n", dvalue);
	} else {
		dprintf(CRITICAL, "[fgauge_read_current] current(discharging) = %d mA\r\n", dvalue);
	}

// Auto adjust value
	if (R_FG_VALUE != 20) {
		dprintf(CRITICAL, "[fgauge_read_current] Auto adjust value due to the Rfg is %d\n Ori current=%d, ", R_FG_VALUE, dvalue);

		dvalue = (dvalue*20)/R_FG_VALUE;

		dprintf(CRITICAL, "[fgauge_read_current] new current=%d\n", dvalue);
	}

// K current
	if (R_FG_BOARD_SLOPE != R_FG_BOARD_BASE) {
		dvalue = ( (dvalue*R_FG_BOARD_BASE) + (R_FG_BOARD_SLOPE/2) ) / R_FG_BOARD_SLOPE;
	}

// current compensate
	if (g_fg_is_charging == KAL_TRUE) {
		dvalue = dvalue + Current_Compensate_Value;
	} else {
		dvalue = dvalue - Current_Compensate_Value;
	}

	dprintf(CRITICAL, "[fgauge_read_current] ori current=%d\n", dvalue);

	dvalue = ((dvalue*CAR_TUNE_VALUE)/100);

	dvalue = use_chip_trim_value(dvalue);

	dprintf(CRITICAL, "[fgauge_read_current] final current=%d (ratio=%d)\n", dvalue, CAR_TUNE_VALUE);

	*(kal_int32*)(data) = dvalue;


	return;
}

void fgauge_read_IM_current(void *data)
{
	kal_uint16 uvalue16 = 0;
	kal_int32 dvalue = 0;
	int m = 0;
	uint64_t Temp_Value = 0;
	kal_int32 Current_Compensate_Value=0;
	kal_uint32 ret = 0;



	uvalue16 = pmic_get_register_value(MT6351_PMIC_FG_R_CURR);
	dprintf(CRITICAL, "[fgauge_read_IM_current] : FG_CURRENT = %x\r\n", uvalue16);

//calculate the real world data
	dvalue = (kal_uint32) uvalue16;
	if ( dvalue == 0 ) {
		Temp_Value = (uint64_t) dvalue;
		g_fg_is_charging = KAL_FALSE;
	} else if ( dvalue > 32767 ) { // > 0x8000
		Temp_Value = (uint64_t)(dvalue - 65535);
		Temp_Value = Temp_Value - (Temp_Value*2);
		g_fg_is_charging = KAL_FALSE;
	} else {
		Temp_Value = (uint64_t) dvalue;
		g_fg_is_charging = KAL_TRUE;
	}

	Temp_Value = Temp_Value * UNIT_FGCURRENT;
	//do_div(Temp_Value, 100000);
	Temp_Value=Temp_Value/100000;
	dvalue = (kal_uint32)Temp_Value;

	if ( g_fg_is_charging == KAL_TRUE ) {
		dprintf(CRITICAL, "[fgauge_read_IM_current] current(charging) = %d mA\r\n", dvalue);
	} else {
		dprintf(CRITICAL, "[fgauge_read_IM_current] current(discharging) = %d mA\r\n", dvalue);
	}

// Auto adjust value
	if (R_FG_VALUE != 20) {
		dprintf(CRITICAL, "[fgauge_read_IM_current] Auto adjust value due to the Rfg is %d\n Ori current=%d, ", R_FG_VALUE, dvalue);

		dvalue = (dvalue*20)/R_FG_VALUE;

		dprintf(CRITICAL, "[fgauge_read_IM_current] new current=%d\n", dvalue);
	}

// K current
	if (R_FG_BOARD_SLOPE != R_FG_BOARD_BASE) {
		dvalue = ( (dvalue*R_FG_BOARD_BASE) + (R_FG_BOARD_SLOPE/2) ) / R_FG_BOARD_SLOPE;
	}

// current compensate
	if (g_fg_is_charging == KAL_TRUE) {
		dvalue = dvalue + Current_Compensate_Value;
	} else {
		dvalue = dvalue - Current_Compensate_Value;
	}

	dprintf(CRITICAL, "[fgauge_read_IM_current] ori current=%d\n", dvalue);

	dvalue = ((dvalue*CAR_TUNE_VALUE)/100);

	dvalue = use_chip_trim_value(dvalue);

	dprintf(CRITICAL,"[fgauge_read_IM_current] final current=%d (ratio=%d)\n", dvalue, CAR_TUNE_VALUE);

	*(kal_int32*)(data) = dvalue;


	return;
}



void fgauge_initialization(void *data)
{

	kal_uint32 ret=0;
	kal_int32 current_temp = 0;
	int m = 0;

	get_hw_chip_diff_trim_value();

// 1. HW initialization
	//FGADC clock is 32768Hz from RTC
	//Enable FGADC in current mode at 32768Hz with auto-calibration

	//(1)    Enable VA2
	//(2)    Enable FGADC clock for digital
	pmic_set_register_value(MT6351_PMIC_RG_FGADC_ANA_CK_PDN,0);//    mt6325_upmu_set_rg_fgadc_ana_ck_pdn(0);
	pmic_set_register_value(MT6351_PMIC_RG_FGADC_DIG_CK_PDN,0);//    mt6325_upmu_set_rg_fgadc_dig_ck_pdn(0);

	//(3)    Set current mode, auto-calibration mode and 32KHz clock source
	ret=pmic_config_interface(MT6351_FGADC_CON0, 0x0028, 0x00FF, 0x0);
	//(4)    Enable FGADC
	ret=pmic_config_interface(MT6351_FGADC_CON0, 0x0029, 0x00FF, 0x0);

	//reset HW FG
	ret=pmic_config_interface(MT6351_FGADC_CON0, 0x7100, 0xFF00, 0x0);
	dprintf(CRITICAL,"******** [fgauge_initialization] reset HW FG!\n" );

	//set FG_OSR
	ret=pmic_config_interface(MT6351_FGADC_CON11, 0x8, 0xF, 0x0);
	dprintf(CRITICAL, "[fgauge_initialization] Reg[0x%x]=0x%x\n",MT6351_FGADC_CON11, upmu_get_reg_value(MT6351_FGADC_CON11));

	//make sure init finish
	m = 0;
	while (current_temp == 0) {
		fgauge_read_current(&current_temp);
		m++;
		if (m>1000) {
			dprintf(CRITICAL, "[fgauge_initialization] timeout!\r\n");
			break;
		}
	}

	dprintf(CRITICAL, "******** [fgauge_initialization] Done!\n" );


	return ;
}

void do_ptim(void)
{
	kal_uint32 i;
	kal_uint32 vbat_reg;

	//PMICLOG("[do_ptim] start \n");
	//pmic_auxadc_lock();
	//pmic_set_register_value(PMIC_RG_AUXADC_RST,1);
	//pmic_set_register_value(PMIC_RG_AUXADC_RST,0);

	upmu_set_reg_value(0x0eac,0x0006);

	pmic_set_register_value(MT6351_PMIC_AUXADC_IMP_AUTORPT_PRD,6);
	pmic_set_register_value(MT6351_PMIC_RG_AUXADC_SMPS_CK_PDN,0);
	pmic_set_register_value(MT6351_PMIC_RG_AUXADC_SMPS_CK_PDN_HWEN,0);

	pmic_set_register_value(MT6351_PMIC_RG_AUXADC_CK_PDN_HWEN,0);
	pmic_set_register_value(MT6351_PMIC_RG_AUXADC_CK_PDN,0);

	pmic_set_register_value(MT6351_PMIC_AUXADC_CLR_IMP_CNT_STOP,1);
	pmic_set_register_value(MT6351_PMIC_AUXADC_IMPEDANCE_IRQ_CLR,1);

	//restore to initial state
	pmic_set_register_value(MT6351_PMIC_AUXADC_CLR_IMP_CNT_STOP,0);
	pmic_set_register_value(MT6351_PMIC_AUXADC_IMPEDANCE_IRQ_CLR,0);

	//set issue interrupt
	//pmic_set_register_value(MT6351_PMIC_RG_INT_EN_AUXADC_IMP,1);

	pmic_set_register_value(MT6351_PMIC_AUXADC_IMPEDANCE_CHSEL,0);
	pmic_set_register_value(MT6351_PMIC_AUXADC_IMP_AUTORPT_EN,1);
	pmic_set_register_value(MT6351_PMIC_AUXADC_IMPEDANCE_CNT,3);
	pmic_set_register_value(MT6351_PMIC_AUXADC_IMPEDANCE_MODE,1);

//	MT6351_PMICLOG("[do_ptim] end %d %d \n",pmic_get_register_value(MT6351_PMIC_RG_AUXADC_SMPS_CK_PDN),pmic_get_register_value(MT6351_PMIC_RG_AUXADC_SMPS_CK_PDN_HWEN));

	while (pmic_get_register_value(MT6351_PMIC_AUXADC_IMPEDANCE_IRQ_STATUS)==0) {
		//MT6351_PMICLOG("[do_ptim] MT6351_PMIC_AUXADC_IMPEDANCE_IRQ_STATUS= %d \n",pmic_get_register_value(MT6351_PMIC_AUXADC_IMPEDANCE_IRQ_STATUS));
		mdelay(1);
	}

	//disable
	pmic_set_register_value(MT6351_PMIC_AUXADC_IMP_AUTORPT_EN,0);//typo
	pmic_set_register_value(MT6351_PMIC_AUXADC_IMPEDANCE_MODE,0);

	//clear irq
	pmic_set_register_value(MT6351_PMIC_AUXADC_CLR_IMP_CNT_STOP,1);
	pmic_set_register_value(MT6351_PMIC_AUXADC_IMPEDANCE_IRQ_CLR,1);
	pmic_set_register_value(MT6351_PMIC_AUXADC_CLR_IMP_CNT_STOP,0);
	pmic_set_register_value(MT6351_PMIC_AUXADC_IMPEDANCE_IRQ_CLR,0);


	//MT6351_PMICLOG("[do_ptim2] 0xee8=0x%x  0x2c6=0x%x\n", upmu_get_reg_value(0xee8),upmu_get_reg_value(0x2c6));


	//pmic_set_register_value(MT6351_PMIC_RG_INT_STATUS_AUXADC_IMP,1);//write 1 to clear !
	//pmic_set_register_value(MT6351_PMIC_RG_INT_EN_AUXADC_IMP,0);


	vbat_reg=pmic_get_register_value(MT6351_PMIC_AUXADC_ADC_OUT_IMP_AVG);
	ptim_bat_vol=(vbat_reg*3*18000)/32768;

	fgauge_read_IM_current((void *)&ptim_R_curr);

}

void enable_dummy_load(kal_uint32 en)
{
	kal_uint32 reg;

	if (en==1) {
		/*1. disable isink pdn */
		pmic_set_register_value(MT6351_PMIC_RG_DRV_ISINK3_CK_PDN, 0);
		pmic_set_register_value(MT6351_PMIC_RG_DRV_ISINK2_CK_PDN, 0);
		pmic_set_register_value(MT6351_PMIC_RG_DRV_ISINK1_CK_PDN, 0);
		pmic_set_register_value(MT6351_PMIC_RG_DRV_ISINK0_CK_PDN, 0);
		pmic_set_register_value(MT6351_PMIC_RG_DRV_32K_CK_PDN, 0);

		/*1. disable isink pdn */
		pmic_set_register_value(MT6351_PMIC_RG_DRV_CHRIND_CK_PDN, 0);
		pmic_set_register_value(MT6351_PMIC_RG_DRV_ISINK7_CK_PDN, 0);
		pmic_set_register_value(MT6351_PMIC_RG_DRV_ISINK6_CK_PDN, 0);
		pmic_set_register_value(MT6351_PMIC_RG_DRV_ISINK5_CK_PDN, 0);
		pmic_set_register_value(MT6351_PMIC_RG_DRV_ISINK4_CK_PDN, 0);

		/* enable isink step */
		pmic_set_register_value(MT6351_PMIC_ISINK_CH2_STEP, 0x7);
		pmic_set_register_value(MT6351_PMIC_ISINK_CH3_STEP, 0x7);
		pmic_set_register_value(MT6351_PMIC_ISINK_CH6_STEP, 0x7);
		pmic_set_register_value(MT6351_PMIC_ISINK_CH7_STEP, 0x7);

		/*enable isink */
		pmic_set_register_value(MT6351_PMIC_ISINK_CH7_BIAS_EN, 0x1);
		pmic_set_register_value(MT6351_PMIC_ISINK_CH6_BIAS_EN, 0x1);
		pmic_set_register_value(MT6351_PMIC_ISINK_CH3_BIAS_EN, 0x1);
		pmic_set_register_value(MT6351_PMIC_ISINK_CH2_BIAS_EN, 0x1);
		pmic_set_register_value(MT6351_PMIC_ISINK_CHOP7_EN, 0x1);
		pmic_set_register_value(MT6351_PMIC_ISINK_CHOP6_EN, 0x1);
		pmic_set_register_value(MT6351_PMIC_ISINK_CHOP3_EN, 0x1);
		pmic_set_register_value(MT6351_PMIC_ISINK_CHOP2_EN, 0x1);
		pmic_set_register_value(MT6351_PMIC_ISINK_CH7_EN, 0x1);
		pmic_set_register_value(MT6351_PMIC_ISINK_CH6_EN, 0x1);
		pmic_set_register_value(MT6351_PMIC_ISINK_CH3_EN, 0x1);
		pmic_set_register_value(MT6351_PMIC_ISINK_CH2_EN, 0x1);
		/*PMICLOG("[enable dummy load]\n"); */
		pmic_read_interface(0x23a, &reg, 0xffff, 0);
		dprintf(INFO, "[isink 2 [0x23a]=0x%x \n", reg);
		pmic_read_interface(0x258, &reg, 0xffff, 0);
		dprintf(INFO, "[isink 2 [0x258]=0x%x \n", reg);
		pmic_read_interface(0x83c, &reg, 0xffff, 0);
		dprintf(INFO, "[isink 2 [0x83c]=0x%x \n", reg);
		pmic_read_interface(0x83e, &reg, 0xffff, 0);
		dprintf(INFO, "[isink 3 [0x83e]=0x%x] \n", reg);
		pmic_read_interface(0x840, &reg, 0xffff, 0);
		dprintf(INFO, "[isink 6 [0x840]=0x%x] \n", reg);
		pmic_read_interface(0x842, &reg, 0xffff, 0);
		dprintf(INFO, "[isink 7 [0x842]=0x%x] \n", reg);
		pmic_read_interface(0x848, &reg, 0xffff, 0);
		dprintf(INFO, "[isink 7 [0x848]=0x%x] \n", reg);
		pmic_read_interface(0x844, &reg, 0xffff, 0);
		dprintf(INFO, "[isink 7 [0x844]=0x%x] \n", reg);
	} else {
		/*upmu_set_reg_value(0x828,0x0cc0); */
#if MT6328
		pmic_set_register_value(MT6351_PMIC_ISINK_CH2_EN, 0);
		pmic_set_register_value(MT6351_PMIC_ISINK_CH3_EN, 0);
#endif
		pmic_set_register_value(MT6351_PMIC_ISINK_CH7_EN, 0);
		pmic_set_register_value(MT6351_PMIC_ISINK_CH6_EN, 0);
		pmic_set_register_value(MT6351_PMIC_ISINK_CH3_EN, 0);
		pmic_set_register_value(MT6351_PMIC_ISINK_CH2_EN, 0);
		/*pmic_set_register_value(PMIC_RG_VIBR_EN,0); */
		/*PMICLOG("[disable dummy load]\n"); */
	}

}


int get_rac_val(void)
{

	int volt_1=0;
	int volt_2=0;
	int curr_1=0;
	int curr_2=0;
	int rac_cal=0;
	int ret=0;
	kal_bool retry_state = KAL_FALSE;
	int retry_count=0;

	do {
		//adc and fg--------------------------------------------------------
		do_ptim();

		dprintf(INFO, "[1,Trigger ADC PTIM mode] volt1=%d, curr_1=%d\n", ptim_bat_vol, ptim_R_curr);
		volt_1=ptim_bat_vol;
		curr_1=ptim_R_curr;

		dprintf(INFO, "[2,enable dummy load]");
		enable_dummy_load(1);
		/* debug to measure bat volt & Isense */
		/* pmic_set_register_value(MT6351_PMIC_RG_VIBR_EN, 0x1); */
		mdelay(1);
		//Wait --------------------------------------------------------------

		//adc and fg--------------------------------------------------------
		do_ptim();

		dprintf(INFO, "[3,Trigger ADC PTIM mode again]0717 volt2=%d, curr_2=%d\n", ptim_bat_vol, ptim_R_curr);
		volt_2=ptim_bat_vol;
		curr_2=ptim_R_curr;

		//Disable dummy load-------------------------------------------------
		enable_dummy_load(0);
		/* debug to measure bat volt & Isense */
		/*pmic_set_register_value(MT6351_PMIC_RG_VIBR_EN, 0); */

		//Calculate Rac------------------------------------------------------
		if ( (curr_2-curr_1) >= 700 && (curr_2-curr_1) <= 1200 && (volt_1-volt_2)>=80 ) { //40.0mA
			rac_cal=((volt_1-volt_2)*1000)/(curr_2-curr_1); //m-ohm

			if (rac_cal<0) {
				ret = (rac_cal-(rac_cal*2))*1;
			} else {
				ret = rac_cal*1;
			}
		} else {
			ret=-1;
			dprintf(CRITICAL, "[4,Calculate Rac] bypass due to (curr_x-curr_y) < 40mA\n");
		}

		dprintf(INFO, "[5,Calculate Rac] volt_1=%d,volt_2=%d,curr_1=%d,curr_2=%d,rac_cal=%d,ret=%d,retry_count=%d\n",
		        volt_1,volt_2,curr_1,curr_2,rac_cal,ret,retry_count);

		dprintf(CRITICAL, "[6,Calculate Rac] %d,%d,%d,%d,%d,%d,%d\n",
		        volt_1,volt_2,curr_1,curr_2,rac_cal,ret,retry_count);


		//------------------------
		retry_count++;

		if ((retry_count < 3) && (ret == -1)) retry_state = KAL_TRUE;
		else                                 retry_state = KAL_FALSE;

	} while (retry_state == KAL_TRUE);

	return ret;
}


void get_dlpt_imix_r(void)
{
	int rac_val[5],rac_val_avg=0,rac_val_sum=0;
	int volt[5],curr[5],volt_avg=0,curr_avg=0;
	int imix;
	int i;
	int validcnt=0;
	int min=1000,max=0;

	for (i=0; i<5; i++) {
		rac_val[i]=get_rac_val();
		if (rac_val[i]<=min && rac_val[i]!=-1)
			min=rac_val[i];
		if (rac_val[i]>=max)
			max=rac_val[i];
		if (rac_val[i]!=-1) {
			rac_val_sum+=rac_val[i];
			validcnt++;
		}
	}

	if (validcnt>=4) {
		rac_val_sum=rac_val_sum-min-max;
		imix_r=rac_val_sum/(validcnt-2);
	} else if (validcnt!=0) {
		imix_r=rac_val_sum/validcnt;
	}

	dprintf(CRITICAL, "[dlpt_R] %d,%d,%d,%d,%d [%d:%d:%d]%d\n",rac_val[0],rac_val[1],rac_val[2],rac_val[3],rac_val[4],min,max,validcnt,imix_r);

	return;

}
#endif //#if defined(DLPT_FEATURE_SUPPORT)

/*
* this function is called to disable/enable /QON low counter during DRAM calibration.
*/
void disable_BATFET_counter(unsigned long ms)
{
	//bq25890_set_BATFET_RST_EN(0);
	//mdelay(ms);
	//bq25890_set_BATFET_RST_EN(1);
	dprintf(CRITICAL, "[BATTERY] /QON counter resets back to 0 sec...\n\r");
}
#else

#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <printf.h>

int imix_r=170;

void mt65xx_bat_init(void)
{
	dprintf(CRITICAL, "[BATTERY] Skip mt65xx_bat_init !!\n\r");
	dprintf(CRITICAL, "[BATTERY] If you want to enable power off charging, \n\r");
	dprintf(CRITICAL, "[BATTERY] Please #define CFG_POWER_CHARGING!!\n\r");
}
/*
* this function is called to disable/enable /QON low counter during DRAM calibration.
*/
void disable_BATFET_counter(unsigned long ms)
{
	dprintf(CRITICAL, "[BATTERY] /QON counter still counting...\n\r");
}
#endif
