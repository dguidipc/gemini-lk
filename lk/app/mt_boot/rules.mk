LOCAL_DIR := $(GET_LOCAL_DIR)

MT_BOOT_OBJ_DIR := $(BOOTLOADER_OUT)/build-$(PROJECT)/app/mt_boot

ifeq ($(CUSTOM_SEC_AUTH_SUPPORT), yes)
	OBJS += \
		$(TO_ROOT)../../../mediatek/custom/common/security/fastboot/cust_auth.o
endif

OBJS += \
	$(LOCAL_DIR)/mt_boot.o \
	$(LOCAL_DIR)/decompressor.o\
	$(LOCAL_DIR)/fastboot.o \
	$(LOCAL_DIR)/sec_hrid.o \
	$(LOCAL_DIR)/sys_commands.o\

build_mt_ramdump := no
ifneq ($(TARGET_BUILD_VARIANT),user)
ifeq ($(ARCH_HAVE_MT_RAMDUMP),yes)
	build_mt_ramdump := yes
endif
endif

ifeq ($(build_mt_ramdump),yes)
OBJS += \
	$(LOCAL_DIR)/aee/aee.o \
	$(LOCAL_DIR)/aee/kdump_elf.o \
	$(LOCAL_DIR)/aee/kdump_sd.o \
	$(LOCAL_DIR)/aee/kdump_ext4.o \
	$(LOCAL_DIR)/aee/kdump_null.o \
	$(LOCAL_DIR)/aee/kdump_zip.o \
	$(LOCAL_DIR)/aee/mrdump_elf64.o \
	$(LOCAL_DIR)/aee/mrdump_sddev.o \
	$(LOCAL_DIR)/aee/reboot_record.o
else
OBJS += \
	$(LOCAL_DIR)/aee/dummy.o
endif

ifeq ($(KEDUMP_MINI), yes)
OBJS += $(LOCAL_DIR)/aee/KEDump.o \
	$(LOCAL_DIR)/aee/platform_debug.o
endif

OBJS += \
	$(LOCAL_DIR)/aee/ram_console.o

FASTBOOT_USE_G_ORIGINAL_PROTOCOL := yes

ifeq ($(FASTBOOT_USE_G_ORIGINAL_PROTOCOL), yes)
	SECRO_TYPE := GMP
	OBJS += \
	$(LOCAL_DIR)/dl_commands.o

ifeq ($(PLATFORM_FASTBOOT_EMPTY_STORAGE), yes)
	OBJS += \
	$(LOCAL_DIR)/blockheader.o
endif

	CFLAGS += -DUSE_G_ORIGINAL_PROTOCOL
else
	SECRO_TYPE :=
	OBJS += \
	$(LOCAL_DIR)/sparse_state_machine.o\
	$(LOCAL_DIR)/download_commands.o

endif

ifeq ($(MTK_SECURITY_SW_SUPPORT), yes)
ifeq ($(MTK_SEC_FASTBOOT_UNLOCK_SUPPORT), yes)
	OBJS += $(LOCAL_DIR)/sec_unlock.o
endif
endif

ifeq ($(BUILD_SEC_LIB),yes)
AUTH_DIR  := $(LOCAL_DIR)/auth
SEC_DIR   := $(LOCAL_DIR)/sec
DEVINFO_DIR   := $(LOCAL_DIR)/devinfo
SEC_PLAT_DIR   := $(LOCAL_DIR)/platform
EXPORT_INC_DIR   := $(LOCAL_DIR)/export_inc
INCLUDES += -I$(AUTH_DIR)/inc -I$(SEC_DIR)/inc -I$(SEC_PLAT_DIR) -I$(DEVINFO_DIR) -I$(EXPORT_INC_DIR)
AUTH_LOCAL_OBJS := $(patsubst $(AUTH_DIR)/%.c,$(AUTH_DIR)/%.o,$(wildcard $(AUTH_DIR)/*.c))
SEC_LOCAL_OBJS  := $(patsubst $(SEC_DIR)/%.c,$(SEC_DIR)/%.o,$(wildcard $(SEC_DIR)/*.c))
DEVINFO_LOCAL_OBJS  := $(patsubst $(DEVINFO_DIR)/%.c,$(DEVINFO_DIR)/%.o,$(wildcard $(DEVINFO_DIR)/*.c))
SEC_PLAT_LOCAL_OBJS  := $(patsubst $(SEC_PLAT_DIR)/%.c,$(SEC_PLAT_DIR)/%.o,$(wildcard $(SEC_PLAT_DIR)/*.c))
AUTH_OBJS := $(patsubst $(AUTH_DIR)/%.c,$(MT_BOOT_OBJ_DIR)/auth/%.o,$(wildcard $(AUTH_DIR)/*.c))
SEC_OBJS  := $(patsubst $(SEC_DIR)/%.c,$(MT_BOOT_OBJ_DIR)/sec/%.o,$(wildcard $(SEC_DIR)/*.c))
DEVINFO_OBJS  := $(patsubst $(DEVINFO_DIR)/%.c,$(MT_BOOT_OBJ_DIR)/devinfo/%.o,$(wildcard $(DEVINFO_DIR)/*.c))
SEC_PLAT_OBJS  := $(patsubst $(SEC_PLAT_DIR)/%.c,$(MT_BOOT_OBJ_DIR)/platform/%.o,$(wildcard $(SEC_PLAT_DIR)/*.c))
OBJS += $(AUTH_LOCAL_OBJS) $(SEC_LOCAL_OBJS) $(SEC_PLAT_LOCAL_OBJS) $(DEVINFO_LOCAL_OBJS)
endif

ifeq ($(BUILD_HW_CRYPTO_LIB),yes)
HW_CRYPTO_DIR  := $(LOCAL_DIR)/hw_crypto
INCLUDES += -I$(HW_CRYPTO_DIR)/inc
HW_CRYPTO_LOCAL_OBJS := $(patsubst $(HW_CRYPTO_DIR)/%.c,$(HW_CRYPTO_DIR)/%.o,$(wildcard $(HW_CRYPTO_DIR)/*.c))
HW_CRYPTO_OBJS := $(patsubst $(HW_CRYPTO_DIR)/%.c,$(MT_BOOT_OBJ_DIR)/hw_crypto/%.o,$(wildcard $(HW_CRYPTO_DIR)/*.c))
OBJS += $(HW_CRYPTO_LOCAL_OBJS)
endif
