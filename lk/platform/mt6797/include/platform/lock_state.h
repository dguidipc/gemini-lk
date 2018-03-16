#ifndef _LOCK_STATE_H_
#define _LOCK_STATE_H_

/* LKS means "LocK State" */
typedef enum {
	LKS_DEFAULT                = 0x01,
	LKS_MP_DEFAULT,
	LKS_UNLOCK,
	LKS_LOCK,
	LKS_VERIFIED,
	LKS_CUSTOM,
} LKS;

/* LKCS means "LocK Crtitical State" */
typedef enum {
	LKCS_UNLOCK                 = 0x01,
	LKCS_LOCK,
} LKCS;

/* Secure boot runtime switch (preseved switch for customization) */
typedef enum {
	SBOOT_RUNTIME_OFF        = 0,
	SBOOT_RUNTIME_ON         = 1
} SBOOT_RUNTIME;

#endif