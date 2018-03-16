#define LOG_TAG "RDMA"

#include "platform/ddp_info.h"
#include "platform/ddp_matrix_para.h"
#include "platform/ddp_reg.h"
#include "platform/ddp_log.h"
#include "platform/ddp_rdma.h"
#include "platform/ddp_dump.h"
#include "lcm_drv.h"  // to check if UFO enabled.
//#include "platform/mt_clkmgr.h"

enum RDMA_INPUT_FORMAT {
	RDMA_INPUT_FORMAT_BGR565    = 0,
	RDMA_INPUT_FORMAT_RGB888    = 1,
	RDMA_INPUT_FORMAT_RGBA8888  = 2,
	RDMA_INPUT_FORMAT_ARGB8888  = 3,
	RDMA_INPUT_FORMAT_VYUY      = 4,
	RDMA_INPUT_FORMAT_YVYU      = 5,

	RDMA_INPUT_FORMAT_RGB565    = 6,
	RDMA_INPUT_FORMAT_BGR888    = 7,
	RDMA_INPUT_FORMAT_BGRA8888  = 8,
	RDMA_INPUT_FORMAT_ABGR8888  = 9,
	RDMA_INPUT_FORMAT_UYVY      = 10,
	RDMA_INPUT_FORMAT_YUYV      = 11,

	RDMA_INPUT_FORMAT_UNKNOW    = 32,
};

static enum RDMA_INPUT_FORMAT rdma_input_format_convert(DpColorFormat fmt);
static unsigned int rdma_input_format_byte_swap(enum RDMA_INPUT_FORMAT inputFormat);
static unsigned int rdma_input_format_bpp(enum RDMA_INPUT_FORMAT inputFormat);
static unsigned int rdma_input_format_color_space(enum RDMA_INPUT_FORMAT inputFormat);
static unsigned int rdma_input_format_reg_value(enum RDMA_INPUT_FORMAT inputFormat);
static char *rdma_intput_format_name(int reg );

static unsigned int rdma_index(DISP_MODULE_ENUM module)
{
	int idx = 0;
	switch (module) {
		case DISP_MODULE_RDMA0:
			idx = 0;
			break;
		case DISP_MODULE_RDMA1:
			idx = 1;
			break;
		default:
			DDPERR("invalid rdma module=%d\n", module);// invalid module
			ASSERT(0);
	}
	return idx;
}

int RDMAStart(DISP_MODULE_ENUM module, void * handle)
{
	unsigned int idx = rdma_index(module);
	ASSERT(idx <= RDMA_INSTANCES);

	DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_INT_ENABLE, 0x3F);
	DISP_REG_SET_FIELD(handle,GLOBAL_CON_FLD_ENGINE_EN, idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_GLOBAL_CON, 1);

	return 0;
}

int RDMAStop(DISP_MODULE_ENUM module,void * handle)
{
	unsigned int idx = rdma_index(module);
	ASSERT(idx <= RDMA_INSTANCES);

	DISP_REG_SET_FIELD(handle,GLOBAL_CON_FLD_ENGINE_EN, idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_GLOBAL_CON, 0);
	DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_INT_ENABLE, 0);
	DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_INT_STATUS, 0);
	return 0;
}

int RDMAReset(DISP_MODULE_ENUM module,void * handle)
{
	unsigned int delay_cnt=0;
	static unsigned int cnt=0;

	unsigned int idx = rdma_index(module);

	ASSERT(idx <= RDMA_INSTANCES);

	DISP_REG_SET_FIELD(handle,GLOBAL_CON_FLD_SOFT_RESET, idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_GLOBAL_CON, 1);
	while ((DISP_REG_GET(idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_GLOBAL_CON)&0x700)==0x100) { //polling RESET_STATE is 0
		delay_cnt++;
		if (delay_cnt>10000) {
			DDPERR("RDMAReset(%d) timeout, stage 1! DISP_REG_RDMA_GLOBAL_CON=0x%x \n", idx, DISP_REG_GET(idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_GLOBAL_CON));
			break;
		}
	}
	DISP_REG_SET_FIELD(handle,GLOBAL_CON_FLD_SOFT_RESET, idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_GLOBAL_CON, 0);
	delay_cnt =0;
	while ((DISP_REG_GET(idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_GLOBAL_CON)&0x700)!=0x100) {
		delay_cnt++;
		if (delay_cnt>10000) {
			DDPERR("RDMAReset(%d) timeout, stage 2! DISP_REG_RDMA_GLOBAL_CON=0x%x \n", idx, DISP_REG_GET(idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_GLOBAL_CON));
			break;
		}
	}

	return 0;
}

// set ultra registers
void rdma_set_ultra(unsigned int idx, unsigned int width, unsigned int height, unsigned int bpp, unsigned int frame_rate,void * handle)
{
	// constant
	static const unsigned int blank_overhead = 115; //it is 1.15, need to divide 100 later
	static const unsigned int rdma_fifo_width = 16; //in unit of byte
	static const unsigned int ultra_low_time      = 6; //in unit of us
	static const unsigned int pre_ultra_low_time  = 8; //in unit of us
	static const unsigned int pre_ultra_high_time = 9; //in unit of us
	static const unsigned int fifo_size = 512;
	static const unsigned int fifo_valid_line_ratio = 125; //valid size 1/8 line;
	static const unsigned int fifo_min_size = 32;
	// working variables
	unsigned int consume_levels_per_sec;
	unsigned int ultra_low_level;
	unsigned int pre_ultra_low_level;
	unsigned int pre_ultra_high_level;
	unsigned int ultra_high_ofs;
	unsigned int pre_ultra_low_ofs;
	unsigned int pre_ultra_high_ofs;
	unsigned int fifo_valid_size = 16;

	/* compute fifo valid size */

	fifo_valid_size = (width * bpp * fifo_valid_line_ratio) / (rdma_fifo_width * 1000);
	fifo_valid_size = fifo_valid_size > fifo_min_size ? fifo_valid_size : fifo_min_size;
	//consume_levels_per_sec = ((long long unsigned int)width*height*frame_rate*blank_overhead*bpp)/rdma_fifo_width/100;
	//change calculation order to prevent overflow of unsigned int
	consume_levels_per_sec = (width*height*frame_rate*bpp/rdma_fifo_width/100)*blank_overhead;

	// /1000000 for ultra_low_time in unit of us
	ultra_low_level      = (unsigned int)(ultra_low_time      * consume_levels_per_sec / 1000000 );
	pre_ultra_low_level  = (unsigned int)(pre_ultra_low_time  * consume_levels_per_sec / 1000000 );
	pre_ultra_high_level = (unsigned int)(pre_ultra_high_time * consume_levels_per_sec / 1000000 );

	pre_ultra_low_ofs = pre_ultra_low_level - ultra_low_level;
	ultra_high_ofs = 1;
	pre_ultra_high_ofs = pre_ultra_high_level - pre_ultra_low_level ;
	if (1) {
		ultra_low_level = 0x6b;
		pre_ultra_low_ofs = 0xa0;
		ultra_high_ofs = 1;
		pre_ultra_high_ofs = 1;
	}

	//write ultra_low_level, ultra_high_ofs, pre_ultra_low_ofs, pre_ultra_high_ofs into register DISP_RDMA_MEM_GMC_SETTING_0
	DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_MEM_GMC_SETTING_0,
	             ultra_low_level|(pre_ultra_low_ofs<<8)|(ultra_high_ofs<<16)|(pre_ultra_high_ofs<<24));
	DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_MEM_GMC_SETTING_0, 0x1a01356b);

	DISP_REG_SET_FIELD(handle,FIFO_CON_FLD_OUTPUT_VALID_FIFO_THRESHOLD, idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_FIFO_CON, fifo_valid_size);

}

// fixme: spec has no RDMA format, fix enum definition here
int RDMAConfig(DISP_MODULE_ENUM module,
               enum RDMA_MODE mode,
               unsigned address,
               DpColorFormat inFormat,
               unsigned pitch,
               unsigned width,
               unsigned height,
               unsigned ufoe_enable,
               void * handle)  // ourput setting
{

	unsigned int output_is_yuv = 0;
	enum RDMA_INPUT_FORMAT inputFormat = rdma_input_format_convert(inFormat);
	unsigned int  bpp = rdma_input_format_bpp(inputFormat);
	unsigned int input_is_yuv = rdma_input_format_color_space(inputFormat);
	unsigned int input_swap = rdma_input_format_byte_swap(inputFormat);
	unsigned int input_format_reg = rdma_input_format_reg_value(inputFormat);
	unsigned int idx = rdma_index(module);

	//DDPDBG("RDMAConfig idx %d, mode %d, address 0x%x, inputformat %d, input_swap %u, pitch %u, width %u, height %u\n",
	//                     idx, mode, address, inputFormat,input_swap, pitch,width, height);
	ASSERT(idx <= RDMA_INSTANCES);
	if ((width > RDMA_MAX_WIDTH) || (height > RDMA_MAX_HEIGHT)) {
		DDPERR("RDMA input overflow, w=%d, h=%d, max_w=%d, max_h=%d\n", width, height, RDMA_MAX_WIDTH, RDMA_MAX_HEIGHT);
	}
	if (input_is_yuv==1 && output_is_yuv==0) {
		DISP_REG_SET_FIELD(handle,SIZE_CON_0_FLD_MATRIX_ENABLE, idx * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_SIZE_CON_0, 1);
		DISP_REG_SET_FIELD(handle,SIZE_CON_0_FLD_MATRIX_INT_MTX_SEL, idx * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_SIZE_CON_0, 0x6);
		// set color conversion matrix
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_C00, coef_rdma_601_y2r[0][0] );
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_C01, coef_rdma_601_y2r[0][1] );
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_C02, coef_rdma_601_y2r[0][2] );
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_C10, coef_rdma_601_y2r[1][0] );
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_C11, coef_rdma_601_y2r[1][1] );
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_C12, coef_rdma_601_y2r[1][2] );
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_C20, coef_rdma_601_y2r[2][0] );
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_C21, coef_rdma_601_y2r[2][1] );
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_C22, coef_rdma_601_y2r[2][2] );

		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_PRE_ADD_0, coef_rdma_601_y2r[3][0]);
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_PRE_ADD_1, coef_rdma_601_y2r[3][1]);
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_PRE_ADD_2, coef_rdma_601_y2r[3][2]);
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_POST_ADD_0, coef_rdma_601_y2r[4][0]);
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_POST_ADD_1, coef_rdma_601_y2r[4][1]);
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_POST_ADD_2, coef_rdma_601_y2r[4][2]);
	} else if (input_is_yuv==0 && output_is_yuv==1) {
		DISP_REG_SET_FIELD(handle,SIZE_CON_0_FLD_MATRIX_ENABLE, idx * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_SIZE_CON_0, 1);
		DISP_REG_SET_FIELD(handle,SIZE_CON_0_FLD_MATRIX_INT_MTX_SEL, idx * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_SIZE_CON_0, 0x2);
		// set color conversion matrix
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_C00, coef_rdma_601_r2y[0][0] );
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_C01, coef_rdma_601_r2y[0][1] );
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_C02, coef_rdma_601_r2y[0][2] );
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_C10, coef_rdma_601_r2y[1][0] );
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_C11, coef_rdma_601_r2y[1][1] );
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_C12, coef_rdma_601_r2y[1][2] );
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_C20, coef_rdma_601_r2y[2][0] );
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_C21, coef_rdma_601_r2y[2][1] );
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_C22, coef_rdma_601_r2y[2][2] );

		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_PRE_ADD_0, coef_rdma_601_r2y[3][0]);
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_PRE_ADD_1, coef_rdma_601_r2y[3][1]);
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_PRE_ADD_2, coef_rdma_601_r2y[3][2]);
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_POST_ADD_0, coef_rdma_601_r2y[4][0]);
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_POST_ADD_1, coef_rdma_601_r2y[4][1]);
		DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_POST_ADD_2, coef_rdma_601_r2y[4][2]);
	} else {
		DISP_REG_SET_FIELD(handle,SIZE_CON_0_FLD_MATRIX_ENABLE, idx * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_SIZE_CON_0, 0);
		DISP_REG_SET_FIELD(handle,SIZE_CON_0_FLD_MATRIX_INT_MTX_SEL, idx * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_SIZE_CON_0, 0);
	}

	DISP_REG_SET_FIELD(handle,GLOBAL_CON_FLD_MODE_SEL, idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_GLOBAL_CON, mode);
	// FORMAT & SWAP only works when RDMA memory mode, set both to 0 when RDMA direct link mode.
	DISP_REG_SET_FIELD(handle,MEM_CON_FLD_MEM_MODE_INPUT_FORMAT, idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_MEM_CON, ((mode == RDMA_MODE_DIRECT_LINK) ? 0 : input_format_reg&0xf));
	DISP_REG_SET_FIELD(handle,MEM_CON_FLD_MEM_MODE_INPUT_SWAP, idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_MEM_CON, ((mode == RDMA_MODE_DIRECT_LINK) ? 0 : input_swap));
	DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_MEM_START_ADDR, address);
	DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_MEM_SRC_PITCH, pitch);
	DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_INT_ENABLE, 0x1F);
	DISP_REG_SET_FIELD(handle,SIZE_CON_0_FLD_OUTPUT_FRAME_WIDTH, idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_SIZE_CON_0, width);
	DISP_REG_SET_FIELD(handle,SIZE_CON_1_FLD_OUTPUT_FRAME_HEIGHT, idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_SIZE_CON_1, height);

	rdma_set_ultra(idx, width, height, bpp, 60, handle);
#if 1
	if (ufoe_enable==0) { //UFOE bypassed, enable RDMA underflow intr, else disable RDMA underflow intr
		DISP_REG_SET_FIELD(handle,FIFO_CON_FLD_FIFO_UNDERFLOW_EN, idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_FIFO_CON, 1);
		DISP_REG_SET_FIELD(handle,FIFO_CON_FLD_OUTPUT_VALID_FIFO_THRESHOLD, idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_FIFO_CON, 16);
	} else {
		DISP_REG_SET_FIELD(handle,FIFO_CON_FLD_FIFO_UNDERFLOW_EN, idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_FIFO_CON, 1);
		DISP_REG_SET_FIELD(handle,FIFO_CON_FLD_OUTPUT_VALID_FIFO_THRESHOLD, idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_FIFO_CON, width*3*3/16/2); //FHD:304, HD:203, QHD:151
	}
#endif

	return 0;
}

int RDMAWait(DISP_MODULE_ENUM module,void * handle)
{
	unsigned int cnt = 0;
	unsigned int idx = rdma_index(module);
	// polling interrupt status
	while ((DISP_REG_GET(idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_INT_STATUS) & 0x4) != 0x4) {
		cnt++;
		if (cnt>1000) {
			DDPERR("RDMA%dWait timeout! \n", idx);
			DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_INT_STATUS , 0x0);
			return -1;
		}
	}
	DDPMSG(" RDMA%dWait cnt=%d \n", idx, cnt);
	DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_INT_STATUS , 0x0);

	return 0;
}

void RDMASetTargetLine(DISP_MODULE_ENUM module, unsigned int line,void * handle)
{
	unsigned int idx = rdma_index(module);

	DISP_REG_SET(handle,idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_TARGET_LINE, line);
}

static int RDMAPollingInterrupt(DISP_MODULE_ENUM module, int bit, int timeout)
{
	unsigned int idx = rdma_index(module);
	unsigned int cnt = 0;
	unsigned int regval = 0;

	if (timeout <= 0) {
		while ((DISP_REG_GET(idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_INT_STATUS) & bit)==0);
		cnt = 1;
	} else {
		// time need to update
		cnt = timeout*1000/100;
		while (cnt > 0) {
			cnt--;
			regval = DISP_REG_GET(idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_INT_STATUS);
			if (regval & bit) {
				DISP_CPU_REG_SET(idx*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_INT_STATUS, ~regval);
				break;
			}
			udelay(100);
		}
	}
	//should clear?
	DDPMSG(" RDMA%d polling interrupt ret =%d \n", idx, cnt);
	return cnt;
}

static enum RDMA_INPUT_FORMAT rdma_input_format_convert(DpColorFormat fmt)
{
	enum RDMA_INPUT_FORMAT rdma_fmt = RDMA_INPUT_FORMAT_RGB565;
	switch (fmt) {
		case eBGR565           :
			rdma_fmt = RDMA_INPUT_FORMAT_BGR565    ;
			break;
		case eRGB888           :
			rdma_fmt = RDMA_INPUT_FORMAT_RGB888    ;
			break;
		case eRGBA8888         :
			rdma_fmt = RDMA_INPUT_FORMAT_RGBA8888  ;
			break;
		case eARGB8888         :
			rdma_fmt = RDMA_INPUT_FORMAT_ARGB8888  ;
			break;
		case eVYUY             :
			rdma_fmt = RDMA_INPUT_FORMAT_VYUY      ;
			break;
		case eYVYU             :
			rdma_fmt = RDMA_INPUT_FORMAT_YVYU      ;
			break;
		case eRGB565           :
			rdma_fmt = RDMA_INPUT_FORMAT_RGB565    ;
			break;
		case eBGR888           :
			rdma_fmt = RDMA_INPUT_FORMAT_BGR888    ;
			break;
		case eBGRA8888         :
			rdma_fmt = RDMA_INPUT_FORMAT_BGRA8888  ;
			break;
		case eABGR8888         :
			rdma_fmt = RDMA_INPUT_FORMAT_ABGR8888  ;
			break;
		case eUYVY             :
			rdma_fmt = RDMA_INPUT_FORMAT_UYVY      ;
			break;
		case eYUY2             :
			rdma_fmt = RDMA_INPUT_FORMAT_YUYV      ;
			break;
		default:
			DDPERR("rdma_fmt_convert fmt=%d, rdma_fmt=%d \n", fmt, rdma_fmt);
	}
	return rdma_fmt;
}

static unsigned int rdma_input_format_byte_swap(enum RDMA_INPUT_FORMAT inputFormat)
{
	int input_swap = 0;
	switch (inputFormat) {
		case RDMA_INPUT_FORMAT_BGR565:
		case RDMA_INPUT_FORMAT_RGB888:
		case RDMA_INPUT_FORMAT_RGBA8888:
		case RDMA_INPUT_FORMAT_ARGB8888:
		case RDMA_INPUT_FORMAT_VYUY:
		case RDMA_INPUT_FORMAT_YVYU:
			input_swap = 1;
			break;
		case RDMA_INPUT_FORMAT_RGB565:
		case RDMA_INPUT_FORMAT_BGR888:
		case RDMA_INPUT_FORMAT_BGRA8888:
		case RDMA_INPUT_FORMAT_ABGR8888:
		case RDMA_INPUT_FORMAT_UYVY:
		case RDMA_INPUT_FORMAT_YUYV:
			input_swap = 0;
			break;
		default:
			DDPERR("unknow RDMA input format is %d\n", inputFormat);
			ASSERT(0);
	}
	return input_swap;
}

static unsigned int rdma_input_format_bpp(enum RDMA_INPUT_FORMAT inputFormat)
{
	int bpp = 0;
	switch (inputFormat) {
		case RDMA_INPUT_FORMAT_BGR565:
		case RDMA_INPUT_FORMAT_RGB565:
		case RDMA_INPUT_FORMAT_VYUY:
		case RDMA_INPUT_FORMAT_UYVY:
		case RDMA_INPUT_FORMAT_YVYU:
		case RDMA_INPUT_FORMAT_YUYV:
			bpp = 2;
			break;
		case RDMA_INPUT_FORMAT_RGB888:
		case RDMA_INPUT_FORMAT_BGR888:
			bpp = 3;
			break;
		case RDMA_INPUT_FORMAT_ARGB8888:
		case RDMA_INPUT_FORMAT_ABGR8888:
		case RDMA_INPUT_FORMAT_RGBA8888:
		case RDMA_INPUT_FORMAT_BGRA8888:
			bpp = 4;
			break;
		default:
			DDPERR("unknown RDMA input format = %d\n", inputFormat);
			ASSERT(0);
	}
	return  bpp;
}

static unsigned int rdma_input_format_color_space(enum RDMA_INPUT_FORMAT inputFormat)
{
	int space = 0;
	switch (inputFormat) {
		case RDMA_INPUT_FORMAT_BGR565:
		case RDMA_INPUT_FORMAT_RGB565:
		case RDMA_INPUT_FORMAT_RGB888:
		case RDMA_INPUT_FORMAT_BGR888:
		case RDMA_INPUT_FORMAT_RGBA8888:
		case RDMA_INPUT_FORMAT_BGRA8888:
		case RDMA_INPUT_FORMAT_ARGB8888:
		case RDMA_INPUT_FORMAT_ABGR8888:
			space = 0;
			break;
		case RDMA_INPUT_FORMAT_VYUY:
		case RDMA_INPUT_FORMAT_UYVY:
		case RDMA_INPUT_FORMAT_YVYU:
		case RDMA_INPUT_FORMAT_YUYV:
			space = 1;
			break;
		default:
			DDPERR("unknown RDMA input format = %d\n", inputFormat);
			ASSERT(0);
	}
	return space;
}

static unsigned int rdma_input_format_reg_value(enum RDMA_INPUT_FORMAT inputFormat)
{
	int reg_value = 0;
	switch (inputFormat) {
		case RDMA_INPUT_FORMAT_BGR565:
		case RDMA_INPUT_FORMAT_RGB565:
			reg_value = 0x0;
			break;
		case RDMA_INPUT_FORMAT_RGB888:
		case RDMA_INPUT_FORMAT_BGR888:
			reg_value = 0x1;
			break;
		case RDMA_INPUT_FORMAT_RGBA8888:
		case RDMA_INPUT_FORMAT_BGRA8888:
			reg_value = 0x2;
			break;
		case RDMA_INPUT_FORMAT_ARGB8888:
		case RDMA_INPUT_FORMAT_ABGR8888:
			reg_value = 0x3;
			break;
		case RDMA_INPUT_FORMAT_VYUY:
		case RDMA_INPUT_FORMAT_UYVY:
			reg_value = 0x4;
			break;
		case RDMA_INPUT_FORMAT_YVYU:
		case RDMA_INPUT_FORMAT_YUYV:
			reg_value = 0x5;
			break;
		default:
			DDPERR("unknow RDMA input format is %d\n", inputFormat);
			ASSERT(0);
	}
	return reg_value;
}

static char *  rdma_intput_format_name(int reg )
{
	switch (reg) {
		case 0:
			return "rgb565";
		case 1:
			return "rgb888";
		case 2:
			return "rgba8888";
		case 3:
			return "argb8888";
		case 4:
			return "uyvy";
		case 5:
			return "yuyv";
		default:
			DDPMSG("rdma unknown reg=%d\n", reg);
			return "unknown";
	}
}

static int RDMAClockOn(DISP_MODULE_ENUM module,void * handle)
{
	unsigned int idx = rdma_index(module);
	ddp_enable_module_clock(module);
	DDPMSG("RDMA%dClockOn CG 0x%x \n",idx, DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0));
	return;
}
static int RDMAClockOff(DISP_MODULE_ENUM module,void * handle)
{
	unsigned int idx = rdma_index(module);
	DDPMSG("RDMA%dClockOff\n",idx);
	ddp_disable_module_clock(module);
	return 0;
}

static int RDMAInit(DISP_MODULE_ENUM module,void * handle)
{
	unsigned int idx = rdma_index(module);
	ddp_enable_module_clock(module);
	return 0;
}

static int RDMADeInit(DISP_MODULE_ENUM module,void * handle)
{
	unsigned int idx = rdma_index(module);
	DDPMSG("RDMA%dDeinit\n",idx);
	ddp_disable_module_clock(module);
	return 0;
}

void RDMADump(DISP_MODULE_ENUM module)
{
	unsigned int idx = rdma_index(module);
	DDPMSG("== DISP RDMA%d  ==\n", idx);
	DDPMSG("(0x000)R_INTEN       =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_INT_ENABLE+DISP_RDMA_INDEX_OFFSET*idx));
	DDPMSG("(0x004)R_INTS        =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_INT_STATUS+DISP_RDMA_INDEX_OFFSET*idx));
	DDPMSG("(0x010)R_CON         =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_GLOBAL_CON+DISP_RDMA_INDEX_OFFSET*idx));
	DDPMSG("(0x014)R_SIZE0       =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_SIZE_CON_0+DISP_RDMA_INDEX_OFFSET*idx));
	DDPMSG("(0x018)R_SIZE1       =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_SIZE_CON_1+DISP_RDMA_INDEX_OFFSET*idx));
	DDPMSG("(0x01c)R_TAR_LINE    =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_TARGET_LINE+DISP_RDMA_INDEX_OFFSET*idx));
	DDPMSG("(0x024)R_M_CON       =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_MEM_CON+DISP_RDMA_INDEX_OFFSET*idx));
	DDPMSG("(0xf00)R_M_S_ADDR    =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_MEM_START_ADDR+DISP_RDMA_INDEX_OFFSET*idx));
	DDPMSG("(0x02c)R_M_SRC_PITCH =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_MEM_SRC_PITCH+DISP_RDMA_INDEX_OFFSET*idx));
	DDPMSG("(0x030)R_M_GMC_SET0  =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_MEM_GMC_SETTING_0+DISP_RDMA_INDEX_OFFSET*idx));
	DDPMSG("(0x034)R_M_SLOW_CON  =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_MEM_SLOW_CON+DISP_RDMA_INDEX_OFFSET*idx));
	DDPMSG("(0x038)R_M_GMC_SET1  =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_MEM_GMC_SETTING_1+DISP_RDMA_INDEX_OFFSET*idx));
	DDPMSG("(0x040)R_FIFO_CON    =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_FIFO_CON+DISP_RDMA_INDEX_OFFSET*idx));
	DDPMSG("(0x044)R_FIFO_LOG    =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_FIFO_LOG+DISP_RDMA_INDEX_OFFSET*idx));
	DDPMSG("(0x078)R_PRE_ADD0    =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_PRE_ADD_0+DISP_RDMA_INDEX_OFFSET*idx));
	DDPMSG("(0x07c)R_PRE_ADD1    =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_PRE_ADD_1+DISP_RDMA_INDEX_OFFSET*idx));
	DDPMSG("(0x080)R_PRE_ADD2    =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_PRE_ADD_2+DISP_RDMA_INDEX_OFFSET*idx));
	DDPMSG("(0x084)R_POST_ADD0   =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_POST_ADD_0+DISP_RDMA_INDEX_OFFSET*idx));
	DDPMSG("(0x088)R_POST_ADD1   =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_POST_ADD_1+DISP_RDMA_INDEX_OFFSET*idx));
	DDPMSG("(0x08c)R_POST_ADD2   =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_POST_ADD_2+DISP_RDMA_INDEX_OFFSET*idx));
	DDPMSG("(0x090)R_DUMMY       =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_DUMMY+DISP_RDMA_INDEX_OFFSET*idx));
	DDPMSG("(0x094)R_OUT_SEL     =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_DEBUG_OUT_SEL+DISP_RDMA_INDEX_OFFSET*idx));
	DDPMSG("(0x094)R_M_START     =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_MEM_START_ADDR+DISP_RDMA_INDEX_OFFSET*idx));
	return;
}

void rdma_dump_analysis(DISP_MODULE_ENUM module)
{
	unsigned int idx = (module==DISP_MODULE_RDMA0)?0:1;
	DDPMSG("==DISP RDMA%d ANALYSIS==\n", idx);
	DDPMSG("rdma%d: en=%d, w=%d, h=%d, pitch=%d, addr=0x%x, fmt=%s, fifo_min=%d, \
    in_p_cnt=%d, in_l_cnt=%d, out_p_cnt=%d, out_l_cnt=%d \n",
	       idx,
	       DISP_REG_GET(DISP_REG_RDMA_GLOBAL_CON+DISP_RDMA_INDEX_OFFSET*idx)&0x1,
	       DISP_REG_GET(DISP_REG_RDMA_SIZE_CON_0+DISP_RDMA_INDEX_OFFSET*idx)&0xfff,
	       DISP_REG_GET(DISP_REG_RDMA_SIZE_CON_1+DISP_RDMA_INDEX_OFFSET*idx)&0xfffff,
	       DISP_REG_GET(DISP_REG_RDMA_MEM_SRC_PITCH+DISP_RDMA_INDEX_OFFSET*idx),
	       DISP_REG_GET(DISP_REG_RDMA_MEM_START_ADDR+DISP_RDMA_INDEX_OFFSET*idx),
	       rdma_intput_format_name((DISP_REG_GET(DISP_REG_RDMA_MEM_CON+DISP_RDMA_INDEX_OFFSET*idx)>>4)&0xf),
	       DISP_REG_GET(DISP_REG_RDMA_FIFO_LOG+DISP_RDMA_INDEX_OFFSET*idx),
	       DISP_REG_GET(DISP_REG_RDMA_IN_P_CNT+DISP_RDMA_INDEX_OFFSET*idx),
	       DISP_REG_GET(DISP_REG_RDMA_IN_LINE_CNT+DISP_RDMA_INDEX_OFFSET*idx),
	       DISP_REG_GET(DISP_REG_RDMA_OUT_P_CNT+DISP_RDMA_INDEX_OFFSET*idx),
	       DISP_REG_GET(DISP_REG_RDMA_OUT_LINE_CNT+DISP_RDMA_INDEX_OFFSET*idx)
	      );
	return ;
}

static int RDMAConfig_l(DISP_MODULE_ENUM module, disp_ddp_path_config* pConfig, void* handle)
{
	int i = 0;
	RDMA_CONFIG_STRUCT * rdma_config = & pConfig->rdma_config;
	enum RDMA_MODE mode = rdma_config->address ? RDMA_MODE_MEMORY : RDMA_MODE_DIRECT_LINK;
	LCM_DSI_PARAMS *lcm_config = &(pConfig->dsi_config);
	if (pConfig->dst_dirty) {
		//config to direct link mode
		RDMAConfig(module,
		           RDMA_MODE_DIRECT_LINK, //  link mode
		           0,                     // address
		           eRGB888,               // inputFormat
		           0,                     // pitch
		           pConfig->dst_w,        // width
		           pConfig->dst_h,        // height
		           lcm_config->ufoe_enable,
		           handle);
	} else if (pConfig->rdma_dirty) {
		// decouple mode may use
		RDMAConfig(module,
		           mode,                                                                  //  link mode
		           (mode == RDMA_MODE_DIRECT_LINK) ? 0 : rdma_config->address,            // address
		           (mode == RDMA_MODE_DIRECT_LINK) ? eRGB888 : rdma_config->inputFormat,  // inputFormat
		           (mode == RDMA_MODE_DIRECT_LINK) ? 0 : rdma_config->pitch,              // pitch
		           rdma_config->width,                                                    // width
		           rdma_config->height,                                                   // height
		           lcm_config->ufoe_enable,
		           handle);
	}
	return 0;
}

void rdma_enable_color_transform(DISP_MODULE_ENUM module)
{
	unsigned int idx = rdma_index(module);
	UINT32 value =  DISP_REG_GET(DISP_REG_RDMA_SIZE_CON_0 + DISP_RDMA_INDEX_OFFSET * idx);
	value = value | REG_FLD_VAL((SIZE_CON_0_FLD_MATRIX_EXT_MTX_EN), 1) |
	        REG_FLD_VAL((SIZE_CON_0_FLD_MATRIX_ENABLE), 1);
	DISP_REG_SET(NULL, idx * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_SIZE_CON_0, value);
}

void rdma_disable_color_transform(DISP_MODULE_ENUM module)
{
	unsigned int idx = rdma_index(module);
	UINT32 value =  DISP_REG_GET(DISP_REG_RDMA_SIZE_CON_0 + DISP_RDMA_INDEX_OFFSET * idx);
	value = value | REG_FLD_VAL((SIZE_CON_0_FLD_MATRIX_EXT_MTX_EN), 0) |
	        REG_FLD_VAL((SIZE_CON_0_FLD_MATRIX_ENABLE), 0);
	DISP_REG_SET(NULL, idx * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_SIZE_CON_0, value);
}

void rdma_set_color_matrix(DISP_MODULE_ENUM module,
                           rdma_color_matrix *matrix,
                           rdma_color_pre *pre,
                           rdma_color_post *post)
{
	unsigned int idx = rdma_index(module);
	DISP_REG_SET(NULL, idx * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_C00, matrix->C00);
	DISP_REG_SET(NULL, idx * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_C01, matrix->C01);
	DISP_REG_SET(NULL, idx * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_C02, matrix->C02);
	DISP_REG_SET(NULL, idx * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_C10, matrix->C10);
	DISP_REG_SET(NULL, idx * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_C11, matrix->C11);
	DISP_REG_SET(NULL, idx * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_C12, matrix->C12);
	DISP_REG_SET(NULL, idx * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_C20, matrix->C20);
	DISP_REG_SET(NULL, idx * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_C21, matrix->C21);
	DISP_REG_SET(NULL, idx * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_C22, matrix->C22);

	DISP_REG_SET(NULL, idx * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_PRE_ADD_0, pre->ADD0);
	DISP_REG_SET(NULL, idx * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_PRE_ADD_1, pre->ADD1);
	DISP_REG_SET(NULL, idx * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_PRE_ADD_2, pre->ADD2);

	DISP_REG_SET(NULL, idx * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_POST_ADD_0, post->ADD0);
	DISP_REG_SET(NULL, idx * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_POST_ADD_1, post->ADD1);
	DISP_REG_SET(NULL, idx * DISP_RDMA_INDEX_OFFSET + DISP_REG_RDMA_POST_ADD_2, post->ADD2);
}

DDP_MODULE_DRIVER ddp_driver_rdma = {
	.module          = DISP_MODULE_RDMA0,
	.init            = RDMAInit,
	.deinit          = RDMADeInit,
	.config          = RDMAConfig_l,
	.start           = RDMAStart,
	.trigger         = NULL,
	.stop            = RDMAStop,
	.reset           = RDMAReset,
	.power_on        = RDMAClockOn,
	.power_off       = RDMAClockOff,
	.is_idle         = NULL,
	.is_busy         = NULL,
	.dump_info       = NULL,
	.bypass          = NULL,
	.build_cmdq      = NULL,
	.set_lcm_utils   = NULL,
	.polling_irq     = RDMAPollingInterrupt,
};
