#ifndef _MT_SPM_
#define _MT_SPM_

#include <platform/mt_reg_base.h>
#include <platform/mt_typedefs.h>

#define SPM_BASE SLEEP_BASE
/**************************************
 * Define and Declare
 **************************************/
#define INFRA_TOPAXI_PROTECTEN   (INFRACFG_AO_BASE + 0x0220)
#define INFRA_TOPAXI_PROTECTSTA1 (INFRACFG_AO_BASE + 0x0228)

#define SPM_REG(offset)         (SPM_BASE + offset)
#define POWERON_CONFIG_EN           SPM_REG(0x0000)
#define PWR_STATUS          SPM_REG(0x0180) /* correct */
#define PWR_STATUS_2ND  SPM_REG(0x0184) /* correct */
#define MD1_PWR_CON         SPM_REG(0x0320) /* correct */
#define MD_EXT_BUCK_ISO     SPM_REG(0x0390)


/**************************************
 * Macro and Inline
 **************************************/
#define spm_read(addr)          DRV_Reg32(addr)
#define spm_write(addr, val)        DRV_WriteReg32(addr, val)

#define  SPM_PROJECT_CODE    0xB16
/* Define MTCMOS power control */
#define PWR_RST_B                        (0x1 << 0)
#define PWR_ISO                          (0x1 << 1)
#define PWR_ON                           (0x1 << 2)
#define PWR_ON_2ND                       (0x1 << 3)
#define PWR_CLK_DIS                      (0x1 << 4)
#define SRAM_CKISO                       (0x1 << 5)
#define SRAM_ISOINT_B                    (0x1 << 6)

/* Define MTCMOS Bus Protect Mask */
#define MD1_PROT_MASK                    ((0x1 << 3)|(0x1 << 4)|(0x1 << 5)|(0x1 << 7)|(0x1 << 10)|(0x1 << 16))

/* Define MTCMOS Power Status Mask */
#define MD1_PWR_STA_MASK                 (0x1 << 0)

/* Define Non-CPU SRAM Mask */
#define MD1_SRAM_PDN                     (0x1 << 8)
#define MD1_SRAM_PDN_ACK                 (0x0 << 12)

#define STA_POWER_DOWN  0
#define STA_POWER_ON    1

extern spm_mtcmos_ctrl_md1(int state);

#endif
