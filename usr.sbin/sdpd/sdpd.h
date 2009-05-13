/*	$NetBSD: sdpd.h,v 1.1.2.2 2009/05/13 19:20:39 jym Exp $	*/

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
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * Copyright (c) 2004 Maksim Yevmenkin <m_evmenkin@yahoo.com>
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
 */

#ifndef _SDPD_H_
#define _SDPD_H_

#include <sys/types.h>
#include <sys/queue.h>

#include <bluetooth.h>
#include <sdp.h>
#include <stdbool.h>

/*
 * Service Record entry
 */
struct record {
	int			fd;	/* owner */
	bool			valid;	/* record is current */
	uint32_t		handle;	/* ServiceRecord handle */
	sdp_data_t		data;	/* ServiceRecord data */
	bdaddr_t		bdaddr;	/* restricted device address */
	int			refcnt;	/* reference count */
	fd_set			refset;	/* reference bitset */
	LIST_ENTRY(record)	next;	/* next ServiceRecord */
	uint8_t			ext[0];	/* raw data storage ... */
};

typedef	struct record record_t;

/*
 * File descriptor (client) index entry
 */
struct fd_idx {
	bool			valid;	/* descriptor is valid */
	bool			server;	/* descriptor is listening */
	bool			control;/* descriptor is control socket */
	bool			priv;	/* descriptor may modify service record db */
	uint16_t		omtu;	/* outgoing MTU */
	uint16_t		offset;	/* stored ContinuationState */
	bdaddr_t		bdaddr;	/* clients local device address */
};

typedef struct fd_idx fd_idx_t;

/*
 * SDP server
 */
struct server {
	uint16_t		imtu;	/* incoming MTU */
	uint8_t *		ibuf;	/* input buffer */
	size_t			ctllen;	/* control msg buffer length */
	uint8_t *		ctlbuf;	/* control msg buffer */
	sdp_pdu_t		pdu;	/* PDU header */
	uint16_t		omtu;	/* outgoing MTU */
	uint8_t *		obuf;	/* output buffer */
	uint32_t		handle;	/* next ServiceRecordHandle */
	LIST_HEAD(, record)	rlist;	/* ServiceRecord list */
	int			fdmax;	/* descriptor max index */
	fd_idx_t *		fdidx;	/* descriptor index */
	fd_set			fdset;	/* current descriptor set */
	const char *		sgroup;	/* privileged group */
};

typedef struct server server_t;

/* compat.c */
uint16_t compat_register_request(server_t *, int);
uint16_t compat_change_request(server_t *, int);

/* db.c */
bool	db_init(server_t *);
bool	db_next(server_t *, int, record_t **);
void	db_select_ssp(server_t *, int, sdp_data_t *);
void	db_select_handle(server_t *, int, uint32_t);
bool	db_create(server_t *, int, const bdaddr_t *, uint32_t, sdp_data_t *);
void	db_unselect(server_t *, int);
void	db_release(server_t *, int);

/* log.c */
void	log_open(char const *, bool);
void	log_close(void);
void	log_emerg(char const *, ...);
void	log_alert(char const *, ...);
void	log_crit(char const *, ...);
void	log_err(char const *, ...);
void	log_warning(char const *, ...);
void	log_notice(char const *, ...);
void	log_info(char const *, ...);
void	log_debug(char const *, ...);

/* record.c */
uint16_t record_insert_request(server_t *, int);
uint16_t record_update_request(server_t *, int);
uint16_t record_remove_request(server_t *, int);

/* server.c */
bool	server_init(server_t *, const char *, const char *);
void	server_shutdown(server_t *);
bool	server_do(server_t *);
void	server_error_response(server_t *, int, uint16_t);

/* service.c */
uint16_t service_search_request(server_t *, int);
uint16_t service_attribute_request(server_t *, int);
uint16_t service_search_attribute_request(server_t *, int);

#endif /* _SDPD_H_ */
