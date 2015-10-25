/* $NetBSD: awin_sysconfig.c,v 1.2 2015/10/25 20:46:46 bouyer Exp $ */

/*-
 * Copyright (c) 2014 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "opt_allwinner.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: awin_sysconfig.c,v 1.2 2015/10/25 20:46:46 bouyer Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <machine/bootconfig.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#include <evbarm/awin/awin_sysconfig.h>

static paddr_t awin_sysconfig_base = 0;
#define AWIN_SYSCONFIG_SIZE	0x10000

#define AWIN_SYSCONFIG_TYPE_UNKNOWN	0
#define AWIN_SYSCONFIG_TYPE_SINGLE_WORD	1
#define AWIN_SYSCONFIG_TYPE_STRING	2
#define AWIN_SYSCONFIG_TYPE_MULTI_WORD	3
#define AWIN_SYSCONFIG_TYPE_GPIO	4

static uint8_t awin_sysconfig[AWIN_SYSCONFIG_SIZE];

struct awin_sysconfig_head {
	uint32_t	count;
	uint32_t	version[3];
} __packed;

struct awin_sysconfig_main_key {
	char		name[32];
	uint32_t	count;
	uint32_t	offset;
} __packed;

struct awin_sysconfig_sub_key {
	char		name[32];
	uint32_t	offset;
	uint16_t	count;
	uint16_t	type;
} __packed;

struct awin_sysconfig_gpio {
	char		name[32];
	uint32_t	port;
	uint32_t	port_num;
	int32_t		mul_sel;
	int32_t		pull;
	int32_t		drv_level;
	int32_t		data;
} __packed;

static bool awin_sysconfig_parse(const char *, const char *,
				 struct awin_sysconfig_sub_key *);

bool
awin_sysconfig_init(void)
{
	if (get_bootconf_option(boot_args, "sysconfig",
	    BOOTOPT_TYPE_HEXINT, &awin_sysconfig_base) == 0) {
		return false;
	}
	if (awin_sysconfig_base == 0) {
		return false;
	}

	const uint8_t * const sysconfig = (const uint8_t *)
	    (awin_sysconfig_base + KERNEL_BASE_VOFFSET);
	memcpy(awin_sysconfig, sysconfig, AWIN_SYSCONFIG_SIZE);

	return true;
}

static bool
awin_sysconfig_parse(const char *key, const char *subkey,
    struct awin_sysconfig_sub_key *value)
{
	struct awin_sysconfig_head head;
	struct awin_sysconfig_main_key main_key;
	struct awin_sysconfig_sub_key sub_key;
	unsigned int off, n;

	KASSERT(awin_sysconfig_base != 0);

	memcpy(&head, &awin_sysconfig[0], sizeof(head));

	for (n = 0, off = sizeof(head); n < head.count;
	     n++, off += sizeof(main_key)) {
		memcpy(&main_key, &awin_sysconfig[off], sizeof(main_key));
		if (strcmp(key, main_key.name) == 0) {
			break;
		}
	}
	if (n == head.count) {
		return false;
	}

	for (n = 0, off = main_key.offset << 2; n < main_key.count;
	    n++, off += sizeof(sub_key)) {
		memcpy(&sub_key, &awin_sysconfig[off], sizeof(sub_key));
		if (strcmp(subkey, sub_key.name) == 0) {
			break;
		}
	}
	if (n == main_key.count) {
		return false;
	}

	memcpy(value, &sub_key, sizeof(*value));

	return true;
}

int
awin_sysconfig_get_int(const char *key, const char *subkey)
{
	struct awin_sysconfig_sub_key value;
	int ret;

	if (awin_sysconfig_parse(key, subkey, &value) == false)
		return -1;

	if (value.type != AWIN_SYSCONFIG_TYPE_SINGLE_WORD)
		return -1;

	memcpy(&ret, &awin_sysconfig[value.offset << 2], sizeof(ret));

	return ret;
}

const char *
awin_sysconfig_get_string(const char *key, const char *subkey)
{
	struct awin_sysconfig_sub_key value;

	if (awin_sysconfig_parse(key, subkey, &value) == false)
		return NULL;

	if (value.type != AWIN_SYSCONFIG_TYPE_STRING)
		return NULL;

	return &awin_sysconfig[value.offset << 2];
}

const char *
awin_sysconfig_get_gpio(const char *key, const char *subkey)
{
	static char gpio_str[6];
	struct awin_sysconfig_sub_key value;
	struct awin_sysconfig_gpio gpio;

	if (awin_sysconfig_parse(key, subkey, &value) == false)
		return NULL;

	if (value.type != AWIN_SYSCONFIG_TYPE_GPIO)
		return NULL;
	
	memcpy(&gpio, &awin_sysconfig[(value.offset << 2) - 32], sizeof(gpio));

	snprintf(gpio_str, sizeof(gpio_str), "%cP%c%d",
	    gpio.mul_sel == 0 ? '<' : '>',
	    (gpio.port - 1) + 'A', gpio.port_num);

	return gpio_str;
}
