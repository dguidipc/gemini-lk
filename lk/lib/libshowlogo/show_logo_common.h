/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */
#ifndef _SHOW_LOGO_COMMON_H
#define _SHOW_LOGO_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif


#define RGB565_TO_ARGB8888(x)   \
    ((((x) &   0x1F) << 3) |    \
     (((x) &  0x7E0) << 5) |    \
     (((x) & 0xF800) << 8) |    \
     (0xFF << 24)) // opaque

#define RGB565_TO_ABGR8888(x)   \
    ((((x) &   0x1F) << 19) |   \
     (((x) &  0x7E0) <<  5) |   \
     (((x) & 0xF800) >>  8) |   \
     (0xFF << 24)) // opaque

#define ARGB8888_TO_ABGR8888(x) \
    ((((x) &   0xFF) << 16)  |  \
      ((x) & 0xFF00)         |  \
     (((x) & 0xFF0000) >> 16)|  \
      ((x) & 0xFF000000))

#define ARGB8888_TO_RGB565(x)      \
    (((((x) >> 19) & 0x1F) << 11)| \
     ((((x) >> 10) & 0x3F) <<  5)| \
      (((x) >>  3) & 0x1F))

#define ARGB8888_TO_BGR565(x)      \
     ((((x) >> 19) & 0x1F)       | \
     ((((x) >> 10) & 0x3F) <<  5)| \
      (((x) >>  3) & 0x1F) << 11)

#define ALIGN_TO(x, n)  \
    (((x) + ((n) - 1)) & ~((n) - 1))


// define the rectangle parameters
typedef struct {
     int left, top, right, bottom;
} RECT_REGION_T;

// dedfine the LCM SCREEM parameters
typedef struct {
     int         width;
     int         height;
     int         bits_per_pixel;
     int         rotation;                  // phical screen rotation:0 , 90, 180, 270
     int         needAllign;                // if need adjust the width or height with 32: no need  (0), need (1)
     int         allignWidth;
     int         need180Adjust;             // if need adjust the drawing logo for 180 roration: no need  (0), need (1)
     int         fb_size;
     int         fill_dst_bits;
     int         red_offset;                // if red_offset is 0: logo use format BGR565 or ABGR8888,  red_offset is 11/16: RGB565, ARGB8888
     int         blue_offset;               // if blue_offset is 11/16: logo use format BGR565 or ABGR8888,  blue_offset is 0: RGB565, ARGB8888
} LCM_SCREEN_T;

/* internal use function */

int  check_rect_valid(RECT_REGION_T rect);
void fill_point_buffer(unsigned int *fill_addr, unsigned int src_color, LCM_SCREEN_T phical_screen, unsigned int bits);
void fill_rect_with_content_by_32bit_argb8888(unsigned int *fill_addr, RECT_REGION_T rect, unsigned int *src_addr, LCM_SCREEN_T phical_screen, unsigned int bits);
void fill_rect_with_content_by_32bit_rgb565(unsigned int *fill_addr, RECT_REGION_T rect, unsigned short *src_addr, LCM_SCREEN_T phical_screen, unsigned int bits);
void fill_rect_with_color_by_32bit(unsigned int *fill_addr, RECT_REGION_T rect, unsigned int src_color, LCM_SCREEN_T phical_screen);
void fill_rect_with_content_by_16bit_argb8888(unsigned short *fill_addr, RECT_REGION_T rect, unsigned int *src_addr, LCM_SCREEN_T phical_screen);
void fill_rect_with_content_by_16bit_rgb565(unsigned short *fill_addr, RECT_REGION_T rect, unsigned short *src_addr, LCM_SCREEN_T phical_screen);
void fill_rect_with_color_by_16bit(unsigned short *fill_addr, RECT_REGION_T rect, unsigned int src_color, LCM_SCREEN_T phical_screen);


/* public interface */

/*
 * Draw a rectangle with logo content
 *
 * @parameter
 *
 */
void fill_rect_with_content(void *fill_addr, RECT_REGION_T rect, void *src_addr, LCM_SCREEN_T phical_screen, unsigned int bits);


/*
 * Draw a rectangle with spcial color
 *
 * @parameter
 *
 */
void fill_rect_with_color(void *fill_addr, RECT_REGION_T rect, unsigned int src_color, LCM_SCREEN_T phical_screen);







#ifdef __cplusplus
}
#endif
#endif
