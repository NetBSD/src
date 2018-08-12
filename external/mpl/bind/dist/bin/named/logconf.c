/*	$NetBSD: logconf.c,v 1.2 2018/08/12 13:02:27 christos Exp $	*/

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

#include <isc/file.h>
#include <isc/offset.h>
#include <isc/print.h>
#include <isc/result.h>
#include <isc/stdio.h>
#include <isc/string.h>
#include <isc/syslog.h>
#include <isc/util.h>

#include <isccfg/cfg.h>
#include <isccfg/log.h>

#include <named/log.h>
#include <named/logconf.h>

#define CHECK(op) \
	do { result = (op); 				  	 \
	       if (result != ISC_R_SUCCESS) goto cleanup; 	 \
	} while (/*CONSTCOND*/0)

/*%
 * Set up a logging category according to the named.conf data
 * in 'ccat' and add it to 'logconfig'.
 */
static isc_result_t
category_fromconf(const cfg_obj_t *ccat, isc_logconfig_t *logconfig) {
	isc_result_t result;
	const char *catname;
	isc_logcategory_t *category;
	isc_logmodule_t *module;
	const cfg_obj_t *destinations = NULL;
	const cfg_listelt_t *element = NULL;

	catname = cfg_obj_asstring(cfg_tuple_get(ccat, "name"));
	category = isc_log_categorybyname(named_g_lctx, catname);
	if (category == NULL) {
		cfg_obj_log(ccat, named_g_lctx, ISC_LOG_ERROR,
			    "unknown logging category '%s' ignored",
			    catname);
		/*
		 * Allow further processing by returning success.
		 */
		return (ISC_R_SUCCESS);
	}

	if (logconfig == NULL)
		return (ISC_R_SUCCESS);

	module = NULL;

	destinations = cfg_tuple_get(ccat, "destinations");
	for (element = cfg_list_first(destinations);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		const cfg_obj_t *channel = cfg_listelt_value(element);
		const char *channelname = cfg_obj_asstring(channel);

		result = isc_log_usechannel(logconfig, channelname, category,
					    module);
		if (result != ISC_R_SUCCESS) {
			isc_log_write(named_g_lctx, CFG_LOGCATEGORY_CONFIG,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
				      "logging channel '%s': %s", channelname,
				      isc_result_totext(result));
			return (result);
		}
	}
	return (ISC_R_SUCCESS);
}

/*%
 * Set up a logging channel according to the named.conf data
 * in 'cchan' and add it to 'logconfig'.
 */
static isc_result_t
channel_fromconf(const cfg_obj_t *channel, isc_logconfig_t *logconfig) {
	isc_result_t result;
	isc_logdestination_t dest;
	unsigned int type;
	unsigned int flags = 0;
	int level;
	const char *channelname;
	const cfg_obj_t *fileobj = NULL;
	const cfg_obj_t *syslogobj = NULL;
	const cfg_obj_t *nullobj = NULL;
	const cfg_obj_t *stderrobj = NULL;
	const cfg_obj_t *severity = NULL;
	int i;

	channelname = cfg_obj_asstring(cfg_map_getname(channel));

	(void)cfg_map_get(channel, "file", &fileobj);
	(void)cfg_map_get(channel, "syslog", &syslogobj);
	(void)cfg_map_get(channel, "null", &nullobj);
	(void)cfg_map_get(channel, "stderr", &stderrobj);

	i = 0;
	if (fileobj != NULL)
		i++;
	if (syslogobj != NULL)
		i++;
	if (nullobj != NULL)
		i++;
	if (stderrobj != NULL)
		i++;

	if (i != 1) {
		cfg_obj_log(channel, named_g_lctx, ISC_LOG_ERROR,
			      "channel '%s': exactly one of file, syslog, "
			      "null, and stderr must be present", channelname);
		return (ISC_R_FAILURE);
	}

	type = ISC_LOG_TONULL;

	if (fileobj != NULL) {
		const cfg_obj_t *pathobj = cfg_tuple_get(fileobj, "file");
		const cfg_obj_t *sizeobj = cfg_tuple_get(fileobj, "size");
		const cfg_obj_t *versionsobj =
				 cfg_tuple_get(fileobj, "versions");
		const cfg_obj_t *suffixobj =
				 cfg_tuple_get(fileobj, "suffix");
		isc_int32_t versions = ISC_LOG_ROLLNEVER;
		isc_log_rollsuffix_t suffix = isc_log_rollsuffix_increment;
		isc_offset_t size = 0;
		isc_uint64_t maxoffset;

		/*
		 * isc_offset_t is a signed integer type, so the maximum
		 * value is all 1s except for the MSB.
		 */
		switch (sizeof(isc_offset_t)) {
		case 4:
			maxoffset = 0x7fffffffULL;
			break;
		case 8:
			maxoffset = 0x7fffffffffffffffULL;
			break;
		default:
			INSIST(0);
		}

		type = ISC_LOG_TOFILE;

		if (versionsobj != NULL && cfg_obj_isuint32(versionsobj))
			versions = cfg_obj_asuint32(versionsobj);
		else if (versionsobj != NULL && cfg_obj_isstring(versionsobj) &&
		    strcasecmp(cfg_obj_asstring(versionsobj), "unlimited") == 0)
			versions = ISC_LOG_ROLLINFINITE;
		if (sizeobj != NULL &&
		    cfg_obj_isuint64(sizeobj) &&
		    cfg_obj_asuint64(sizeobj) < maxoffset)
			size = (isc_offset_t)cfg_obj_asuint64(sizeobj);
		if (suffixobj != NULL && cfg_obj_isstring(suffixobj) &&
		    strcasecmp(cfg_obj_asstring(suffixobj), "timestamp") == 0)
			suffix = isc_log_rollsuffix_timestamp;

		dest.file.stream = NULL;
		dest.file.name = cfg_obj_asstring(pathobj);
		dest.file.versions = versions;
		dest.file.suffix = suffix;
		dest.file.maximum_size = size;
	} else if (syslogobj != NULL) {
		int facility = LOG_DAEMON;

		type = ISC_LOG_TOSYSLOG;

		if (cfg_obj_isstring(syslogobj)) {
			const char *facilitystr = cfg_obj_asstring(syslogobj);
			(void)isc_syslog_facilityfromstring(facilitystr,
							    &facility);
		}
		dest.facility = facility;
	} else if (stderrobj != NULL) {
		type = ISC_LOG_TOFILEDESC;
		dest.file.stream = stderr;
		dest.file.name = NULL;
		dest.file.versions = ISC_LOG_ROLLNEVER;
		dest.file.suffix = isc_log_rollsuffix_increment;
		dest.file.maximum_size = 0;
	}

	/*
	 * Munge flags.
	 */
	{
		const cfg_obj_t *printcat = NULL;
		const cfg_obj_t *printsev = NULL;
		const cfg_obj_t *printtime = NULL;
		const cfg_obj_t *buffered = NULL;

		(void)cfg_map_get(channel, "print-category", &printcat);
		(void)cfg_map_get(channel, "print-severity", &printsev);
		(void)cfg_map_get(channel, "print-time", &printtime);
		(void)cfg_map_get(channel, "buffered", &buffered);

		if (printcat != NULL && cfg_obj_asboolean(printcat))
			flags |= ISC_LOG_PRINTCATEGORY;
		if (printsev != NULL && cfg_obj_asboolean(printsev))
			flags |= ISC_LOG_PRINTLEVEL;
		if (buffered != NULL && cfg_obj_asboolean(buffered))
			flags |= ISC_LOG_BUFFERED;
		if (printtime != NULL && cfg_obj_isboolean(printtime)) {
			if (cfg_obj_asboolean(printtime))
				flags |= ISC_LOG_PRINTTIME;
		} else if (printtime != NULL) {	/* local/iso8601/iso8601-utc */
			const char *s = cfg_obj_asstring(printtime);
			flags |= ISC_LOG_PRINTTIME;
			if (strcasecmp(s, "iso8601") == 0)
				flags |= ISC_LOG_ISO8601;
			else if (strcasecmp(s, "iso8601-utc") == 0)
				flags |= ISC_LOG_ISO8601 | ISC_LOG_UTC;
		}
	}

	level = ISC_LOG_INFO;
	if (cfg_map_get(channel, "severity", &severity) == ISC_R_SUCCESS) {
		if (cfg_obj_isstring(severity)) {
			const char *str = cfg_obj_asstring(severity);
			if (strcasecmp(str, "critical") == 0)
				level = ISC_LOG_CRITICAL;
			else if (strcasecmp(str, "error") == 0)
				level = ISC_LOG_ERROR;
			else if (strcasecmp(str, "warning") == 0)
				level = ISC_LOG_WARNING;
			else if (strcasecmp(str, "notice") == 0)
				level = ISC_LOG_NOTICE;
			else if (strcasecmp(str, "info") == 0)
				level = ISC_LOG_INFO;
			else if (strcasecmp(str, "dynamic") == 0)
				level = ISC_LOG_DYNAMIC;
		} else
			/* debug */
			level = cfg_obj_asuint32(severity);
	}

	if (logconfig == NULL)
		result = ISC_R_SUCCESS;
	else
		result = isc_log_createchannel(logconfig, channelname,
					       type, level, &dest, flags);

	if (result == ISC_R_SUCCESS && type == ISC_LOG_TOFILE) {
		FILE *fp;

		/*
		 * Test to make sure that file is a plain file.
		 * Fix defect #22771
		*/
		result = isc_file_isplainfile(dest.file.name);
		if (result == ISC_R_SUCCESS || result == ISC_R_FILENOTFOUND) {
			/*
			 * Test that the file can be opened, since
			 * isc_log_open() can't effectively report
			 * failures when called in isc_log_doit().
			 */
			result = isc_stdio_open(dest.file.name, "a", &fp);
			if (result != ISC_R_SUCCESS) {
				if (logconfig != NULL && !named_g_nosyslog)
					syslog(LOG_ERR,
						"isc_stdio_open '%s' failed: "
						"%s", dest.file.name,
						isc_result_totext(result));
				fprintf(stderr,
					"isc_stdio_open '%s' failed: %s\n",
					dest.file.name,
					isc_result_totext(result));
			} else
				(void)isc_stdio_close(fp);
			goto done;
		}
		if (logconfig != NULL && !named_g_nosyslog)
			syslog(LOG_ERR, "isc_file_isplainfile '%s' failed: %s",
			       dest.file.name, isc_result_totext(result));
		fprintf(stderr, "isc_file_isplainfile '%s' failed: %s\n",
			dest.file.name, isc_result_totext(result));
	}

 done:
	return (result);
}

isc_result_t
named_logconfig(isc_logconfig_t *logconfig, const cfg_obj_t *logstmt) {
	isc_result_t result;
	const cfg_obj_t *channels = NULL;
	const cfg_obj_t *categories = NULL;
	const cfg_listelt_t *element;
	isc_boolean_t default_set = ISC_FALSE;
	isc_boolean_t unmatched_set = ISC_FALSE;
	const cfg_obj_t *catname;

	if (logconfig != NULL)
		CHECK(named_log_setdefaultchannels(logconfig));

	(void)cfg_map_get(logstmt, "channel", &channels);
	for (element = cfg_list_first(channels);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		const cfg_obj_t *channel = cfg_listelt_value(element);
		CHECK(channel_fromconf(channel, logconfig));
	}

	(void)cfg_map_get(logstmt, "category", &categories);
	for (element = cfg_list_first(categories);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		const cfg_obj_t *category = cfg_listelt_value(element);
		CHECK(category_fromconf(category, logconfig));
		if (!default_set) {
			catname = cfg_tuple_get(category, "name");
			if (strcmp(cfg_obj_asstring(catname), "default") == 0)
				default_set = ISC_TRUE;
		}
		if (!unmatched_set) {
			catname = cfg_tuple_get(category, "name");
			if (strcmp(cfg_obj_asstring(catname), "unmatched") == 0)
				unmatched_set = ISC_TRUE;
		}
	}

	if (logconfig != NULL && !default_set)
		CHECK(named_log_setdefaultcategory(logconfig));

	if (logconfig != NULL && !unmatched_set)
		CHECK(named_log_setunmatchedcategory(logconfig));

	return (ISC_R_SUCCESS);

 cleanup:
	return (result);
}
