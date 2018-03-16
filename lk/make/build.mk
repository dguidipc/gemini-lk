# comment out or override if you want to see the full output of each command
NOECHO ?= @

ifeq ($(HOST_OS),darwin)
MKIMAGE := ./scripts/mkimage.darwin
else
MKIMAGE := ./scripts/mkimage
endif

$(OUTBIN): $(OUTELF)
	@echo generating image: $@
	$(NOECHO)$(SIZE) $<
	$(NOCOPY)$(OBJCOPY) -O binary $< $@
	$(NOECHO)cp -f $@ $(BUILDDIR)/lk-no-mtk-header.bin
	$(MKIMAGE) $@ img_hdr_lk.cfg > $(BUILDDIR)/lk_header.bin
	$(NOECHO)mv $(BUILDDIR)/lk_header.bin $@

ifeq ($(ENABLE_TRUSTZONE), 1)
$(OUTELF): $(ALLOBJS) $(LINKER_SCRIPT) $(OUTPUT_TZ_BIN)
ifeq ($(BUILD_SEC_LIB),yes)
	@echo delete old security library
	@rm -rf $(LK_TOP_DIR)/app/mt_boot/lib/libauth.a
	@rm -rf $(LK_TOP_DIR)/app/mt_boot/lib/libsec.a
	@rm -rf $(LK_TOP_DIR)/app/mt_boot/lib/libsplat.a
	@rm -rf $(LK_TOP_DIR)/app/mt_boot/lib/libdevinfo.a
	@echo linking security library
	@ar cq $(LK_TOP_DIR)/app/mt_boot/lib/libauth.a $(AUTH_OBJS)
	@ar cq $(LK_TOP_DIR)/app/mt_boot/lib/libsec.a $(SEC_OBJS)
	@ar cq $(LK_TOP_DIR)/app/mt_boot/lib/libsplat.a $(SEC_PLAT_OBJS)
	@ar cq $(LK_TOP_DIR)/app/mt_boot/lib/libdevinfo.a $(DEVINFO_OBJS)
endif
	@echo linking $@
	$(NOECHO)$(LD) $(LDFLAGS) -T $(LINKER_SCRIPT) $(OUTPUT_TZ_BIN) $(ALLOBJS) $(LIBGCC) $(LIBSEC) $(LIBSEC_PLAT) -o $@
else
$(OUTELF): $(ALLOBJS) $(LINKER_SCRIPT)
ifeq ($(BUILD_SEC_LIB),yes)
	@echo delete old security library
	@rm -rf $(LK_TOP_DIR)/app/mt_boot/lib/libauth.a
	@rm -rf $(LK_TOP_DIR)/app/mt_boot/lib/libsec.a
	@rm -rf $(LK_TOP_DIR)/app/mt_boot/lib/libsplat.a
	@rm -rf $(LK_TOP_DIR)/app/mt_boot/lib/libdevinfo.a
	@echo linking security library
	@ar cq $(LK_TOP_DIR)/app/mt_boot/lib/libauth.a $(AUTH_OBJS)
	@ar cq $(LK_TOP_DIR)/app/mt_boot/lib/libsec.a $(SEC_OBJS)
	@ar cq $(LK_TOP_DIR)/app/mt_boot/lib/libsplat.a $(SEC_PLAT_OBJS)
	@ar cq $(LK_TOP_DIR)/app/mt_boot/lib/libdevinfo.a $(DEVINFO_OBJS)
endif
ifeq ($(BUILD_HW_CRYPTO_LIB),yes)
	@echo delete old hw crypto library
	@rm -rf $(LK_TOP_DIR)/app/mt_boot/lib/libhw_crypto.a
	@echo linking hw crypto library
	@ar cq $(LK_TOP_DIR)/app/mt_boot/lib/libhw_crypto.a $(HW_CRYPTO_OBJS)
endif
	@echo linking $@
	$(NOECHO)$(LD) $(LDFLAGS) -T $(LINKER_SCRIPT) $(ALLOBJS) $(LIBGCC) $(LIBSEC) $(LIBSEC_PLAT) $(LIBHW_CRYPTO) -o $@
endif

$(OUTELF).sym: $(OUTELF)
	@echo generating symbols: $@
	$(NOECHO)$(OBJDUMP) -t $< | $(CPPFILT) > $@

$(OUTELF).lst: $(OUTELF)
	@echo generating listing: $@
	$(NOECHO)$(OBJDUMP) -Mreg-names-raw -d $< | $(CPPFILT) > $@

$(OUTELF).debug.lst: $(OUTELF)
	@echo generating listing: $@
	$(NOECHO)$(OBJDUMP) -Mreg-names-raw -S $< | $(CPPFILT) > $@

$(OUTELF).size: $(OUTELF)
	@echo generating size map: $@
	$(NOECHO)$(NM) -S --size-sort $< > $@

ifeq ($(ENABLE_TRUSTZONE), 1)
$(OUTPUT_TZ_BIN): $(INPUT_TZ_BIN)
	@echo generating TZ output from TZ input
	$(NOECHO)$(OBJCOPY) -I binary -B arm -O elf32-littlearm $(INPUT_TZ_BIN) $(OUTPUT_TZ_BIN)
endif

include arch/$(ARCH)/compile.mk

