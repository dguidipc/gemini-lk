#define LOG_TAG "MISC"

#include <platform/ddp_reg.h>
#include <platform/ddp_misc.h>

void ddp_bypass_color(int idx, unsigned int width, unsigned int height, void * handle)
{
	DISP_REG_SET(handle,DISP_COLOR_INTERNAL_IP_WIDTH, width); // width
	DISP_REG_SET(handle,DISP_COLOR_INTERNAL_IP_HEIGHT, height); // height
	DISP_REG_SET(handle,DISP_COLOR_CFG_MAIN, 0x200032bc); //bypass color engine
	DISP_REG_SET(handle,DISP_COLOR_START, 0x2); // start
	DISP_REG_SET(handle,DISP_COLOR_START, 0x3); // start
	DISP_REG_SET(handle,DISP_COLOR_CM1_EN, 0); // disable R2Y
	DISP_REG_SET(handle,DISP_COLOR_CM2_EN, 0); // disable Y2R
}

void ddp_bypass_ccorr(unsigned int width, unsigned int height, void * handle)
{
	DISP_REG_SET(NULL,DISP_REG_CCORR_SIZE, (width<<16) | height); // size
	DISP_REG_SET(NULL,DISP_REG_CCORR_EN, 1);
	DISP_REG_MASK(NULL,DISP_REG_CCORR_CFG, 1, 1);
}

void ddp_bypass_aal(unsigned int width, unsigned int height, void * handle)
{
	DISP_REG_SET(handle,DISP_AAL_EN, 0x0); //bypass mode
	DISP_REG_SET_FIELD(handle,CFG_FLD_RELAY_MODE,   DISP_AAL_CFG, 0x1); //relay_mode
	DISP_REG_SET_FIELD(handle,SIZE_FLD_HSIZE,   DISP_AAL_SIZE, width); //width
	DISP_REG_SET_FIELD(handle,SIZE_FLD_VSIZE,   DISP_AAL_SIZE, height); //height
}

void ddp_bypass_gamma(unsigned int width, unsigned int height, void * handle)
{
	//Disable means bypass, so don't need to set any thing
	DISP_REG_SET(handle,DISP_REG_GAMMA_EN, 0x0); //bypass mode
	DISP_REG_SET(handle,DISP_REG_GAMMA_SIZE, (width<<16)|height);
}

static int config_color(DISP_MODULE_ENUM module, disp_ddp_path_config* pConfig, void* cmdq)
{
	int color_idx;
	if (module == DISP_MODULE_COLOR0)
		color_idx = 0;
	else
		color_idx = 1;

	if (!pConfig->dst_dirty) {
		return 0;
	}
	ddp_bypass_color(color_idx,pConfig->dst_w, pConfig->dst_h,cmdq);
	ddp_bypass_ccorr(pConfig->dst_w, pConfig->dst_h,cmdq);
	return 0;
}

static int config_aal(DISP_MODULE_ENUM module, disp_ddp_path_config* pConfig, void* cmdq)
{
	if (!pConfig->dst_dirty) {
		return 0;
	}
	ddp_bypass_aal(pConfig->dst_w, pConfig->dst_h,cmdq);
	return 0;
}

static int config_gamma(DISP_MODULE_ENUM module, disp_ddp_path_config* pConfig, void* cmdq)
{
	if (!pConfig->dst_dirty) {
		return 0;
	}
	ddp_bypass_gamma(pConfig->dst_w, pConfig->dst_h,cmdq);
	return 0;
}

static int clock_on(DISP_MODULE_ENUM module,void * handle)
{
	ddp_enable_module_clock(module);
	return 0;
}

static int clock_off(DISP_MODULE_ENUM module,void * handle)
{
	ddp_disable_module_clock(module);
	return 0;
}

///////////////////////////////////////////////////////////

// color
DDP_MODULE_DRIVER ddp_driver_color = {
	.init            = clock_on,
	.deinit          = clock_off,
	.config          = config_color,
	.start           = NULL,
	.trigger         = NULL,
	.stop            = NULL,
	.reset           = NULL,
	.power_on        = clock_on,
	.power_off       = clock_off,
	.is_idle         = NULL,
	.is_busy         = NULL,
	.dump_info       = NULL,
	.bypass          = NULL,
	.build_cmdq      = NULL,
	.set_lcm_utils   = NULL,
};

// aal
DDP_MODULE_DRIVER ddp_driver_aal = {
	.init            = clock_on,
	.deinit          = clock_off,
	.config          = config_aal,
	.start           = NULL,
	.trigger         = NULL,
	.stop            = NULL,
	.reset           = NULL,
	.power_on        = clock_on,
	.power_off       = clock_off,
	.is_idle         = NULL,
	.is_busy         = NULL,
	.dump_info       = NULL,
	.bypass          = NULL,
	.build_cmdq      = NULL,
	.set_lcm_utils   = NULL,
};

// gamma
DDP_MODULE_DRIVER ddp_driver_gamma = {
	.init            = clock_on,
	.deinit          = clock_off,
	.config          = config_gamma,
	.start           = NULL,
	.trigger         = NULL,
	.stop            = NULL,
	.reset           = NULL,
	.power_on        = clock_on,
	.power_off       = clock_off,
	.is_idle         = NULL,
	.is_busy         = NULL,
	.dump_info       = NULL,
	.bypass          = NULL,
	.build_cmdq      = NULL,
	.set_lcm_utils   = NULL,
};
