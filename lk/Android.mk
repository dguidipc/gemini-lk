LOCAL_PATH := $(call my-dir)
LK_ROOT_DIR := $(PWD)
HOST_OS ?= $(shell uname | tr '[A-Z]' '[a-z]')
export HOST_OS LCM_WIDTH LCM_HEIGHT

ifdef LK_PROJECT
    LK_DIR := $(LOCAL_PATH)
    INSTALLED_LK_TARGET := $(PRODUCT_OUT)/lk.bin
    INSTALLED_LOGO_TARGET := $(PRODUCT_OUT)/logo.bin

  ifeq ($(wildcard $(TARGET_PREBUILT_LK)),)
    TARGET_LK_OUT ?= $(if $(filter /% ~%,$(TARGET_OUT_INTERMEDIATES)),,$(LK_ROOT_DIR)/)$(TARGET_OUT_INTERMEDIATES)/BOOTLOADER_OBJ
    BUILT_LK_TARGET := $(TARGET_LK_OUT)/build-$(LK_PROJECT)/lk.bin
    ifeq ($(LK_CROSS_COMPILE),)
    ifeq ($(TARGET_ARCH), arm)
      LK_CROSS_COMPILE := $(LK_ROOT_DIR)/$(TARGET_TOOLS_PREFIX)
    else ifeq ($(TARGET_2ND_ARCH), arm)
      LK_CROSS_COMPILE := $(LK_ROOT_DIR)/$($(TARGET_2ND_ARCH_VAR_PREFIX)TARGET_TOOLS_PREFIX)
    endif
    endif
    LK_MAKE_OPTION := $(if $(SHOW_COMMANDS),NOECHO=) $(if $(LK_CROSS_COMPILE),TOOLCHAIN_PREFIX=$(LK_CROSS_COMPILE)) BOOTLOADER_OUT=$(TARGET_LK_OUT) ROOTDIR=$(LK_ROOT_DIR)

.KATI_RESTAT: $(BUILT_LK_TARGET)
$(BUILT_LK_TARGET): FORCE
	$(hide) mkdir -p $(dir $@)
	$(MAKE) -C $(LK_DIR) $(LK_MAKE_OPTION) $(LK_PROJECT)

$(TARGET_PREBUILT_LK): $(BUILT_LK_TARGET)
	$(hide) mkdir -p $(dir $@)
	$(hide) cp -f $(BUILT_LK_TARGET) $@
	$(hide) cp -f $(dir $(BUILT_LK_TARGET))logo.bin $(dir $@)logo.bin

  else
    BUILT_LK_TARGET := $(TARGET_PREBUILT_LK)
  endif#TARGET_PREBUILT_LK

$(INSTALLED_LK_TARGET): $(BUILT_LK_TARGET)
	$(hide) mkdir -p $(dir $@)
	$(hide) cp -f $< $@

$(INSTALLED_LOGO_TARGET): $(BUILT_LK_TARGET)
	$(hide) mkdir -p $(dir $@)
	$(hide) cp -f $(dir $<)$(notdir $@) $@

.PHONY: lk save-lk clean-lk
droidcore: $(INSTALLED_LK_TARGET) $(INSTALLED_LOGO_TARGET)
lk: $(INSTALLED_LK_TARGET) $(INSTALLED_LOGO_TARGET)
save-lk: $(TARGET_PREBUILT_LK)

clean-lk:
	$(hide) rm -rf $(INSTALLED_LK_TARGET) $(INSTALLED_LOGO_TARGET) $(TARGET_LK_OUT)

droid: check-lk-config
check-mtk-config: check-lk-config
check-lk-config:
ifneq (yes,$(strip $(DISABLE_MTK_CONFIG_CHECK)))
	python device/mediatek/build/build/tools/check_kernel_config.py -c $(MTK_TARGET_PROJECT_FOLDER)/ProjectConfig.mk -l $(LK_DIR)/project/$(LK_PROJECT).mk -p $(MTK_PROJECT_NAME)
else
	-python device/mediatek/build/build/tools/check_kernel_config.py -c $(MTK_TARGET_PROJECT_FOLDER)/ProjectConfig.mk -l $(LK_DIR)/project/$(LK_PROJECT).mk -p $(MTK_PROJECT_NAME)
endif

  ifeq ($(strip $(PLATFORM_FASTBOOT_EMPTY_STORAGE)),yes)
    INSTALLED_LK_FES_TARGET := $(PRODUCT_OUT)/FES/lk.bin
    TARGET_LK_FES_OUT ?= $(if $(filter /% ~%,$(TARGET_OUT_INTERMEDIATES)),,$(LK_ROOT_DIR)/)$(TARGET_OUT_INTERMEDIATES)/LK_FES_OBJ
    BUILT_LK_FES_TARGET := $(TARGET_LK_FES_OUT)/build-$(LK_PROJECT)/lk.bin
    LK_FES_MAKE_OPTION := $(if $(SHOW_COMMANDS),NOECHO=) $(if $(LK_CROSS_COMPILE),TOOLCHAIN_PREFIX=$(LK_CROSS_COMPILE)) BOOTLOADER_OUT=$(TARGET_LK_FES_OUT) ROOTDIR=$(LK_ROOT_DIR)
    LK_FES_MAKE_OPTION += MTK_SECURITY_SW_SUPPORT=no
    LK_FES_MAKE_OPTION += CFG_DTB_EARLY_LOADER_SUPPORT=no
    LK_FES_MAKE_OPTION += PLATFORM_FASTBOOT_EMPTY_STORAGE=yes
    LK_FES_MAKE_OPTION += APPSBOOTHDR_FILES=

.KATI_RESTAT: $(BUILT_LK_FES_TARGET)
$(BUILT_LK_FES_TARGET): FORCE
	$(hide) mkdir -p $(dir $@)
	$(MAKE) -C $(LK_DIR) $(LK_FES_MAKE_OPTION) $(LK_PROJECT)

$(INSTALLED_LK_FES_TARGET): $(BUILT_LK_FES_TARGET)
	$(hide) mkdir -p $(dir $@)
	$(hide) cp -f $< $@

mtk-fes: $(INSTALLED_LK_FES_TARGET)

  endif
endif#LK_PROJECT
