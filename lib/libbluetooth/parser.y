/*	$NetBSD: parser.y,v 1.1 2006/06/19 15:44:36 gdamore Exp $	*/

%{
/*-
 * Copyright (c) 2006 Itronix Inc.
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
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * parser.y
 *
 * Copyright (c) 2001-2002 Maksim Yevmenkin <m_evmenkin@yahoo.com>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: parser.y,v 1.1 2006/06/19 15:44:36 gdamore Exp $
 * $FreeBSD: src/usr.sbin/bluetooth/hcsecd/parser.y,v 1.4 2004/09/14 20:04:33 emax Exp $
 */

#include <bluetooth.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <dev/bluetooth/btdev.h>

struct bt_handle {
	FILE *fp;
};

int	_btyyparse	(void);
int	_btyylex	(void);
void	_btyyerror	(const char *);

static	int	hexcpy	(uint8_t *, const char *, int);

extern	int		 _btyylineno;
extern	FILE		*_btyyin;

static  void		*arg;
static	const bdaddr_t	*bdaddr;
static  int		 count;
static	bt_cfgentry_t	*found;
static  void		(*func)(bt_cfgentry_t *, void *);

static	bt_cfgentry_t	*entry;
static	uint8_t		 hid_buffer[1024];
static	int		 hid_length;
%}

%union {
	char		*string;
	bdaddr_t	 bdaddr;
	int		 value;
}

%token <string> T_STRING T_HEXSTRING
%token <bdaddr>	T_BDADDRSTRING
%token <value>	T_VALUE
%token T_DEVICE
%token T_BDADDR
%token T_NAME
%token T_KEY
%token T_PIN
%token T_TYPE
%token T_HID
%token T_HSET
%token T_CONTROL_PSM
%token T_CONTROL_CHANNEL
%token T_INTERRUPT_PSM
%token T_RECONNECT_INITIATE
%token T_BATTERY_POWER
%token T_NORMALLY_CONNECTABLE
%token T_HID_DESCRIPTOR

%token T_TRUE
%token T_FALSE
%token T_ERROR

%%

config:
	line
	| config line
	;

line:
	T_DEVICE
		{
			entry = malloc(sizeof(bt_cfgentry_t));
			if (entry == NULL)
				YYABORT;

			memset(entry, 0, sizeof(bt_cfgentry_t));
		}
	'{' options '}'
		{
			if (bdaddr != NULL) {
				if (bdaddr_same(&entry->bdaddr, bdaddr)) {
					bt_freeconfig(found);
					found = entry;
				} else
					bt_freeconfig(entry);
			}

			if (func != NULL) {
				(*func)(entry, arg);
				bt_freeconfig(entry);
			}

			entry = NULL;
			count++;
		}
	;

options:
	option ';'
	| options option ';'
	;

option:
	bdaddr
	| type
	| name
	| key
	| pin
	| control_psm
	| control_channel
	| interrupt_psm
	| reconnect_initiate
	| battery_power
	| normally_connectable
	| hid_descriptor
	| parser_error
	;

bdaddr:
	T_BDADDR T_BDADDRSTRING
		{
			bdaddr_copy(&entry->bdaddr, &$2);
		}
	;

type:
	T_TYPE T_HID
		{
			entry->type = BTDEV_HID;
		}
	| T_TYPE T_HSET
		{
			entry->type = BTDEV_HSET;
		}
	;

name:
	T_NAME T_STRING
		{
			if (entry->name != NULL)
				free(entry->name);

			entry->name = strdup($2);
			if (entry->name == NULL)
				YYABORT;
		}
	;

key:
	T_KEY T_HEXSTRING
		{
			if (entry->key == NULL) {
				entry->key = malloc(HCI_KEY_SIZE);
				if (entry->key == NULL)
					YYABORT;
			}

			memset(entry->key, 0, HCI_KEY_SIZE);
			hexcpy(entry->key, $2, HCI_KEY_SIZE);
		}
	;

pin:
	T_PIN T_STRING
		{
			if (entry->pin == NULL) {
				entry->pin = malloc(HCI_PIN_SIZE);
				if (entry->pin == NULL)
					YYABORT;
			}

			strncpy((char *)entry->pin, $2, HCI_PIN_SIZE);
		}
	| T_PIN T_HEXSTRING
		{
			if (entry->pin == NULL) {
				entry->pin = malloc(HCI_PIN_SIZE);
				if (entry->pin == NULL)
					YYABORT;
			}

			memset(entry->pin, 0, HCI_PIN_SIZE);
			hexcpy(entry->pin, $2, HCI_PIN_SIZE);
		}
	;

control_psm:
	T_CONTROL_PSM T_HEXSTRING
		{
			if (strlen($2) > sizeof(entry->control_psm) * 2) {
				errno = EINVAL;
				YYABORT;
			}

			entry->control_psm = (uint16_t)strtoul($2, NULL, 16);
		}
	;

control_channel:
	T_CONTROL_CHANNEL T_VALUE
		{
			entry->control_channel = (uint8_t)$2;
		}
	;

interrupt_psm:
	T_INTERRUPT_PSM T_HEXSTRING
		{
			if (strlen($2) > sizeof(entry->interrupt_psm) * 2) {
				errno = EINVAL;
				YYABORT;
			}

			entry->interrupt_psm = (uint16_t)strtoul($2, NULL, 16);
		}
	;

reconnect_initiate:
	T_RECONNECT_INITIATE T_TRUE
		{
			entry->reconnect_initiate = 1;
		}
	| T_RECONNECT_INITIATE T_FALSE
		{
			entry->reconnect_initiate = 0;
		}
	;

battery_power:
	T_BATTERY_POWER T_TRUE
		{
			entry->battery_power = 1;
		}
	| T_BATTERY_POWER T_FALSE
		{
			entry->battery_power = 0;
		}
	;

normally_connectable:
	T_NORMALLY_CONNECTABLE T_TRUE
		{
			entry->normally_connectable = 1;
		}
	| T_NORMALLY_CONNECTABLE T_FALSE
		{
			entry->normally_connectable = 0;
		}
	;

hid_descriptor:
	T_HID_DESCRIPTOR
		{
			hid_length = 0;
		}
	'{' hid_descriptor_bytes '}'
		{
			if (entry->hid_descriptor != NULL)
				free(entry->hid_descriptor);

			entry->hid_descriptor = malloc(hid_length);
			if (entry->hid_descriptor == NULL)
				YYABORT;

			memcpy(entry->hid_descriptor, hid_buffer, hid_length);
			entry->hid_length = hid_length;
		}
	;

hid_descriptor_bytes:
	hid_descriptor_byte
	| hid_descriptor_bytes hid_descriptor_byte
	;

hid_descriptor_byte:
	T_HEXSTRING
		{
			if (hid_length >= sizeof(hid_buffer)) {
				errno = ENOBUFS;
				YYABORT;
			}

			if (strlen($1) != 2) {
				errno = EINVAL;
				YYABORT;
			}

			hid_buffer[hid_length++] = (uint8_t)strtoul($1, NULL, 16);
		}
	;

parser_error:
	T_ERROR
		{
			errno = EINVAL;
			YYABORT;
		}

%%

/* Display parser error message */
void
_btyyerror(const char *message)
{

	/* fprintf(stderr, "%s in line %d", message, _btyylineno); */
	count = -1;
}

/* Convert hex ASCII to int4 */
static int
hexa2int4(char a)
{

	if ('0' <= a && a <= '9')
		return (a - '0');

	if ('A' <= a && a <= 'F')
		return (a - 'A' + 0xa);

	if ('a' <= a && a <= 'f')
		return (a - 'a' + 0xa);

	/* NOTREACHED */
	return 0;
}

/* Convert ASCII hex string to uint8 array */
static int
hexcpy(uint8_t *dest, const char *src, int len)
{
	int n;

	for (n = 0 ; n < len && *src != '\0' ; n++) {
		*dest++ = (hexa2int4(src[0]) << 4) | hexa2int4(src[1]);
		src += 2;
	}

	return n;
}

/* Open config file */
bt_handle_t
bt_openconfig(const char *name)
{
	bt_handle_t handle;
	
	handle = malloc(sizeof(bt_handle_t));
	if (handle == NULL)
		return NULL;

	memset(handle, 0, sizeof(bt_handle_t));

	if (name == NULL)
		name = "/etc/bluetooth/bluetooth.conf";

	handle->fp = fopen(name, "r");
	if (handle->fp == NULL) {
		free(handle);
		return NULL;
	}

	return handle;
}

/* Find config entry for device */
bt_cfgentry_t *
bt_getconfig(bt_handle_t handle, const bdaddr_t *device)
{

	if (handle == NULL || handle->fp == NULL) {
		errno = EBADF;
		return NULL;
	}

	arg = NULL;
	bdaddr = device;
	count = 0;
	found = NULL;
	func = NULL;

	_btyyin = handle->fp;
	rewind(_btyyin);
	_btyyparse();
	_btyyin = NULL;

	if (found == NULL)
		errno = ENOENT;

	return found;
}

/* Call function for each config entry */
int
bt_eachconfig(bt_handle_t handle, void (*f)(bt_cfgentry_t *, void *), void *a)
{
	
	if (handle == NULL || handle->fp == NULL) {
		errno = EBADF;
		return -1;
	}

	arg = a;
	bdaddr = NULL;
	count = 0;
	found = NULL;
	func = f;

	_btyyin = handle->fp;
	rewind(_btyyin);
	_btyyparse();
	_btyyin = NULL;

	return count;
}

/* Free config entry */
void
bt_freeconfig(bt_cfgentry_t *cfg)
{

	if (cfg == NULL)
		return;

	if (cfg->name)
		free(cfg->name);

	if (cfg->pin)
		free(cfg->pin);

	if (cfg->key)
		free(cfg->key);

	if (cfg->hid_descriptor)
		free(cfg->hid_descriptor);

	free(cfg);
}

/* Close config file */
int
bt_closeconfig(bt_handle_t handle)
{
	int rv;

	if (handle == NULL) {
		errno = EBADF;
		return -1;
	}

	rv = fclose(handle->fp);
	handle->fp = NULL;
	free(handle);

	return rv;
}

#define	OPT	"\t%-23s "
#define Q	"\""
#define HEX	"0x%4.4x"
#define DEC	"%d"
#define KEY	"0x%s"
#define STR	"%s"
#define SLEN	"%.*s"
#define EOL	";\n"

/* Print config entry */
int
bt_printconfig(FILE *fp, bt_cfgentry_t *cfg)
{
	int i, n;

	if (cfg == NULL || fp == NULL)
		return -1;

	fprintf(fp, "device {\n");
	fprintf(fp, OPT STR EOL, "bdaddr", bt_ntoa(&cfg->bdaddr, NULL));

	if (cfg->type != 0) {
		const char *type;

		switch (cfg->type) {
		case BTDEV_HID:
			type = "hid";
			break;

		case BTDEV_HSET:
			type = "hset";
			break;

		default:
			type = "unknown";
			break;
		}

		fprintf(fp, OPT STR EOL, "type", type);
	}

	if (cfg->name != NULL)
		fprintf(fp, OPT Q STR Q EOL, "name", cfg->name);

	if (cfg->pin != NULL)
		fprintf(fp, OPT Q SLEN Q EOL, "pin", HCI_PIN_SIZE, cfg->pin);

	if (cfg->key != NULL) {
		fprintf(fp, OPT STR, "key", "0x");

		n = HCI_KEY_SIZE - 1;
		while (n > 1 && cfg->key[n] == 0)
			n--;

		for (i = 0 ; i <= n ; i++)
			fprintf(fp, "%2.2x", cfg->key[i]);

		fprintf(fp, EOL);
	}

	if (cfg->control_psm != 0)
		fprintf(fp, OPT HEX EOL, "control_psm", cfg->control_psm);

	if (cfg->interrupt_psm != 0)
		fprintf(fp, OPT HEX EOL, "interrupt_psm", cfg->interrupt_psm);

	if (cfg->control_channel != 0)
		fprintf(fp, OPT DEC EOL, "control_channel", cfg->control_channel);

	if (cfg->reconnect_initiate != 0)
		fprintf(fp, OPT STR EOL, "reconnect_initiate", "true");

	if (cfg->battery_power != 0)
		fprintf(fp, OPT STR EOL, "battery_power", "true");

	if (cfg->normally_connectable != 0)
		fprintf(fp, OPT STR EOL, "normally_connectable", "true");

	if (cfg->hid_descriptor != NULL) {
		fprintf(fp, OPT "{", "hid_descriptor");

		for (i = 0 ; i < cfg->hid_length ; i++)
			fprintf(fp, "%s0x%2.2x",
			    ((i % 8) ? " " : "\n\t\t"),
			    cfg->hid_descriptor[i]);

		fprintf(fp, "\n\t}" EOL);
	}

	fprintf(fp, "}\n");

	return 0;
}
