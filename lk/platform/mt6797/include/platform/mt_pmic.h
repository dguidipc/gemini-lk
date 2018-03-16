#ifndef _MT_PMIC_LK_SW_H_
#define _MT_PMIC_LK_SW_H_

#include <platform/mt_typedefs.h>

//==============================================================================
// PMIC Define
//==============================================================================

#define PMIC6351_E1_CID_CODE    0x5110
#define PMIC6351_E2_CID_CODE    0x5120
#define PMIC6351_E3_CID_CODE    0x5130
#define PMIC6351_E4_CID_CODE    0x5140
#define PMIC6351_E5_CID_CODE    0x5150
#define PMIC6351_E6_CID_CODE    0x5160

#if 0
#define VBAT_CHANNEL_NUMBER      7
#define ISENSE_CHANNEL_NUMBER	 6
#define VCHARGER_CHANNEL_NUMBER  4
#define VBATTEMP_CHANNEL_NUMBER  5
#else

#define AUXADC_CHANNEL_MASK	0x1f
#define AUXADC_CHANNEL_SHIFT	0
#define AUXADC_CHIP_MASK	0x03
#define AUXADC_CHIP_SHIFT	5
#define AUXADC_USER_MASK	0x0f
#define AUXADC_USER_SHIFT	8


/* ADC Channel Number */
typedef enum {
	//MT6351
	AUX_BATSNS_AP =		0x000,
	AUX_ISENSE_AP,
	AUX_VCDT_AP,
	AUX_BATON_AP,
	AUX_TSENSE_AP,
	AUX_TSENSE_MD =		0x005,
	AUX_VACCDET_AP =	0x007,
	AUX_VISMPS_AP =		0x00B,
	AUX_ICLASSAB_AP =	0x016,
	AUX_HP_AP =		0x017,
	AUX_CH10_AP =		0x018,
	AUX_VBIF_AP =		0x019,

	AUX_CH0_6311 =		0x020,
	AUX_CH1_6311 =		0x021,

	AUX_ADCVIN0_MD =	0x10F,
	AUX_ADCVIN0_GPS = 	0x20C,
	AUX_CH12 = 		0x1011,
	AUX_CH13 = 		0x2011,
	AUX_CH14 = 		0x3011,
	AUX_CH15 = 		0x4011,
} upmu_adc_chl_list_enum;

typedef enum {
	AP = 0,
	MD,
	GPS,
	AUX_USER_MAX
} upmu_adc_user_list_enum;
typedef enum {
	MT6351_CHIP = 0,
	MT6311_CHIP,
	ADC_CHIP_MAX
} upmu_adc_chip_list_enum;

#endif

//#define VOLTAGE_FULL_RANGE     1800
//#define ADC_PRECISE         32768 // 10 bits

typedef enum {
    CHARGER_UNKNOWN = 0,
    STANDARD_HOST,          // USB : 450mA
    CHARGING_HOST,
    NONSTANDARD_CHARGER,    // AC : 450mA~1A
    STANDARD_CHARGER,       // AC : ~1A
    APPLE_2_1A_CHARGER,     // 2.1A apple charger
    APPLE_1_0A_CHARGER,     // 1A apple charger
    APPLE_0_5A_CHARGER,     // 0.5A apple charger
} CHARGER_TYPE;


//==============================================================================
// PMIC Exported Function
//==============================================================================
extern U32 pmic_read_interface (U32 RegNum, U32 *val, U32 MASK, U32 SHIFT);
extern U32 pmic_config_interface (U32 RegNum, U32 val, U32 MASK, U32 SHIFT);
extern U32 pmic_IsUsbCableIn (void);
extern kal_bool upmu_is_chr_det(void);
extern int pmic_detect_powerkey(void);
extern int pmic_detect_powerkey(void);
extern kal_uint32 upmu_get_reg_value(kal_uint32 reg);
extern void PMIC_DUMP_ALL_Register(void);
extern U32 pmic_init (void);
extern kal_uint32 PMIC_IMM_GetOneChannelValue(kal_uint8 dwChannel, int deCount, int trimd);
extern int get_bat_sense_volt(int times);
extern int get_i_sense_volt(int times);
extern int get_charger_volt(int times);
extern int get_tbat_volt(int times);
extern CHARGER_TYPE mt_charger_type_detection(void);
extern void vibr_Enable_HW(void);
extern void vibr_Disable_HW(void);
extern U32 get_mt6351_pmic_chip_version(void);

//==============================================================================
// PMIC Status Code
//==============================================================================
#define PMIC_TEST_PASS               0x0000
#define PMIC_TEST_FAIL               0xB001
#define PMIC_EXCEED_I2C_FIFO_LENGTH  0xB002
#define PMIC_CHRDET_EXIST            0xB003
#define PMIC_CHRDET_NOT_EXIST        0xB004

//==============================================================================
// PMIC Register Index
//==============================================================================
#include "upmu_hw.h"

#endif // _MT_PMIC_LK_SW_H_

