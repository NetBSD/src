/*	$NetBSD: control.c,v 1.4 2019/02/24 20:01:27 christos Exp $	*/

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


/*! \file */

#include <config.h>

#include <stdbool.h>

#include <isc/app.h>
#include <isc/event.h>
#include <isc/lex.h>
#include <isc/mem.h>
#include <isc/string.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/result.h>

#include <isccc/alist.h>
#include <isccc/cc.h>
#include <isccc/result.h>

#include <named/control.h>
#include <named/globals.h>
#include <named/log.h>
#include <named/os.h>
#include <named/server.h>
#ifdef HAVE_LIBSCF
#include <named/smf_globals.h>
#endif

static isc_result_t
getcommand(isc_lex_t *lex, char **cmdp) {
	isc_result_t result;
	isc_token_t token;

	REQUIRE(cmdp != NULL && *cmdp == NULL);

	result = isc_lex_gettoken(lex, ISC_LEXOPT_EOF, &token);
	if (result != ISC_R_SUCCESS)
		return (result);

	isc_lex_ungettoken(lex, &token);

	if (token.type != isc_tokentype_string)
		return (ISC_R_FAILURE);

	*cmdp = token.value.as_textregion.base;

	return (ISC_R_SUCCESS);
}

static inline bool
command_compare(const char *str, const char *command) {
	return (strcasecmp(str, command) == 0);
}

/*%
 * This function is called to process the incoming command
 * when a control channel message is received.
 */
isc_result_t
named_control_docommand(isccc_sexpr_t *message, bool readonly,
			isc_buffer_t **text)
{
	isccc_sexpr_t *data;
	char *cmdline = NULL;
	char *command = NULL;
	isc_result_t result;
	int log_level;
	isc_buffer_t src;
	isc_lex_t *lex = NULL;
#ifdef HAVE_LIBSCF
	named_smf_want_disable = 0;
#endif

	data = isccc_alist_lookup(message, "_data");
	if (!isccc_alist_alistp(data)) {
		/*
		 * No data section.
		 */
		return (ISC_R_FAILURE);
	}

	result = isccc_cc_lookupstring(data, "type", &cmdline);
	if (result != ISC_R_SUCCESS) {
		/*
		 * We have no idea what this is.
		 */
		return (result);
	}

	result = isc_lex_create(named_g_mctx, strlen(cmdline), &lex);
	if (result != ISC_R_SUCCESS)
		return (result);

	isc_buffer_init(&src, cmdline, strlen(cmdline));
	isc_buffer_add(&src, strlen(cmdline));
	result = isc_lex_openbuffer(lex, &src);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	result = getcommand(lex, &command);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	/*
	 * Compare the 'command' parameter against all known control commands.
	 */
	if ((command_compare(command, NAMED_COMMAND_NULL) &&
	     strlen(cmdline) == 4) ||
	    command_compare(command, NAMED_COMMAND_STATUS))
	{
		log_level = ISC_LOG_DEBUG(1);
	} else {
		log_level = ISC_LOG_INFO;
	}

	/*
	 * If this listener should have read-only access, reject
	 * restricted commands here. rndc nta is handled specially
	 * below.
	 */
	if (readonly &&
	    !command_compare(command, NAMED_COMMAND_NTA) &&
	    !command_compare(command, NAMED_COMMAND_NULL) &&
	    !command_compare(command, NAMED_COMMAND_STATUS) &&
	    !command_compare(command, NAMED_COMMAND_SHOWZONE) &&
	    !command_compare(command, NAMED_COMMAND_TESTGEN) &&
	    !command_compare(command, NAMED_COMMAND_ZONESTATUS))
	{
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_CONTROL, log_level,
			      "rejecting restricted control channel "
			      "command '%s'", cmdline);
		result = ISC_R_FAILURE;
		goto cleanup;
	}

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_CONTROL, log_level,
		      "received control channel command '%s'",
		      cmdline);

	/*
	 * After the lengthy "halt" and "stop", the commands are
	 * handled in alphabetical order of the NAMED_COMMAND_ macros.
	 */
	if (command_compare(command, NAMED_COMMAND_HALT)) {
#ifdef HAVE_LIBSCF
		/*
		 * If we are managed by smf(5), AND in chroot, then
		 * we cannot connect to the smf repository, so just
		 * return with an appropriate message back to rndc.
		 */
		if (named_smf_got_instance == 1 && named_smf_chroot == 1) {
			result = named_smf_add_message(text);
			goto cleanup;
		}
		/*
		 * If we are managed by smf(5) but not in chroot,
		 * try to disable ourselves the smf way.
		 */
		if (named_smf_got_instance == 1 && named_smf_chroot == 0)
			named_smf_want_disable = 1;
		/*
		 * If named_smf_got_instance = 0, named_smf_chroot
		 * is not relevant and we fall through to
		 * isc_app_shutdown below.
		 */
#endif
		/* Do not flush master files */
		named_server_flushonshutdown(named_g_server, false);
		named_os_shutdownmsg(cmdline, *text);
		isc_app_shutdown();
		result = ISC_R_SUCCESS;
	} else if (command_compare(command, NAMED_COMMAND_STOP)) {
		/*
		 * "stop" is the same as "halt" except it does
		 * flush master files.
		 */
#ifdef HAVE_LIBSCF
		if (named_smf_got_instance == 1 && named_smf_chroot == 1) {
			result = named_smf_add_message(text);
			goto cleanup;
		}
		if (named_smf_got_instance == 1 && named_smf_chroot == 0)
			named_smf_want_disable = 1;
#endif
		named_server_flushonshutdown(named_g_server, true);
		named_os_shutdownmsg(cmdline, *text);
		isc_app_shutdown();
		result = ISC_R_SUCCESS;
	} else if (command_compare(command, NAMED_COMMAND_ADDZONE) ||
		   command_compare(command, NAMED_COMMAND_MODZONE)) {
		result = named_server_changezone(named_g_server, cmdline, text);
	} else if (command_compare(command, NAMED_COMMAND_DELZONE)) {
		result = named_server_delzone(named_g_server, lex, text);
	} else if (command_compare(command, NAMED_COMMAND_DNSTAP) ||
		   command_compare(command, NAMED_COMMAND_DNSTAPREOPEN)) {
		result = named_server_dnstap(named_g_server, lex, text);
	} else if (command_compare(command, NAMED_COMMAND_DUMPDB)) {
		named_server_dumpdb(named_g_server, lex, text);
		result = ISC_R_SUCCESS;
	} else if (command_compare(command, NAMED_COMMAND_DUMPSTATS)) {
		result = named_server_dumpstats(named_g_server);
	} else if (command_compare(command, NAMED_COMMAND_FLUSH)) {
		result = named_server_flushcache(named_g_server, lex);
	} else if (command_compare(command, NAMED_COMMAND_FLUSHNAME)) {
		result = named_server_flushnode(named_g_server, lex, false);
	} else if (command_compare(command, NAMED_COMMAND_FLUSHTREE)) {
		result = named_server_flushnode(named_g_server, lex, true);
	} else if (command_compare(command, NAMED_COMMAND_FREEZE)) {
		result = named_server_freeze(named_g_server, true, lex, text);
	} else if (command_compare(command, NAMED_COMMAND_LOADKEYS) ||
		   command_compare(command, NAMED_COMMAND_SIGN)) {
		result = named_server_rekey(named_g_server, lex, text);
	} else if (command_compare(command, NAMED_COMMAND_MKEYS)) {
		result = named_server_mkeys(named_g_server, lex, text);
	} else if (command_compare(command, NAMED_COMMAND_NOTIFY)) {
		result = named_server_notifycommand(named_g_server, lex, text);
	} else if (command_compare(command, NAMED_COMMAND_NOTRACE)) {
		named_g_debuglevel = 0;
		isc_log_setdebuglevel(named_g_lctx, named_g_debuglevel);
		result = ISC_R_SUCCESS;
	} else if (command_compare(command, NAMED_COMMAND_NTA)) {
		result = named_server_nta(named_g_server, lex, readonly, text);
	} else if (command_compare(command, NAMED_COMMAND_NULL)) {
		result = ISC_R_SUCCESS;
	} else if (command_compare(command, NAMED_COMMAND_QUERYLOG)) {
		result = named_server_togglequerylog(named_g_server, lex);
	} else if (command_compare(command, NAMED_COMMAND_RECONFIG)) {
		result = named_server_reconfigcommand(named_g_server);
	} else if (command_compare(command, NAMED_COMMAND_RECURSING)) {
		result = named_server_dumprecursing(named_g_server);
	} else if (command_compare(command, NAMED_COMMAND_REFRESH)) {
		result = named_server_refreshcommand(named_g_server, lex, text);
	} else if (command_compare(command, NAMED_COMMAND_RELOAD)) {
		result = named_server_reloadcommand(named_g_server, lex, text);
	} else if (command_compare(command, NAMED_COMMAND_RETRANSFER)) {
		result = named_server_retransfercommand(named_g_server,
							lex, text);
	} else if (command_compare(command, NAMED_COMMAND_SCAN)) {
		named_server_scan_interfaces(named_g_server);
		result = ISC_R_SUCCESS;
	} else if (command_compare(command, NAMED_COMMAND_SECROOTS)) {
		result = named_server_dumpsecroots(named_g_server, lex, text);
	} else if (command_compare(command, NAMED_COMMAND_SERVESTALE)) {
		result = named_server_servestale(named_g_server, lex, text);
	} else if (command_compare(command, NAMED_COMMAND_SHOWZONE)) {
		result = named_server_showzone(named_g_server, lex, text);
	} else if (command_compare(command, NAMED_COMMAND_SIGNING)) {
		result = named_server_signing(named_g_server, lex, text);
	} else if (command_compare(command, NAMED_COMMAND_STATUS)) {
		result = named_server_status(named_g_server, text);
	} else if (command_compare(command, NAMED_COMMAND_SYNC)) {
		result = named_server_sync(named_g_server, lex, text);
	} else if (command_compare(command, NAMED_COMMAND_TCPTIMEOUTS)) {
		result = named_server_tcptimeouts(lex, text);
	} else if (command_compare(command, NAMED_COMMAND_TESTGEN)) {
		result = named_server_testgen(lex, text);
	} else if (command_compare(command, NAMED_COMMAND_THAW) ||
		   command_compare(command, NAMED_COMMAND_UNFREEZE)) {
		result = named_server_freeze(named_g_server, false, lex, text);
	} else if (command_compare(command, NAMED_COMMAND_TIMERPOKE)) {
		isc_timermgr_poke(named_g_timermgr);
		result = ISC_R_SUCCESS;
	} else if (command_compare(command, NAMED_COMMAND_TRACE)) {
		result = named_server_setdebuglevel(named_g_server, lex);
	} else if (command_compare(command, NAMED_COMMAND_TSIGDELETE)) {
		result = named_server_tsigdelete(named_g_server, lex, text);
	} else if (command_compare(command, NAMED_COMMAND_TSIGLIST)) {
		result = named_server_tsiglist(named_g_server, text);
	} else if (command_compare(command, NAMED_COMMAND_VALIDATION)) {
		result = named_server_validation(named_g_server, lex, text);
	} else if (command_compare(command, NAMED_COMMAND_ZONESTATUS)) {
		result = named_server_zonestatus(named_g_server, lex, text);
	} else {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_CONTROL, ISC_LOG_WARNING,
			      "unknown control channel command '%s'",
			      command);
		result = DNS_R_UNKNOWNCOMMAND;
	}

 cleanup:
	if (lex != NULL)
		isc_lex_destroy(&lex);

	return (result);
}
