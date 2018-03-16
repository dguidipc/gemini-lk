#ifndef _SEC_POLICY_CONFIG_COMMON_H_
#define _SEC_POLICY_CONFIG_COMMON_H_

/*Policy Mask*/
#define DL_FORBIDDEN_BIT  0
#define VFY_BIT           1
#define HASH_BIND_BIT     2

#define CREATE_POLICY_ENTRY(bind, verify, dl) ((bind << HASH_BIND_BIT) | (verify << VFY_BIT) | (dl << DL_FORBIDDEN_BIT))
#define DL_FORBIDDEN_BIT_SET(policy) ((policy >> DL_FORBIDDEN_BIT) & 0x1)
#define VFY_BIT_SET(policy) ((policy >> VFY_BIT) & 0x1)
#define HASH_BIND_BIT_SET(policy) ((policy >> HASH_BIND_BIT) & 0x1)

#define PL_SW_ID          0
#define LK_SW_ID          0
#define LOGO_SW_ID        0
#define BOOT_SW_ID        0
#define RECOVERY_SW_ID    0
#define SYSTEM_SW_ID      0
#define TEE1_SW_ID        0
#define TEE2_SW_ID        0
#define SCP1_SW_ID        0
#define SCP2_SW_ID        0
#define OEMKEYSTORE_SW_ID 0
#define KEYSTORE_SW_ID    0
#define USERDATA_SW_ID    0
#define MD1IMG_SW_ID      0
#define MD1DSP_SW_ID      0
#define MD1ARM7_SW_ID     0
#define MD3IMG_SW_ID      0


/*Image Binding Hash*/
#define PL_BINDING_HASH  "fdd62730afd983f367b267037d1668c164ab51568485ba305621cc28d6268d96"
#define LK_BINDING_HASH  0
#define LOGO_BINDING_HASH  0
#define BOOT_BINDING_HASH  0
#define RECOVERY_BINDING_HASH  0
#define SYSTEM_BINDING_HASH  0
#define TEE1_BINDING_HASH 0
#define TEE2_BINDING_HASH 0
#define SCP1_BINDING_HASH 0
#define SCP2_BINDING_HASH 0
#define OEMKEYSTORE_BINDING_HASH 0
#define KEYSTORE_BINDING_HASH 0
#define USERDATA_BINDING_HASH 0
#define MD1IMG_BINDING_HASH 0
#define MD1DSP_BINDING_HASH 0
#define MD1ARM7_BINDING_HASH 0
#define MD3IMG_BINDING_HASH 0

/*Image Binding Hash*/
#define  CUST1_BINDING_HASH         0
#define  CUST2_BINDING_HASH         0

/***********************************
CUSTOM IMG Config
************************************/
#define  CUST1_IMG_NAME        ""
#define  CUST2_IMG_NAME        ""

#endif
