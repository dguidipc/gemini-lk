
#define LOG_TAG "ddp_path"

#include "platform/mt_irq.h"
#include "platform/mt_irq.h"

#include "platform/disp_drv_platform.h"
#include "platform/ddp_reg.h"
#include "platform/ddp_path.h"
#include "platform/ddp_info.h"
#include "platform/ddp_log.h"

typedef struct module_map_s {
	DISP_MODULE_ENUM module;
	int bit;
} module_map_t;

typedef struct {
	int m;
	int v;
} m_to_b;

typedef struct mout_s {
	int id;
	m_to_b out_id_bit_map[5];
	volatile unsigned int *reg;
	unsigned int reg_val;
} mout_t;

typedef struct selection_s {
	int id;
	int id_bit_map[5];
	volatile unsigned int *reg;
	unsigned int reg_val;
} sel_t;

#define DDP_ENING_NUM    16

#define DDP_MOUT_NUM     5
#define DDP_SEL_OUT_NUM  5
#define DDP_SEL_IN_NUM   12
#define DDP_MUTEX_MAX    5

unsigned int module_list_scenario[DDP_SCENARIO_MAX][DDP_ENING_NUM] = {
	/*PRIMARY_DISP*/
	{
		DISP_MODULE_OVL0, DISP_MODULE_OVL0_2L, DISP_MODULE_OVL1_2L, DISP_MODULE_OVL0_VIRTUAL,
		DISP_MODULE_COLOR0, DISP_MODULE_CCORR, DISP_MODULE_AAL, DISP_MODULE_GAMMA, DISP_MODULE_OD,
		DISP_MODULE_DITHER, DISP_MODULE_RDMA0, DISP_PATH0, DISP_MODULE_UFOE,
		DISP_MODULE_PWM0, DISP_MODULE_DSI0, -1
	},
	/*PRIMARY_RDMA0_COLOR0_DISP*/ {DISP_MODULE_RDMA0, DISP_MODULE_COLOR0, DISP_MODULE_CCORR, DISP_MODULE_AAL,   DISP_MODULE_GAMMA, DISP_MODULE_DITHER,DISP_MODULE_UFOE, DISP_MODULE_PWM0, DISP_MODULE_DSI0, -1,  -1, -1},
	/*PRIMARY_RDMA0_DISP*/         {DISP_MODULE_RDMA0, DISP_MODULE_DSI0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
	/*PRIMARY_BYPASS_RDMA*/       {DISP_MODULE_OVL0,  DISP_MODULE_COLOR0, DISP_MODULE_CCORR, DISP_MODULE_AAL,   DISP_MODULE_GAMMA, DISP_MODULE_DITHER, DISP_MODULE_UFOE,  DISP_MODULE_PWM0, DISP_MODULE_DSI0, -1, -1, -1},
	/*PRIMARY_OVL_MEMOUT*/        {DISP_MODULE_OVL0,  DISP_MODULE_WDMA0,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	/*PRIMARY_DITHER_MEMOUT*/     {DISP_MODULE_OVL0,  DISP_MODULE_COLOR0, DISP_MODULE_CCORR, DISP_MODULE_AAL,   DISP_MODULE_GAMMA, DISP_MODULE_DITHER, DISP_MODULE_WDMA0, -1, -1, -1, -1, -1},
	/*PRIMARY_UFOE_MEMOUT*/       {DISP_MODULE_OVL0,  DISP_MODULE_COLOR0, DISP_MODULE_CCORR,DISP_MODULE_AAL,   DISP_MODULE_GAMMA, DISP_MODULE_DITHER, DISP_MODULE_RDMA0, DISP_MODULE_UFOE, DISP_MODULE_WDMA0,-1, -1, -1},
	/*SUB_DISP*/                   {DISP_MODULE_OVL1,  DISP_MODULE_RDMA1,  DISP_MODULE_DPI, -1,-1,-1,-1,-1,-1,-1,-1},
	/*SUB_RDMA1_DISP*/            {DISP_MODULE_RDMA1, DISP_MODULE_DPI, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	/*SUB_OVL_MEMOUT*/            {DISP_MODULE_OVL1,  DISP_MODULE_WDMA1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
	/*PRIMARY_ALL*/                {DISP_MODULE_OVL0,  DISP_MODULE_WDMA0,  DISP_MODULE_COLOR0,DISP_MODULE_CCORR, DISP_MODULE_AAL,   DISP_MODULE_GAMMA,  DISP_MODULE_DITHER,DISP_MODULE_RDMA0,DISP_MODULE_PWM0, DISP_MODULE_DSI0, -1, -1},
	/*SUB_ALL*/                    {DISP_MODULE_OVL1,  DISP_MODULE_WDMA1,  DISP_MODULE_RDMA1,  DISP_MODULE_DPI,  -1,-1, -1,-1,-1,-1, -1,-1},
	/*MULTIPLE_OVL*/               {DISP_MODULE_OVL1,  DISP_MODULE_OVL0,   DISP_MODULE_WDMA0, -1, -1, -1, -1, -1, -1,-1,-1,-1},
};

// 1st para is mout's input, 2nd para is mout's output
static mout_t mout_map[DDP_MOUT_NUM] = {
	/* OVL_MOUT */
	{DISP_MODULE_OVL0_VIRTUAL,
		{{DISP_MODULE_COLOR0, 1 << 0}, {DISP_MODULE_WDMA0, 1 << 1}, {-1, 0} },
		DISP_REG_CONFIG_DISP_OVL0_MOUT_EN, 0},
	{DISP_MODULE_OVL1,
		{{DISP_MODULE_RDMA1, 1 << 0}, {DISP_MODULE_WDMA1, 1 << 1}, {DISP_MODULE_OVL0_VIRTUAL, 1 << 2},
			{-1, 0} },
		DISP_REG_CONFIG_DISP_OVL1_MOUT_EN, 0},
	/* DITHER0_MOUT */
	{DISP_MODULE_DITHER,
		{{DISP_MODULE_RDMA0, 1 << 0}, {DISP_PATH0, 1 << 1}, {DISP_MODULE_WDMA0, 1 << 2},
			{-1, 0} },
		DISP_REG_CONFIG_DISP_DITHER_MOUT_EN, 0},
	/* UFOE_MOUT */
	{DISP_MODULE_UFOE,
		{{DISP_MODULE_DSI0, 1 << 0}, {DISP_MODULE_SPLIT0, 1 << 1},
			{DISP_MODULE_DPI, 1 << 2}, {DISP_MODULE_WDMA0, 1 << 3},
			{DISP_MODULE_DSI1, 1 << 4} },
		DISP_REG_CONFIG_DISP_UFOE_MOUT_EN, 0},
	/* DSC_MOUT */
	{DISP_MODULE_DSC,
		{{DISP_MODULE_DSI0, 1 << 0}, {DISP_MODULE_DSI1, 1 << 1},
			{DISP_MODULE_DPI, 1 << 2}, {DISP_MODULE_WDMA1, 1 << 3},
			{-1, 0} },
		DISP_REG_CONFIG_DISP_DSC_MOUT_EN, 0}
};

static sel_t sel_out_map[DDP_SEL_OUT_NUM] = {
	/* OVL0_SOUT */
	{DISP_MODULE_OVL0_2L,
		{DISP_MODULE_OVL0_VIRTUAL, DISP_MODULE_OVL1_2L, -1},
		DISP_REG_CONFIG_DISP_OVL0_SOUT_SEL_IN, 0},
	/* OVL1_SOUT */
	{DISP_MODULE_OVL1_2L,
		{DISP_MODULE_OVL1, DISP_MODULE_OVL0_VIRTUAL, -1},
		DISP_REG_CONFIG_DISP_OVL1_SOUT_SEL_IN, 0},
	/* DISP_PATH_SOUT */
	{DISP_PATH0,
		{DISP_MODULE_UFOE, DISP_MODULE_DSC, -1},
		DISP_REG_CONFIG_DISP_PATH0_SOUT_SEL_IN, 0},
	/* RDMA0_SOUT */
	{DISP_MODULE_RDMA0,
		{DISP_PATH0, DISP_MODULE_COLOR0, DISP_MODULE_DSI0, DISP_MODULE_DSI1, DISP_MODULE_DPI},
		DISP_REG_CONFIG_DISP_RDMA0_SOUT_SEL_IN, 0},
	/* RDMA1_SOUT */
	{DISP_MODULE_RDMA1,
		{DISP_MODULE_UFOE, DISP_MODULE_DSC, -1},
		DISP_REG_CONFIG_DISP_RDMA1_SOUT_SEL_IN, 0},
};

// 1st para is sout's output, 2nd para is sout's input
static sel_t sel_in_map[DDP_SEL_IN_NUM] = {
	/* OVL0_SEL */
	{DISP_MODULE_OVL0_VIRTUAL,
		{DISP_MODULE_OVL0_2L, DISP_MODULE_OVL1, DISP_MODULE_OVL1_2L, -1},
		DISP_REG_CONFIG_DISP_OVL0_SEL_IN, 0},

	/* COLOR_SEL */
	{DISP_MODULE_COLOR0,
		{DISP_MODULE_RDMA0, DISP_MODULE_OVL0_VIRTUAL, -1},
		DISP_REG_CONFIG_DISP_COLOR0_SEL_IN, 0},

	/* DISP_PATH0_SEL */
	{DISP_PATH0,
		{DISP_MODULE_RDMA0, DISP_MODULE_DITHER, -1},
		DISP_REG_CONFIG_DISP_PATH0_SEL_IN, 0},

	/* UFOE_SEL */
	{DISP_MODULE_UFOE,
		{DISP_PATH0, DISP_MODULE_RDMA1, -1},
		DISP_REG_CONFIG_DISP_UFOE_SEL_IN, 0},

	/* DSC_SEL */
	{DISP_MODULE_DSC,
		{DISP_PATH0, DISP_MODULE_RDMA1, -1},
		DISP_REG_CONFIG_DISP_DSC_SEL_IN, 0},

	/* DSI_SEL */
	{DISP_MODULE_DSI0,
		{DISP_MODULE_UFOE, DISP_MODULE_SPLIT0, DISP_MODULE_RDMA0, DISP_MODULE_DSC, -1},
		DISP_REG_CONFIG_DSI0_SEL_IN, 0},
	{DISP_MODULE_DSI1,
		{DISP_MODULE_SPLIT0, DISP_MODULE_RDMA0, DISP_MODULE_DSC, DISP_MODULE_UFOE, -1},
		DISP_REG_CONFIG_DSI1_SEL_IN, 0},
	{DISP_MODULE_DSIDUAL,
		{DISP_MODULE_UFOE, DISP_MODULE_SPLIT0, -1, -1, -1},
		DISP_REG_CONFIG_DSI0_SEL_IN, 0},
	{DISP_MODULE_DSIDUAL,
		{DISP_MODULE_SPLIT0, -1, -1, -1, -1},
		DISP_REG_CONFIG_DSI1_SEL_IN, 0},

	/* DPI0_SEL */
	{DISP_MODULE_DPI,
		{DISP_MODULE_UFOE, DISP_MODULE_RDMA0, DISP_MODULE_DSC, -1},
		DISP_REG_CONFIG_DPI0_SEL_IN, 0},

	/* WDMA_SEL */
	{DISP_MODULE_WDMA0,
		{DISP_MODULE_OVL0_VIRTUAL, DISP_MODULE_DITHER, DISP_MODULE_UFOE, -1},
		DISP_REG_CONFIG_DISP_WDMA0_SEL_IN, 0},
	{DISP_MODULE_WDMA1,
		{DISP_MODULE_OVL1, DISP_MODULE_DSC, -1},
		DISP_REG_CONFIG_DISP_WDMA1_SEL_IN, 0},
};


//module bit in mutex
static module_map_t module_mutex_map[DISP_MODULE_NUM] = {
	{DISP_MODULE_OVL0, 10},
	{DISP_MODULE_OVL1, 11},
	{DISP_MODULE_OVL0_2L, 12},
	{DISP_MODULE_OVL1_2L, 15},
	{DISP_MODULE_OVL0_VIRTUAL, -1},
	{DISP_MODULE_RDMA0, 13},
	{DISP_MODULE_RDMA1, 14},
	{DISP_MODULE_WDMA0, 16},
	{DISP_MODULE_COLOR0, 18},
	{DISP_MODULE_CCORR, 19},
	{DISP_MODULE_AAL, 20},
	{DISP_MODULE_GAMMA, 21},
	{DISP_MODULE_DITHER, 23},
	{DISP_PATH0, -1},
	{DISP_MODULE_UFOE, 24},
	{DISP_MODULE_DSC, 25},
	{DISP_MODULE_PWM0, 26},
	{DISP_MODULE_WDMA1, 17},
	{DISP_MODULE_DPI, -1},
	{DISP_MODULE_SMI, -1},
	{DISP_MODULE_CONFIG, -1},
	{DISP_MODULE_CMDQ, -1},
	{DISP_MODULE_MUTEX, -1},
	{DISP_MODULE_COLOR1, -1},
	{DISP_MODULE_RDMA2, -1},
	{DISP_MODULE_PWM1, -1},
	{DISP_MODULE_OD, 22},
	{DISP_MODULE_MERGE, -1},
	{DISP_MODULE_SPLIT0, -1},
	{DISP_MODULE_SPLIT1, -1},
	{DISP_MODULE_DSI0, -1},
	{DISP_MODULE_DSI1, -1},
	{DISP_MODULE_DSIDUAL, -1},
	{DISP_MODULE_SMI_LARB0, -1},
	{DISP_MODULE_SMI_LARB5, -1},
	{DISP_MODULE_SMI_COMMON, -1},
	{DISP_MODULE_MIPI0, -1},
	{DISP_MODULE_MIPI1, -1},
	{DISP_MODULE_UNKNOWN, -1},
};

//module can be connect if 1
static module_map_t module_can_connect[DISP_MODULE_NUM] = {
	{DISP_MODULE_OVL0, 0},
	{DISP_MODULE_OVL1, 1},
	{DISP_MODULE_OVL0_2L, 1},
	{DISP_MODULE_OVL1_2L, 1},
	{DISP_MODULE_OVL0_VIRTUAL, 1},
	{DISP_MODULE_RDMA0, 1},
	{DISP_MODULE_RDMA1, 1},
	{DISP_MODULE_WDMA0, 1},
	{DISP_MODULE_COLOR0, 1},
	{DISP_MODULE_CCORR, 0},
	{DISP_MODULE_AAL, 0},
	{DISP_MODULE_GAMMA, 0},
	{DISP_MODULE_DITHER, 1},
	{DISP_PATH0, 1},
	{DISP_MODULE_UFOE, 1},
	{DISP_MODULE_DSC, 1},
	{DISP_MODULE_PWM0, 0},
	{DISP_MODULE_WDMA1, 1},
	{DISP_MODULE_DPI, 1},
	{DISP_MODULE_SMI, 0},
	{DISP_MODULE_CONFIG, 0},
	{DISP_MODULE_CMDQ, 0},
	{DISP_MODULE_MUTEX, 0},
	{DISP_MODULE_COLOR1, 0},
	{DISP_MODULE_RDMA2, 0},
	{DISP_MODULE_PWM1, 0},
	{DISP_MODULE_OD, 0},
	{DISP_MODULE_MERGE, 0},
	{DISP_MODULE_SPLIT0, 1},
	{DISP_MODULE_SPLIT1, 0},
	{DISP_MODULE_DSI0, 1},
	{DISP_MODULE_DSI1, 1},
	{DISP_MODULE_DSIDUAL, 1},
	{DISP_MODULE_SMI_LARB0, 0},
	{DISP_MODULE_SMI_LARB5, 0},
	{DISP_MODULE_SMI_COMMON, 0},
	{DISP_MODULE_MIPI0, 0},
	{DISP_MODULE_MIPI1, 0},
	{DISP_MODULE_UNKNOWN, 0},
};


char* ddp_get_scenario_name(DDP_SCENARIO_ENUM scenario)
{
	switch (scenario) {
		case DDP_SCENARIO_PRIMARY_DISP                 :
			return "primary_disp";
		case DDP_SCENARIO_PRIMARY_RDMA0_COLOR0_DISP    :
			return "primary_rdma0_color_disp";
		case DDP_SCENARIO_PRIMARY_RDMA0_DISP           :
			return "primary_rdma0_disp";
		case DDP_SCENARIO_PRIMARY_BYPASS_RDMA          :
			return "primary_bypass_rdma";
		case DDP_SCENARIO_PRIMARY_OVL_MEMOUT           :
			return "primary_ovl_memout";
		case DDP_SCENARIO_PRIMARY_DITHER_MEMOUT        :
			return "primary_dither_memout";
		case DDP_SCENARIO_PRIMARY_UFOE_MEMOUT          :
			return "primary_ufoe_memout";
		case DDP_SCENARIO_SUB_DISP                     :
			return "sub_disp";
		case DDP_SCENARIO_SUB_RDMA1_DISP               :
			return "sub_rdma1_disp";
		case DDP_SCENARIO_SUB_OVL_MEMOUT               :
			return "sub_ovl_memout";
		case DDP_SCENARIO_DISPLAY_INTERFACE            :
			return "display_interface";
		case DDP_SCENARIO_PRIMARY_ALL                  :
			return "primary_all";
		case DDP_SCENARIO_SUB_ALL                      :
			return "sub_all";
		default:
			DDPMSG("invalid scenario id=%d\n", scenario);
			return "unknown";
	}
}

int ddp_is_scenario_on_primary(DDP_SCENARIO_ENUM scenario)
{
	int on_primary = 0;
	switch (scenario) {
		case DDP_SCENARIO_PRIMARY_DISP:
		case DDP_SCENARIO_PRIMARY_RDMA0_COLOR0_DISP:
		case DDP_SCENARIO_PRIMARY_RDMA0_DISP:
		case DDP_SCENARIO_PRIMARY_BYPASS_RDMA:
		case DDP_SCENARIO_PRIMARY_OVL_MEMOUT:
		case DDP_SCENARIO_PRIMARY_DITHER_MEMOUT:
		case DDP_SCENARIO_PRIMARY_UFOE_MEMOUT:
		case DDP_SCENARIO_PRIMARY_ALL:
			on_primary = 1;
			break;
		case DDP_SCENARIO_SUB_DISP:
		case DDP_SCENARIO_SUB_RDMA1_DISP:
		case DDP_SCENARIO_SUB_OVL_MEMOUT:
		case DDP_SCENARIO_SUB_ALL:
			on_primary = 0;
			break;
		default:
			DDPMSG("invalid scenario id=%d\n", scenario);
	}

	return on_primary;

}


char* ddp_get_mutex_sof_name(MUTEX_SOF mode)
{
	switch (mode) {
		case SOF_SINGLE  :
			return "single";
		case SOF_DSI0    :
			return "dsi0";
		case SOF_DSI1    :
			return "dsi1";
		case SOF_DPI0    :
			return "dpi0";
		default:
			DDPMSG("invalid sof =%d\n", mode);
			return "unknown";
	}
}

char* ddp_get_mode_name(DDP_MODE ddp_mode)
{
	switch (ddp_mode) {
		case DDP_VIDEO_MODE  :
			return "vido_mode";
		case DDP_CMD_MODE    :
			return "cmd_mode";
		default:
			DDPMSG("invalid ddp mode =%d\n", ddp_mode);
			return "unknown";
	}
}

static int ddp_get_module_num_l(int* module_list)
{
	unsigned int num = 0;
	while (*(module_list+num)!=-1) {
		num++;
		if (num==DDP_ENING_NUM)
			break;
	}
	return num;
}

// config mout/msel to creat a compelte path
static void ddp_connect_path_l(int *module_list, void * handle)
{
	unsigned int i, j, k;
	int step=0;
	unsigned int mout=0;
	unsigned int reg_mout = 0;
	unsigned int mout_idx = 0;
	unsigned int module_num = ddp_get_module_num_l(module_list);
	unsigned int dst_module;

	DDPDBG("connect_path: %s to %s\n",ddp_get_module_name(module_list[0]),
	       ddp_get_module_name(module_list[module_num-1]));
	// connect mout
	for (i = 0 ; i < module_num - 1 ; i++) {
		for (j = 0 ; j < DDP_MOUT_NUM ; j++) {
			if (module_list[i] == mout_map[j].id) {
				//find next module which can be connected
				step = i+1;
				while (module_can_connect[module_list[step]].bit==0 && step<module_num) {
					step++;
				}
				ASSERT(step<module_num);
				mout = mout_map[j].reg_val;
				for (k = 0 ; k < 5 ; k++) {
					if (mout_map[j].out_id_bit_map[k].m == -1)
						break;
					if (mout_map[j].out_id_bit_map[k].m == module_list[step]) {
						mout |= mout_map[j].out_id_bit_map[k].v;
						reg_mout |= mout;
						mout_idx = j;
						break;
					}
				}
				mout_map[j].reg_val = mout;
				mout = 0;
			}
		}
		if (reg_mout) {
			DISP_REG_SET(handle,mout_map[mout_idx].reg, reg_mout);
			reg_mout = 0;
			mout_idx = 0;
		}

	}
	// connect out select
	for (i = 0 ; i < module_num - 1 ; i++) {
		for (j = 0 ; j < DDP_SEL_OUT_NUM ; j++) {
			if (module_list[i] == sel_out_map[j].id) {
				step = i+1;
				//find next module which can be connected
				while (module_can_connect[module_list[step]].bit==0 && step<module_num) {
					step++;
				}
				ASSERT(step<module_num);
				for (k = 0 ; k < 4 ; k++) {
					if (sel_out_map[j].id_bit_map[k] == -1)
						break;
					if (sel_out_map[j].id_bit_map[k] == module_list[step]) {
						DISP_REG_SET(handle,sel_out_map[j].reg, (kal_uint16)k);
						break;
					}
				}
			}
		}
	}
	// connect input select
	for (i = 1 ; i < module_num ; i++) {
		for (j = 0 ; j < DDP_SEL_IN_NUM ; j++) {
			if (module_list[i] == sel_in_map[j].id) {
				step = i-1;
				//find next module which can be connected
				while (module_can_connect[module_list[step]].bit==0 && step>0) {
					step--;
				}
				ASSERT(step>=0);
				for (k = 0 ; k < 4 ; k++) {
					if (sel_in_map[j].id_bit_map[k] == -1)
						break;
					if (sel_in_map[j].id_bit_map[k] == module_list[step]) {
						DISP_REG_SET(handle,sel_in_map[j].reg, (kal_uint16)k);
						break;
					}
				}
			}
		}
	}
}

static void ddp_check_path_l(int *module_list)
{
	unsigned int i, j, k;
	int step = 0;
	int valid =0;
	unsigned int mout;
	unsigned int path_error = 0;
	unsigned int module_num = ddp_get_module_num_l(module_list);
	unsigned int dst_module;
	DDPDBG("check_path: %s to %s\n",ddp_get_module_name(module_list[0])
	       ,ddp_get_module_name(module_list[module_num-1]));
	// check mout
	for (i = 0 ; i < module_num - 1 ; i++) {
		for (j = 0 ; j < DDP_MOUT_NUM ; j++) {
			if (module_list[i] == mout_map[j].id) {
				mout = 0;
				//find next module which can be connected
				step = i+1;
				while (module_can_connect[module_list[step]].bit==0 && step<module_num) {
					step++;
				}
				ASSERT(step<module_num);
				for (k = 0 ; k < 5 ; k++) {
					if (mout_map[j].out_id_bit_map[k].m == -1)
						break;
					if (mout_map[j].out_id_bit_map[k].m == module_list[step]) {
						mout |= mout_map[j].out_id_bit_map[k].v;
						valid =1;
						break;
					}
				}
				if (valid) {
					valid =0;
					if ((DISP_REG_GET(mout_map[j].reg) & mout)==0) {
						path_error += 1;
						DDPERR("%s mout, expect=0x%x, real=0x%x \n",
						       ddp_get_module_name(module_list[i]),
						       mout,
						       DISP_REG_GET(mout_map[j].reg));
					} else if (DISP_REG_GET(mout_map[j].reg) != mout) {
						DDPMSG("warning: %s mout expect=0x%x, real=0x%x \n",
						       ddp_get_module_name(module_list[i]),
						       mout,
						       DISP_REG_GET(mout_map[j].reg));
					}
				}
				break;
			}
		}
	}
	// check out select
	for (i = 0 ; i < module_num - 1 ; i++) {
		for (j = 0 ; j < DDP_SEL_OUT_NUM ; j++) {
			if (module_list[i] == sel_out_map[j].id) {
				//find next module which can be connected
				step = i+1;
				while (module_can_connect[module_list[step]].bit==0 && step<module_num) {
					step++;
				}
				ASSERT(step<module_num);
				for (k = 0 ; k < 4 ; k++) {
					if (sel_out_map[j].id_bit_map[k] == -1)
						break;
					if (sel_out_map[j].id_bit_map[k] == module_list[step]) {
						if (DISP_REG_GET(sel_out_map[j].reg) != k) {
							path_error += 1;
							DDPERR("out_s %s not connect to %s, expect=0x%x, real=0x%x \n",
							       ddp_get_module_name(module_list[i]),ddp_get_module_name(module_list[step]),
							       k,
							       DISP_REG_GET(sel_out_map[j].reg));
						}
						break;
					}
				}
			}
		}
	}
	// check input select
	for (i = 1 ; i < module_num ; i++) {
		for (j = 0 ; j < DDP_SEL_IN_NUM ; j++) {
			if (module_list[i] == sel_in_map[j].id) {
				//find next module which can be connected
				step = i-1;
				while (module_can_connect[module_list[step]].bit==0 && step>0) {
					step--;
				}
				ASSERT(step>=0);
				for (k = 0 ; k < 4 ; k++) {
					if (sel_in_map[j].id_bit_map[k] == -1)
						break;
					if (sel_in_map[j].id_bit_map[k] == module_list[step]) {
						if (DISP_REG_GET(sel_in_map[j].reg) != k) {
							path_error += 1;
							DDPERR("in_s %s not connect to %s, expect=0x%x, real=0x%x \n",
							       ddp_get_module_name(module_list[step]),ddp_get_module_name(module_list[i]),
							       k,
							       DISP_REG_GET(sel_in_map[j].reg));
						}
						break;
					}
				}
			}
		}
	}
	if (path_error == 0) {
		DDPMSG("path: %s to %s is connected\n",ddp_get_module_name(module_list[0]),
		       ddp_get_module_name(module_list[module_num - 1]));
	} else {
		DDPERR("path: %s to %s not connected!!!\n",ddp_get_module_name(module_list[0]),
		       ddp_get_module_name(module_list[module_num - 1]));
	}
}

static void ddp_disconnect_path_l(int *module_list,void * handle)
{
	unsigned int i, j, k;
	int step = 0;
	int valid =0;
	unsigned int mout = 0;
	unsigned int reg_mout = 0;
	unsigned int mout_idx = 0;
	unsigned int module_num = ddp_get_module_num_l(module_list);
	unsigned int dst_module;

	DDPDBG("disconnect_path: %s to %s\n",ddp_get_module_name(module_list[0]),
	       ddp_get_module_name(module_list[module_num-1]));
	for (i = 0 ; i < module_num - 1 ; i++) {
		for (j = 0 ; j < DDP_MOUT_NUM ; j++) {
			if (module_list[i] == mout_map[j].id) {
				//find next module which can be connected
				step = i+1;
				while (module_can_connect[module_list[step]].bit==0 && step<module_num) {
					step++;
				}
				ASSERT(step<module_num);
				for (k = 0 ; k < 5 ; k++) {
					if (mout_map[j].out_id_bit_map[k].m == -1)
						break;
					if (mout_map[j].out_id_bit_map[k].m == module_list[step]) {
						mout |= mout_map[j].out_id_bit_map[k].v;
						reg_mout |= mout;
						mout_idx = j;
						DDPDBG("disconnect mout %s to %s \n", ddp_get_module_name(module_list[i]),
						       ddp_get_module_name(module_list[step]));
						break;
					}
				}
				//update mout_value
				mout_map[j].reg_val &= ~mout;
				mout = 0;
			}
		}
		if (reg_mout) {
			DISP_REG_SET(handle,mout_map[mout_idx].reg,  mout_map[mout_idx].reg_val);
			reg_mout = 0;
			mout_idx = 0;
		}
	}
}


static MUTEX_SOF ddp_get_mutex_sof(DISP_MODULE_ENUM dest_module, DDP_MODE ddp_mode)
{
	MUTEX_SOF mode = SOF_SINGLE;
	switch (dest_module) {
		case DISP_MODULE_DSI0: {
			mode = (ddp_mode==DDP_VIDEO_MODE ? SOF_DSI0 : SOF_SINGLE);
			break;
		}
		case DISP_MODULE_DSI1: {
			mode = (ddp_mode==DDP_VIDEO_MODE ? SOF_DSI1 : SOF_SINGLE);
			break;
		}
		case DISP_MODULE_DSIDUAL: {
			mode = (ddp_mode==DDP_VIDEO_MODE ? SOF_DSI0 : SOF_SINGLE);
			break;
		}
		case DISP_MODULE_DPI: {
			mode = SOF_DPI0;
			break;
		}
		case DISP_MODULE_WDMA0:
		case DISP_MODULE_WDMA1:
			mode = SOF_SINGLE;
			break;
		default:
			DDPERR("get mutex sof, invalid param dst module = %s(%d), dis mode %s\n",
			       ddp_get_module_name(dest_module), dest_module,ddp_get_mode_name(ddp_mode));
	}
	DDPDBG("mutex sof: %s dst module %s:%s\n",
	       ddp_get_mutex_sof_name(mode), ddp_get_module_name(dest_module),ddp_get_mode_name(ddp_mode));
	return mode;
}

// id: mutex ID, 0~5
static int ddp_mutex_set_l(int mutex_id, int *module_list, DDP_MODE ddp_mode, void *handle)
{
	int i=0;
	kal_uint32 value = 0;
	int module_num = ddp_get_module_num_l(module_list);
	MUTEX_SOF mode = ddp_get_mutex_sof(module_list[module_num-1],ddp_mode);

	if (mutex_id < DISP_MUTEX_DDP_FIRST || mutex_id > DISP_MUTEX_DDP_LAST) {
		DDPERR("exceed mutex max (0 ~ %d)\n",DISP_MUTEX_DDP_LAST);
		return -1;
	}
	for (i = 0 ; i < module_num ; i++) {
		if (module_mutex_map[module_list[i]].bit != -1) {
			value |= (1 << module_mutex_map[module_list[i]].bit);
		}
	}
	DISP_REG_SET(handle,DISP_REG_CONFIG_MUTEX_MOD(mutex_id), value);
	DISP_REG_SET(handle,DISP_REG_CONFIG_MUTEX_SOF(mutex_id), ((mode << 6) | (mode)));
	DDPMSG("mutex %d value=0x%x, sof=%s\n",mutex_id, value, ddp_get_mutex_sof_name(mode));
	return 0;
}

static void ddp_check_mutex_l(int mutex_id, int* module_list, DDP_MODE ddp_mode)
{
	int i=0;
	kal_uint32 real_value = 0;
	kal_uint32 expect_value = 0;
	kal_uint32 real_sof  = 0;
	MUTEX_SOF  expect_sof  = SOF_SINGLE;
	int module_num = ddp_get_module_num_l(module_list);

	if (mutex_id < DISP_MUTEX_DDP_FIRST || mutex_id > DISP_MUTEX_DDP_LAST) {
		DDPERR("check mutex fail:exceed mutex max (0 ~ %d)\n",DISP_MUTEX_DDP_LAST);
		return -1;
	}
	real_value = DISP_REG_GET(DISP_REG_CONFIG_MUTEX_MOD(mutex_id));
	for (i = 0; i < module_num; i++) {
		if (module_mutex_map[module_list[i]].bit != -1)
			expect_value |= (1 << module_mutex_map[module_list[i]].bit);
	}
	if (expect_value != real_value ) {
		DDPERR("mutex %d module error expect 0x%x, real 0x%x\n",mutex_id,expect_value,real_value);
	}
	real_sof = (DISP_REG_GET(DISP_REG_CONFIG_MUTEX_SOF(mutex_id)) & 0x3);
	expect_sof = ddp_get_mutex_sof(module_list[module_num-1], ddp_mode);
	if ((kal_uint32)expect_sof != real_sof) {
		DDPERR("mutex %d sof error expect %s, real %s\n", mutex_id,
		       ddp_get_mutex_sof_name(expect_sof),
		       ddp_get_mutex_sof_name((MUTEX_SOF)real_sof));
	}
}

static int ddp_mutex_enable_l(int mutex_idx, void *handle)
{
	DDPDBG("mutex %d enable\n", mutex_idx);
	DISP_REG_SET(handle,DISP_REG_CONFIG_MUTEX_EN(mutex_idx),1);
	return 0;
}

int ddp_get_module_num(DDP_SCENARIO_ENUM scenario)
{
	return ddp_get_module_num_l(module_list_scenario[scenario]);
}

static void ddp_print_scenario(DDP_SCENARIO_ENUM scenario)
{
	int i =0;
	char path[512]= {'\0'};
	int num = ddp_get_module_num(scenario);
	for (i=0; i< num; i++) {
		strcat(path,ddp_get_module_name(module_list_scenario[scenario][i]));
	}
	DDPMSG("scenario %s have modules: %s\n",ddp_get_scenario_name(scenario),path);
}

static int  ddp_find_module_index(DDP_SCENARIO_ENUM ddp_scenario, DISP_MODULE_ENUM module)
{
	int i=0;
	for (i=0; i< DDP_ENING_NUM-1; i++) {
		if (module_list_scenario[ddp_scenario][i] == module) {
			return i;
		}
	}
	DDPDBG("find module: can not find module %s on scenario %s\n",ddp_get_module_name(module),
	       ddp_get_scenario_name(ddp_scenario));
	return -1;
}

// set display interface when kernel init
int ddp_set_dst_module(DDP_SCENARIO_ENUM scenario, DISP_MODULE_ENUM dst_module)
{
	int i = 0;
	int j = 0;
	DDPMSG("ddp_set_dst_module, scenario=%s, dst_module=%s \n",
	       ddp_get_scenario_name(scenario), ddp_get_module_name(dst_module));
	if (ddp_find_module_index(scenario,dst_module) > 0) {
		DDPDBG("%s is already on path\n",ddp_get_module_name(dst_module));
		return 0;
	}
	i = ddp_get_module_num_l(module_list_scenario[scenario])-1;
	if (dst_module == DISP_MODULE_DSIDUAL) {
		module_list_scenario[scenario][i++] = DISP_MODULE_SPLIT0;
		ASSERT(i<DDP_ENING_NUM);
	} else {
		if (ddp_get_dst_module(scenario)==DISP_MODULE_DSIDUAL) {
			module_list_scenario[scenario][i--] = -1;
		}
	}
	module_list_scenario[scenario][i] = dst_module;

	if (scenario == DDP_SCENARIO_PRIMARY_ALL) { //primary display
		ddp_set_dst_module(DDP_SCENARIO_PRIMARY_DISP,dst_module);
		ddp_set_dst_module(DDP_SCENARIO_PRIMARY_OVL_MEMOUT,dst_module);
	} else if (scenario==DDP_SCENARIO_SUB_ALL) { //sub display
		ddp_set_dst_module(DDP_SCENARIO_SUB_DISP,dst_module);
		ddp_set_dst_module(DDP_SCENARIO_SUB_OVL_MEMOUT,dst_module);
	}
	ddp_print_scenario(scenario);
	return 0;
}

DISP_MODULE_ENUM ddp_get_dst_module(DDP_SCENARIO_ENUM ddp_scenario)
{
	DDPMSG("ddp_get_dst_module, scneario=%s, dst_module=%s \n",
	       ddp_get_scenario_name(ddp_scenario),
	       ddp_get_module_name(module_list_scenario[ddp_scenario][ddp_get_module_num_l(module_list_scenario[ddp_scenario])-1]));

	return module_list_scenario[ddp_scenario][ddp_get_module_num_l(module_list_scenario[ddp_scenario])-1];
}

int *ddp_get_scenario_list(DDP_SCENARIO_ENUM ddp_scenario)
{
	return module_list_scenario[ddp_scenario];
}

int ddp_insert_module(DDP_SCENARIO_ENUM ddp_scenario, DISP_MODULE_ENUM place,  DISP_MODULE_ENUM module)
{
	int i = DDP_ENING_NUM-1;
	int idx = ddp_find_module_index(ddp_scenario,place);
	if ( idx < 0) {
		return -1;
	}
	//should not over load
	ASSERT(module_list_scenario[ddp_scenario][i] == -1);
	for (i=DDP_ENING_NUM-2; i>=idx; i--) {
		module_list_scenario[ddp_scenario][i+1] = module_list_scenario[ddp_scenario][i];
	}
	module_list_scenario[ddp_scenario][i] = module;
	{
		int *modules = ddp_get_scenario_list(ddp_scenario);
		int module_num = ddp_get_module_num(ddp_scenario);
		DDPMSG("after insert module, module list is: \n");
		for (i=0; i<module_num; i++) {
			printk("%s-", ddp_get_module_name(modules[i]));
		}
	}
	return 0;
}

int  ddp_remove_module(DDP_SCENARIO_ENUM ddp_scenario, DISP_MODULE_ENUM module)
{
	int i = 0;
	int idx = ddp_find_module_index(ddp_scenario,module);

	if (idx < 0) {
		return -1;
	}
	//should not over load
	ASSERT(module_list_scenario[ddp_scenario][i] == -1);
	for (i=idx; i<DDP_ENING_NUM-1; i++) {
		module_list_scenario[ddp_scenario][i] = module_list_scenario[ddp_scenario][i+1];
	}
	module_list_scenario[ddp_scenario][i] = -1;
	return 0;
}

void ddp_connect_path(DDP_SCENARIO_ENUM scenario, void *handle)
{
	DDPDBG("path connect on scenario %s\n", ddp_get_scenario_name(scenario));
	if (scenario == DDP_SCENARIO_PRIMARY_ALL) {
		ddp_connect_path_l(module_list_scenario[DDP_SCENARIO_PRIMARY_DISP], handle);
		ddp_connect_path_l(module_list_scenario[DDP_SCENARIO_PRIMARY_OVL_MEMOUT],handle);
	} else if (scenario == DDP_SCENARIO_SUB_ALL) {
		ddp_connect_path_l(module_list_scenario[DDP_SCENARIO_SUB_DISP], handle);
		ddp_connect_path_l(module_list_scenario[DDP_SCENARIO_SUB_OVL_MEMOUT], handle);
	} else {
		ddp_connect_path_l(module_list_scenario[scenario], handle);
	}
	return ;
}

void ddp_disconnect_path(DDP_SCENARIO_ENUM scenario, void *handle)
{
	DDPDBG("path disconnect on scenario %s\n", ddp_get_scenario_name(scenario));

	if (scenario == DDP_SCENARIO_PRIMARY_ALL) {
		ddp_disconnect_path_l(module_list_scenario[DDP_SCENARIO_PRIMARY_DISP], handle);
		ddp_disconnect_path_l(module_list_scenario[DDP_SCENARIO_PRIMARY_OVL_MEMOUT], handle);
	} else if (scenario == DDP_SCENARIO_SUB_ALL) {
		ddp_disconnect_path_l(module_list_scenario[DDP_SCENARIO_SUB_DISP], handle);
		ddp_disconnect_path_l(module_list_scenario[DDP_SCENARIO_SUB_OVL_MEMOUT], handle);
	} else {
		ddp_disconnect_path_l(module_list_scenario[scenario], handle);
	}
	return ;
}

void ddp_check_path(DDP_SCENARIO_ENUM scenario)
{
	DDPDBG("path check path on scenario %s\n", ddp_get_scenario_name(scenario));

	if (scenario == DDP_SCENARIO_PRIMARY_ALL) {
		ddp_check_path_l(module_list_scenario[DDP_SCENARIO_PRIMARY_DISP]);
		ddp_check_path_l(module_list_scenario[DDP_SCENARIO_PRIMARY_OVL_MEMOUT]);
	} else if (scenario == DDP_SCENARIO_SUB_ALL) {
		ddp_check_path_l(module_list_scenario[DDP_SCENARIO_SUB_DISP]);
		ddp_check_path_l(module_list_scenario[DDP_SCENARIO_SUB_OVL_MEMOUT]);
	} else {
		ddp_check_path_l(module_list_scenario[scenario]);
	}
	return ;
}

void ddp_check_mutex(int mutex_id, DDP_SCENARIO_ENUM scenario, DDP_MODE mode)
{
	DDPDBG("check mutex %d on scenario %s\n",mutex_id,
	       ddp_get_scenario_name(scenario));
	ddp_check_mutex_l(mutex_id,module_list_scenario[scenario],mode);
	return ;
}

int ddp_mutex_set(int mutex_id,DDP_SCENARIO_ENUM scenario,DDP_MODE mode, void * handle)
{
	return ddp_mutex_set_l(mutex_id, module_list_scenario[scenario],mode, handle);
}

int ddp_mutex_clear(int mutex_id, void * handle)
{
	DDPDBG("mutex %d clear\n", mutex_id);
	DISP_REG_SET(handle,DISP_REG_CONFIG_MUTEX_MOD(mutex_id),0);
	DISP_REG_SET(handle,DISP_REG_CONFIG_MUTEX_SOF(mutex_id),0);
	return 0;
}

int ddp_mutex_enable(int mutex_id,DDP_SCENARIO_ENUM scenario,void * handle)
{
	return ddp_mutex_enable_l(mutex_id, handle);
}

int ddp_mutex_disenable(int mutex_id,DDP_SCENARIO_ENUM scenario,void * handle)
{
	DDPDBG("mutex %d disable\n", mutex_id);
	DISP_REG_SET(handle,DISP_REG_CONFIG_MUTEX_EN(mutex_id),0);
	return 0;
}

int ddp_check_engine_status(int mutexID)
{
	// check engines' clock bit &  enable bit & status bit before unlock mutex
	// should not needed, in comdq do?
	int result = 0;
	return result;
}

int ddp_path_top_clock_on(void)
{
	DDPMSG("ddp path top clock on\n");
	DISP_REG_SET(NULL,DISP_REG_CONFIG_MMSYS_DUMMY0,0xFFFFFFFF);
	ddp_enable_module_clock(DISP_MODULE_SMI);
	ddp_enable_module_clock(DISP_MODULE_MUTEX);
	DDPMSG("ddp CG:%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0));
	return 0;
}

int ddp_path_top_clock_off(void)
{
	DDPMSG("ddp path top clock off\n");
	ddp_disable_module_clock(DISP_MODULE_MUTEX);
	ddp_disable_module_clock(DISP_MODULE_SMI);
	return 0;
}

int ddp_path_m4u_off(void)
{
	DDPMSG("ddp path m4u off\n");
#ifdef MTKFB_NO_M4U
	//m4u disable
	DISP_REG_SET(0,DISP_REG_SMI_LARB_MMU_EN,0);
	DISP_REG_SET(0,DISP_REG_SMI_LARB5_MMU_EN,0);
//	DISP_REG_SET(0,DISP_REG_SMI_LARB_MMU_EN,0xfffffff8);
//	DISP_REG_SET(0,DISP_REG_SMI_LARB5_MMU_EN,0xffffffcf);
#endif
	return 0;
}
