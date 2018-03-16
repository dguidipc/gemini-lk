#ifndef __CUST_KEY_H__
#define __CUST_KEY_H__

#include<cust_kpd.h>

#define MT65XX_META_KEY		42	/* KEY_2 */
/* MTK_PMIC_RST_KEY is defined in cust_kpd.h */
#define MT_CAMERA_KEY 		9

#ifdef MT65XX_RECOVERY_KEY
#define MT65XX_BOOT_MENU_KEY       MT65XX_RECOVERY_KEY     /* KEY_VOLUMEUP */
#else
#define MT65XX_BOOT_MENU_KEY       17     /* KEY_VOLUMEUP */
#endif

#define MT65XX_MENU_SELECT_KEY     MT65XX_BOOT_MENU_KEY

#ifdef MT65XX_FACTORY_KEY
#define MT65XX_MENU_OK_KEY         MT65XX_FACTORY_KEY     /* KEY_VOLUMEDOWN */
#else
#define MT65XX_MENU_OK_KEY         0     /* KEY_VOLUMEDOWN */
#endif

#endif /* __CUST_KEY_H__ */
