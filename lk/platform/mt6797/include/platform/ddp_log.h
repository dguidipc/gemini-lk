#ifndef _H_DDP_LOG_
#define _H_DDP_LOG_
//#include "xlog.h"
#define DDP_DFINFO 0
#include <debug.h>

#ifndef LOG_TAG
#define LOG_TAG "unknow"
#endif


#ifndef ASSERT
#define ASSERT(expr) if(!(expr)) printk("LK_DDP, assert fail! %s, %d\n", __FILE__, __LINE__);
#endif

#define DISP_LOG_V( fmt, args...)   dprintf(DDP_DFINFO,"[LK_DDP/"LOG_TAG"]"fmt,##args)
#define DISP_LOG_D( fmt, args...)   dprintf(DDP_DFINFO,"[LK_DDP/"LOG_TAG"]"fmt,##args)
#define DISP_LOG_I( fmt, args...)   dprintf(DDP_DFINFO,"[LK_DDP/"LOG_TAG"]"fmt,##args)
#define DISP_LOG_W( fmt, args...)   dprintf(DDP_DFINFO,"[LK_DDP/"LOG_TAG"]"fmt,##args)
#define DISP_LOG_E( fmt, args...)   dprintf(DDP_DFINFO,"[LK_DDP/"LOG_TAG"]error:"fmt,##args)
#define DISP_LOG_F( fmt, args...)   dprintf(DDP_DFINFO,"[LK_DDP/"LOG_TAG"]error:"fmt,##args)

#define DDPMSG(string, args...)    dprintf(DDP_DFINFO,"[LK_DDP/"LOG_TAG"]"string,##args)  // default on, important msg, not err
#define DDPERR(string, args...)    dprintf(DDP_DFINFO,"[LK_DDP/"LOG_TAG"]error:"string,##args)  //default on, err msg
#define DDPDBG(string, args...)     dprintf(DDP_DFINFO,"[LK_DDP/"LOG_TAG"]"string,##args)  // default on, important msg, not err
#endif
