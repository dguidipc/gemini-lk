#ifndef _DISP_LCM_H_
#define _DISP_LCM_H_
#include "lcm_drv.h"

#define MAX_LCM_NUMBER  2


typedef struct {
	LCM_PARAMS          *params;
	LCM_DRIVER          *drv;
	LCM_INTERFACE_ID    lcm_if_id;
	int                 module;
	int                 is_inited;
	int                 is_connected;
} disp_lcm_handle, *pdisp_lcm_handle;

disp_lcm_handle* disp_lcm_probe(char* plcm_name, LCM_INTERFACE_ID lcm_id);
int disp_lcm_init(disp_lcm_handle *plcm);
LCM_PARAMS* disp_lcm_get_params(disp_lcm_handle *plcm);
LCM_INTERFACE_ID disp_lcm_get_interface_id(disp_lcm_handle *plcm);
int disp_lcm_update(disp_lcm_handle *plcm, int x, int y, int w, int h, int force);
int disp_lcm_esd_check(disp_lcm_handle *plcm);
int disp_lcm_esd_recover(disp_lcm_handle *plcm);
int disp_lcm_suspend(disp_lcm_handle *plcm);
int disp_lcm_resume(disp_lcm_handle *plcm);
int disp_lcm_set_backlight(disp_lcm_handle *plcm, int level);
int disp_lcm_read_fb(disp_lcm_handle *plcm);
int disp_lcm_ioctl(disp_lcm_handle *plcm, LCM_IOCTL ioctl, unsigned int arg);
int disp_lcm_is_video_mode(disp_lcm_handle *plcm);
int disp_lcm_is_inited(disp_lcm_handle *plcm);
const char* disp_lcm_get_name(disp_lcm_handle *plcm);
int disp_lcm_width(disp_lcm_handle *plcm);
int disp_lcm_height(disp_lcm_handle *plcm);
int disp_lcm_x(disp_lcm_handle *plcm);
int disp_lcm_y(disp_lcm_handle *plcm);

#endif

