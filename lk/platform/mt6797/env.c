/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2015. All rights reserved.
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
 */

#include <sys/types.h>
#include <stdint.h>
#include <printf.h>
#include <malloc.h>
#include <string.h>

#include <platform/env.h>
#include <platform/partition.h>
#include <platform/mt_typedefs.h>
#include <platform/errno.h>

#define MODULE_NAME "LK_ENV"

static env_t g_env[SYSENV_AREA_MAX];
static int env_valid[SYSENV_AREA_MAX];
static char *env_buffer[SYSENV_AREA_MAX];
static char env_get_char(int index, unsigned int area);
static char *env_get_addr(int index, unsigned int area);
static int envmatch (uchar *s1, int i2, unsigned int area);
static int write_env_area(char *env_buf, unsigned int area);
static int read_env_area(char *env_buf, unsigned int area);

#ifndef min
#define min(x, y)   (x < y ? x : y)
#endif

void env_init(void)
{
	int ret, i, area, checksum;
	for (area = 0; area < SYSENV_AREA_MAX; area++) {
		checksum = 0;
		env_valid[area] = 0;
		env_buffer[area] = (char *)malloc(CFG_ENV_SIZE);
		memset(env_buffer[area],0x00,CFG_ENV_SIZE);
		g_env[area].env_data = env_buffer[area] + CFG_ENV_DATA_OFFSET;
		ret = read_env_area(env_buffer[area], area);
		if(ret<0) {
			dprintf(CRITICAL, "[%s]read_env_area %d fail, ret = %d\n", MODULE_NAME, area, ret);
			env_valid[area] = 0;
			continue;
		}

		memcpy(g_env[area].sig,env_buffer[area],sizeof(g_env[area].sig));
		memcpy(g_env[area].sig_1,env_buffer[area]+CFG_ENV_SIG_1_OFFSET,sizeof(g_env[area].sig_1));

		if(!strcmp(g_env[area].sig,ENV_SIG) && !strcmp(g_env[area].sig_1,ENV_SIG)){
			g_env[area].checksum = *((int *)env_buffer[area]+CFG_ENV_CHECKSUM_OFFSET/4);
			for(i=0;i<(int)(CFG_ENV_DATA_SIZE);i++)
				checksum += g_env[area].env_data[i];
			if(checksum != g_env[area].checksum){
				dprintf(CRITICAL, "[%s]checksum of area %d mismatch s %d d %d!\n",MODULE_NAME, area, g_env[area].checksum,checksum);
				env_valid[area] = 0;
			}else{
				dprintf(CRITICAL, "[%s]ENV of area %d initialize sucess\n", MODULE_NAME, area);
				env_valid[area] = 1;
			}
		}else{
			dprintf(CRITICAL, "[%s]ENV SIG of area %d is wrong\n",MODULE_NAME, area);
			env_valid[area] = 0;
		}
		if(!env_valid[area])
			memset(env_buffer[area],0x00,CFG_ENV_SIZE);
	}
}

static char *get_env_with_area(char *name, unsigned int area)
{
	int i, nxt;
	dprintf(CRITICAL, "[%s]get_env %s from area %d\n",MODULE_NAME,name, area);

	if (area >= SYSENV_AREA_MAX) {
		dprintf(CRITICAL, "[%s]Invalid area %d\n", MODULE_NAME, area);
		return NULL;
	}

	if(!env_valid[area])
		return NULL;

	for (i=0; env_get_char(i, area) != '\0'; i=nxt+1) {
		int val;

		for (nxt=i; env_get_char(nxt, area) != '\0'; ++nxt) {
			if (nxt >= CFG_ENV_SIZE) {
				return (NULL);
			}
		}
		if ((val=envmatch((uchar *)name, i, area)) < 0)
			continue;
		return ((char *)env_get_addr(val, area));
	}

	return (NULL);
}

char *get_env(char *name) {
	return get_env_with_area(name, SYSENV_RW_AREA);
}

char *get_ro_env(char *name) {
	return get_env_with_area(name, SYSENV_RO_AREA);
}

static char env_get_char(int index, unsigned int area)
{
	return *(g_env[area].env_data+index);
}

static char *env_get_addr(int index, unsigned int area)
{
	return (g_env[area].env_data+index);

}
static int envmatch(uchar *s1, int i2, unsigned int area)
{
	if (s1 == NULL)
		return -1;
	while (*s1 == env_get_char(i2++, area))
		if (*s1++ == '=')
			return(i2);
	if (*s1 == '\0' && env_get_char(i2-1, area) == '=')
		return(i2);
	return(-1);
}

int set_env(char *name, char *value) {
	return set_env_with_area(name, value, SYSENV_RW_AREA);
}

int set_env_with_area(char *name, char *value, int area)
{
	int len, oldval, ret;
	uchar *env, *nxt = NULL;

	if (area != SYSENV_RW_AREA) {
		dprintf(CRITICAL, "[%s]Can not write read-only sysenv area\n", MODULE_NAME);
		return -1;
	}
	if (name == NULL || value == NULL) {
		dprintf(CRITICAL, "[%s]invalid parameter in set_env_with_area\n", MODULE_NAME);
		return -1;
	}

	uchar *env_data = (uchar *)g_env[area].env_data;

	dprintf(CRITICAL, "[%s]set_env %s %s\n",MODULE_NAME,name,value);

	oldval = -1;

	if(!env_valid[area]){
		env = env_data;
		goto add;
	}

	for (env=env_data; *env; env=nxt+1) {
		for (nxt=env; *nxt; ++nxt)
			;
		if ((oldval = envmatch((uchar *)name, env-env_data, area)) >= 0)
			break;
	}

	if (oldval>0) {
		if (*++nxt == '\0') {
			if (env > env_data) {
				env--;
			} else {
				*env = '\0';
			}
		} else {
			for (;;) {
				*env = *nxt++;
				if ((*env == '\0') && (*nxt == '\0'))
					break;
				++env;
			}
		}
		*++env = '\0';
	}

	for (env=env_data; *env || *(env+1); ++env)
		;
	if (env > env_data)
		++env;

add:
	/*
	* Overflow when:
	* "name" + "=" + "val" +"\0\0"  > ENV_SIZE - (env-env_data)
	*/
	len = strlen(name) + 2;
	/* add '=' for first arg, ' ' for all others */
	len += strlen(value) + 1;

	if (len > (&env_data[CFG_ENV_DATA_SIZE]-env)) {
		dprintf(CRITICAL, "## Error: environment overflow, \"%s\" deleted\n", name);
		return -1;
	}
	while ((*env = *name++) != '\0')
		env++;

	*env = '=';

	while ((*++env = *value++) != '\0')
		;


	/* end is marked with double '\0' */
	*++env = '\0';
	memset(env,0x00,CFG_ENV_DATA_SIZE-(env-env_data));
	ret = write_env_area(env_buffer[area], area);
	if (ret < 0) {
		dprintf(CRITICAL, "[%s]write env fail\n",MODULE_NAME);
		memset(env_buffer[area],0x00,CFG_ENV_SIZE);
		return -1;
	}
	env_valid[area] = 1;
	return 0;

}

void print_env()
{
	int i,nxt,area,exit;
	for (area = 0; area < SYSENV_AREA_MAX; area++) {
		dprintf(CRITICAL, "[%s]env area: %d\n", MODULE_NAME, area);
		uchar *env = (uchar *)g_env[area].env_data;
		if(!env_valid[area]){
			dprintf(CRITICAL, "[%s]no valid env\n", MODULE_NAME);
			continue;
		}
		dprintf(CRITICAL, "[%s]env:\n", MODULE_NAME);
		exit = 0;
		for (i=0; env_get_char(i, area) != '\0'; i=nxt+1) {
			for (nxt=i; env_get_char(nxt, area) != '\0'; ++nxt) {
				if (nxt >= (int)(CFG_ENV_DATA_SIZE)) {
					exit = 1;
					break;
				}
			}
			if (exit == 1)
				break;
			dprintf(CRITICAL, "[%s]%s\n", MODULE_NAME, env+i);
		}
	}
}
static int write_env_area(char *env_buf, unsigned int area)
{
	part_t *part;
	part_dev_t *dev;

	size_t count;
	long len;
	int i,checksum = 0;
	off_t offset;
	size_t size = CFG_ENV_SIZE;

	if (area >= SYSENV_AREA_MAX) {
		dprintf(CRITICAL, "[%s]Invalid area %d\n", MODULE_NAME, area);
		return -1;
	}

	dev = mt_part_get_device();
	if (!dev)
	{	return -ENODEV;
	}

	part = mt_part_get_partition(ENV_PART);
	if (!part)
	{	return -ENOENT;
	}

	count = min(sizeof(ENV_SIG), sizeof(g_env[area].sig));
	memcpy(env_buf,ENV_SIG,count);
	memcpy(env_buf+CFG_ENV_SIG_1_OFFSET,ENV_SIG,count);

	for (i=0; i<(int)CFG_ENV_DATA_SIZE; i++) {
		checksum += *(env_buf+CFG_ENV_DATA_OFFSET+i);
	}
	dprintf(CRITICAL, "checksum %d\n",checksum);

	*((int *)env_buf+CFG_ENV_CHECKSUM_OFFSET/4) = checksum;

	offset = (area == SYSENV_RW_AREA) ? CFG_ENV_RW_OFFSET : CFG_ENV_RO_OFFSET;
	len = partition_write(ENV_PART, offset, (uchar*)env_buf, size);
	if (len < 0)
		return -EIO;
	return 0;
}

static int read_env_area(char *env_buf, unsigned int area)
{
	part_t *part;
	part_dev_t *dev;
	long len;
	off_t offset;
	size_t size = CFG_ENV_SIZE;

	if (area >= SYSENV_AREA_MAX) {
		dprintf(CRITICAL, "[%s]Invalid area %d\n", MODULE_NAME, area);
		return -EINVAL;
	}

	dev = mt_part_get_device();
	if (!dev)
		return -ENODEV;

	part = mt_part_get_partition(ENV_PART);
	if (!part)
		return -ENOENT;

	offset = (area == SYSENV_RW_AREA) ? CFG_ENV_RW_OFFSET : CFG_ENV_RO_OFFSET;
	len = partition_read(ENV_PART, offset, (uchar*)env_buf, size);
	if (len < 0)
		return -EIO;
	return 0;
}
