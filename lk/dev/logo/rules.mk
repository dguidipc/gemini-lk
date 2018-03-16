LOCAL_DIR := $(GET_LOCAL_DIR)
BOOT_LOGO_DIR := $(LOCAL_DIR)

#fix no boot_logo config
#LOCAL_CFLAGS += -DBOOT_LOGO=wvga

ifeq ($(strip $(BOOT_LOGO)),)
  BOOT_LOGO = fwvga
endif

ifeq ($(strip $(MTK_LK_CAMERA_SUPPORT)), yes)
  BOOT_LOGO = fhd
endif

$(info BOOT_LOGO = $(BOOT_LOGO))
$(info lk/logo/dir=$(LOCAL_DIR),builddir=$(BUILDDIR))

ifeq ($(HOST_OS),darwin)
BMP_TO_RAW := $(BOOT_LOGO_DIR)/tool/bmp_to_raw.darwin
ZPIPE := $(BOOT_LOGO_DIR)/tool/zpipe.darwin
MKIMG := $(LOCAL_DIR)/../../scripts/mkimage.darwin
else
BMP_TO_RAW := $(BOOT_LOGO_DIR)/tool/bmp_to_raw
ZPIPE := $(BOOT_LOGO_DIR)/tool/zpipe
MKIMG := $(LOCAL_DIR)/../../scripts/mkimage
endif
IMG_HDR_CFG := $(LOCAL_DIR)/img_hdr_logo.cfg

EMPTY :=
UNDER_LINE := _
TEMP := $(strip $(subst $(UNDER_LINE), $(EMPTY), $(BOOT_LOGO)))
COUNT := $(words $(TEMP))
BASE_LOGO := $(word $(COUNT),$(TEMP))
EXIST := $(shell if [ -e $(BOOT_LOGO_DIR)/$(BASE_LOGO) ]; then echo "exist"; else echo "noexist"; fi;)
ifeq ($(EXIST), "noexist")
  BASE_LOGO := $(BOOT_LOGO)
endif

SUPPORT_PUMP_EXPRESS = no
ifeq ($(strip $(MTK_PUMP_EXPRESS_SUPPORT)), yes)
  SUPPORT_PUMP_EXPRESS = yes
else
  ifeq ($(strip $(MTK_PUMP_EXPRESS_PLUS_SUPPORT)), yes)
    SUPPORT_PUMP_EXPRESS = yes
  endif
endif

BOOT_LOGO_RESOURCE := $(BUILDDIR)/$(BOOT_LOGO_DIR)/$(BOOT_LOGO).raw
LOGO_IMAGE := $(BUILDDIR)/logo.bin

SUPPORT_PROTOCOL1_RAT_CONFIG = no
SUPPORT_CARRIEREXPRESS_PACK = no
ifdef MTK_CARRIEREXPRESS_PACK
ifneq ($(strip $(MTK_CARRIEREXPRESS_PACK)), no)
	SUPPORT_CARRIEREXPRESS_PACK = yes
	RAT_CONFIG = $(strip $(MTK_PROTOCOL1_RAT_CONFIG))
	ifneq (,$(RAT_CONFIG))
		ifneq (,$(findstring L,$(RAT_CONFIG)))
			SUPPORT_PROTOCOL1_RAT_CONFIG = yes
		endif
	endif
endif
endif

ifeq ($(strip $(SUPPORT_CARRIEREXPRESS_PACK)),yes)
RESOLUTION := $(word $(COUNT),$(TEMP))
RESOURCE_OBJ_LIST :=   \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_uboot.raw
else
RESOURCE_OBJ_LIST :=   \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_uboot.raw
endif

ifneq ($(strip $(MACH_TYPE)), 2701)
ifneq ($(strip $(MTK_ALPS_BOX_SUPPORT)), yes)
RESOURCE_OBJ_LIST +=   \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_battery.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_low_battery.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_charger_ov.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_num_0.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_num_1.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_num_2.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_num_3.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_num_4.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_num_5.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_num_6.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_num_7.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_num_8.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_num_9.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_num_percent.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_bat_animation_01.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_bat_animation_02.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_bat_animation_03.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_bat_animation_04.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_bat_animation_05.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_bat_animation_06.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_bat_animation_07.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_bat_animation_08.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_bat_animation_09.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_bat_animation_10.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_bat_10_01.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_bat_10_02.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_bat_10_03.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_bat_10_04.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_bat_10_05.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_bat_10_06.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_bat_10_07.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_bat_10_08.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_bat_10_09.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_bat_10_10.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_bat_bg.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_bat_img.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_bat_100.raw
ifeq ($(strip $(SUPPORT_CARRIEREXPRESS_PACK)),yes)
RESOURCE_OBJ_LIST +=   \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_kernel.raw
else
RESOURCE_OBJ_LIST +=   \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_kernel.raw
endif
endif
endif

ifeq ($(strip $(SUPPORT_PUMP_EXPRESS)), yes)
RESOURCE_OBJ_LIST +=   \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_fast_charging_100.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_fast_charging_ani-01.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_fast_charging_ani-02.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_fast_charging_ani-03.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_fast_charging_ani-04.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_fast_charging_ani-05.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_fast_charging_ani-06.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_fast_charging_00.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_fast_charging_01.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_fast_charging_02.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_fast_charging_03.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_fast_charging_04.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_fast_charging_05.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_fast_charging_06.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_fast_charging_07.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_fast_charging_08.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_fast_charging_09.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_fast_charging_percent.raw
endif

ifeq ($(strip $(MTK_WIRELESS_CHARGER_SUPPORT)), yes)
RESOURCE_OBJ_LIST +=   \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_num_00.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_num_01.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_num_02.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_num_03.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_num_04.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_num_05.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_num_06.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_num_07.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_num_08.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_num_09.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_num_percent.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_bat_10_0.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_bat_10_1.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_bat_10_2.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_bat_10_3.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_bat_30_0.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_bat_30_1.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_bat_30_2.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_bat_30_3.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_bat_60_0.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_bat_60_1.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_bat_60_2.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_bat_60_3.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_bat_90_0.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_bat_90_1.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_bat_90_2.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_bat_90_3.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_bat_0.raw \
            $(BOOT_LOGO_DIR)/$(BASE_LOGO)/$(BASE_LOGO)_wireless_bat_100.raw

endif

ifeq ($(strip $(SUPPORT_CARRIEREXPRESS_PACK)), yes)

ifeq ($(filter OP01, $(subst _, $(space), $(MTK_REGIONAL_OP_PACK))), OP01)
ifeq ($(strip $(SUPPORT_PROTOCOL1_RAT_CONFIG)), yes)
RESOURCE_OBJ_LIST +=   \
	$(BOOT_LOGO_DIR)/cmcc_lte_$(RESOLUTION)/cmcc_lte_$(RESOLUTION)_uboot.raw \
	$(BOOT_LOGO_DIR)/cmcc_lte_$(RESOLUTION)/cmcc_lte_$(RESOLUTION)_kernel.raw
else
RESOURCE_OBJ_LIST +=   \
	$(BOOT_LOGO_DIR)/cmcc_$(RESOLUTION)/cmcc_$(RESOLUTION)_uboot.raw \
	$(BOOT_LOGO_DIR)/cmcc_$(RESOLUTION)/cmcc_$(RESOLUTION)_kernel.raw
endif
endif

ifeq ($(filter OP02, $(subst _, $(space), $(MTK_REGIONAL_OP_PACK))), OP02)
ifeq ($(strip $(SUPPORT_PROTOCOL1_RAT_CONFIG)), yes)
RESOURCE_OBJ_LIST +=   \
	$(BOOT_LOGO_DIR)/cu_lte_$(RESOLUTION)/cu_lte_$(RESOLUTION)_uboot.raw \
	$(BOOT_LOGO_DIR)/cu_lte_$(RESOLUTION)/cu_lte_$(RESOLUTION)_kernel.raw
else
RESOURCE_OBJ_LIST +=   \
	$(BOOT_LOGO_DIR)/cu_$(RESOLUTION)/cu_$(RESOLUTION)_uboot.raw \
	$(BOOT_LOGO_DIR)/cu_$(RESOLUTION)/cu_$(RESOLUTION)_kernel.raw
endif
endif

ifeq ($(filter OP09, $(subst _, $(space), $(MTK_REGIONAL_OP_PACK))), OP09)
ifeq ($(strip $(SUPPORT_PROTOCOL1_RAT_CONFIG)), yes)
RESOURCE_OBJ_LIST +=   \
	$(BOOT_LOGO_DIR)/ct_lte_$(RESOLUTION)/ct_lte_$(RESOLUTION)_uboot.raw \
	$(BOOT_LOGO_DIR)/ct_lte_$(RESOLUTION)/ct_lte_$(RESOLUTION)_kernel.raw
else
RESOURCE_OBJ_LIST +=   \
	$(BOOT_LOGO_DIR)/ct_$(RESOLUTION)/ct_$(RESOLUTION)_uboot.raw \
	$(BOOT_LOGO_DIR)/ct_$(RESOLUTION)/ct_$(RESOLUTION)_kernel.raw
endif
endif

endif

GENERATED += \
            $(BOOT_LOGO_RESOURCE) \
            $(LOGO_IMAGE) \
            $(addprefix $(BUILDDIR)/,$(RESOURCE_OBJ_LIST))


all:: $(LOGO_IMAGE)

$(LOGO_IMAGE):$(MKIMG) $(BOOT_LOGO_RESOURCE)
	@echo "MKING $(LOGO_IMAGE) start"
	@echo $(MKIMG)
	@echo $(BOOT_LOGO_RESOURCE)
	@echo $(LOGO_IMAGE)
	$(MKIMG) $(BOOT_LOGO_RESOURCE) $(IMG_HDR_CFG) > $(LOGO_IMAGE)

$(BOOT_LOGO_RESOURCE): $(addprefix $(BUILDDIR)/,$(RESOURCE_OBJ_LIST)) $(ZPIPE)
	@$(MKDIR)
	@echo "zpiping "
	$(ZPIPE) -l 9 $@ $(addprefix $(BUILDDIR)/,$(RESOURCE_OBJ_LIST))


$(BUILDDIR)/%.raw: %.bmp $(BMP_TO_RAW)
	@$(MKDIR)
	@echo "Compiling_BMP_TO_RAW $<"
	$(BMP_TO_RAW) $@ $<

