#Makefile to generate appsboot.mbn

ifeq ($(BOOTLOADER_OUT),.)
APPSBOOTHEADER_DIR  := $(BUILDDIR)
else
APPSBOOTHEADER_DIR  := $(BOOTLOADER_OUT)/../..
endif

SRC_DIR  := target/$(TARGET)/tools
COMPILER := gcc

ifeq ($(EMMC_BOOT), 1)
  APPSBOOTHDR_FILES := $(APPSBOOTHEADER_DIR)/EMMCBOOT.MBN
else
  ifeq ($(BUILD_NANDWRITE), 1)
    APPSBOOTHDR_FILES :=
  else
    APPSBOOTHDR_FILES := $(APPSBOOTHEADER_DIR)/appsboot.mbn
  endif
endif

APPSBOOTHEADER: $(APPSBOOTHDR_FILES)


$(APPSBOOTHEADER_DIR)/appsboot.mbn: $(OUTBIN) $(BUILDDIR)/mkheader
	@mkdir -p $(dir $@)
	cp $(OUTBIN) $(APPSBOOTHEADER_DIR)/appsboot.raw
	$(BUILDDIR)/mkheader $(OUTBIN) $(APPSBOOTHEADER_DIR)/appsboothd.mbn
	cat $(APPSBOOTHEADER_DIR)/appsboothd.mbn $(OUTBIN) > $(APPSBOOTHEADER_DIR)/appsboot.mbn
	rm -f $(APPSBOOTHEADER_DIR)/appsboothd.mbn

$(APPSBOOTHEADER_DIR)/EMMCBOOT.MBN: $(OUTBIN) $(BUILDDIR)/mkheader
	@mkdir -p $(dir $@)
	cp $(OUTBIN) $(APPSBOOTHEADER_DIR)/emmc_appsboot.raw
	$(BUILDDIR)/mkheader $(OUTBIN) $(APPSBOOTHEADER_DIR)/emmc_appsboothd.mbn
	cat $(APPSBOOTHEADER_DIR)/emmc_appsboothd.mbn $(OUTBIN) > $(APPSBOOTHEADER_DIR)/EMMCBOOT.MBN
	cat $(APPSBOOTHEADER_DIR)/emmc_appsboothd.mbn $(OUTBIN) > $(APPSBOOTHEADER_DIR)/emmc_appsboot.mbn
	rm -f $(APPSBOOTHEADER_DIR)/emmc_appsboothd.mbn

$(BUILDDIR)/mkheader: $(SRC_DIR)/mkheader.c $(MAKEFILE_LIST)
	@mkdir -p $(dir $@)
	$(COMPILER) -DMEMBASE=$(MEMBASE) $< -o $@
