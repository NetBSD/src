/*	$NetBSD: control.h,v 1.1.1.1 2018/08/12 12:07:44 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#ifndef NAMED_CONTROL_H
#define NAMED_CONTROL_H 1

/*! \file
 * \brief
 * The name server command channel.
 */

#include <isccc/types.h>

#include <isccfg/aclconf.h>

#include <named/types.h>

#define NAMED_CONTROL_PORT			953

#define NAMED_COMMAND_STOP		"stop"
#define NAMED_COMMAND_HALT		"halt"
#define NAMED_COMMAND_RELOAD		"reload"
#define NAMED_COMMAND_RECONFIG		"reconfig"
#define NAMED_COMMAND_REFRESH		"refresh"
#define NAMED_COMMAND_RETRANSFER	"retransfer"
#define NAMED_COMMAND_DUMPSTATS		"stats"
#define NAMED_COMMAND_QUERYLOG		"querylog"
#define NAMED_COMMAND_DUMPDB		"dumpdb"
#define NAMED_COMMAND_SECROOTS		"secroots"
#define NAMED_COMMAND_TRACE		"trace"
#define NAMED_COMMAND_NOTRACE		"notrace"
#define NAMED_COMMAND_FLUSH		"flush"
#define NAMED_COMMAND_FLUSHNAME		"flushname"
#define NAMED_COMMAND_FLUSHTREE		"flushtree"
#define NAMED_COMMAND_STATUS		"status"
#define NAMED_COMMAND_TSIGLIST		"tsig-list"
#define NAMED_COMMAND_TSIGDELETE	"tsig-delete"
#define NAMED_COMMAND_FREEZE		"freeze"
#define NAMED_COMMAND_UNFREEZE		"unfreeze"
#define NAMED_COMMAND_THAW		"thaw"
#define NAMED_COMMAND_TIMERPOKE		"timerpoke"
#define NAMED_COMMAND_RECURSING		"recursing"
#define NAMED_COMMAND_NULL		"null"
#define NAMED_COMMAND_NOTIFY		"notify"
#define NAMED_COMMAND_VALIDATION	"validation"
#define NAMED_COMMAND_SCAN 		"scan"
#define NAMED_COMMAND_SIGN 		"sign"
#define NAMED_COMMAND_LOADKEYS 		"loadkeys"
#define NAMED_COMMAND_ADDZONE		"addzone"
#define NAMED_COMMAND_MODZONE		"modzone"
#define NAMED_COMMAND_DELZONE		"delzone"
#define NAMED_COMMAND_SHOWZONE		"showzone"
#define NAMED_COMMAND_SYNC		"sync"
#define NAMED_COMMAND_SIGNING		"signing"
#define NAMED_COMMAND_ZONESTATUS	"zonestatus"
#define NAMED_COMMAND_NTA		"nta"
#define NAMED_COMMAND_TESTGEN		"testgen"
#define NAMED_COMMAND_MKEYS		"managed-keys"
#define NAMED_COMMAND_DNSTAPREOPEN	"dnstap-reopen"
#define NAMED_COMMAND_DNSTAP		"dnstap"
#define NAMED_COMMAND_TCPTIMEOUTS	"tcp-timeouts"
#define NAMED_COMMAND_SERVESTALE	"serve-stale"

isc_result_t
named_controls_create(named_server_t *server, named_controls_t **ctrlsp);
/*%<
 * Create an initial, empty set of command channels for 'server'.
 */

void
named_controls_destroy(named_controls_t **ctrlsp);
/*%<
 * Destroy a set of command channels.
 *
 * Requires:
 *	Shutdown of the channels has completed.
 */

isc_result_t
named_controls_configure(named_controls_t *controls, const cfg_obj_t *config,
			 cfg_aclconfctx_t *aclconfctx);
/*%<
 * Configure zero or more command channels into 'controls'
 * as defined in the configuration parse tree 'config'.
 * The channels will evaluate ACLs in the context of
 * 'aclconfctx'.
 */

void
named_controls_shutdown(named_controls_t *controls);
/*%<
 * Initiate shutdown of all the command channels in 'controls'.
 */

isc_result_t
named_control_docommand(isccc_sexpr_t *message, isc_boolean_t readonly,
			isc_buffer_t **text);

#endif /* NAMED_CONTROL_H */
