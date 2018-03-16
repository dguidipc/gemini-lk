ifndef DRVGEN_OUT
DRVGEN_OUT := $(BUILDDIR)
endif

DRVGEN_OUT_PATH := $(DRVGEN_OUT)/inc

ALL_DRVGEN_FILE :=

ifeq ($(filter mt2601,$(PLATFORM)),)
  ALL_DRVGEN_FILE += inc/cust_kpd.h
  ALL_DRVGEN_FILE += inc/cust_eint.h
  ALL_DRVGEN_FILE += inc/cust_gpio_boot.h
  ALL_DRVGEN_FILE += inc/cust_gpio_usage.h
  ALL_DRVGEN_FILE += inc/cust_adc.h
  ALL_DRVGEN_FILE += inc/pmic_drv.h
  ALL_DRVGEN_FILE += inc/pmic_drv.c
endif

ifeq ($(filter mt2601 mt8127 mt8163,$(PLATFORM)),)
  ALL_DRVGEN_FILE += inc/cust_eint_md1.h
endif

ifeq ($(filter mt2601 mt6572 mt6582 mt6592 mt8127,$(PLATFORM)),)
  ALL_DRVGEN_FILE += inc/cust_eint.dtsi
endif

ifeq ($(filter mt2601 mt6580 mt6570,$(PLATFORM)),)
  ALL_DRVGEN_FILE += inc/cust_power.h
endif

ifeq ($(filter mt2601 mt6572 mt6582 mt6592 mt8127 mt8163,$(PLATFORM)),)
  ALL_DRVGEN_FILE += inc/cust_clk_buf.h
endif

ifeq ($(filter mt2601 mt6572 mt6582 mt6592 mt8127 mt8163,$(PLATFORM)),)
  ALL_DRVGEN_FILE += inc/cust_i2c.h
endif

ifeq ($(PLATFORM),mt2601)
  ALL_DRVGEN_FILE += inc/cust_kpd.h
  ALL_DRVGEN_FILE += inc/cust_eint.h
  ALL_DRVGEN_FILE += inc/cust_gpio_usage.h
  ALL_DRVGEN_FILE += include/target/cust_gpio_boot.h
  ALL_DRVGEN_FILE += include/target/cust_power.h
endif

ifeq ($(PLATFORM),mt6752)
  ALL_DRVGEN_FILE += inc/cust_eint_md2.h
endif

ifeq ($(PLATFORM),mt6595)
  ALL_DRVGEN_FILE += inc/cust_gpio_suspend.h
endif

ifeq ($(PLATFORM),mt6580)
  ALL_DRVGEN_FILE += inc/cust_i2c.dtsi
endif

ifeq ($(PLATFORM),mt6570)
  ALL_DRVGEN_FILE += inc/cust_i2c.dtsi
endif

ifeq ($(PLATFORM),mt8127)
  ALL_DRVGEN_FILE += inc/cust_eint_ext.h
endif

ifeq ($(PLATFORM),mt6735)
  ALL_DRVGEN_FILE += inc/cust_adc.dtsi
  ALL_DRVGEN_FILE += inc/cust_i2c.dtsi
  ALL_DRVGEN_FILE += inc/cust_md1_eint.dtsi
  ALL_DRVGEN_FILE += inc/cust_kpd.dtsi
  ALL_DRVGEN_FILE += inc/cust_clk_buf.dtsi
  ALL_DRVGEN_FILE += inc/cust_gpio.dtsi
  ALL_DRVGEN_FILE += inc/cust_adc.dtsi
  ALL_DRVGEN_FILE += inc/cust_pmic.dtsi
  ALL_DRVGEN_FILE += inc/mt6735-pinfunc.h
  ALL_DRVGEN_FILE += inc/pinctrl-mtk-mt6735.h
endif

ifeq ($(PLATFORM),$(filter $(PLATFORM),mt6797 mt6757 elbrus mt6570 mt6799))
  PMIC_DRV_C_TARGET = pmic_drv_c
  PMIC_DRV_H_TARGET = pmic_drv_h
else
  PMIC_DRV_C_TARGET = pmic_c
  PMIC_DRV_H_TARGET = pmic_h
endif

DRVGEN_FILE_LIST := $(addprefix $(DRVGEN_OUT)/,$(ALL_DRVGEN_FILE))
DRVGEN_TOOL := $(PWD)/scripts/dct/DrvGen.py
DWS_FILE := $(PWD)/target/$(TARGET)/dct/$(if $(CUSTOM_KERNEL_DCT),$(CUSTOM_KERNEL_DCT),dct)/codegen.dws
DRVGEN_FIG := $(wildcard $(dir $(DRVGEN_TOOL))config/*.fig)
DRVGEN_PREBUILT_PATH := $(PWD)/target/$(TARGET)
DRVGEN_PREBUILT_CHECK := $(filter-out $(wildcard $(addprefix $(DRVGEN_PREBUILT_PATH)/,$(ALL_DRVGEN_FILE))),$(addprefix $(DRVGEN_PREBUILT_PATH)/,$(ALL_DRVGEN_FILE)))

.PHONY: drvgen
drvgen: $(DRVGEN_FILE_LIST) $(wildcard $(dir $DRVGEN_TOOL)/config/*.fig)
ifneq ($(DRVGEN_PREBUILT_CHECK),)

$(DRVGEN_OUT)/inc/cust_kpd.h: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) kpd_h

$(DRVGEN_OUT)/inc/cust_eint.h: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) eint_h

$(DRVGEN_OUT)/inc/cust_gpio_boot.h: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) gpio_boot_h

$(DRVGEN_OUT)/inc/cust_gpio_usage.h: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) gpio_usage_h

$(DRVGEN_OUT)/inc/cust_adc.h: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) adc_h

$(DRVGEN_OUT)/inc/cust_eint_md1.h: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) md1_eint_h

$(DRVGEN_OUT)/inc/cust_power.h: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) power_h

$(DRVGEN_OUT)/inc/pmic_drv.h: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) $(PMIC_DRV_H_TARGET)

$(DRVGEN_OUT)/inc/cust_i2c.h: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) i2c_h

$(DRVGEN_OUT)/inc/cust_clk_buf.h: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) clk_buf_h

$(DRVGEN_OUT)/inc/cust_eint_md2.h: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) md2_eint_h

$(DRVGEN_OUT)/inc/cust_gpio_suspend.h: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) suspend_h

$(DRVGEN_OUT)/inc/cust_eint_ext.h: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) eint_ext_h

$(DRVGEN_OUT)/inc/cust_eint.dtsi: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) eint_dtsi

$(DRVGEN_OUT)/inc/pmic_drv.c: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) $(PMIC_DRV_C_TARGET)

$(DRVGEN_OUT)/inc/cust_i2c.dtsi: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) i2c_dtsi

$(DRVGEN_OUT)/inc/cust_adc.dtsi: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) adc_dtsi

$(DRVGEN_OUT)/inc/cust_md1_eint.dtsi: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) md1_eint_dtsi

$(DRVGEN_OUT)/inc/cust_kpd.dtsi: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) kpd_dtsi

$(DRVGEN_OUT)/inc/cust_clk_buf.dtsi: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) clk_buf_dtsi

$(DRVGEN_OUT)/inc/cust_gpio.dtsi: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) gpio_dtsi

$(DRVGEN_OUT)/inc/cust_pmic.dtsi: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) pmic_dtsi

$(DRVGEN_OUT)/inc/mt6735-pinfunc.h: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) mt6735_pinfunc_h

$(DRVGEN_OUT)/inc/pinctrl-mtk-mt6735.h: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	@$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) pinctrl_mtk_mt6735_h

else
$(DRVGEN_FILE_LIST): $(DRVGEN_OUT)/% : $(DRVGEN_PREBUILT_PATH)/%
	@mkdir -p $(dir $@)
	cp -f $< $@
endif
