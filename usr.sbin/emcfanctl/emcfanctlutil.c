/*	$NetBSD: emcfanctlutil.c,v 1.1 2025/03/11 13:56:48 brad Exp $	*/

/*
 * Copyright (c) 2025 Brad Spencer <brad@anduin.eldar.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef __RCSID
__RCSID("$NetBSD: emcfanctlutil.c,v 1.1 2025/03/11 13:56:48 brad Exp $");
#endif

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include <dev/i2c/emcfanreg.h>
#include <dev/i2c/emcfaninfo.h>

#define EXTERN
#include "emcfanctl.h"
#include "emcfanctlconst.h"
#include "emcfanctlutil.h"

int
emcfan_read_register(int fd, uint8_t reg, uint8_t *res, bool debug)
{
	int err;

	err = lseek(fd, reg, SEEK_SET);
	if (err != -1) {
		err = read(fd, res, 1);
		if (err == -1)
			err = errno;
		else
			err = 0;
	} else {
		err = errno;
	}

	if (debug)
		fprintf(stderr,"emcfan_read_register: reg=0x%02X, res=0x%02x, return err: %d\n",reg, *res, err);

	return err;
}

int
emcfan_write_register(int fd, uint8_t reg, uint8_t value, bool debug)
{
	int err;

	err = lseek(fd, reg, SEEK_SET);
	if (err != -1) {
		err = write(fd, &value, 1);
		if (err == -1)
			err = errno;
		else
			err = 0;
	} else {
		err = errno;
	}

	if (debug)
		fprintf(stderr,"emcfan_write_register: reg=0x%02X, value=0x%02X, return err: %d\n",reg, value, err);

	return err;
}

int
emcfan_rmw_register(int fd, uint8_t reg, uint8_t value,
    const struct emcfan_bits_translate translation[],
    long unsigned int translation_size,
    int tindex,
    bool debug)
{
	int err = 0;
	uint8_t current, oldcurrent;

	err = emcfan_read_register(fd, reg, &oldcurrent, debug);
	if (err != 0)
		return(err);


	current = oldcurrent & ~translation[tindex].clear_mask;
	current = current | translation[tindex].bit_mask;

	if (debug)
		fprintf(stderr,"tindex=%d, clear_mask=0x%02X 0x%02X, value=%d (0x%02X), bit_mask=0x%02X 0x%02X, oldcurrent=%d (0x%02X), new current=%d (0x%02X)\n",tindex,
		    translation[tindex].clear_mask,
		    (uint8_t)~translation[tindex].clear_mask,
		    value,value,
		    translation[tindex].bit_mask,
		    (uint8_t)~translation[tindex].bit_mask,
		    oldcurrent,oldcurrent,current,current);

	err = emcfan_write_register(fd, reg, current, debug);

	return(err);
}

char *
emcfan_product_to_name(uint8_t product_id)
{
	for(long unsigned int i = 0;i < __arraycount(emcfan_chip_infos); i++)
		if (product_id == emcfan_chip_infos[i].product_id)
			return(__UNCONST(emcfan_chip_infos[i].name));
	return(NULL);
}

void
emcfan_family_to_name(int family, char *name, int len)
{
	switch(family) {
	case EMCFAN_FAMILY_210X:
		snprintf(name, len, "%s", "EMC210x");
		break;
	case EMCFAN_FAMILY_230X:
		snprintf(name, len, "%s", "EMC230x");
		break;
	case EMCFAN_FAMILY_UNKNOWN:
	default:
		snprintf(name, len, "%s", "UNKNOWN");
		break;
	}

	return;
}

int
emcfan_find_info(uint8_t product)
{
	for(long unsigned int i = 0;i < __arraycount(emcfan_chip_infos); i++)
		if (product == emcfan_chip_infos[i].product_id)
			return(i);

	return(-1);
}

bool
emcfan_reg_is_real(int iindex, uint8_t reg)
{
	int segment;
	uint64_t index;

	segment = reg / 64;
	index = reg % 64;

	return(emcfan_chip_infos[iindex].register_void[segment] & ((uint64_t)1 << index));
}

static int
emcfan_hunt_by_name(const struct emcfan_registers the_registers[], long unsigned int the_registers_size, char *the_name)
{
	int r = -1;

	for(long unsigned int i = 0;i < the_registers_size;i++) {
		if (strcmp(the_name, the_registers[i].name) == 0) {
			r = the_registers[i].reg;
			break;
		}
	}

	return(r);
}

int
emcfan_reg_by_name(uint8_t product_id, int product_family, char *name)
{
	int r = -1;

	switch(product_family) {
	case EMCFAN_FAMILY_210X:
		switch(product_id) {
		case EMCFAN_PRODUCT_2101:
		case EMCFAN_PRODUCT_2101R:
			r = emcfan_hunt_by_name(emcfanctl_2101_registers,__arraycount(emcfanctl_2101_registers),name);
			break;
		case EMCFAN_PRODUCT_2103_1:
			r = emcfan_hunt_by_name(emcfanctl_2103_1_registers,__arraycount(emcfanctl_2103_1_registers),name);
			break;
		case EMCFAN_PRODUCT_2103_24:
			r = emcfan_hunt_by_name(emcfanctl_2103_24_registers,__arraycount(emcfanctl_2103_24_registers),name);
			break;
		case EMCFAN_PRODUCT_2104:
			r = emcfan_hunt_by_name(emcfanctl_2104_registers,__arraycount(emcfanctl_2104_registers),name);
			break;
		case EMCFAN_PRODUCT_2106:
			r = emcfan_hunt_by_name(emcfanctl_2106_registers,__arraycount(emcfanctl_2106_registers),name);
			break;
		default:
			printf("UNSUPPORTED YET %d\n",product_id);
			exit(99);
			break;
		};
		break;
	case EMCFAN_FAMILY_230X:
		r = emcfan_hunt_by_name(emcfanctl_230x_registers,__arraycount(emcfanctl_230x_registers),name);
		break;
	};

	return(r);
}

static const char *
emcfan_hunt_by_reg(const struct emcfan_registers the_registers[], long unsigned int the_registers_size, uint8_t the_reg)
{
	const char *r = NULL;

	for(long unsigned int i = 0;i < the_registers_size;i++) {
		if (the_reg == the_registers[i].reg) {
			r = the_registers[i].name;
			break;
		}
	}

	return(r);
}

const char *
emcfan_regname_by_reg(uint8_t product_id, int product_family, uint8_t reg)
{
	const char *r = NULL;

	switch(product_family) {
	case EMCFAN_FAMILY_210X:
		switch(product_id) {
		case EMCFAN_PRODUCT_2101:
		case EMCFAN_PRODUCT_2101R:
			r = emcfan_hunt_by_reg(emcfanctl_2101_registers,__arraycount(emcfanctl_2101_registers),reg);
			break;
		case EMCFAN_PRODUCT_2103_1:
			r = emcfan_hunt_by_reg(emcfanctl_2103_1_registers,__arraycount(emcfanctl_2103_1_registers),reg);
			break;
		case EMCFAN_PRODUCT_2103_24:
			r = emcfan_hunt_by_reg(emcfanctl_2103_24_registers,__arraycount(emcfanctl_2103_24_registers),reg);
			break;
		case EMCFAN_PRODUCT_2104:
			r = emcfan_hunt_by_reg(emcfanctl_2104_registers,__arraycount(emcfanctl_2104_registers),reg);
			break;
		case EMCFAN_PRODUCT_2106:
			r = emcfan_hunt_by_reg(emcfanctl_2106_registers,__arraycount(emcfanctl_2106_registers),reg);
			break;
		default:
			printf("UNSUPPORTED YET %d\n",product_id);
			exit(99);
			break;
		};
		break;
	case EMCFAN_FAMILY_230X:
		r = emcfan_hunt_by_reg(emcfanctl_230x_registers,__arraycount(emcfanctl_230x_registers),reg);
		break;
	};

	return(r);
}

int
find_translated_blob_by_bits_instance(const struct emcfan_bits_translate translation[],
    long unsigned int translation_size,
    uint8_t bits, int instance)
{
	int r = -10191;
	uint8_t clear_mask;
	uint8_t b;

	for(long unsigned int i = 0;i < translation_size;i++) {
		if (instance == translation[i].instance) {
			clear_mask = translation[i].clear_mask;
			b = bits & clear_mask;
			if (b == translation[i].bit_mask) {
				r = i;
				break;
			}
		}

	}

	return(r);
}

int
find_translated_bits_by_hint_instance(const struct emcfan_bits_translate translation[],
    long unsigned int translation_size,
    int human_value, int instance)
{
	int r = -10191;

	for(long unsigned int i = 0;i < translation_size;i++) {
		if (instance == translation[i].instance &&
		    human_value == translation[i].human_int) {
			r = i;
			break;
		}
	}

	return(r);
}

int
find_translated_bits_by_hint(const struct emcfan_bits_translate translation[],
    long unsigned int translation_size,
    int human_value)
{
	int r = -10191;

	for(long unsigned int i = 0;i < translation_size;i++) {
		if (human_value == translation[i].human_int) {
			r = i;
			break;
		}
	}

	return(r);
}

int
find_translated_bits_by_str(const struct emcfan_bits_translate translation[],
    long unsigned int translation_size,
    char *human_str)
{
	int r = -10191;

	for(long unsigned int i = 0;i < translation_size;i++) {
		if (strcmp(human_str,translation[i].human_str) == 0) {
			r = i;
			break;
		}
	}

	return(r);
}

int
find_translated_bits_by_str_instance(const struct emcfan_bits_translate translation[],
    long unsigned int translation_size,
    char *human_str, int instance)
{
	int r = -10191;

	for(long unsigned int i = 0;i < translation_size;i++) {
		if (instance == translation[i].instance &&
		    strcmp(human_str,translation[i].human_str) == 0) {
			r = i;
			break;
		}
	}

	return(r);
}

int
find_human_int(const struct emcfan_bits_translate translation[],
    long unsigned int translation_size,
    uint8_t bits)
{
	int r = -10191;

	for(long unsigned int i = 0;i < translation_size;i++) {
		if (bits == translation[i].bit_mask) {
			r = translation[i].human_int;
			break;
		}
	}

	return(r);
}

char *
find_human_str(const struct emcfan_bits_translate translation[],
    long unsigned int translation_size,
    uint8_t bits)
{
	char *r = NULL;

	for(long unsigned int i = 0;i < translation_size;i++) {
		if (bits == translation[i].bit_mask) {
			r = __UNCONST(translation[i].human_str);
			break;
		}
	}

	return(r);
}
