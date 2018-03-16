#include "lcm_drv.h"
#include "lcm_define.h"
#include <platform/ddp_manager.h>
#include <platform/disp_lcm.h>
#include <platform/disp_drv_platform.h>
#include <platform/disp_drv_log.h>

#if defined(MTK_LCM_DEVICE_TREE_SUPPORT)
#include <libfdt.h>

extern void *g_fdt;
extern unsigned char lcm_name_list[][128];
extern LCM_DRIVER lcm_common_drv;

static LCM_DTS lcm_dts;

static struct LCM_setting_table {
	unsigned cmd;
	unsigned char count;
	unsigned char para_list[64];
};
#endif

// This macro and arrya is designed for multiple LCM support
// for multiple LCM, we should assign I/F Port id in lcm driver, such as DPI0, DSI0/1
static disp_lcm_handle _disp_lcm_driver[MAX_LCM_NUMBER] = {0};
static LCM_PARAMS _disp_lcm_params;
// these 2 variables are defined in mt65xx_lcm_list.c
extern LCM_DRIVER* lcm_driver_list[];
extern unsigned int lcm_count;

int _lcm_count(void)
{
	return lcm_count;
}

int _is_lcm_inited(disp_lcm_handle *plcm)
{
	if (plcm) {
		if (plcm->params && plcm->drv)
			return 1;
	} else {
		DISPERR("WARNING, invalid lcm handle: 0x%08x\n", (unsigned int)plcm);
		return 0;
	}
}

LCM_PARAMS *_get_lcm_params_by_handle(disp_lcm_handle *plcm)
{
	if (plcm) {
		return plcm->params;
	} else {
		DISPERR("WARNING, invalid lcm handle: 0x%08x\n", (unsigned int)plcm);
		return NULL;
	}
}

LCM_DRIVER *_get_lcm_driver_by_handle(disp_lcm_handle *plcm)
{
	if (plcm) {
		return plcm->drv;
	} else {
		DISPERR("WARNING, invalid lcm handle: 0x%08x\n", (unsigned int)plcm);
		return NULL;
	}
}


static DISP_MODULE_ENUM _get_dst_module_by_lcm(disp_lcm_handle *plcm)
{
	if (plcm == NULL) {
		DISPERR("plcm is null\n");
		return DISP_MODULE_UNKNOWN;
	}

	if (plcm->params->type == LCM_TYPE_DSI) {
		if (plcm->lcm_if_id == LCM_INTERFACE_DSI0) {
			return DISP_MODULE_DSI0;
		} else if (plcm->lcm_if_id == LCM_INTERFACE_DSI1) {
			return DISP_MODULE_DSI1;
		} else if (plcm->lcm_if_id == LCM_INTERFACE_DSI_DUAL) {
			return DISP_MODULE_DSIDUAL;
		} else {
			return DISP_MODULE_DSI0;
		}
	} else if (plcm->params->type == LCM_TYPE_DPI) {
		return DISP_MODULE_DPI;
	} else {
		DISPERR("can't find primary path dst module\n");
		return DISP_MODULE_UNKNOWN;
	}
}


disp_path_handle _display_interface_path_init(disp_lcm_handle *plcm)
{
	DISP_MODULE_ENUM dst_module = DISP_MODULE_UNKNOWN;
	disp_ddp_path_config data_config;
	disp_path_handle handle = NULL;

	DISPFUNC();

	if (plcm == NULL) {
		DISPERR("plcm is null\n");
		return NULL;
	}

	handle = dpmgr_create_path(DDP_SCENARIO_DISPLAY_INTERFACE, NULL);
	if (handle) {
		DISPCHECK("dpmgr create path SUCCESS(0x%08x)\n", handle);
	} else {
		DISPCHECK("dpmgr create path FAIL\n");
		return NULL;
	}

	dst_module = _get_dst_module_by_lcm(plcm);
	dpmgr_path_set_dst_module(handle, dst_module);
	DISPCHECK("dpmgr set dst module FINISHED(%s)\n", ddp_get_module_name(dst_module));


	dpmgr_set_lcm_utils(handle, plcm->drv);
	dpmgr_path_init(handle, CMDQ_DISABLE);

	memset((void*)&data_config, 0, sizeof(disp_ddp_path_config));

	memcpy(&(data_config.dsi_config), &(plcm->params->dsi), sizeof(LCM_DSI_PARAMS));

	data_config.dst_w = plcm->params->width;
	data_config.dst_h = plcm->params->height;
	data_config.dst_dirty = 1;

	dpmgr_path_config(handle, &data_config, CMDQ_DISABLE);

	return handle;
}

int _display_interface_path_deinit(disp_path_handle handle)
{
	DISPFUNC();
	if (handle == NULL) {
		DISPERR("handle is null\n");
		return -1;
	}

	dpmgr_path_deinit(handle, CMDQ_DISABLE);
	dpmgr_destroy_path(handle);

	return 0;
}

void _dump_lcm_info(disp_lcm_handle *plcm)
{
	int i = 0;
	LCM_DRIVER *l = NULL;
	LCM_PARAMS *p = NULL;

	if (plcm == NULL) {
		DISPERR("plcm is null\n");
		return;
	}

	l = plcm->drv;
	p = plcm->params;

	DISPCHECK("******** dump lcm driver information ********\n");

	if (l && p) {
		DISPCHECK("[LCM], name: %s\n", l->name);

		DISPCHECK("[LCM] resolution: %d x %d\n", p->width, p->height);
		DISPCHECK("[LCM] physical size: %d x %d\n", p->physical_width, p->physical_height);
		DISPCHECK("[LCM] physical size: %d x %d\n", p->physical_width, p->physical_height);
		DISPCHECK("[LCM] lcm_if:%d, cmd_if:%d\n", p->lcm_if, p->lcm_cmd_if);
		switch (p->lcm_if) {
			case LCM_INTERFACE_DSI0:
				DISPCHECK("[LCM] interface: DSI0\n");
				break;
			case LCM_INTERFACE_DSI1:
				DISPCHECK("[LCM] interface: DSI1\n");
				break;
			case LCM_INTERFACE_DPI0:
				DISPCHECK("[LCM] interface: DPI0\n");
				break;
			case LCM_INTERFACE_DPI1:
				DISPCHECK("[LCM] interface: DPI1\n");
				break;
			case LCM_INTERFACE_DBI0:
				DISPCHECK("[LCM] interface: DBI0\n");
				break;
			default:
				DISPCHECK("[LCM] interface: unknown\n");
				break;
		}

		switch (p->type) {
			case LCM_TYPE_DBI:
				DISPCHECK("[LCM] Type: DBI\n");
				break;
			case LCM_TYPE_DSI:
				DISPCHECK("[LCM] Type: DSI\n");

				break;
			case LCM_TYPE_DPI:
				DISPCHECK("[LCM] Type: DPI\n");
				break;
			default:
				DISPCHECK("[LCM] TYPE: unknown\n");
				break;
		}

		if (p->type == LCM_TYPE_DSI) {
			switch (p->dsi.mode) {
				case CMD_MODE:
					DISPCHECK("[LCM] DSI Mode: CMD_MODE\n");
					break;
				case SYNC_PULSE_VDO_MODE:
					DISPCHECK("[LCM] DSI Mode: SYNC_PULSE_VDO_MODE\n");
					break;
				case SYNC_EVENT_VDO_MODE:
					DISPCHECK("[LCM] DSI Mode: SYNC_EVENT_VDO_MODE\n");
					break;
				case BURST_VDO_MODE:
					DISPCHECK("[LCM] DSI Mode: BURST_VDO_MODE\n");
					break;
				default:
					DISPCHECK("[LCM] DSI Mode: Unknown\n");
					break;
			}
		}

		if (p->type == LCM_TYPE_DSI) {
			DISPCHECK("[LCM] LANE_NUM: %d,data_format: %d,vertical_sync_active: %d\n",p->dsi.LANE_NUM,p->dsi.data_format);
#ifdef MY_TODO
#error
#endif
			DISPCHECK("[LCM] vact: %d, vbp: %d, vfp: %d, vact_line: %d, hact: %d, hbp: %d, hfp: %d, hblank: %d, hactive: %d\n",p->dsi.vertical_sync_active, p->dsi.vertical_backporch,p->dsi.vertical_frontporch,p->dsi.vertical_active_line,p->dsi.horizontal_sync_active,p->dsi.horizontal_backporch,p->dsi.horizontal_frontporch,p->dsi.horizontal_blanking_pixel,p->dsi.horizontal_active_pixel);
			DISPCHECK("[LCM] pll_select: %d, pll_div1: %d, pll_div2: %d, fbk_div: %d,fbk_sel: %d, rg_bir: %d\n",p->dsi.pll_select,p->dsi.pll_div1,p->dsi.pll_div2,p->dsi.fbk_div,p->dsi.fbk_sel,p->dsi.rg_bir);
			DISPCHECK("[LCM] rg_bic: %d, rg_bp: %d,	PLL_CLOCK: %d, dsi_clock: %d, ssc_range: %d,	ssc_disable: %d, compatibility_for_nvk: %d, cont_clock: %d\n", p->dsi.rg_bic,  p->dsi.rg_bp,p->dsi.PLL_CLOCK,p->dsi.dsi_clock,p->dsi.ssc_range,p->dsi.ssc_disable,p->dsi.compatibility_for_nvk,p->dsi.cont_clock);
			DISPCHECK("[LCM] lcm_ext_te_enable: %d, noncont_clock: %d, noncont_clock_period: %d\n", p->dsi.lcm_ext_te_enable,p->dsi.noncont_clock,p->dsi.noncont_clock_period);
		}
	}

	return;
}

// TODO: remove this workaround!!!
extern UINT32 DSI_dcs_read_lcm_reg_v2(DISP_MODULE_ENUM module, void* cmdq, UINT8 cmd, UINT8 *buffer, UINT8 buffer_size);

#if defined(MTK_LCM_DEVICE_TREE_SUPPORT)
/**
 * If the property is not found or the value is not set,
 * return default value 0.
 */
int disp_fdt_getprop_u8_array(int nodeoffset,
                              const char *name, unsigned char *out_value)
{
	unsigned int i;
	unsigned int *data = NULL;
	int len = 0;

	data = (unsigned int*)fdt_getprop(g_fdt, nodeoffset, name, &len);
	if (len > 0) {
		len = len / sizeof(unsigned int);
		for (i=0; i<len; i++) {
			*(out_value+i) = (unsigned char)((fdt32_to_cpu(*(data+i))) & 0xFF);
		}
	} else {
		*out_value = 0;
	}

	return len;
}


/**
 * If the property is not found or the value is not set,
 * use default value 0.
 */
int disp_fdt_getprop_u32_array(int nodeoffset,
                               const char *name, unsigned int *out_value)
{
	unsigned int i;
	unsigned int *data = NULL;
	int len = 0;

	data = (unsigned int*)fdt_getprop(g_fdt, nodeoffset, name, &len);
	if (len > 0) {
		len = len / sizeof(unsigned int);
		for (i=0; i<len; i++) {
			*(out_value+i) = fdt32_to_cpu(*(data+i));
		}
	} else {
		*out_value = 0;
	}

	return len;
}


void parse_lcm_params_dt_node(int node_offset, LCM_PARAMS *lcm_params)
{
	if (!lcm_params) {
		DISPERR("%s:%d: Error access to LCM_PARAMS(NULL)\n", __FILE__,
		        __LINE__);
		return;
	}

	memset(lcm_params, 0x0, sizeof(LCM_PARAMS));

	disp_fdt_getprop_u32_array(node_offset, "lcm_params-types", &lcm_params->type);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-resolution", &lcm_params->width);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-io_select_mode", &lcm_params->io_select_mode);

	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dbi-port", &lcm_params->dbi.port);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dbi-clock_freq", &lcm_params->dbi.clock_freq);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dbi-data_width", &lcm_params->dbi.data_width);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dbi-data_format", &(lcm_params->dbi.data_format.color_order));
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dbi-cpu_write_bits", &lcm_params->dbi.cpu_write_bits);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dbi-io_driving_current", &lcm_params->dbi.io_driving_current);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dbi-msb_io_driving_current", &lcm_params->dbi.msb_io_driving_current);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dbi-ctrl_io_driving_current", &lcm_params->dbi.ctrl_io_driving_current);

	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dbi-te_mode", &lcm_params->dbi.te_mode);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dbi-te_edge_polarity", &lcm_params->dbi.te_edge_polarity);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dbi-te_vs_width_cnt_div", &lcm_params->dbi.te_vs_width_cnt_div);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dbi-te_hs_delay_cnt", &lcm_params->dbi.te_hs_delay_cnt);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dbi-te_vs_width_cnt", &lcm_params->dbi.te_vs_width_cnt);

	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dbi-serial-params0", &(lcm_params->dbi.serial.cs_polarity));
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dbi-serial-params1", &(lcm_params->dbi.serial.css));
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dbi-serial-params2", &(lcm_params->dbi.serial.sif_3wire));
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dbi-parallel-params0", &(lcm_params->dbi.parallel.write_setup));
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dbi-parallel-params1", &(lcm_params->dbi.parallel.read_hold));

	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-mipi_pll_clk_ref", &lcm_params->dpi.mipi_pll_clk_ref);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-mipi_pll_clk_div1", &lcm_params->dpi.mipi_pll_clk_div1);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-mipi_pll_clk_div2", &lcm_params->dpi.mipi_pll_clk_div2);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-mipi_pll_clk_fbk_div", &lcm_params->dpi.mipi_pll_clk_fbk_div);

	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-dpi_clk_div", &lcm_params->dpi.dpi_clk_div);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-dpi_clk_duty", &lcm_params->dpi.dpi_clk_duty);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-PLL_CLOCK", &lcm_params->dpi.PLL_CLOCK);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-dpi_clock", &lcm_params->dpi.dpi_clock);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-ssc_disable", &lcm_params->dpi.ssc_disable);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-ssc_range", &lcm_params->dpi.ssc_range);

	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-width", &lcm_params->dpi.width);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-height", &lcm_params->dpi.height);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-bg_width", &lcm_params->dpi.bg_width);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-bg_height", &lcm_params->dpi.bg_height);

	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-clk_pol", &lcm_params->dpi.clk_pol);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-de_pol", &lcm_params->dpi.de_pol);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-vsync_pol", &lcm_params->dpi.vsync_pol);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-hsync_pol", &lcm_params->dpi.hsync_pol);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-hsync_pulse_width", &lcm_params->dpi.hsync_pulse_width);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-hsync_back_porch", &lcm_params->dpi.hsync_back_porch);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-hsync_front_porch", &lcm_params->dpi.hsync_front_porch);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-vsync_pulse_width", &lcm_params->dpi.vsync_pulse_width);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-vsync_back_porch", &lcm_params->dpi.vsync_back_porch);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-vsync_front_porch", &lcm_params->dpi.vsync_front_porch);

	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-format", &lcm_params->dpi.format);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-rgb_order", &lcm_params->dpi.rgb_order);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-is_serial_output", &lcm_params->dpi.is_serial_output);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-i2x_en", &lcm_params->dpi.i2x_en);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-i2x_edge", &lcm_params->dpi.i2x_edge);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-embsync", &lcm_params->dpi.embsync);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-lvds_tx_en", &lcm_params->dpi.lvds_tx_en);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-bit_swap", &lcm_params->dpi.bit_swap);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-intermediat_buffer_num", &lcm_params->dpi.intermediat_buffer_num);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-io_driving_current", &lcm_params->dpi.io_driving_current);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dpi-lsb_io_driving_current", &lcm_params->dpi.lsb_io_driving_current);

	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-mode", &lcm_params->dsi.mode);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-switch_mode", &lcm_params->dsi.switch_mode);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-DSI_WMEM_CONTI", &lcm_params->dsi.DSI_WMEM_CONTI);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-DSI_RMEM_CONTI", &lcm_params->dsi.DSI_RMEM_CONTI);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-VC_NUM", &lcm_params->dsi.VC_NUM);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-lane_num", &lcm_params->dsi.LANE_NUM);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-data_format", &(lcm_params->dsi.data_format.color_order));
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-intermediat_buffer_num", &lcm_params->dsi.intermediat_buffer_num);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-ps", &lcm_params->dsi.PS);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-word_count", &lcm_params->dsi.word_count);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-packet_size", &lcm_params->dsi.packet_size);

	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-vertical_sync_active", &lcm_params->dsi.vertical_sync_active);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-vertical_backporch", &lcm_params->dsi.vertical_backporch);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-vertical_frontporch", &lcm_params->dsi.vertical_frontporch);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-vertical_frontporch_for_low_power", &lcm_params->dsi.vertical_frontporch_for_low_power);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-vertical_active_line", &lcm_params->dsi.vertical_active_line);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-horizontal_sync_active", &lcm_params->dsi.horizontal_sync_active);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-horizontal_backporch", &lcm_params->dsi.horizontal_backporch);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-horizontal_frontporch", &lcm_params->dsi.horizontal_frontporch);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-horizontal_blanking_pixel", &lcm_params->dsi.horizontal_blanking_pixel);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-horizontal_active_pixel", &lcm_params->dsi.horizontal_active_pixel);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-horizontal_bllp", &lcm_params->dsi.horizontal_bllp);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-line_byte", &lcm_params->dsi.line_byte);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-horizontal_sync_active_byte", &lcm_params->dsi.horizontal_sync_active_byte);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-horizontal_backportch_byte", &lcm_params->dsi.horizontal_backporch_byte);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-horizontal_frontporch_byte", &lcm_params->dsi.horizontal_frontporch_byte);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-rgb_byte", &lcm_params->dsi.rgb_byte);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-horizontal_sync_active_word_count", &lcm_params->dsi.horizontal_sync_active_word_count);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-horizontal_backporch_word_count", &lcm_params->dsi.horizontal_backporch_word_count);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-horizontal_frontporch_word_count", &lcm_params->dsi.horizontal_frontporch_word_count);

	disp_fdt_getprop_u8_array(node_offset, "lcm_params-dsi-HS_TRAIL", &lcm_params->dsi.HS_TRAIL);
	disp_fdt_getprop_u8_array(node_offset, "lcm_params-dsi-ZERO", &lcm_params->dsi.HS_ZERO);
	disp_fdt_getprop_u8_array(node_offset, "lcm_params-dsi-HS_PRPR", &lcm_params->dsi.HS_PRPR);
	disp_fdt_getprop_u8_array(node_offset, "lcm_params-dsi-LPX", &lcm_params->dsi.LPX);
	disp_fdt_getprop_u8_array(node_offset, "lcm_params-dsi-TA_SACK", &lcm_params->dsi.TA_SACK);
	disp_fdt_getprop_u8_array(node_offset, "lcm_params-dsi-TA_GET", &lcm_params->dsi.TA_GET);
	disp_fdt_getprop_u8_array(node_offset, "lcm_params-dsi-TA_SURE", &lcm_params->dsi.TA_SURE);
	disp_fdt_getprop_u8_array(node_offset, "lcm_params-dsi-TA_GO", &lcm_params->dsi.TA_GO);
	disp_fdt_getprop_u8_array(node_offset, "lcm_params-dsi-CLK_TRAIL", &lcm_params->dsi.CLK_TRAIL);
	disp_fdt_getprop_u8_array(node_offset, "lcm_params-dsi-CLK_ZERO", &lcm_params->dsi.CLK_ZERO);
	disp_fdt_getprop_u8_array(node_offset, "lcm_params-dsi-LPX_WAIT", &lcm_params->dsi.LPX_WAIT);
	disp_fdt_getprop_u8_array(node_offset, "lcm_params-dsi-CONT_DET", &lcm_params->dsi.CONT_DET);
	disp_fdt_getprop_u8_array(node_offset, "lcm_params-dsi-CLK_HS_PRPR", &lcm_params->dsi.CLK_HS_PRPR);
	disp_fdt_getprop_u8_array(node_offset, "lcm_params-dsi-CLK_HS_POST", &lcm_params->dsi.CLK_HS_POST);
	disp_fdt_getprop_u8_array(node_offset, "lcm_params-dsi-DA_HS_EXIT", &lcm_params->dsi.DA_HS_EXIT);
	disp_fdt_getprop_u8_array(node_offset, "lcm_params-dsi-CLK_HS_EXIT", &lcm_params->dsi.CLK_HS_EXIT);

	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-pll_select", &lcm_params->dsi.pll_select);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-pll_div1", &lcm_params->dsi.pll_div1);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-pll_div2", &lcm_params->dsi.pll_div2);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-fbk_div", &lcm_params->dsi.fbk_div);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-fbk_sel", &lcm_params->dsi.fbk_sel);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-rg_bir", &lcm_params->dsi.rg_bir);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-rg_bic", &lcm_params->dsi.rg_bic);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-rg_bp", &lcm_params->dsi.rg_bp);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-pll_clock", &lcm_params->dsi.PLL_CLOCK);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-dsi_clock", &lcm_params->dsi.dsi_clock);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-ssc_disable", &lcm_params->dsi.ssc_disable);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-ssc_range", &lcm_params->dsi.ssc_range);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-compatibility_for_nvk", &lcm_params->dsi.compatibility_for_nvk);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-cont_clock", &lcm_params->dsi.cont_clock);

	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-ufoe_enable", &lcm_params->dsi.ufoe_enable);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-ufoe_params", &(lcm_params->dsi.ufoe_params.compress_ratio));
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-edp_panel", &lcm_params->dsi.edp_panel);

	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-customization_esd_check_enable", &lcm_params->dsi.customization_esd_check_enable);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-esd_check_enable", &lcm_params->dsi.esd_check_enable);

	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-lcm_int_te_monitor", &lcm_params->dsi.lcm_int_te_monitor);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-lcm_int_te_period", &lcm_params->dsi.lcm_int_te_period);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-lcm_ext_te_monitor", &lcm_params->dsi.lcm_ext_te_monitor);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-lcm_ext_te_enable", &lcm_params->dsi.lcm_ext_te_enable);

	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-noncont_clock", &lcm_params->dsi.noncont_clock);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-noncont_clock_period", &lcm_params->dsi.noncont_clock_period);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-clk_lp_per_line_enable", &lcm_params->dsi.clk_lp_per_line_enable);

	disp_fdt_getprop_u8_array(node_offset, "lcm_params-dsi-lcm_esd_check_table0", &(lcm_params->dsi.lcm_esd_check_table[0].cmd));
	disp_fdt_getprop_u8_array(node_offset, "lcm_params-dsi-lcm_esd_check_table1", &(lcm_params->dsi.lcm_esd_check_table[1].cmd));
	disp_fdt_getprop_u8_array(node_offset, "lcm_params-dsi-lcm_esd_check_table2", &(lcm_params->dsi.lcm_esd_check_table[2].cmd));

	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-switch_mode_enable", &lcm_params->dsi.switch_mode_enable);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-dual_dsi_type", &lcm_params->dsi.dual_dsi_type);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-lane_swap_en", &lcm_params->dsi.lane_swap_en);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-lane_swap0", &(lcm_params->dsi.lane_swap[0][0]));
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-lane_swap1", &(lcm_params->dsi.lane_swap[1][0]));
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-dsi-vertical_vfp_lp", &lcm_params->dsi.vertical_vfp_lp);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-physical_width", &lcm_params->physical_width);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-physical_height", &lcm_params->physical_height);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-od_table_size", &lcm_params->od_table_size);
	disp_fdt_getprop_u32_array(node_offset, "lcm_params-od_table", &lcm_params->od_table);
}


void parse_lcm_ops_dt_node(int node_offset, LCM_DTS *lcm_dts)
{
	unsigned int i;
	unsigned char *tmp;
	unsigned char dts[sizeof(LCM_DATA)*MAX_SIZE];
	int len = 0;
	int tmp_len;

	if (!lcm_dts) {
		DISPERR("%s:%d: Error access to LCM_PARAMS(NULL)\n", __FILE__,
		        __LINE__);
		return;
	}

	// parse LCM init table
	len = disp_fdt_getprop_u8_array(node_offset, "init", dts);
	if (len <= 0) {
		DISPERR("%s:%d: Cannot find LCM init table, cannot skip it! \n", __FILE__, __LINE__);
		return;
	}
	if (len > (sizeof(LCM_DATA)*INIT_SIZE)) {
		DISPERR("%s:%d: LCM init table overflow: %d \n", __FILE__, __LINE__, len);
		return;
	}
	DISPMSG("%s:%d: len: %d \n", __FILE__, __LINE__, len);

	tmp = &(dts[0]);
	for (i=0; i<INIT_SIZE; i++) {
		lcm_dts->init[i].func = (*tmp) & 0xFF;
		lcm_dts->init[i].type = (*(tmp+1)) & 0xFF;
		lcm_dts->init[i].size = (*(tmp+2)) & 0xFF;
		tmp_len = 3;

//		DISPMSG("dts: %d, %d, %d \n", *tmp, *(tmp+1), i);
		switch (lcm_dts->init[i].func) {
			case LCM_FUNC_GPIO:
				memcpy(&(lcm_dts->init[i].data_t1), tmp+3, lcm_dts->init[i].size);
				break;

			case LCM_FUNC_I2C:
				memcpy(&(lcm_dts->init[i].data_t2), tmp+3, lcm_dts->init[i].size);
				break;

			case LCM_FUNC_UTIL:
				memcpy(&(lcm_dts->init[i].data_t1), tmp+3, lcm_dts->init[i].size);
				break;

			case LCM_FUNC_CMD:
				switch (lcm_dts->init[i].type) {
					case LCM_UTIL_WRITE_CMD_V1:
						memcpy(&(lcm_dts->init[i].data_t5), tmp+3, lcm_dts->init[i].size);
						break;

					case LCM_UTIL_WRITE_CMD_V2:
						memcpy(&(lcm_dts->init[i].data_t3), tmp+3, lcm_dts->init[i].size);
						break;

					default:
						DISPERR("%s/%d: %d \n", __FILE__, __LINE__, lcm_dts->init[i].type);
						return;
				}
				break;

			default:
				DISPERR("%s/%d: %d \n", __FILE__, __LINE__, lcm_dts->init[i].func);
				return;
		}
		tmp_len = tmp_len + lcm_dts->init[i].size;

		if (tmp_len < len) {
			tmp = tmp + tmp_len;
			len = len - tmp_len;
		} else {
			break;
		}
	}
	lcm_dts->init_size = i + 1;
	if (lcm_dts->init_size > INIT_SIZE) {
		DISPERR("%s:%d: LCM init table overflow: %d\n", __FILE__, __LINE__, len);
		return;
	}

	// parse LCM compare_id table
	len = disp_fdt_getprop_u8_array(node_offset, "compare_id", dts);
	if (len <= 0) {
		DISPERR("%s:%d: Cannot find LCM compare_id table, skip it! \n", __FILE__, __LINE__);
	} else {
		if (len > (sizeof(LCM_DATA)*COMPARE_ID_SIZE)) {
			DISPERR("%s:%d: LCM compare_id table overflow: %d \n", __FILE__, __LINE__, len);
			return;
		}

		tmp = &(dts[0]);
		for (i=0; i<COMPARE_ID_SIZE; i++) {
			lcm_dts->compare_id[i].func = (*tmp) & 0xFF;
			lcm_dts->compare_id[i].type = (*(tmp+1)) & 0xFF;
			lcm_dts->compare_id[i].size = (*(tmp+2)) & 0xFF;
			tmp_len = 3;

			switch (lcm_dts->compare_id[i].func) {
				case LCM_FUNC_GPIO:
					memcpy(&(lcm_dts->compare_id[i].data_t1), tmp+3, lcm_dts->compare_id[i].size);
					break;

				case LCM_FUNC_I2C:
					memcpy(&(lcm_dts->compare_id[i].data_t2), tmp+3, lcm_dts->compare_id[i].size);
					break;

				case LCM_FUNC_UTIL:
					memcpy(&(lcm_dts->compare_id[i].data_t1), tmp+3, lcm_dts->compare_id[i].size);
					break;

				case LCM_FUNC_CMD:
					switch (lcm_dts->compare_id[i].type) {
						case LCM_UTIL_WRITE_CMD_V1:
							memcpy(&(lcm_dts->compare_id[i].data_t5), tmp+3, lcm_dts->compare_id[i].size);
							break;

						case LCM_UTIL_WRITE_CMD_V2:
							memcpy(&(lcm_dts->compare_id[i].data_t3), tmp+3, lcm_dts->compare_id[i].size);
							break;

						case LCM_UTIL_READ_CMD_V2:
							memcpy(&(lcm_dts->compare_id[i].data_t4), tmp+3, lcm_dts->compare_id[i].size);
							break;

						default:
							DISPERR("%s:%d: %d \n", __FILE__, __LINE__, (unsigned int)lcm_dts->compare_id[i].type);
							return;
					}
					break;

				default:
					DISPERR("%s:%d: %d \n", __FILE__, __LINE__, (unsigned int)lcm_dts->compare_id[i].func);
					return;
			}
			tmp_len = tmp_len + lcm_dts->compare_id[i].size;

			if (tmp_len < len) {
				tmp = tmp + tmp_len;
				len = len - tmp_len;
			} else {
				break;
			}
		}
		lcm_dts->compare_id_size = i + 1;
		if (lcm_dts->compare_id_size > COMPARE_ID_SIZE) {
			DISPERR("%s:%d: LCM compare_id table overflow: %d\n", __FILE__, __LINE__,
			       len);
			return;
		}
	}

	// parse LCM suspend table
	len = disp_fdt_getprop_u8_array(node_offset, "suspend", dts);
	if (len <= 0) {
		DISPERR("%s:%d: Cannot find LCM suspend table, cannot skip it! \n", __FILE__, __LINE__);
		return;
	}
	if (len > (sizeof(LCM_DATA)*SUSPEND_SIZE)) {
		DISPERR("%s:%d: LCM suspend table overflow: %d \n", __FILE__, __LINE__, len);
		return;
	}

	tmp = &(dts[0]);
	for (i=0; i<SUSPEND_SIZE; i++) {
		lcm_dts->suspend[i].func = (*tmp) & 0xFF;
		lcm_dts->suspend[i].type = (*(tmp+1)) & 0xFF;
		lcm_dts->suspend[i].size = (*(tmp+2)) & 0xFF;
		tmp_len = 3;

		switch (lcm_dts->suspend[i].func) {
			case LCM_FUNC_GPIO:
				memcpy(&(lcm_dts->suspend[i].data_t1), tmp+3, lcm_dts->suspend[i].size);
				break;

			case LCM_FUNC_I2C:
				memcpy(&(lcm_dts->suspend[i].data_t2), tmp+3, lcm_dts->suspend[i].size);
				break;

			case LCM_FUNC_UTIL:
				memcpy(&(lcm_dts->suspend[i].data_t1), tmp+3, lcm_dts->suspend[i].size);
				break;

			case LCM_FUNC_CMD:
				switch (lcm_dts->suspend[i].type) {
					case LCM_UTIL_WRITE_CMD_V1:
						memcpy(&(lcm_dts->suspend[i].data_t5), tmp+3, lcm_dts->suspend[i].size);
						break;

					case LCM_UTIL_WRITE_CMD_V2:
						memcpy(&(lcm_dts->suspend[i].data_t3), tmp+3, lcm_dts->suspend[i].size);
						break;

					default:
						DISPERR("%s:%d: %d \n", __FILE__, __LINE__, lcm_dts->suspend[i].type);
						return;
				}
				break;

			default:
				DISPERR("%s:%d: %d \n", __FILE__, __LINE__, (unsigned int)lcm_dts->suspend[i].func);
				return;
		}
		tmp_len = tmp_len + lcm_dts->suspend[i].size;

		if (tmp_len < len) {
			tmp = tmp + tmp_len;
			len = len - tmp_len;
		} else {
			break;
		}
	}
	lcm_dts->suspend_size = i + 1;
	if (lcm_dts->suspend_size > SUSPEND_SIZE) {
		DISPERR("%s:%d: LCM suspend table overflow: %d\n", __FILE__, __LINE__, len);
		return;
	}

	// parse LCM backlight table
	len = disp_fdt_getprop_u8_array(node_offset, "backlight", dts);
	if (len <= 0) {
		DISPERR("%s:%d: Cannot find LCM backlight table, skip it! \n", __FILE__, __LINE__);
	} else {
		if (len > (sizeof(LCM_DATA)*BACKLIGHT_SIZE)) {
			DISPERR("%s:%d: LCM backlight table overflow: %d \n", __FILE__, __LINE__, len);
			return;
		}

		tmp = &(dts[0]);
		for (i=0; i<BACKLIGHT_SIZE; i++) {
			lcm_dts->backlight[i].func = (*tmp) & 0xFF;
			lcm_dts->backlight[i].type = (*(tmp+1)) & 0xFF;
			lcm_dts->backlight[i].size = (*(tmp+2)) & 0xFF;
			tmp_len = 3;

			switch (lcm_dts->backlight[i].func) {
				case LCM_FUNC_GPIO:
					memcpy(&(lcm_dts->backlight[i].data_t1), tmp+3, lcm_dts->backlight[i].size);
					break;

				case LCM_FUNC_I2C:
					memcpy(&(lcm_dts->backlight[i].data_t2), tmp+3, lcm_dts->backlight[i].size);
					break;

				case LCM_FUNC_UTIL:
					memcpy(&(lcm_dts->backlight[i].data_t1), tmp+3, lcm_dts->backlight[i].size);
					break;

				case LCM_FUNC_CMD:
					switch (lcm_dts->backlight[i].type) {
						case LCM_UTIL_WRITE_CMD_V1:
							memcpy(&(lcm_dts->backlight[i].data_t5), tmp+3, lcm_dts->backlight[i].size);
							break;

						case LCM_UTIL_WRITE_CMD_V2:
							memcpy(&(lcm_dts->backlight[i].data_t3), tmp+3, lcm_dts->backlight[i].size);
							break;

						default:
							DISPERR("%s:%d: %d \n", __FILE__, __LINE__, lcm_dts->backlight[i].type);
							return;
					}
					break;

				default:
					DISPERR("%s:%d: %d \n", __FILE__, __LINE__, (unsigned int)lcm_dts->backlight[i].func);
					return;
			}
			tmp_len = tmp_len + lcm_dts->backlight[i].size;

			if (tmp_len < len) {
				tmp = tmp + tmp_len;
				len = len - tmp_len;
			} else {
				break;
			}
		}
		lcm_dts->backlight_size = i + 1;
		if (lcm_dts->backlight_size > BACKLIGHT_SIZE) {
			DISPERR("%s:%d: LCM backlight table overflow: %d\n", __FILE__, __LINE__,
			       len);
			return;
		}
	}
}


int check_lcm_node_from_DT(void)
{
	int offset;
	char lcm_node[128] = {0};

	sprintf(lcm_node, "mediatek,lcm_params-%s", lcm_name_list[0]);
	DISPMSG("LCM PARAMS DT compatible: %s\n", lcm_node);

	/* Load LCM parameters from DT */
	offset = fdt_node_offset_by_compatible(g_fdt, -1, lcm_node);

	if (offset < 0) {
		DISPMSG("LCM PARAMS DT node: Not found, errno %d\n", offset);
		return -1;
	}

	sprintf(lcm_node, "mediatek,lcm_ops-%s", lcm_name_list[0]);
	DISPMSG("LCM OPS DT compatible: %s\n", lcm_node);

	/* Load LCM parameters from DT */
	offset = fdt_node_offset_by_compatible(g_fdt, -1, lcm_node);

	if (offset < 0) {
		DISPMSG("LCM OPS DT node: Not found, errno %d\n", offset);
		return -1;
	}

	return 0;
}


void load_lcm_resources_from_DT(LCM_DRIVER *lcm_drv)
{
	int offset;
	char lcm_node[128] = { 0 };

	if (!lcm_drv) {
		DISPERR("%s:%d: Error access to LCM_DRIVER(NULL)\n", __FILE__,
		        __LINE__);
		return;
	}

	memset((unsigned char*)(&lcm_dts), 0x0, sizeof(LCM_DTS));

	sprintf(lcm_node, "mediatek,lcm_params-%s", lcm_name_list[0]);
	DISPMSG("LCM PARAMS DT compatible: %s\n", lcm_node);

	/* Load LCM parameters from DT */
	offset = fdt_node_offset_by_compatible(g_fdt, -1, lcm_node);

	if (offset < 0)
		DISPMSG("LCM PARAMS DT node: Not found, errno %d\n", offset);
	else
		parse_lcm_params_dt_node(offset, &(lcm_dts.params));

	sprintf(lcm_node, "mediatek,lcm_ops-%s", lcm_name_list[0]);
	DISPMSG("LCM OPS DT compatible: %s\n", lcm_node);

	/* Load LCM operations from DT */
	offset = fdt_node_offset_by_compatible(g_fdt, -1, lcm_node);

	if (offset < 0)
		DISPMSG("LCM OPS DT node: Not found, errno %d\n", offset);
	else
		parse_lcm_ops_dt_node(offset, &lcm_dts);

	if (lcm_drv->parse_dts) {
		lcm_drv->parse_dts(&lcm_dts, 1);
		lcm_drv->name = lcm_name_list[0];
	} else
		DISPMSG("LCM parse_dts: Not found \n");
}
#endif

// if lcm handle already probed, should return lcm handle directly
disp_lcm_handle* disp_lcm_probe(char* plcm_name, LCM_INTERFACE_ID lcm_id)
{
	DISPFUNC();

	int ret = 0;
	bool isLCMFound = false;
	bool isLCMConnected = false;

	LCM_DRIVER *lcm_drv = NULL;
	LCM_PARAMS *lcm_param = NULL;
	disp_lcm_handle *plcm = NULL;

#if defined(MTK_LCM_DEVICE_TREE_SUPPORT)
	bool isLCMDtFound = false;

	if (check_lcm_node_from_DT() == 0) {
		lcm_drv = &lcm_common_drv;
		isLCMFound = true;
		isLCMDtFound = true;
	} else
#endif
		if (_lcm_count() == 0) {
			DISPERR("no lcm driver defined in linux kernel driver\n");
			return NULL;
		} else if (_lcm_count() == 1) {
			lcm_drv = lcm_driver_list[0];
			isLCMFound = true;
		} else {
			// in lk, plcm_name should always be NULL
			if (plcm_name == NULL) {
				int i = 0;
				disp_path_handle handle = NULL;
				disp_lcm_handle hlcm;
				disp_lcm_handle *plcm = &hlcm;
				LCM_PARAMS hlcm_param;

				for (i=0; i<_lcm_count(); i++) {
					memset((void*)&hlcm, 0, sizeof(disp_lcm_handle));
					memset((void*)&hlcm_param, 0, sizeof(LCM_PARAMS));

					lcm_drv= lcm_driver_list[i];
					lcm_drv->get_params(&hlcm_param);
					plcm->drv = lcm_drv;
					plcm->params = &hlcm_param;
					plcm->lcm_if_id = plcm->params->lcm_if;
					DISPMSG("we will check lcm: %s\n", lcm_drv->name);
					if (lcm_id == LCM_INTERFACE_NOTDEFINED ||(lcm_id != LCM_INTERFACE_NOTDEFINED && plcm->lcm_if_id == lcm_id)) {
						handle = _display_interface_path_init(plcm);
						if (handle == NULL) {
							DISPERR("_display_interface_path_init returns NULL\n");
							goto FAIL;
						}

						if (lcm_drv->init_power) {
							lcm_drv->init_power();
						}

						if (lcm_drv->compare_id != NULL) {
							if (lcm_drv->compare_id() != 0) {
								isLCMFound = true;
								_display_interface_path_deinit(handle);
								dprintf(0,"we will use lcm: %s\n", lcm_drv->name);
								break;
							}
						}

						_display_interface_path_deinit(handle);
					}
				}

				if (isLCMFound == false) {
					DISPERR("we have checked all lcm driver, but no lcm found\n");
					lcm_drv = lcm_driver_list[0];
					isLCMFound = true;
				}
			} else {
				int i = 0;
				for (i=0; i<_lcm_count(); i++) {
					lcm_drv = lcm_driver_list[i];
					if (!strcmp(lcm_drv->name, plcm_name)) {
						isLCMFound = true;
						break;
					}
				}
			}
		}

	if (isLCMFound == false) {
		DISPERR("FATAL ERROR!!!No LCM Driver defined\n");
		return NULL;
	}

	plcm = &_disp_lcm_driver[0];
	lcm_param = &_disp_lcm_params;
	if (plcm && lcm_param) {
		plcm->params = lcm_param;
		plcm->drv = lcm_drv;
	} else {
		DISPERR("FATAL ERROR!!!kzalloc plcm and plcm->params failed\n");
		goto FAIL;
	}

#if defined(MTK_LCM_DEVICE_TREE_SUPPORT)
	if (isLCMDtFound == true)
		load_lcm_resources_from_DT(plcm->drv);
#endif

	plcm->drv->get_params(plcm->params);
	plcm->lcm_if_id = plcm->params->lcm_if;
	// below code is for lcm driver forward compatible
	if (plcm->params->type == LCM_TYPE_DSI && plcm->params->lcm_if == LCM_INTERFACE_NOTDEFINED) plcm->lcm_if_id = LCM_INTERFACE_DSI0;
	if (plcm->params->type == LCM_TYPE_DPI && plcm->params->lcm_if == LCM_INTERFACE_NOTDEFINED) plcm->lcm_if_id = LCM_INTERFACE_DPI0;
	if (plcm->params->type == LCM_TYPE_DBI && plcm->params->lcm_if == LCM_INTERFACE_NOTDEFINED) plcm->lcm_if_id = LCM_INTERFACE_DBI0;
#if 1
	plcm->is_connected = isLCMConnected;
#endif

	_dump_lcm_info(plcm);
	return plcm;

FAIL:

	return NULL;
}

extern int ddp_dsi_dump(DISP_MODULE_ENUM module, int level);
extern int DSI_BIST_Pattern_Test(DISP_MODULE_ENUM module, void* cmdq, bool enable, unsigned int color);
extern int ddp_dsi_start(DISP_MODULE_ENUM module, cmdqRecHandle cmdq);

int disp_lcm_init(disp_lcm_handle *plcm)
{
	DISPFUNC();
	LCM_DRIVER *lcm_drv = NULL;
	bool isLCMConnected = false;

	if (_is_lcm_inited(plcm)) {
		lcm_drv = plcm->drv;

		if (lcm_drv->init_power) {
			DISPCHECK("lcm init_power \n");
			lcm_drv->init_power();
		}

		if (lcm_drv->init) {
			if (!disp_lcm_is_inited(plcm)) {
				DISPCHECK("lcm init \n");
				lcm_drv->init();
			}
		} else {
			DISPERR("FATAL ERROR, lcm_drv->init is null\n");
			return -1;
		}

#ifndef MACH_FPGA
		if (LCM_TYPE_DSI == plcm->params->type) {
			int ret = 0;
			char buffer = 0;

			// FIXME
			//ret = DSI_dcs_read_lcm_reg_v2(_get_dst_module_by_lcm(plcm), NULL, 0x0A, &buffer,1); //[cc]
			ret = 1;//FIXME: workaround
			DISPMSG("read: %d\n", buffer);
			if (ret == 0) {
				isLCMConnected = 0;
				DISPMSG("lcm is not connected\n");
			} else {
				isLCMConnected = 1;
				DISPMSG("lcm is connected\n");
			}
		}
		if (plcm->params->dsi.edp_panel == 1) {
			isLCMConnected = 1;
		}

#else
		isLCMConnected = 1;
#endif
		plcm->is_connected = isLCMConnected;

		//ddp_dsi_start(DISP_MODULE_DSI0, NULL);
		//DSI_BIST_Pattern_Test(DISP_MODULE_DSI0,NULL,true, 0x00ffff00);
		//ddp_dsi_dump(DISP_MODULE_DSI0, 2);
		return 0;
	} else {
		DISPERR("plcm is null\n");
		return -1;
	}
}

LCM_PARAMS* disp_lcm_get_params(disp_lcm_handle *plcm)
{
	DISPFUNC();

	if (_is_lcm_inited(plcm))
		return plcm->params;
	else
		return NULL;
}

const char* disp_lcm_get_name(disp_lcm_handle *plcm)
{
	DISPFUNC();

	if (_is_lcm_inited(plcm))
		return plcm->drv->name;
	else
		return NULL;
}

int disp_lcm_width(disp_lcm_handle *plcm)
{
	LCM_PARAMS *plcm_param = NULL;
	int w = 0;
	if (_is_lcm_inited(plcm))
		plcm_param = _get_lcm_params_by_handle(plcm);
	if (plcm_param == NULL) {
		DISPERR("plcm_param is null\n");
		return 0;
	}

	w = plcm_param->virtual_width;
	if (w == 0)
		w = plcm_param->width;

	return w;
}

int disp_lcm_height(disp_lcm_handle *plcm)
{
	LCM_PARAMS *plcm_param = NULL;
	int h = 0;
	if (_is_lcm_inited(plcm))
		plcm_param = _get_lcm_params_by_handle(plcm);
	if (plcm_param == NULL) {
		DISPERR("plcm_param is null\n");
		return 0;
	}

	h = plcm_param->virtual_height;
	if (h == 0)
		h = plcm_param->height;

	return h;
}

int disp_lcm_x(disp_lcm_handle *plcm)
{
	LCM_PARAMS *plcm_param = NULL;
	if (_is_lcm_inited(plcm))
		plcm_param = _get_lcm_params_by_handle(plcm);
	if (plcm_param == NULL) {
		DISPERR("plcm_param is null\n");
		return 0;
	}
	return plcm_param->lcm_x;
}

int disp_lcm_y(disp_lcm_handle *plcm)
{
	LCM_PARAMS *plcm_param = NULL;
	if (_is_lcm_inited(plcm))
		plcm_param = _get_lcm_params_by_handle(plcm);
	if (plcm_param == NULL) {
		DISPERR("plcm_param is null\n");
		return 0;
	}
	return plcm_param->lcm_y;
}


LCM_INTERFACE_ID disp_lcm_get_interface_id(disp_lcm_handle *plcm)
{
	DISPFUNC();

	if (_is_lcm_inited(plcm))
		return plcm->lcm_if_id;
	else
		return LCM_INTERFACE_NOTDEFINED;
}

int disp_lcm_update(disp_lcm_handle *plcm, int x, int y, int w, int h, int force)
{

	LCM_DRIVER *lcm_drv = NULL;
	LCM_INTERFACE_ID lcm_id = LCM_INTERFACE_NOTDEFINED;
	LCM_PARAMS *plcm_param = NULL;

	if (_is_lcm_inited(plcm)) {
		plcm_param = _get_lcm_params_by_handle(plcm);
		DISPCHECK("(x=%d,y=%d),(w=%d,h=%d)\n",x,y,w,h);
		lcm_drv = plcm->drv;
		if (lcm_drv->update) {
			lcm_drv->update(x, y, w, h);
		} else {
			if (disp_lcm_is_video_mode(plcm)) {
				// do nothing
			} else {
				DISPERR("FATAL ERROR, lcm is cmd mode lcm_drv->update is null\n");
			}
			return -1;
		}

		return 0;
	} else {
		DISPERR("lcm_drv is null\n");
		return -1;
	}
}


int disp_lcm_esd_check(disp_lcm_handle *plcm)
{
	DISPFUNC();
	LCM_DRIVER *lcm_drv = NULL;

	if (_is_lcm_inited(plcm)) {
		lcm_drv = plcm->drv;
		if (lcm_drv->esd_check) {
			lcm_drv->esd_check();
		} else {
			DISPERR("FATAL ERROR, lcm_drv->esd_check is null\n");
			return -1;
		}

		return 0;
	} else {
		DISPERR("lcm_drv is null\n");
		return -1;
	}
}



int disp_lcm_esd_recover(disp_lcm_handle *plcm)
{
	DISPFUNC();
	LCM_DRIVER *lcm_drv = NULL;

	if (_is_lcm_inited(plcm)) {
		lcm_drv = plcm->drv;
		if (lcm_drv->esd_recover) {
			lcm_drv->esd_recover();
		} else {
			DISPERR("FATAL ERROR, lcm_drv->esd_check is null\n");
			return -1;
		}

		return 0;
	} else {
		DISPERR("lcm_drv is null\n");
		return -1;
	}
}

int disp_lcm_suspend(disp_lcm_handle *plcm)
{
	DISPFUNC();
	LCM_DRIVER *lcm_drv = NULL;

	if (_is_lcm_inited(plcm)) {
		lcm_drv = plcm->drv;
		if (lcm_drv->suspend) {
			lcm_drv->suspend();
		} else {
			DISPERR("FATAL ERROR, lcm_drv->suspend is null\n");
			return -1;
		}

		if (lcm_drv->suspend_power) {
			lcm_drv->suspend_power();
		}

		return 0;
	} else {
		DISPERR("lcm_drv is null\n");
		return -1;
	}
}

int disp_lcm_resume(disp_lcm_handle *plcm)
{
	DISPFUNC();
	LCM_DRIVER *lcm_drv = NULL;

	if (_is_lcm_inited(plcm)) {
		lcm_drv = plcm->drv;

		if (lcm_drv->resume_power) {
			lcm_drv->resume_power();
		}

		if (lcm_drv->resume) {
			lcm_drv->resume();
		} else {
			DISPERR("FATAL ERROR, lcm_drv->resume is null\n");
			return -1;
		}

		return 0;
	} else {
		DISPERR("lcm_drv is null\n");
		return -1;
	}
}



#ifdef MY_TODO
#error "maybe CABC can be moved into lcm_ioctl??"
#endif
int disp_lcm_set_backlight(disp_lcm_handle *plcm, int level)
{
	DISPFUNC();
	LCM_DRIVER *lcm_drv = NULL;

	if (_is_lcm_inited(plcm)) {
		lcm_drv = plcm->drv;
		if (lcm_drv->set_backlight) {
			lcm_drv->set_backlight(level);
		} else {
			DISPERR("FATAL ERROR, lcm_drv->esd_check is null\n");
			return -1;
		}

		return 0;
	} else {
		DISPERR("lcm_drv is null\n");
		return -1;
	}
}




int disp_lcm_ioctl(disp_lcm_handle *plcm, LCM_IOCTL ioctl, unsigned int arg)
{

}

int disp_lcm_is_inited(disp_lcm_handle *plcm)
{
	if (_is_lcm_inited(plcm))
		return plcm->is_inited;
	else
		return 0;
}

int disp_lcm_is_video_mode(disp_lcm_handle *plcm)
{
	DISPFUNC();
	LCM_PARAMS *lcm_param = NULL;
	LCM_INTERFACE_ID lcm_id = LCM_INTERFACE_NOTDEFINED;

	if (_is_lcm_inited(plcm))
		lcm_param =  plcm->params;
	else
		ASSERT(0);

	switch (lcm_param->type) {
		case LCM_TYPE_DBI:
			return FALSE;
			break;
		case LCM_TYPE_DSI:
			break;
		case LCM_TYPE_DPI:
			return TRUE;
			break;
		default:
			DISPMSG("[LCM] TYPE: unknown\n");
			break;
	}

	if (lcm_param->type == LCM_TYPE_DSI) {
		switch (lcm_param->dsi.mode) {
			case CMD_MODE:
				return FALSE;
				break;
			case SYNC_PULSE_VDO_MODE:
			case SYNC_EVENT_VDO_MODE:
			case BURST_VDO_MODE:
				return TRUE;
				break;
			default:
				DISPMSG("[LCM] DSI Mode: Unknown\n");
				break;
		}
	}

	ASSERT(0);
}

