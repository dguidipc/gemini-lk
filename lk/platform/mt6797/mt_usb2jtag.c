
/*=======================================================================*/
/* HEADER FILES                                                          */
/*=======================================================================*/
#include <debug.h>
#include <platform/env.h>
#include <platform/mt_gpio.h>
#include <cust_gpio_usage.h>
#include <libfdt.h>
#include <upmu_hw.h>

void usb2jtag_hw_init(void)
{

	*(unsigned int *) 0x11290808 = *(unsigned int *) 0x11290808 & 0xFFFDFFFF;
	*(unsigned int *) 0x11290800 = *(unsigned int *) 0x11290800 | 0x00000001;
	*(unsigned int *) 0x11290820 = *(unsigned int *) 0x11290820 | 0x00000200;
	*(unsigned int *) 0x11290818 = *(unsigned int *) 0x11290818 & 0xFF7FFFFF;
	*(unsigned short *)0x10001F00 |=0x4000;

	dprintf(CRITICAL, "[MTK Debug] Jade usb2jtag_init start()\n");
	dprintf(CRITICAL, "[MTK Debug]&0x%x=0x%x\n",0x11290808,*(unsigned int *)0x11290808);
	dprintf(CRITICAL, "[MTK Debug]&0x%x=0x%x\n",0x11290800,*(unsigned int *)0x11290800);
	dprintf(CRITICAL, "[MTK Debug]&0x%x=0x%x\n",0x11290820,*(unsigned int *)0x11290820);
	dprintf(CRITICAL, "[MTK Debug]&0x%x=0x%x\n",0x11290818,*(unsigned int *)0x11290818);
	dprintf(CRITICAL, "[MTK Debug]&0x%x=0x%x\n",0x10001f00,*(unsigned int *)0x10001f00);
	dprintf(CRITICAL, "[MTK Debug]usb2jtag_init() done\n");
}


unsigned int get_usb2jtag(void)
{
	unsigned int ap_jtag_status;

	ap_jtag_status = (get_env("usb2jtag") == NULL) ? 0 : atoi(get_env("usb2jtag"));
	dprintf(CRITICAL,"[USB2JTAG] current setting is %d.\n", ap_jtag_status);
	return ap_jtag_status;
}

unsigned int set_usb2jtag(unsigned int en)
{
	char *USB2JTAG[2] = {"0","1"};

	dprintf(CRITICAL,"[USB2JTAG] current setting is %d.\n",(get_env("usb2jtag") == NULL) ? 0 : atoi(get_env("usb2jtag")));
	if (set_env("usb2jtag", USB2JTAG[en]) == 0) {
		dprintf(CRITICAL,"[USB2JTAG]set USB2JTAG %s success.\n",USB2JTAG[en]);
		return 0;
	} else {
		dprintf(CRITICAL,"[USB2JTAG]set USB2JTAG %s fail.\n",USB2JTAG[en]);
		return 1;
	}
}

void usb2jtag_init(void)
{
	if (get_usb2jtag() == 1)
		usb2jtag_hw_init();
}
