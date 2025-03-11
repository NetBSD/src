/*	$NetBSD: emcfanctloutputs.c,v 1.1 2025/03/11 13:56:48 brad Exp $	*/

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
__RCSID("$NetBSD: emcfanctloutputs.c,v 1.1 2025/03/11 13:56:48 brad Exp $");
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
#include <mj.h>

#include <dev/i2c/emcfanreg.h>
#include <dev/i2c/emcfaninfo.h>

#define EXTERN
#include "emcfanctl.h"
#include "emcfanctlconst.h"
#include "emcfanctlutil.h"
#include "emcfanctloutputs.h"

int
output_emcfan_info(int fd, uint8_t product_id, int product_family, bool jsonify, bool debug)
{
	int err = 0;
	uint8_t res;
	mj_t obj;
	char *s = NULL;
	char *pn;
	char fn[8];

	err = emcfan_read_register(fd, EMCFAN_REVISION, &res, debug);
	if (err != 0)
		goto out;

	if (jsonify) {
		memset(&obj, 0x0, sizeof(obj));
		mj_create(&obj, "object");
		mj_append_field(&obj, "revision", "integer", res);
		mj_append_field(&obj, "product_id", "integer", product_id);
		mj_append_field(&obj, "product_family", "integer", product_family);
		pn = emcfan_product_to_name(product_id);
		mj_append_field(&obj, "chip_name", "string", pn, strlen(pn));
		emcfan_family_to_name(product_family, fn, sizeof(fn));
		mj_append_field(&obj, "family_name", "string", fn, strlen(fn));
		mj_asprint(&s, &obj, MJ_JSON_ENCODE);
		printf("%s",s);
		if (s != NULL)
			free(s);
	} else {
		emcfan_family_to_name(product_family, fn, sizeof(fn));
		printf("Product Family: %s\n", fn);
		printf("Chip name: %s\n", emcfan_product_to_name(product_id));
		printf("Revision: %d\n", res);
	}

 out:
	return(err);
}

static void
output_emcfan_generic_reg_list(uint8_t product_id, const struct emcfan_registers the_registers[], long unsigned int the_registers_size, bool jsonify, bool debug)
{
	mj_t array;
	mj_t obj;
	char *s = NULL;
	int iindex;

	iindex = emcfan_find_info(product_id);
	if (iindex == -1) {
		printf("Unknown info for product_id: %d\n",product_id);
		exit(2);
	}

	if (debug)
		fprintf(stderr, "output_emcfan_generic_reg_list: iindex=%d\n",iindex);

	if (jsonify) {
		memset(&array, 0x0, sizeof(array));
		mj_create(&array, "array");
	}

	for(long unsigned int i = 0;i < the_registers_size;i++) {
		if (emcfan_reg_is_real(iindex, the_registers[i].reg)) {
			if (jsonify) {
				memset(&obj, 0x0, sizeof(obj));
				mj_create(&obj, "object");
				mj_append_field(&obj, "register_name", "string", the_registers[i].name, strlen(the_registers[i].name));
				mj_append_field(&obj, "register", "integer", the_registers[i].reg);
				mj_append(&array, "object", &obj);
				mj_delete(&obj);
			} else {
				printf("%s\t%d\t0x%02X\n",the_registers[i].name,the_registers[i].reg,the_registers[i].reg);
			}
		}
	}

	if (jsonify) {
		mj_asprint(&s, &array, MJ_JSON_ENCODE);
		printf("%s",s);
		if (s != NULL)
			free(s);
		mj_delete(&array);
	}
}

void
output_emcfan_register_list(uint8_t product_id, int product_family, bool jsonify, bool debug)
{
	if (debug)
		fprintf(stderr,"output_emcfan_list: product_id=%d, product_family=%d\n",product_id, product_family);

	switch(product_family) {
	case EMCFAN_FAMILY_210X:
		switch(product_id) {
		case EMCFAN_PRODUCT_2101:
		case EMCFAN_PRODUCT_2101R:
			output_emcfan_generic_reg_list(product_id, emcfanctl_2101_registers, __arraycount(emcfanctl_2101_registers), jsonify, debug);
			break;
		case EMCFAN_PRODUCT_2103_1:
			output_emcfan_generic_reg_list(product_id, emcfanctl_2103_1_registers, __arraycount(emcfanctl_2103_1_registers), jsonify, debug);
			break;
		case EMCFAN_PRODUCT_2103_24:
			output_emcfan_generic_reg_list(product_id, emcfanctl_2103_24_registers, __arraycount(emcfanctl_2103_24_registers), jsonify, debug);
			break;
		case EMCFAN_PRODUCT_2104:
			output_emcfan_generic_reg_list(product_id, emcfanctl_2104_registers, __arraycount(emcfanctl_2104_registers), jsonify, debug);
			break;
		case EMCFAN_PRODUCT_2106:
			output_emcfan_generic_reg_list(product_id, emcfanctl_2106_registers, __arraycount(emcfanctl_2106_registers), jsonify, debug);
			break;
		default:
			printf("UNSUPPORTED YET %d\n",product_id);
			exit(99);
			break;
		};
		break;
	case EMCFAN_FAMILY_230X:
		output_emcfan_generic_reg_list(product_id, emcfanctl_230x_registers, __arraycount(emcfanctl_230x_registers), jsonify, debug);
		break;
	};
}

static int
output_emcfan_230x_read_reg(int fd, uint8_t product_id, int product_family, uint8_t start, uint8_t end, bool jsonify, bool debug)
{
	int err = 0;
	uint8_t res;
	mj_t array;
	mj_t obj;
	char *s = NULL;
	int iindex;
	const char *rn;

	iindex = emcfan_find_info(product_id);
	if (iindex == -1) {
		printf("Unknown info for product_id: %d\n",product_id);
		exit(2);
	}

	if (debug)
		fprintf(stderr, "output_emcfan_230x_read_reg: product_id=%d, product_family=%d, iindex=%d\n",product_id, product_family, iindex);

	if (jsonify) {
		memset(&array, 0x0, sizeof(array));
		mj_create(&array, "array");
	}

	for(int i = start; i <= end; i++) {
		if (emcfan_reg_is_real(iindex, i)) {
			err = emcfan_read_register(fd, i, &res, debug);
			if (err != 0)
				break;
			if (jsonify) {
				memset(&obj, 0x0, sizeof(obj));
				mj_create(&obj, "object");
				rn = emcfan_regname_by_reg(product_id, product_family, i);
				mj_append_field(&obj, "register_name", "string", rn, strlen(rn));
				mj_append_field(&obj, "register", "integer", i);
				mj_append_field(&obj, "register_value", "integer", res);
				mj_append(&array, "object", &obj);
				mj_delete(&obj);
			} else {
				printf("%s;%d (0x%02X);%d (0x%02X)\n",emcfan_regname_by_reg(product_id, product_family, i),i,i,res,res);
			}
		}
	}

	if (jsonify) {
		mj_asprint(&s, &array, MJ_JSON_ENCODE);
		printf("%s",s);
		if (s != NULL)
			free(s);
		mj_delete(&array);
	}

	return(err);
}

int
output_emcfan_register_read(int fd, uint8_t product_id, int product_family, uint8_t start, uint8_t end, bool jsonify, bool debug)
{
	int err = 0;

	if (debug)
		fprintf(stderr,"output_emcfan_register_read: start=%d 0x%02X, end=%d 0x%02X\n",start, start, end, end);

	switch(product_family) {
	case EMCFAN_FAMILY_210X:
		err = output_emcfan_230x_read_reg(fd, product_id, product_family, start, end, jsonify, debug);
		break;
	case EMCFAN_FAMILY_230X:
		err = output_emcfan_230x_read_reg(fd, product_id, product_family, start, end, jsonify, debug);
		break;
	};

	return(err);
}

int
output_emcfan_minexpected_rpm(int fd, uint8_t product_id, int product_family, uint8_t config_reg, bool jsonify, bool debug)
{
	int err = 0;
	uint8_t raw_res, res;
	uint8_t clear_mask;
	int human_value;
	char *s = NULL;
	mj_t obj;

	err = emcfan_read_register(fd, config_reg, &raw_res, debug);
	if (err != 0)
		goto out;

	clear_mask = fan_minexpectedrpm[0].clear_mask;
	res = raw_res & clear_mask;
	if (debug)
		fprintf(stderr,"%s: clear_mask=0x%02X 0x%02X, raw_res=%d (0x%02X), res=%d (0x%02X)\n",__func__,clear_mask,(uint8_t)~clear_mask,raw_res,raw_res,res,res);
	human_value = find_human_int(fan_minexpectedrpm, __arraycount(fan_minexpectedrpm), res);

	if (human_value == -10191)
		return(EINVAL);

	if (jsonify) {
		memset(&obj, 0x0, sizeof(obj));
		mj_create(&obj, "object");
		mj_append_field(&obj, "minimum_expected_rpm", "integer", human_value);
		mj_append_field(&obj, "register", "integer", config_reg);
		mj_append_field(&obj, "register_value", "integer", raw_res);
		mj_asprint(&s, &obj, MJ_JSON_ENCODE);
		printf("%s",s);
		if (s != NULL)
			free(s);
	} else {
		printf("Minumum expected rpm:%d\n",human_value);
	}

 out:
	return(err);
}

int
output_emcfan_edges(int fd, uint8_t product_id, int product_family, uint8_t config_reg, bool jsonify, bool debug)
{
	int err = 0;
	uint8_t raw_res, res;
	uint8_t clear_mask;
	int human_value;
	char *s = NULL;
	mj_t obj;

	err = emcfan_read_register(fd, config_reg, &raw_res, debug);
	if (err != 0)
		goto out;

	clear_mask = fan_numedges[0].clear_mask;
	res = raw_res & clear_mask;
	if (debug)
		fprintf(stderr,"%s: clear_mask=0x%02X 0x%02X, raw_res=%d (0x%02X), res=%d (0x%02X)\n",__func__,clear_mask,(uint8_t)~clear_mask,raw_res,raw_res,res,res);
	human_value = find_human_int(fan_numedges, __arraycount(fan_numedges), res);

	if (human_value == -10191)
		return(EINVAL);

	if (jsonify) {
		memset(&obj, 0x0, sizeof(obj));
		mj_create(&obj, "object");
		mj_append_field(&obj, "num_edges", "integer", human_value);
		mj_append_field(&obj, "register", "integer", config_reg);
		mj_append_field(&obj, "register_value", "integer", raw_res);
		mj_asprint(&s, &obj, MJ_JSON_ENCODE);
		printf("%s",s);
		if (s != NULL)
			free(s);
	} else {
		printf("Number of edges:%d\n",human_value);
	}

 out:
	return(err);
}

static int
output_emcfan_simple_int(int fd, uint8_t product_id, int product_family, uint8_t reg, const char *what, const char *whatj, bool jsonify, bool debug)
{
	int err = 0;
	uint8_t res;
	char *s = NULL;
	mj_t obj;

	err = emcfan_read_register(fd, reg, &res, debug);
	if (err != 0)
		goto out;

	if (jsonify) {
		memset(&obj, 0x0, sizeof(obj));
		mj_create(&obj, "object");
		mj_append_field(&obj, whatj, "integer", res);
		mj_append_field(&obj, "register", "integer", reg);
		mj_append_field(&obj, "register_value", "integer", res);
		mj_asprint(&s, &obj, MJ_JSON_ENCODE);
		printf("%s",s);
		if (s != NULL)
			free(s);
	} else {
		printf("%s:%d\n",what, res);
	}

 out:
	return(err);
}

int
output_emcfan_drive(int fd, uint8_t product_id, int product_family, uint8_t reg, bool jsonify, bool debug)
{
	return(output_emcfan_simple_int(fd, product_id, product_family, reg, "Drive", "drive_level", jsonify, debug));
}

int
output_emcfan_divider(int fd, uint8_t product_id, int product_family, uint8_t reg, bool jsonify, bool debug)
{
	return(output_emcfan_simple_int(fd, product_id, product_family, reg, "Divider", "frequency_divider", jsonify, debug));
}

int
output_emcfan_pwm_basefreq(int fd, uint8_t product_id, int product_family, uint8_t reg, int the_fan, bool jsonify, bool debug)
{
	int err = 0;
	uint8_t res;
	int tindex;
	char *s = NULL;
	mj_t obj;

	err = emcfan_read_register(fd, reg, &res, debug);
	if (err != 0)
		goto out;

	tindex = find_translated_blob_by_bits_instance(fan_pwm_basefreq, __arraycount(fan_pwm_basefreq), res, the_fan);

	if (debug)
		fprintf(stderr,"%s: reg=%d 0x%02X, res=0x%02x, tindex=%d, the_fan=%d\n",__func__,reg,reg,res,tindex,the_fan);

	if (jsonify) {
		memset(&obj, 0x0, sizeof(obj));
		mj_create(&obj, "object");
		mj_append_field(&obj, "fan", "integer", the_fan+1);
		mj_append_field(&obj, "pwm_base_frequency", "integer", fan_pwm_basefreq[tindex].human_int);
		mj_append_field(&obj, "register", "integer", reg);
		mj_append_field(&obj, "register_value", "integer", res);
		mj_asprint(&s, &obj, MJ_JSON_ENCODE);
		printf("%s",s);
		if (s != NULL)
			free(s);
	} else {
		printf("PWM Base Frequency:%d\n",fan_pwm_basefreq[tindex].human_int);
	}

 out:
	return(err);
}

int
output_emcfan_polarity(int fd, uint8_t product_id, int product_family, uint8_t reg, int the_fan, bool jsonify, bool debug)
{
	int err = 0;
	uint8_t res;
	int mask;
	bool inverted = false;
	char *s = NULL;
	mj_t obj;

	err = emcfan_read_register(fd, reg, &res, debug);
	if (err != 0)
		goto out;

	if (product_id == EMCFAN_PRODUCT_2101 ||
	    product_id == EMCFAN_PRODUCT_2101R) {
		mask = 0x10;
	} else {
		mask = 1 << the_fan;
	}

	if (res & mask)
		inverted = true;

	if (jsonify) {
		memset(&obj, 0x0, sizeof(obj));
		mj_create(&obj, "object");
		mj_append_field(&obj, "fan", "integer", the_fan+1);
		mj_append_field(&obj, "inverted", "integer", inverted);
		mj_append_field(&obj, "register", "integer", reg);
		mj_append_field(&obj, "register_value", "integer", res);
		mj_asprint(&s, &obj, MJ_JSON_ENCODE);
		printf("%s",s);
		if (s != NULL)
			free(s);
	} else {
		printf("Inverted:%s\n",(inverted ? "Yes" : "No"));
	}

 out:
	return(err);
}

int
output_emcfan_pwm_output_type(int fd, uint8_t product_id, int product_family, uint8_t reg, int the_fan, bool jsonify, bool debug)
{
	int err = 0;
	uint8_t res;
	int mask;
	bool pushpull = false;
	char *s = NULL;
	mj_t obj;

	err = emcfan_read_register(fd, reg, &res, debug);
	if (err != 0)
		goto out;

	mask = 1 << the_fan;

	if (res & mask)
		pushpull= true;

	if (jsonify) {
		memset(&obj, 0x0, sizeof(obj));
		mj_create(&obj, "object");
		mj_append_field(&obj, "fan", "integer", the_fan+1);
		mj_append_field(&obj, "pwm_output_type", "integer", pushpull);
		mj_append_field(&obj, "register", "integer", reg);
		mj_append_field(&obj, "register_value", "integer", res);
		mj_asprint(&s, &obj, MJ_JSON_ENCODE);
		printf("%s",s);
		if (s != NULL)
			free(s);
	} else {
		printf("PWM Output Type:%s\n",(pushpull ? "push-pull" : "open drain"));
	}

 out:
	return(err);
}

int
output_emcfan_fan_status(int fd, uint8_t product_id, int product_family, uint8_t start_reg, uint8_t end_reg, int the_fan, bool jsonify, bool debug)
{
	int err = 0;
	uint8_t res[4];
	bool stalled = false;
	bool spin_up_fail = false;
	bool drive_fail = false;
	uint8_t stall_mask = 0;
	uint8_t spin_mask = 0;
	uint8_t drive_mask = 0;
	char *s = NULL;
	mj_t obj;

	res[0] = res[1] = res[2] = res[3] = 0;

	if (product_family == EMCFAN_FAMILY_210X) {
		err = emcfan_read_register(fd, start_reg, &res[0], debug);
		if (err != 0)
			goto out;
		err = emcfan_read_register(fd, start_reg, &res[0], debug);
		if (err != 0)
			goto out;

		switch(the_fan) {
		case 0:
			stall_mask = 0b00000001;
			spin_mask = 0b00000010;
			drive_mask = 0b00100000;
			break;
		case 1:
			stall_mask = 0b00000100;
			spin_mask = 0b00001000;
			drive_mask = 0b01000000;
			break;
		default:
			fprintf(stderr,"No status for fan: %d\n", the_fan + 1);
			err = EINVAL;
		};
		if (debug)
			fprintf(stderr,"%s: product_family=%d, stall_mask=0x%02X, spin_mask=0x%02X, drive_mask=0x%02X, res=0x%02X\n",__func__,
			    product_family, stall_mask, spin_mask, drive_mask, res[0]);
		stalled = (res[0] & stall_mask);
		spin_up_fail = (res[0] & spin_mask);
		drive_fail = (res[0] & drive_mask);
	} else {
		int j = 0;
		for(uint8_t i = start_reg; i <= end_reg;i++,j++) {
			err = emcfan_read_register(fd, i, &res[j], debug);
			if (err != 0)
				goto out;
		}
		j = 0;
		for(uint8_t i = start_reg; i <= end_reg;i++,j++) {
			err = emcfan_read_register(fd, i, &res[j], debug);
			if (err != 0)
				goto out;
		}

		if (debug)
			fprintf(stderr,"%s: product_family=%d, res[0]=0x%02X, res[1]=0x%02X, res[2]=0x%02X, res[3]=0x%02X\n",
			    __func__, product_family, res[0], res[1], res[2], res[3]);
		stalled = (res[1] & (1 << the_fan));
		spin_up_fail = (res[2] & (1 << the_fan));
		drive_fail = (res[3] & (1 << the_fan));
	}

	if (jsonify) {
		memset(&obj, 0x0, sizeof(obj));
		mj_create(&obj, "object");
		mj_append_field(&obj, "fan", "integer", the_fan+1);
		mj_append_field(&obj, "stalled", "integer", stalled);
		mj_append_field(&obj, "spin_up_fail", "integer", spin_up_fail);
		mj_append_field(&obj, "drive_fail", "integer", drive_fail);
		mj_append_field(&obj, "register1", "integer", start_reg);
		mj_append_field(&obj, "register1_value", "integer", res[0]);
		mj_append_field(&obj, "register2", "integer", start_reg+1);
		mj_append_field(&obj, "register2_value", "integer", res[1]);
		mj_append_field(&obj, "register3", "integer", start_reg+2);
		mj_append_field(&obj, "register3_value", "integer", res[2]);
		mj_append_field(&obj, "register4", "integer", start_reg+3);
		mj_append_field(&obj, "register4_value", "integer", res[3]);
		mj_asprint(&s, &obj, MJ_JSON_ENCODE);
		printf("%s",s);
		if (s != NULL)
			free(s);
	} else {
		printf("Stalled: %s\n",stalled ? "Yes" : "No");
		printf("Spin up failed: %s\n",spin_up_fail ? "Yes" : "No");
		printf("Drive failed: %s\n",drive_fail ? "Yes" : "No");
	}

 out:
	return(err);
}

int
output_emcfan_apd(int fd, uint8_t product_id, int product_family, uint8_t reg, bool jsonify, bool debug)
{
	int err = 0;
	uint8_t res;
	bool antiparalleldiode = false;
	char *s = NULL;
	mj_t obj;

	err = emcfan_read_register(fd, reg, &res, debug);
	if (err != 0)
		goto out;

	if (res & 0x01)
		antiparalleldiode = true;

	if (jsonify) {
		memset(&obj, 0x0, sizeof(obj));
		mj_create(&obj, "object");
		mj_append_field(&obj, "apd", "integer", antiparalleldiode);
		mj_append_field(&obj, "register", "integer", reg);
		mj_append_field(&obj, "register_value", "integer", res);
		mj_asprint(&s, &obj, MJ_JSON_ENCODE);
		printf("%s",s);
		if (s != NULL)
			free(s);
	} else {
		printf("APD:%s\n",(antiparalleldiode ? "On" : "Off"));
	}

 out:
	return(err);
}

int
output_emcfan_smbusto(int fd, uint8_t product_id, int product_family, uint8_t reg, int instance, bool jsonify, bool debug)
{
	int err = 0;
	uint8_t res;
	int tindex;
	bool smbusto = false;
	char *s = NULL;
	mj_t obj;

	err = emcfan_read_register(fd, reg, &res, debug);
	if (err != 0)
		goto out;

	tindex = find_translated_blob_by_bits_instance(smbus_timeout, __arraycount(smbus_timeout), res, instance);

	if (debug)
		fprintf(stderr,"%s: reg=%d 0x%02X, res=0x%02x, tindex=%d, instance=%d\n",__func__,reg,reg,res,tindex,instance);

	smbusto = (res & smbus_timeout[tindex].clear_mask) ? true : false;

	if (jsonify) {
		memset(&obj, 0x0, sizeof(obj));
		mj_create(&obj, "object");
		mj_append_field(&obj, "smbus_timeout", "integer", smbusto);
		mj_append_field(&obj, "register", "integer", reg);
		mj_append_field(&obj, "register_value", "integer", res);
		mj_asprint(&s, &obj, MJ_JSON_ENCODE);
		printf("%s",s);
		if (s != NULL)
			free(s);
	} else {
		printf("SMBUS timeout:%s\n",(smbusto ? "Off" : "On"));
	}

 out:
	return(err);
}

