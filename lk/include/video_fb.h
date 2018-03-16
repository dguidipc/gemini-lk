

#ifndef _VIDEO_FB_H_
#define _VIDEO_FB_H_

#define CS_BG_COL            0x00
#define CS_FG_COL            0xa0


#define CFB_FMT_8BIT            0
#define CFB_555RGB_15BIT        1
#define CFB_565RGB_16BIT        2
#define CFB_X888RGB_32BIT       3
#define CFB_888RGB_24BIT        4
#define CFB_332RGB_8BIT         5

#define GDF_16BIT_565RGB        2
#define GDF_32BIT_X888RGB       3


typedef struct {
	unsigned int pciBase;
	unsigned int isaBase;
	unsigned int dprBase;
	unsigned int frameAdrs;
	unsigned int vprBase;
	unsigned int cprBase;
	unsigned int memSize;
	unsigned int mode;
	unsigned int fg;
	unsigned int bg;
	unsigned int gdfIndex;
	unsigned int gdfBytesPP;
	unsigned int plnSizeX;
	unsigned int plnSizeY;
	unsigned int winSizeX;
	unsigned int winSizeY;
	char modeIdent[80];
} GraphicDevice;


void *video_hw_init (void);       /* returns GraphicDevice struct or NULL */

void video_set_lut (
    unsigned int index,           /* color number */
    unsigned char r,              /* red */
    unsigned char g,              /* green */
    unsigned char b               /* blue */
);


#endif /*_VIDEO_FB_H_ */
