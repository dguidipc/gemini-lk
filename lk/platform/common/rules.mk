LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += -I$(LOCAL_DIR)/include

# security wrapper
OBJS += $(LOCAL_DIR)/sec_wrapper.o

# partition interface
OBJS += \
	$(LOCAL_DIR)/partition/part_internal_wrapper.o \
	$(LOCAL_DIR)/partition/common_interface.o

ifeq ($(MTK_EMMC_SUPPORT),yes)
OBJS += $(LOCAL_DIR)/plinfo/plinfo_emmc.o
ifneq ($(filter MTK_NEW_COMBO_EMMC_SUPPORT, $(strip $(DEFINES))),)
OBJS += $(LOCAL_DIR)/partition/combo_emmc_interface.o
OBJS += $(LOCAL_DIR)/block/combo_emmc_generic_inter.o
else
OBJS += $(LOCAL_DIR)/partition/emmc_interface.o
OBJS += $(LOCAL_DIR)/block/emmc_generic_inter.o
endif
else ifeq ($(MTK_UFS_BOOTING),yes)
OBJS += $(LOCAL_DIR)/plinfo/plinfo_ufs.o
OBJS += $(LOCAL_DIR)/partition/ufs_interface.o
OBJS += $(LOCAL_DIR)/block/ufs_generic_inter.o
MODULES += $(LOCAL_DIR)/ufs
#else ifeq ($(MTK_NAND_UBIFS_SUPPORT),yes)
else
OBJS += $(LOCAL_DIR)/plinfo/plinfo_nand.o
OBJS += $(LOCAL_DIR)/partition/nand_interface.o
OBJS += $(LOCAL_DIR)/block/nand_generic_inter.o
endif # ifeq ($(MTK_EMMC_SUPPORT),yes)

# aee platform debug
ifeq ($(MTK_AEE_PLATFORM_DEBUG_SUPPORT),yes)
MODULES += $(LOCAL_DIR)/aee_platform_debug
MODULES += $(LOCAL_DIR)/spm
endif

