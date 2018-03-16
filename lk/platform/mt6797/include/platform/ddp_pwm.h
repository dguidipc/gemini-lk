#ifndef __DDP_PWM_H__
#define __DDP_PWM_H__


typedef enum {
	DISP_PWM0 = 0x1,
	DISP_PWM1 = 0x2,
	DISP_PWM_ALL = (DISP_PWM0 | DISP_PWM1)
} disp_pwm_id_t;


disp_pwm_id_t disp_pwm_get_main(void);

void disp_pwm_init(disp_pwm_id_t id);

int disp_pwm_is_enabled(disp_pwm_id_t id);

int disp_pwm_set_backlight(disp_pwm_id_t id, int level_1024);

/* For backward compatible */
int disp_bls_set_backlight(int level_256);

#endif
