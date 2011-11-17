/*	$NetBSD: iscsic_globals.h,v 1.4 2011/11/17 16:20:47 joerg Exp $	*/

/*-
 * Copyright (c) 2005,2006,2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wasabi Systems, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef _ISCSIC_GLOBALS_H
#define _ISCSIC_GLOBALS_H

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/scsiio.h>

/* iSCSI daemon ioctl interface */
#include <iscsid.h>

#include <iscsi_ioctl.h>

/* -------------------------  Global Constants  ----------------------------- */

/* Version information */

#define VERSION_MAJOR		3
#define VERSION_MINOR		1
#define VERSION_STRING		"NetBSD iSCSI Software Initiator CLI 20110407 "


#define TRUE   1
#define FALSE  0

#define BUF_SIZE     8192

/* ---------------------------  Global Types  ------------------------------- */

typedef int (*cmdproc_t) (int, char **);

typedef struct {
	const char	*cmd;
	cmdproc_t	 proc;
} command_t;


/* -------------------------  Global Variables  ----------------------------- */

int driver;						/* handle to driver (for ioctls) */
uint8_t buf[BUF_SIZE];			/* buffer for daemon comm and driver I/O */

/* -------------------------  Global Functions  ----------------------------- */

#define min(a,b) ((a < b) ? a : b)

/* Debugging stuff */

#ifdef ISCSI_DEBUG

int debug_level;				/* How much info to display */

#define DEBOUT(x) printf x
#define DEB(lev,x) {if (debug_level >= lev) printf x ;}

#define STATIC static

#else

#define DEBOUT(x)
#define DEB(lev,x)

#define STATIC static

#endif

/*
 * Convert uint64 to 6-byte string in network byte order (for ISID field)
*/
static __inline void
hton6(uint8_t * d, uint64_t x)
{
#if BYTE_ORDER == LITTLE_ENDIAN
	uint8_t *s = ((uint8_t *)(void *)&x) + 5;
	*d++ = *s--;
	*d++ = *s--;
	*d++ = *s--;
	*d++ = *s--;
	*d++ = *s--;
	*d = *s;
#else
	memcpy(d, &((uint8_t *)&x)[2], 6);
#endif
}

/*
 * Convert uint64 from network byte order (for LUN field)
*/
static __inline uint64_t
ntohq(uint64_t x)
{
#if BYTE_ORDER == LITTLE_ENDIAN
	uint8_t *s = (uint8_t *)(void *)&x;

	return (uint64_t) ((uint64_t) s[0] << 56 | (uint64_t) s[1] << 48 |
			(uint64_t) s[2] << 40 | (uint64_t) s[3] << 32 |
			(uint64_t) s[4] << 24 | (uint64_t) s[5] << 16 |
			(uint64_t) s[6] <<  8 | (uint64_t) s[7]);
#else
	return x;
#endif
}

#define htonq(x)  ntohq(x)


/* we usually have to get the id out of a message */
#define GET_SYM_ID(x, y)	do {					\
	iscsid_sym_id_t	*__param;					\
	__param = (iscsid_sym_id_t *)(void *)(y);			\
	(void) memcpy(&x, &__param->id, sizeof(x));			\
} while (/*CONSTCOND*/0)


/* Check whether ID is present */
#define NO_ID(sid) (!(sid)->id && !(sid)->name[0])

/* iscsic_main.c */

void arg_error(char *, const char *, ...) __printflike(2, 3) __dead;
void arg_missing(const char *) __dead;
void io_error(const char *, ...) __printflike(1, 2) __dead;
void gen_error(const char *, ...) __printflike(1, 2) __dead;
void check_extra_args(int, char **);
void status_error(unsigned) __dead;
void status_error_slist(unsigned) __dead;

void send_request(unsigned, size_t, void *);
iscsid_response_t *get_response(int);
void free_response(iscsid_response_t *);

/* iscsic_daemonif.c */

int add_target(int, char **);
int add_portal(int, char **);
int remove_target(int, char **);
int slp_find_targets(int, char **);
int refresh_targets(int, char **);
int list_targets(int, char **);
int add_send_target(int, char **);
int remove_send_target(int, char **);
int list_send_targets(int, char **);
int add_isns_server(int, char **);
int remove_isns_server(int, char **);
int find_isns_servers(int, char **);
int list_isns_servers(int, char **);
int refresh_isns(int, char **);
int login(int, char **);
int logout(int, char **);
int add_connection(int, char **);
int remove_connection(int, char **);
int add_initiator(int, char **);
int remove_initiator(int, char **);
int list_initiators(int, char **);
int list_sessions(int, char **);
int get_version(int, char **);
#ifdef ISCSI_DEBUG
int kill_daemon(int, char **);
#endif

/* iscsic_driverif.c */

uint32_t get_sessid(int, char **, int);
void dump_data(const char *, const void *, size_t);
int do_ioctl(iscsi_iocommand_parameters_t *, int);
int set_node_name(int, char **);
int inquiry(int, char **);
int test_unit_ready(int, char **);
int read_capacity(int, char **);
int report_luns(int, char **);

/* iscsic_parse.c */

int cl_get_target(iscsid_add_target_req_t **, int, char **, int);
int cl_get_portal(iscsid_add_portal_req_t *, int, char **);
int cl_get_isns(iscsid_add_isns_server_req_t *, int, char **);
int cl_get_send_targets(iscsid_add_target_req_t *, int, char **);
int cl_get_auth_opts(iscsid_set_target_authentication_req_t *, int, char **);
int cl_get_target_opts(iscsid_get_set_target_options_t *, int, char **);
int cl_get_id(char, iscsid_sym_id_t *, int, char **);
int cl_get_symname(uint8_t *, int, char **);
int cl_get_string(char, char *, int, char **);
int cl_get_opt(char, int, char **);
char cl_get_char(char, int, char **);
int cl_get_int(char, int, char **);
int cl_get_uint(char, int, char **);
uint64_t cl_get_longlong(char, int, char **);

#ifdef ISCSI_TEST_MODE
int test(int, char **);
#endif


#endif /* !_ISCSIC_GLOBALS_H */
