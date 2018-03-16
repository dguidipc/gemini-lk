

#ifndef _SHOW_LOGO_LOG_H
#define _SHOW_LOGO_LOG_H

#ifdef __cplusplus
extern "C" {
#endif


#ifdef  BUILD_LK

#ifdef MTK_LOG_ENABLE
#undef MTK_LOG_ENABLE
#endif
#define MTK_LOG_ENABLE 1
#include <debug.h>
#include <lib/zlib.h>

#ifndef  LOG_ANIM
#define  LOG_ANIM(x...)     dprintf(0, x)

#endif

#else

#include <cutils/log.h>
#include "zlib.h"

#ifndef  LOG_ANIM
#define  LOG_ANIM(x...)      ALOGD(x)

#endif

#endif




#ifdef __cplusplus
}
#endif
#endif
