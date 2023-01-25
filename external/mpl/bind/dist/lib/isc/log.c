/*	$NetBSD: log.c,v 1.8 2023/01/25 21:43:31 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h> /* dev_t FreeBSD 2.1 */
#include <time.h>

#include <isc/atomic.h>
#include <isc/dir.h>
#include <isc/file.h>
#include <isc/log.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/platform.h>
#include <isc/print.h>
#include <isc/rwlock.h>
#include <isc/stat.h>
#include <isc/stdio.h>
#include <isc/string.h>
#include <isc/time.h>
#include <isc/util.h>

#define LCTX_MAGIC	    ISC_MAGIC('L', 'c', 't', 'x')
#define VALID_CONTEXT(lctx) ISC_MAGIC_VALID(lctx, LCTX_MAGIC)

#define LCFG_MAGIC	   ISC_MAGIC('L', 'c', 'f', 'g')
#define VALID_CONFIG(lcfg) ISC_MAGIC_VALID(lcfg, LCFG_MAGIC)

#define RDLOCK(lp)   RWLOCK(lp, isc_rwlocktype_read);
#define WRLOCK(lp)   RWLOCK(lp, isc_rwlocktype_write);
#define RDUNLOCK(lp) RWUNLOCK(lp, isc_rwlocktype_read);
#define WRUNLOCK(lp) RWUNLOCK(lp, isc_rwlocktype_write);

/*
 * XXXDCL make dynamic?
 */
#define LOG_BUFFER_SIZE (8 * 1024)

/*!
 * This is the structure that holds each named channel.  A simple linked
 * list chains all of the channels together, so an individual channel is
 * found by doing strcmp()s with the names down the list.  Their should
 * be no performance penalty from this as it is expected that the number
 * of named channels will be no more than a dozen or so, and name lookups
 * from the head of the list are only done when isc_log_usechannel() is
 * called, which should also be very infrequent.
 */
typedef struct isc_logchannel isc_logchannel_t;

struct isc_logchannel {
	char *name;
	unsigned int type;
	int level;
	unsigned int flags;
	isc_logdestination_t destination;
	ISC_LINK(isc_logchannel_t) link;
};

/*!
 * The logchannellist structure associates categories and modules with
 * channels.  First the appropriate channellist is found based on the
 * category, and then each structure in the linked list is checked for
 * a matching module.  It is expected that the number of channels
 * associated with any given category will be very short, no more than
 * three or four in the more unusual cases.
 */
typedef struct isc_logchannellist isc_logchannellist_t;

struct isc_logchannellist {
	const isc_logmodule_t *module;
	isc_logchannel_t *channel;
	ISC_LINK(isc_logchannellist_t) link;
};

/*!
 * This structure is used to remember messages for pruning via
 * isc_log_[v]write1().
 */
typedef struct isc_logmessage isc_logmessage_t;

struct isc_logmessage {
	char *text;
	isc_time_t time;
	ISC_LINK(isc_logmessage_t) link;
};

/*!
 * The isc_logconfig structure is used to store the configurable information
 * about where messages are actually supposed to be sent -- the information
 * that could changed based on some configuration file, as opposed to the
 * the category/module specification of isc_log_[v]write[1] that is compiled
 * into a program, or the debug_level which is dynamic state information.
 */
struct isc_logconfig {
	unsigned int magic;
	isc_log_t *lctx;
	ISC_LIST(isc_logchannel_t) channels;
	ISC_LIST(isc_logchannellist_t) * channellists;
	unsigned int channellist_count;
	unsigned int duplicate_interval;
	int_fast32_t highest_level;
	char *tag;
	bool dynamic;
};

/*!
 * This isc_log structure provides the context for the isc_log functions.
 * The log context locks itself in isc_log_doit, the internal backend to
 * isc_log_write.  The locking is necessary both to provide exclusive access
 * to the buffer into which the message is formatted and to guard against
 * competing threads trying to write to the same syslog resource.  (On
 * some systems, such as BSD/OS, stdio is thread safe but syslog is not.)
 * Unfortunately, the lock cannot guard against a _different_ logging
 * context in the same program competing for syslog's attention.  Thus
 * There Can Be Only One, but this is not enforced.
 * XXXDCL enforce it?
 *
 * Note that the category and module information is not locked.
 * This is because in the usual case, only one isc_log_t is ever created
 * in a program, and the category/module registration happens only once.
 * XXXDCL it might be wise to add more locking overall.
 */
struct isc_log {
	/* Not locked. */
	unsigned int magic;
	isc_mem_t *mctx;
	isc_logcategory_t *categories;
	unsigned int category_count;
	isc_logmodule_t *modules;
	unsigned int module_count;
	atomic_int_fast32_t debug_level;
	isc_rwlock_t lcfg_rwl;
	/* Locked by isc_log lcfg_rwl */
	isc_logconfig_t *logconfig;
	isc_mutex_t lock;
	/* Locked by isc_log lock. */
	char buffer[LOG_BUFFER_SIZE];
	ISC_LIST(isc_logmessage_t) messages;
	atomic_bool dynamic;
	atomic_int_fast32_t highest_level;
};

/*!
 * Used when ISC_LOG_PRINTLEVEL is enabled for a channel.
 */
static const char *log_level_strings[] = { "debug",   "info",  "notice",
					   "warning", "error", "critical" };

/*!
 * Used to convert ISC_LOG_* priorities into syslog priorities.
 * XXXDCL This will need modification for NT.
 */
static const int syslog_map[] = { LOG_DEBUG,   LOG_INFO, LOG_NOTICE,
				  LOG_WARNING, LOG_ERR,	 LOG_CRIT };

/*!
 * When adding new categories, a corresponding ISC_LOGCATEGORY_foo
 * definition needs to be added to <isc/log.h>.
 *
 * The default category is provided so that the internal default can
 * be overridden.  Since the default is always looked up as the first
 * channellist in the log context, it must come first in isc_categories[].
 */
LIBISC_EXTERNAL_DATA isc_logcategory_t isc_categories[] = { { "default",
							      0 }, /* "default
								      must come
								      first. */
							    { "general", 0 },
							    { NULL, 0 } };

/*!
 * See above comment for categories on LIBISC_EXTERNAL_DATA, and apply it to
 * modules.
 */
LIBISC_EXTERNAL_DATA isc_logmodule_t isc_modules[] = {
	{ "socket", 0 }, { "time", 0 },	  { "interface", 0 }, { "timer", 0 },
	{ "file", 0 },	 { "netmgr", 0 }, { "other", 0 },     { NULL, 0 }
};

/*!
 * This essentially constant structure must be filled in at run time,
 * because its channel member is pointed to a channel that is created
 * dynamically with isc_log_createchannel.
 */
static isc_logchannellist_t default_channel;

/*!
 * libisc logs to this context.
 */
LIBISC_EXTERNAL_DATA isc_log_t *isc_lctx = NULL;

/*!
 * Forward declarations.
 */
static void
assignchannel(isc_logconfig_t *lcfg, unsigned int category_id,
	      const isc_logmodule_t *module, isc_logchannel_t *channel);

static void
sync_channellist(isc_logconfig_t *lcfg);

static void
sync_highest_level(isc_log_t *lctx, isc_logconfig_t *lcfg);

static isc_result_t
greatest_version(isc_logfile_t *file, int versions, int *greatest);

static void
isc_log_doit(isc_log_t *lctx, isc_logcategory_t *category,
	     isc_logmodule_t *module, int level, bool write_once,
	     const char *format, va_list args) ISC_FORMAT_PRINTF(6, 0);

/*@{*/
/*!
 * Convenience macros.
 */

#define FACILITY(channel)	 (channel->destination.facility)
#define FILE_NAME(channel)	 (channel->destination.file.name)
#define FILE_STREAM(channel)	 (channel->destination.file.stream)
#define FILE_VERSIONS(channel)	 (channel->destination.file.versions)
#define FILE_SUFFIX(channel)	 (channel->destination.file.suffix)
#define FILE_MAXSIZE(channel)	 (channel->destination.file.maximum_size)
#define FILE_MAXREACHED(channel) (channel->destination.file.maximum_reached)

/*@}*/
/****
**** Public interfaces.
****/

/*
 * Establish a new logging context, with default channels.
 */
void
isc_log_create(isc_mem_t *mctx, isc_log_t **lctxp, isc_logconfig_t **lcfgp) {
	isc_log_t *lctx;
	isc_logconfig_t *lcfg = NULL;

	REQUIRE(mctx != NULL);
	REQUIRE(lctxp != NULL && *lctxp == NULL);
	REQUIRE(lcfgp == NULL || *lcfgp == NULL);

	lctx = isc_mem_get(mctx, sizeof(*lctx));
	lctx->mctx = NULL;
	isc_mem_attach(mctx, &lctx->mctx);
	lctx->categories = NULL;
	lctx->category_count = 0;
	lctx->modules = NULL;
	lctx->module_count = 0;
	atomic_init(&lctx->debug_level, 0);

	ISC_LIST_INIT(lctx->messages);

	isc_mutex_init(&lctx->lock);
	isc_rwlock_init(&lctx->lcfg_rwl, 0, 0);

	/*
	 * Normally setting the magic number is the last step done
	 * in a creation function, but a valid log context is needed
	 * by isc_log_registercategories and isc_logconfig_create.
	 * If either fails, the lctx is destroyed and not returned
	 * to the caller.
	 */
	lctx->magic = LCTX_MAGIC;

	isc_log_registercategories(lctx, isc_categories);
	isc_log_registermodules(lctx, isc_modules);
	isc_logconfig_create(lctx, &lcfg);

	sync_channellist(lcfg);

	lctx->logconfig = lcfg;

	atomic_init(&lctx->highest_level, lcfg->highest_level);
	atomic_init(&lctx->dynamic, lcfg->dynamic);

	*lctxp = lctx;
	if (lcfgp != NULL) {
		*lcfgp = lcfg;
	}
}

void
isc_logconfig_create(isc_log_t *lctx, isc_logconfig_t **lcfgp) {
	isc_logconfig_t *lcfg;
	isc_logdestination_t destination;
	int level = ISC_LOG_INFO;

	REQUIRE(lcfgp != NULL && *lcfgp == NULL);
	REQUIRE(VALID_CONTEXT(lctx));

	lcfg = isc_mem_get(lctx->mctx, sizeof(*lcfg));

	lcfg->lctx = lctx;
	lcfg->channellists = NULL;
	lcfg->channellist_count = 0;
	lcfg->duplicate_interval = 0;
	lcfg->highest_level = level;
	lcfg->tag = NULL;
	lcfg->dynamic = false;
	ISC_LIST_INIT(lcfg->channels);
	lcfg->magic = LCFG_MAGIC;

	/*
	 * Create the default channels:
	 *      default_syslog, default_stderr, default_debug and null.
	 */
	destination.facility = LOG_DAEMON;
	isc_log_createchannel(lcfg, "default_syslog", ISC_LOG_TOSYSLOG, level,
			      &destination, 0);

	destination.file.stream = stderr;
	destination.file.name = NULL;
	destination.file.versions = ISC_LOG_ROLLNEVER;
	destination.file.suffix = isc_log_rollsuffix_increment;
	destination.file.maximum_size = 0;
	isc_log_createchannel(lcfg, "default_stderr", ISC_LOG_TOFILEDESC, level,
			      &destination, ISC_LOG_PRINTTIME);

	/*
	 * Set the default category's channel to default_stderr,
	 * which is at the head of the channels list because it was
	 * just created.
	 */
	default_channel.channel = ISC_LIST_HEAD(lcfg->channels);

	destination.file.stream = stderr;
	destination.file.name = NULL;
	destination.file.versions = ISC_LOG_ROLLNEVER;
	destination.file.suffix = isc_log_rollsuffix_increment;
	destination.file.maximum_size = 0;
	isc_log_createchannel(lcfg, "default_debug", ISC_LOG_TOFILEDESC,
			      ISC_LOG_DYNAMIC, &destination, ISC_LOG_PRINTTIME);

	isc_log_createchannel(lcfg, "null", ISC_LOG_TONULL, ISC_LOG_DYNAMIC,
			      NULL, 0);

	*lcfgp = lcfg;
}

void
isc_logconfig_use(isc_log_t *lctx, isc_logconfig_t *lcfg) {
	isc_logconfig_t *old_cfg;

	REQUIRE(VALID_CONTEXT(lctx));
	REQUIRE(VALID_CONFIG(lcfg));
	REQUIRE(lcfg->lctx == lctx);

	/*
	 * Ensure that lcfg->channellist_count == lctx->category_count.
	 * They won't be equal if isc_log_usechannel has not been called
	 * since any call to isc_log_registercategories.
	 */
	sync_channellist(lcfg);

	WRLOCK(&lctx->lcfg_rwl);
	old_cfg = lctx->logconfig;
	lctx->logconfig = lcfg;
	sync_highest_level(lctx, lcfg);
	WRUNLOCK(&lctx->lcfg_rwl);

	isc_logconfig_destroy(&old_cfg);
}

void
isc_log_destroy(isc_log_t **lctxp) {
	isc_log_t *lctx;
	isc_logconfig_t *lcfg;
	isc_mem_t *mctx;
	isc_logmessage_t *message;

	REQUIRE(lctxp != NULL && VALID_CONTEXT(*lctxp));

	lctx = *lctxp;
	*lctxp = NULL;
	mctx = lctx->mctx;

	/* Stop the logging as a first thing */
	atomic_store_release(&lctx->debug_level, 0);
	atomic_store_release(&lctx->highest_level, 0);
	atomic_store_release(&lctx->dynamic, false);

	WRLOCK(&lctx->lcfg_rwl);
	lcfg = lctx->logconfig;
	lctx->logconfig = NULL;
	WRUNLOCK(&lctx->lcfg_rwl);

	if (lcfg != NULL) {
		isc_logconfig_destroy(&lcfg);
	}

	isc_rwlock_destroy(&lctx->lcfg_rwl);
	isc_mutex_destroy(&lctx->lock);

	while ((message = ISC_LIST_HEAD(lctx->messages)) != NULL) {
		ISC_LIST_UNLINK(lctx->messages, message, link);

		isc_mem_put(mctx, message,
			    sizeof(*message) + strlen(message->text) + 1);
	}

	lctx->buffer[0] = '\0';
	lctx->categories = NULL;
	lctx->category_count = 0;
	lctx->modules = NULL;
	lctx->module_count = 0;
	lctx->mctx = NULL;
	lctx->magic = 0;

	isc_mem_putanddetach(&mctx, lctx, sizeof(*lctx));
}

void
isc_logconfig_destroy(isc_logconfig_t **lcfgp) {
	isc_logconfig_t *lcfg;
	isc_mem_t *mctx;
	isc_logchannel_t *channel;
	char *filename;
	unsigned int i;

	REQUIRE(lcfgp != NULL && VALID_CONFIG(*lcfgp));

	lcfg = *lcfgp;
	*lcfgp = NULL;

	/*
	 * This function cannot be called with a logconfig that is in
	 * use by a log context.
	 */
	REQUIRE(lcfg->lctx != NULL);

	RDLOCK(&lcfg->lctx->lcfg_rwl);
	REQUIRE(lcfg->lctx->logconfig != lcfg);
	RDUNLOCK(&lcfg->lctx->lcfg_rwl);

	mctx = lcfg->lctx->mctx;

	while ((channel = ISC_LIST_HEAD(lcfg->channels)) != NULL) {
		ISC_LIST_UNLINK(lcfg->channels, channel, link);

		if (channel->type == ISC_LOG_TOFILE) {
			/*
			 * The filename for the channel may have ultimately
			 * started its life in user-land as a const string,
			 * but in isc_log_createchannel it gets copied
			 * into writable memory and is not longer truly const.
			 */
			DE_CONST(FILE_NAME(channel), filename);
			isc_mem_free(mctx, filename);

			if (FILE_STREAM(channel) != NULL) {
				(void)fclose(FILE_STREAM(channel));
			}
		}

		isc_mem_free(mctx, channel->name);
		isc_mem_put(mctx, channel, sizeof(*channel));
	}

	for (i = 0; i < lcfg->channellist_count; i++) {
		isc_logchannellist_t *item;
		while ((item = ISC_LIST_HEAD(lcfg->channellists[i])) != NULL) {
			ISC_LIST_UNLINK(lcfg->channellists[i], item, link);
			isc_mem_put(mctx, item, sizeof(*item));
		}
	}

	if (lcfg->channellist_count > 0) {
		isc_mem_put(mctx, lcfg->channellists,
			    lcfg->channellist_count *
				    sizeof(ISC_LIST(isc_logchannellist_t)));
	}

	lcfg->dynamic = false;
	if (lcfg->tag != NULL) {
		isc_mem_free(lcfg->lctx->mctx, lcfg->tag);
	}
	lcfg->tag = NULL;
	lcfg->highest_level = 0;
	lcfg->duplicate_interval = 0;
	lcfg->magic = 0;

	isc_mem_put(mctx, lcfg, sizeof(*lcfg));
}

void
isc_log_registercategories(isc_log_t *lctx, isc_logcategory_t categories[]) {
	isc_logcategory_t *catp;

	REQUIRE(VALID_CONTEXT(lctx));
	REQUIRE(categories != NULL && categories[0].name != NULL);

	/*
	 * XXXDCL This somewhat sleazy situation of using the last pointer
	 * in one category array to point to the next array exists because
	 * this registration function returns void and I didn't want to have
	 * change everything that used it by making it return an isc_result_t.
	 * It would need to do that if it had to allocate memory to store
	 * pointers to each array passed in.
	 */
	if (lctx->categories == NULL) {
		lctx->categories = categories;
	} else {
		/*
		 * Adjust the last (NULL) pointer of the already registered
		 * categories to point to the incoming array.
		 */
		for (catp = lctx->categories; catp->name != NULL;) {
			if (catp->id == UINT_MAX) {
				/*
				 * The name pointer points to the next array.
				 * Ick.
				 */
				DE_CONST(catp->name, catp);
			} else {
				catp++;
			}
		}

		catp->name = (void *)categories;
		catp->id = UINT_MAX;
	}

	/*
	 * Update the id number of the category with its new global id.
	 */
	for (catp = categories; catp->name != NULL; catp++) {
		catp->id = lctx->category_count++;
	}
}

isc_logcategory_t *
isc_log_categorybyname(isc_log_t *lctx, const char *name) {
	isc_logcategory_t *catp;

	REQUIRE(VALID_CONTEXT(lctx));
	REQUIRE(name != NULL);

	for (catp = lctx->categories; catp->name != NULL;) {
		if (catp->id == UINT_MAX) {
			/*
			 * catp is neither modified nor returned to the
			 * caller, so removing its const qualifier is ok.
			 */
			DE_CONST(catp->name, catp);
		} else {
			if (strcmp(catp->name, name) == 0) {
				return (catp);
			}
			catp++;
		}
	}

	return (NULL);
}

void
isc_log_registermodules(isc_log_t *lctx, isc_logmodule_t modules[]) {
	isc_logmodule_t *modp;

	REQUIRE(VALID_CONTEXT(lctx));
	REQUIRE(modules != NULL && modules[0].name != NULL);

	/*
	 * XXXDCL This somewhat sleazy situation of using the last pointer
	 * in one category array to point to the next array exists because
	 * this registration function returns void and I didn't want to have
	 * change everything that used it by making it return an isc_result_t.
	 * It would need to do that if it had to allocate memory to store
	 * pointers to each array passed in.
	 */
	if (lctx->modules == NULL) {
		lctx->modules = modules;
	} else {
		/*
		 * Adjust the last (NULL) pointer of the already registered
		 * modules to point to the incoming array.
		 */
		for (modp = lctx->modules; modp->name != NULL;) {
			if (modp->id == UINT_MAX) {
				/*
				 * The name pointer points to the next array.
				 * Ick.
				 */
				DE_CONST(modp->name, modp);
			} else {
				modp++;
			}
		}

		modp->name = (void *)modules;
		modp->id = UINT_MAX;
	}

	/*
	 * Update the id number of the module with its new global id.
	 */
	for (modp = modules; modp->name != NULL; modp++) {
		modp->id = lctx->module_count++;
	}
}

isc_logmodule_t *
isc_log_modulebyname(isc_log_t *lctx, const char *name) {
	isc_logmodule_t *modp;

	REQUIRE(VALID_CONTEXT(lctx));
	REQUIRE(name != NULL);

	for (modp = lctx->modules; modp->name != NULL;) {
		if (modp->id == UINT_MAX) {
			/*
			 * modp is neither modified nor returned to the
			 * caller, so removing its const qualifier is ok.
			 */
			DE_CONST(modp->name, modp);
		} else {
			if (strcmp(modp->name, name) == 0) {
				return (modp);
			}
			modp++;
		}
	}

	return (NULL);
}

void
isc_log_createchannel(isc_logconfig_t *lcfg, const char *name,
		      unsigned int type, int level,
		      const isc_logdestination_t *destination,
		      unsigned int flags) {
	isc_logchannel_t *channel;
	isc_mem_t *mctx;
	unsigned int permitted = ISC_LOG_PRINTALL | ISC_LOG_DEBUGONLY |
				 ISC_LOG_BUFFERED | ISC_LOG_ISO8601 |
				 ISC_LOG_UTC;

	REQUIRE(VALID_CONFIG(lcfg));
	REQUIRE(name != NULL);
	REQUIRE(type == ISC_LOG_TOSYSLOG || type == ISC_LOG_TOFILE ||
		type == ISC_LOG_TOFILEDESC || type == ISC_LOG_TONULL);
	REQUIRE(destination != NULL || type == ISC_LOG_TONULL);
	REQUIRE(level >= ISC_LOG_CRITICAL);
	REQUIRE((flags & ~permitted) == 0);

	/* XXXDCL find duplicate names? */

	mctx = lcfg->lctx->mctx;

	channel = isc_mem_get(mctx, sizeof(*channel));

	channel->name = isc_mem_strdup(mctx, name);

	channel->type = type;
	channel->level = level;
	channel->flags = flags;
	ISC_LINK_INIT(channel, link);

	switch (type) {
	case ISC_LOG_TOSYSLOG:
		FACILITY(channel) = destination->facility;
		break;

	case ISC_LOG_TOFILE:
		/*
		 * The file name is copied because greatest_version wants
		 * to scribble on it, so it needs to be definitely in
		 * writable memory.
		 */
		FILE_NAME(channel) = isc_mem_strdup(mctx,
						    destination->file.name);
		FILE_STREAM(channel) = NULL;
		FILE_VERSIONS(channel) = destination->file.versions;
		FILE_SUFFIX(channel) = destination->file.suffix;
		FILE_MAXSIZE(channel) = destination->file.maximum_size;
		FILE_MAXREACHED(channel) = false;
		break;

	case ISC_LOG_TOFILEDESC:
		FILE_NAME(channel) = NULL;
		FILE_STREAM(channel) = destination->file.stream;
		FILE_MAXSIZE(channel) = 0;
		FILE_VERSIONS(channel) = ISC_LOG_ROLLNEVER;
		FILE_SUFFIX(channel) = isc_log_rollsuffix_increment;
		break;

	case ISC_LOG_TONULL:
		/* Nothing. */
		break;

	default:
		UNREACHABLE();
	}

	ISC_LIST_PREPEND(lcfg->channels, channel, link);

	/*
	 * If default_stderr was redefined, make the default category
	 * point to the new default_stderr.
	 */
	if (strcmp(name, "default_stderr") == 0) {
		default_channel.channel = channel;
	}
}

isc_result_t
isc_log_usechannel(isc_logconfig_t *lcfg, const char *name,
		   const isc_logcategory_t *category,
		   const isc_logmodule_t *module) {
	isc_log_t *lctx;
	isc_logchannel_t *channel;

	REQUIRE(VALID_CONFIG(lcfg));
	REQUIRE(name != NULL);

	lctx = lcfg->lctx;

	REQUIRE(category == NULL || category->id < lctx->category_count);
	REQUIRE(module == NULL || module->id < lctx->module_count);

	for (channel = ISC_LIST_HEAD(lcfg->channels); channel != NULL;
	     channel = ISC_LIST_NEXT(channel, link))
	{
		if (strcmp(name, channel->name) == 0) {
			break;
		}
	}

	if (channel == NULL) {
		return (ISC_R_NOTFOUND);
	}

	if (category != NULL) {
		assignchannel(lcfg, category->id, module, channel);
	} else {
		/*
		 * Assign to all categories.  Note that this includes
		 * the default channel.
		 */
		for (size_t i = 0; i < lctx->category_count; i++) {
			assignchannel(lcfg, i, module, channel);
		}
	}

	/*
	 * Update the highest logging level, if the current lcfg is in use.
	 */
	if (lcfg->lctx->logconfig == lcfg) {
		sync_highest_level(lctx, lcfg);
	}

	return (ISC_R_SUCCESS);
}

void
isc_log_write(isc_log_t *lctx, isc_logcategory_t *category,
	      isc_logmodule_t *module, int level, const char *format, ...) {
	va_list args;

	/*
	 * Contract checking is done in isc_log_doit().
	 */

	va_start(args, format);
	isc_log_doit(lctx, category, module, level, false, format, args);
	va_end(args);
}

void
isc_log_vwrite(isc_log_t *lctx, isc_logcategory_t *category,
	       isc_logmodule_t *module, int level, const char *format,
	       va_list args) {
	/*
	 * Contract checking is done in isc_log_doit().
	 */
	isc_log_doit(lctx, category, module, level, false, format, args);
}

void
isc_log_write1(isc_log_t *lctx, isc_logcategory_t *category,
	       isc_logmodule_t *module, int level, const char *format, ...) {
	va_list args;

	/*
	 * Contract checking is done in isc_log_doit().
	 */

	va_start(args, format);
	isc_log_doit(lctx, category, module, level, true, format, args);
	va_end(args);
}

void
isc_log_vwrite1(isc_log_t *lctx, isc_logcategory_t *category,
		isc_logmodule_t *module, int level, const char *format,
		va_list args) {
	/*
	 * Contract checking is done in isc_log_doit().
	 */
	isc_log_doit(lctx, category, module, level, true, format, args);
}

void
isc_log_setcontext(isc_log_t *lctx) {
	isc_lctx = lctx;
}

void
isc_log_setdebuglevel(isc_log_t *lctx, unsigned int level) {
	REQUIRE(VALID_CONTEXT(lctx));

	atomic_store_release(&lctx->debug_level, level);
	/*
	 * Close ISC_LOG_DEBUGONLY channels if level is zero.
	 */
	if (level == 0) {
		RDLOCK(&lctx->lcfg_rwl);
		isc_logconfig_t *lcfg = lctx->logconfig;
		if (lcfg != NULL) {
			LOCK(&lctx->lock);
			for (isc_logchannel_t *channel =
				     ISC_LIST_HEAD(lcfg->channels);
			     channel != NULL;
			     channel = ISC_LIST_NEXT(channel, link))
			{
				if (channel->type == ISC_LOG_TOFILE &&
				    (channel->flags & ISC_LOG_DEBUGONLY) != 0 &&
				    FILE_STREAM(channel) != NULL)
				{
					(void)fclose(FILE_STREAM(channel));
					FILE_STREAM(channel) = NULL;
				}
			}
			UNLOCK(&lctx->lock);
		}
		RDUNLOCK(&lctx->lcfg_rwl);
	}
}

unsigned int
isc_log_getdebuglevel(isc_log_t *lctx) {
	REQUIRE(VALID_CONTEXT(lctx));

	return (atomic_load_acquire(&lctx->debug_level));
}

void
isc_log_setduplicateinterval(isc_logconfig_t *lcfg, unsigned int interval) {
	REQUIRE(VALID_CONFIG(lcfg));

	lcfg->duplicate_interval = interval;
}

unsigned int
isc_log_getduplicateinterval(isc_logconfig_t *lcfg) {
	REQUIRE(VALID_CONTEXT(lcfg));

	return (lcfg->duplicate_interval);
}

void
isc_log_settag(isc_logconfig_t *lcfg, const char *tag) {
	REQUIRE(VALID_CONFIG(lcfg));

	if (tag != NULL && *tag != '\0') {
		if (lcfg->tag != NULL) {
			isc_mem_free(lcfg->lctx->mctx, lcfg->tag);
		}
		lcfg->tag = isc_mem_strdup(lcfg->lctx->mctx, tag);
	} else {
		if (lcfg->tag != NULL) {
			isc_mem_free(lcfg->lctx->mctx, lcfg->tag);
		}
		lcfg->tag = NULL;
	}
}

char *
isc_log_gettag(isc_logconfig_t *lcfg) {
	REQUIRE(VALID_CONFIG(lcfg));

	return (lcfg->tag);
}

/* XXXDCL NT  -- This interface will assuredly be changing. */
void
isc_log_opensyslog(const char *tag, int options, int facility) {
	(void)openlog(tag, options, facility);
}

void
isc_log_closefilelogs(isc_log_t *lctx) {
	REQUIRE(VALID_CONTEXT(lctx));

	RDLOCK(&lctx->lcfg_rwl);
	isc_logconfig_t *lcfg = lctx->logconfig;
	if (lcfg != NULL) {
		LOCK(&lctx->lock);
		for (isc_logchannel_t *channel = ISC_LIST_HEAD(lcfg->channels);
		     channel != NULL; channel = ISC_LIST_NEXT(channel, link))
		{
			if (channel->type == ISC_LOG_TOFILE &&
			    FILE_STREAM(channel) != NULL)
			{
				(void)fclose(FILE_STREAM(channel));
				FILE_STREAM(channel) = NULL;
			}
		}
		UNLOCK(&lctx->lock);
	}
	RDUNLOCK(&lctx->lcfg_rwl);
}

/****
**** Internal functions
****/

static void
assignchannel(isc_logconfig_t *lcfg, unsigned int category_id,
	      const isc_logmodule_t *module, isc_logchannel_t *channel) {
	isc_logchannellist_t *new_item;
	isc_log_t *lctx;

	REQUIRE(VALID_CONFIG(lcfg));

	lctx = lcfg->lctx;

	REQUIRE(category_id < lctx->category_count);
	REQUIRE(module == NULL || module->id < lctx->module_count);
	REQUIRE(channel != NULL);

	/*
	 * Ensure lcfg->channellist_count == lctx->category_count.
	 */
	sync_channellist(lcfg);

	new_item = isc_mem_get(lctx->mctx, sizeof(*new_item));

	new_item->channel = channel;
	new_item->module = module;
	ISC_LIST_INITANDPREPEND(lcfg->channellists[category_id], new_item,
				link);

	/*
	 * Remember the highest logging level set by any channel in the
	 * logging config, so isc_log_doit() can quickly return if the
	 * message is too high to be logged by any channel.
	 */
	if (channel->type != ISC_LOG_TONULL) {
		if (lcfg->highest_level < channel->level) {
			lcfg->highest_level = channel->level;
		}
		if (channel->level == ISC_LOG_DYNAMIC) {
			lcfg->dynamic = true;
		}
	}
}

/*
 * This would ideally be part of isc_log_registercategories(), except then
 * that function would have to return isc_result_t instead of void.
 */
static void
sync_channellist(isc_logconfig_t *lcfg) {
	unsigned int bytes;
	isc_log_t *lctx;
	void *lists;

	REQUIRE(VALID_CONFIG(lcfg));

	lctx = lcfg->lctx;

	REQUIRE(lctx->category_count != 0);

	if (lctx->category_count == lcfg->channellist_count) {
		return;
	}

	bytes = lctx->category_count * sizeof(ISC_LIST(isc_logchannellist_t));

	lists = isc_mem_get(lctx->mctx, bytes);

	memset(lists, 0, bytes);

	if (lcfg->channellist_count != 0) {
		bytes = lcfg->channellist_count *
			sizeof(ISC_LIST(isc_logchannellist_t));
		memmove(lists, lcfg->channellists, bytes);
		isc_mem_put(lctx->mctx, lcfg->channellists, bytes);
	}

	lcfg->channellists = lists;
	lcfg->channellist_count = lctx->category_count;
}

static void
sync_highest_level(isc_log_t *lctx, isc_logconfig_t *lcfg) {
	atomic_store(&lctx->highest_level, lcfg->highest_level);
	atomic_store(&lctx->dynamic, lcfg->dynamic);
}

static isc_result_t
greatest_version(isc_logfile_t *file, int versions, int *greatestp) {
	char *bname, *digit_end;
	const char *dirname;
	int version, greatest = -1;
	size_t bnamelen;
	isc_dir_t dir;
	isc_result_t result;
	char sep = '/';
#ifdef _WIN32
	char *bname2;
#endif /* ifdef _WIN32 */

	/*
	 * It is safe to DE_CONST the file.name because it was copied
	 * with isc_mem_strdup().
	 */
	bname = strrchr(file->name, sep);
#ifdef _WIN32
	bname2 = strrchr(file->name, '\\');
	if ((bname != NULL && bname2 != NULL && bname2 > bname) ||
	    (bname == NULL && bname2 != NULL))
	{
		bname = bname2;
		sep = '\\';
	}
#endif /* ifdef _WIN32 */
	if (bname != NULL) {
		*bname++ = '\0';
		dirname = file->name;
	} else {
		DE_CONST(file->name, bname);
		dirname = ".";
	}
	bnamelen = strlen(bname);

	isc_dir_init(&dir);
	result = isc_dir_open(&dir, dirname);

	/*
	 * Replace the file separator if it was taken out.
	 */
	if (bname != file->name) {
		*(bname - 1) = sep;
	}

	/*
	 * Return if the directory open failed.
	 */
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	while (isc_dir_read(&dir) == ISC_R_SUCCESS) {
		if (dir.entry.length > bnamelen &&
		    strncmp(dir.entry.name, bname, bnamelen) == 0 &&
		    dir.entry.name[bnamelen] == '.')
		{
			version = strtol(&dir.entry.name[bnamelen + 1],
					 &digit_end, 10);
			/*
			 * Remove any backup files that exceed versions.
			 */
			if (*digit_end == '\0' && version >= versions) {
				result = isc_file_remove(dir.entry.name);
				if (result != ISC_R_SUCCESS &&
				    result != ISC_R_FILENOTFOUND)
				{
					syslog(LOG_ERR,
					       "unable to remove "
					       "log file '%s': %s",
					       dir.entry.name,
					       isc_result_totext(result));
				}
			} else if (*digit_end == '\0' && version > greatest) {
				greatest = version;
			}
		}
	}
	isc_dir_close(&dir);

	*greatestp = greatest;

	return (ISC_R_SUCCESS);
}

static void
insert_sort(int64_t to_keep[], int64_t versions, int version) {
	int i = 0;
	while (i < versions && version < to_keep[i]) {
		i++;
	}
	if (i == versions) {
		return;
	}
	if (i < versions - 1) {
		memmove(&to_keep[i + 1], &to_keep[i],
			sizeof(to_keep[0]) * (versions - i - 1));
	}
	to_keep[i] = version;
}

static int64_t
last_to_keep(int64_t versions, isc_dir_t *dirp, char *bname, size_t bnamelen) {
	if (versions <= 0) {
		return INT64_MAX;
	}

	int64_t to_keep[ISC_LOG_MAX_VERSIONS] = { 0 };
	int64_t version = 0;
	if (versions > ISC_LOG_MAX_VERSIONS) {
		versions = ISC_LOG_MAX_VERSIONS;
	}
	/*
	 * First we fill 'to_keep' structure using insertion sort
	 */
	memset(to_keep, 0, sizeof(to_keep));
	while (isc_dir_read(dirp) == ISC_R_SUCCESS) {
		if (dirp->entry.length <= bnamelen ||
		    strncmp(dirp->entry.name, bname, bnamelen) != 0 ||
		    dirp->entry.name[bnamelen] != '.')
		{
			continue;
		}

		char *digit_end;
		char *ename = &dirp->entry.name[bnamelen + 1];
		version = strtoull(ename, &digit_end, 10);
		if (*digit_end == '\0') {
			insert_sort(to_keep, versions, version);
		}
	}

	isc_dir_reset(dirp);

	/*
	 * to_keep[versions - 1] is the last one we want to keep
	 */
	return (to_keep[versions - 1]);
}

static isc_result_t
remove_old_tsversions(isc_logfile_t *file, int versions) {
	isc_result_t result;
	char *bname, *digit_end;
	const char *dirname;
	int64_t version, last = INT64_MAX;
	size_t bnamelen;
	isc_dir_t dir;
	char sep = '/';
#ifdef _WIN32
	char *bname2;
#endif /* ifdef _WIN32 */
	/*
	 * It is safe to DE_CONST the file.name because it was copied
	 * with isc_mem_strdup().
	 */
	bname = strrchr(file->name, sep);
#ifdef _WIN32
	bname2 = strrchr(file->name, '\\');
	if ((bname != NULL && bname2 != NULL && bname2 > bname) ||
	    (bname == NULL && bname2 != NULL))
	{
		bname = bname2;
		sep = '\\';
	}
#endif /* ifdef _WIN32 */
	if (bname != NULL) {
		*bname++ = '\0';
		dirname = file->name;
	} else {
		DE_CONST(file->name, bname);
		dirname = ".";
	}
	bnamelen = strlen(bname);

	isc_dir_init(&dir);
	result = isc_dir_open(&dir, dirname);

	/*
	 * Replace the file separator if it was taken out.
	 */
	if (bname != file->name) {
		*(bname - 1) = sep;
	}

	/*
	 * Return if the directory open failed.
	 */
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	last = last_to_keep(versions, &dir, bname, bnamelen);

	/*
	 * Then we remove all files that we don't want to_keep
	 */
	while (isc_dir_read(&dir) == ISC_R_SUCCESS) {
		if (dir.entry.length > bnamelen &&
		    strncmp(dir.entry.name, bname, bnamelen) == 0 &&
		    dir.entry.name[bnamelen] == '.')
		{
			char *ename = &dir.entry.name[bnamelen + 1];
			version = strtoull(ename, &digit_end, 10);
			/*
			 * Remove any backup files that exceed versions.
			 */
			if (*digit_end == '\0' && version < last) {
				result = isc_file_remove(dir.entry.name);
				if (result != ISC_R_SUCCESS &&
				    result != ISC_R_FILENOTFOUND)
				{
					syslog(LOG_ERR,
					       "unable to remove "
					       "log file '%s': %s",
					       dir.entry.name,
					       isc_result_totext(result));
				}
			}
		}
	}

	isc_dir_close(&dir);

	return (ISC_R_SUCCESS);
}

static isc_result_t
roll_increment(isc_logfile_t *file) {
	int i, n, greatest;
	char current[PATH_MAX + 1];
	char newpath[PATH_MAX + 1];
	const char *path;
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(file != NULL);
	REQUIRE(file->versions != 0);

	path = file->name;

	if (file->versions == ISC_LOG_ROLLINFINITE) {
		/*
		 * Find the first missing entry in the log file sequence.
		 */
		for (greatest = 0; greatest < INT_MAX; greatest++) {
			n = snprintf(current, sizeof(current), "%s.%u", path,
				     (unsigned)greatest);
			if (n >= (int)sizeof(current) || n < 0 ||
			    !isc_file_exists(current))
			{
				break;
			}
		}
	} else {
		/*
		 * Get the largest existing version and remove any
		 * version greater than the permitted version.
		 */
		result = greatest_version(file, file->versions, &greatest);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}

		/*
		 * Increment if greatest is not the actual maximum value.
		 */
		if (greatest < file->versions - 1) {
			greatest++;
		}
	}

	for (i = greatest; i > 0; i--) {
		result = ISC_R_SUCCESS;
		n = snprintf(current, sizeof(current), "%s.%u", path,
			     (unsigned)(i - 1));
		if (n >= (int)sizeof(current) || n < 0) {
			result = ISC_R_NOSPACE;
		}
		if (result == ISC_R_SUCCESS) {
			n = snprintf(newpath, sizeof(newpath), "%s.%u", path,
				     (unsigned)i);
			if (n >= (int)sizeof(newpath) || n < 0) {
				result = ISC_R_NOSPACE;
			}
		}
		if (result == ISC_R_SUCCESS) {
			result = isc_file_rename(current, newpath);
		}
		if (result != ISC_R_SUCCESS && result != ISC_R_FILENOTFOUND) {
			syslog(LOG_ERR,
			       "unable to rename log file '%s.%u' to "
			       "'%s.%u': %s",
			       path, i - 1, path, i, isc_result_totext(result));
		}
	}

	n = snprintf(newpath, sizeof(newpath), "%s.0", path);
	if (n >= (int)sizeof(newpath) || n < 0) {
		result = ISC_R_NOSPACE;
	} else {
		result = isc_file_rename(path, newpath);
	}
	if (result != ISC_R_SUCCESS && result != ISC_R_FILENOTFOUND) {
		syslog(LOG_ERR, "unable to rename log file '%s' to '%s.0': %s",
		       path, path, isc_result_totext(result));
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
roll_timestamp(isc_logfile_t *file) {
	int n;
	char newts[PATH_MAX + 1];
	char newpath[PATH_MAX + 1];
	const char *path;
	isc_time_t now;
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(file != NULL);
	REQUIRE(file->versions != 0);

	path = file->name;

	/*
	 * First find all the logfiles and remove the oldest ones
	 * Save one fewer than file->versions because we'll be renaming
	 * the existing file to a timestamped version after this.
	 */
	if (file->versions != ISC_LOG_ROLLINFINITE) {
		remove_old_tsversions(file, file->versions - 1);
	}

	/* Then just rename the current logfile */
	isc_time_now(&now);
	isc_time_formatshorttimestamp(&now, newts, PATH_MAX + 1);
	n = snprintf(newpath, sizeof(newpath), "%s.%s", path, newts);
	if (n >= (int)sizeof(newpath) || n < 0) {
		result = ISC_R_NOSPACE;
	} else {
		result = isc_file_rename(path, newpath);
	}
	if (result != ISC_R_SUCCESS && result != ISC_R_FILENOTFOUND) {
		syslog(LOG_ERR, "unable to rename log file '%s' to '%s.0': %s",
		       path, path, isc_result_totext(result));
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_logfile_roll(isc_logfile_t *file) {
	isc_result_t result;

	REQUIRE(file != NULL);

	/*
	 * Do nothing (not even excess version trimming) if ISC_LOG_ROLLNEVER
	 * is specified.  Apparently complete external control over the log
	 * files is desired.
	 */
	if (file->versions == ISC_LOG_ROLLNEVER) {
		return (ISC_R_SUCCESS);
	} else if (file->versions == 0) {
		result = isc_file_remove(file->name);
		if (result != ISC_R_SUCCESS && result != ISC_R_FILENOTFOUND) {
			syslog(LOG_ERR, "unable to remove log file '%s': %s",
			       file->name, isc_result_totext(result));
		}
		return (ISC_R_SUCCESS);
	}

	switch (file->suffix) {
	case isc_log_rollsuffix_increment:
		return (roll_increment(file));
	case isc_log_rollsuffix_timestamp:
		return (roll_timestamp(file));
	default:
		return (ISC_R_UNEXPECTED);
	}
}

static isc_result_t
isc_log_open(isc_logchannel_t *channel) {
	struct stat statbuf;
	bool regular_file;
	bool roll = false;
	isc_result_t result = ISC_R_SUCCESS;
	const char *path;

	REQUIRE(channel->type == ISC_LOG_TOFILE);
	REQUIRE(FILE_STREAM(channel) == NULL);

	path = FILE_NAME(channel);

	REQUIRE(path != NULL && *path != '\0');

	/*
	 * Determine type of file; only regular files will be
	 * version renamed, and only if the base file exists
	 * and either has no size limit or has reached its size limit.
	 */
	if (stat(path, &statbuf) == 0) {
		regular_file = S_ISREG(statbuf.st_mode) ? true : false;
		/* XXXDCL if not regular_file complain? */
		if ((FILE_MAXSIZE(channel) == 0 &&
		     FILE_VERSIONS(channel) != ISC_LOG_ROLLNEVER) ||
		    (FILE_MAXSIZE(channel) > 0 &&
		     statbuf.st_size >= FILE_MAXSIZE(channel)))
		{
			roll = regular_file;
		}
	} else if (errno == ENOENT) {
		regular_file = true;
		POST(regular_file);
	} else {
		result = ISC_R_INVALIDFILE;
	}

	/*
	 * Version control.
	 */
	if (result == ISC_R_SUCCESS && roll) {
		if (FILE_VERSIONS(channel) == ISC_LOG_ROLLNEVER) {
			return (ISC_R_MAXSIZE);
		}
		result = isc_logfile_roll(&channel->destination.file);
		if (result != ISC_R_SUCCESS) {
			if ((channel->flags & ISC_LOG_OPENERR) == 0) {
				syslog(LOG_ERR,
				       "isc_log_open: isc_logfile_roll '%s' "
				       "failed: %s",
				       FILE_NAME(channel),
				       isc_result_totext(result));
				channel->flags |= ISC_LOG_OPENERR;
			}
			return (result);
		}
	}

	result = isc_stdio_open(path, "a", &FILE_STREAM(channel));

	return (result);
}

ISC_NO_SANITIZE_THREAD bool
isc_log_wouldlog(isc_log_t *lctx, int level) {
	/*
	 * Try to avoid locking the mutex for messages which can't
	 * possibly be logged to any channels -- primarily debugging
	 * messages that the debug level is not high enough to print.
	 *
	 * If the level is (mathematically) less than or equal to the
	 * highest_level, or if there is a dynamic channel and the level is
	 * less than or equal to the debug level, the main loop must be
	 * entered to see if the message should really be output.
	 */
	if (lctx == NULL) {
		return (false);
	}

	int highest_level = atomic_load_acquire(&lctx->highest_level);
	if (level <= highest_level) {
		return (true);
	}
	if (atomic_load_acquire(&lctx->dynamic)) {
		int debug_level = atomic_load_acquire(&lctx->debug_level);
		if (level <= debug_level) {
			return (true);
		}
	}

	return (false);
}

static void
isc_log_doit(isc_log_t *lctx, isc_logcategory_t *category,
	     isc_logmodule_t *module, int level, bool write_once,
	     const char *format, va_list args) {
	int syslog_level;
	const char *time_string;
	char local_time[64];
	char iso8601z_string[64];
	char iso8601l_string[64];
	char level_string[24] = { 0 };
	struct stat statbuf;
	bool matched = false;
	bool printtime, iso8601, utc, printtag, printcolon;
	bool printcategory, printmodule, printlevel, buffered;
	isc_logchannel_t *channel;
	isc_logchannellist_t *category_channels;
	isc_result_t result;

	REQUIRE(lctx == NULL || VALID_CONTEXT(lctx));
	REQUIRE(category != NULL);
	REQUIRE(module != NULL);
	REQUIRE(level != ISC_LOG_DYNAMIC);
	REQUIRE(format != NULL);

	/*
	 * Programs can use libraries that use this logging code without
	 * wanting to do any logging, thus the log context is allowed to
	 * be non-existent.
	 */
	if (lctx == NULL) {
		return;
	}

	REQUIRE(category->id < lctx->category_count);
	REQUIRE(module->id < lctx->module_count);

	if (!isc_log_wouldlog(lctx, level)) {
		return;
	}

	local_time[0] = '\0';
	iso8601l_string[0] = '\0';
	iso8601z_string[0] = '\0';

	RDLOCK(&lctx->lcfg_rwl);
	LOCK(&lctx->lock);

	lctx->buffer[0] = '\0';

	isc_logconfig_t *lcfg = lctx->logconfig;

	category_channels = ISC_LIST_HEAD(lcfg->channellists[category->id]);

	/*
	 * XXXDCL add duplicate filtering? (To not write multiple times
	 * to the same source via various channels).
	 */
	do {
		/*
		 * If the channel list end was reached and a match was
		 * made, everything is finished.
		 */
		if (category_channels == NULL && matched) {
			break;
		}

		if (category_channels == NULL && !matched &&
		    category_channels != ISC_LIST_HEAD(lcfg->channellists[0]))
		{
			/*
			 * No category/module pair was explicitly
			 * configured. Try the category named "default".
			 */
			category_channels =
				ISC_LIST_HEAD(lcfg->channellists[0]);
		}

		if (category_channels == NULL && !matched) {
			/*
			 * No matching module was explicitly configured
			 * for the category named "default".  Use the
			 * internal default channel.
			 */
			category_channels = &default_channel;
		}

		if (category_channels->module != NULL &&
		    category_channels->module != module)
		{
			category_channels = ISC_LIST_NEXT(category_channels,
							  link);
			continue;
		}

		matched = true;

		channel = category_channels->channel;
		category_channels = ISC_LIST_NEXT(category_channels, link);

		int_fast32_t dlevel = atomic_load_acquire(&lctx->debug_level);
		if (((channel->flags & ISC_LOG_DEBUGONLY) != 0) && dlevel == 0)
		{
			continue;
		}

		if (channel->level == ISC_LOG_DYNAMIC) {
			if (dlevel < level) {
				continue;
			}
		} else if (channel->level < level) {
			continue;
		}

		if ((channel->flags & ISC_LOG_PRINTTIME) != 0 &&
		    local_time[0] == '\0')
		{
			isc_time_t isctime;

			TIME_NOW(&isctime);

			isc_time_formattimestamp(&isctime, local_time,
						 sizeof(local_time));
			isc_time_formatISO8601ms(&isctime, iso8601z_string,
						 sizeof(iso8601z_string));
			isc_time_formatISO8601Lms(&isctime, iso8601l_string,
						  sizeof(iso8601l_string));
		}

		if ((channel->flags & ISC_LOG_PRINTLEVEL) != 0 &&
		    level_string[0] == '\0')
		{
			if (level < ISC_LOG_CRITICAL) {
				snprintf(level_string, sizeof(level_string),
					 "level %d: ", level);
			} else if (level > ISC_LOG_DYNAMIC) {
				snprintf(level_string, sizeof(level_string),
					 "%s %d: ", log_level_strings[0],
					 level);
			} else {
				snprintf(level_string, sizeof(level_string),
					 "%s: ", log_level_strings[-level]);
			}
		}

		/*
		 * Only format the message once.
		 */
		if (lctx->buffer[0] == '\0') {
			(void)vsnprintf(lctx->buffer, sizeof(lctx->buffer),
					format, args);

			/*
			 * Check for duplicates.
			 */
			if (write_once) {
				isc_logmessage_t *message, *next;
				isc_time_t oldest;
				isc_interval_t interval;
				size_t size;

				isc_interval_set(&interval,
						 lcfg->duplicate_interval, 0);

				/*
				 * 'oldest' is the age of the oldest
				 * messages which fall within the
				 * duplicate_interval range.
				 */
				TIME_NOW(&oldest);
				if (isc_time_subtract(&oldest, &interval,
						      &oldest) != ISC_R_SUCCESS)
				{
					/*
					 * Can't effectively do the
					 * checking without having a
					 * valid time.
					 */
					message = NULL;
				} else {
					message = ISC_LIST_HEAD(lctx->messages);
				}

				while (message != NULL) {
					if (isc_time_compare(&message->time,
							     &oldest) < 0)
					{
						/*
						 * This message is older
						 * than the
						 * duplicate_interval,
						 * so it should be
						 * dropped from the
						 * history.
						 *
						 * Setting the interval
						 * to be to be longer
						 * will obviously not
						 * cause the expired
						 * message to spring
						 * back into existence.
						 */
						next = ISC_LIST_NEXT(message,
								     link);

						ISC_LIST_UNLINK(lctx->messages,
								message, link);

						isc_mem_put(
							lctx->mctx, message,
							sizeof(*message) + 1 +
								strlen(message->text));

						message = next;
						continue;
					}

					/*
					 * This message is in the
					 * duplicate filtering interval
					 * ...
					 */
					if (strcmp(lctx->buffer,
						   message->text) == 0)
					{
						/*
						 * ... and it is a
						 * duplicate. Unlock the
						 * mutex and get the
						 * hell out of Dodge.
						 */
						goto unlock;
					}

					message = ISC_LIST_NEXT(message, link);
				}

				/*
				 * It wasn't in the duplicate interval,
				 * so add it to the message list.
				 */
				size = sizeof(isc_logmessage_t) +
				       strlen(lctx->buffer) + 1;
				message = isc_mem_get(lctx->mctx, size);
				message->text = (char *)(message + 1);
				size -= sizeof(isc_logmessage_t);
				strlcpy(message->text, lctx->buffer, size);
				TIME_NOW(&message->time);
				ISC_LINK_INIT(message, link);
				ISC_LIST_APPEND(lctx->messages, message, link);
			}
		}

		utc = ((channel->flags & ISC_LOG_UTC) != 0);
		iso8601 = ((channel->flags & ISC_LOG_ISO8601) != 0);
		printtime = ((channel->flags & ISC_LOG_PRINTTIME) != 0);
		printtag = ((channel->flags &
			     (ISC_LOG_PRINTTAG | ISC_LOG_PRINTPREFIX)) != 0 &&
			    lcfg->tag != NULL);
		printcolon = ((channel->flags & ISC_LOG_PRINTTAG) != 0 &&
			      lcfg->tag != NULL);
		printcategory = ((channel->flags & ISC_LOG_PRINTCATEGORY) != 0);
		printmodule = ((channel->flags & ISC_LOG_PRINTMODULE) != 0);
		printlevel = ((channel->flags & ISC_LOG_PRINTLEVEL) != 0);
		buffered = ((channel->flags & ISC_LOG_BUFFERED) != 0);

		if (printtime) {
			if (iso8601) {
				if (utc) {
					time_string = iso8601z_string;
				} else {
					time_string = iso8601l_string;
				}
			} else {
				time_string = local_time;
			}
		} else {
			time_string = "";
		}

		switch (channel->type) {
		case ISC_LOG_TOFILE:
			if (FILE_MAXREACHED(channel)) {
				/*
				 * If the file can be rolled, OR
				 * If the file no longer exists, OR
				 * If the file is less than the maximum
				 * size, (such as if it had been renamed
				 * and a new one touched, or it was
				 * truncated in place)
				 * ... then close it to trigger
				 * reopening.
				 */
				if (FILE_VERSIONS(channel) !=
					    ISC_LOG_ROLLNEVER ||
				    (stat(FILE_NAME(channel), &statbuf) != 0 &&
				     errno == ENOENT) ||
				    statbuf.st_size < FILE_MAXSIZE(channel))
				{
					(void)fclose(FILE_STREAM(channel));
					FILE_STREAM(channel) = NULL;
					FILE_MAXREACHED(channel) = false;
				} else {
					/*
					 * Eh, skip it.
					 */
					break;
				}
			}

			if (FILE_STREAM(channel) == NULL) {
				result = isc_log_open(channel);
				if (result != ISC_R_SUCCESS &&
				    result != ISC_R_MAXSIZE &&
				    (channel->flags & ISC_LOG_OPENERR) == 0)
				{
					syslog(LOG_ERR,
					       "isc_log_open '%s' "
					       "failed: %s",
					       FILE_NAME(channel),
					       isc_result_totext(result));
					channel->flags |= ISC_LOG_OPENERR;
				}
				if (result != ISC_R_SUCCESS) {
					break;
				}
				channel->flags &= ~ISC_LOG_OPENERR;
			}
			FALLTHROUGH;

		case ISC_LOG_TOFILEDESC:
			fprintf(FILE_STREAM(channel), "%s%s%s%s%s%s%s%s%s%s\n",
				printtime ? time_string : "",
				printtime ? " " : "", printtag ? lcfg->tag : "",
				printcolon ? ": " : "",
				printcategory ? category->name : "",
				printcategory ? ": " : "",
				printmodule ? (module != NULL ? module->name
							      : "no_module")
					    : "",
				printmodule ? ": " : "",
				printlevel ? level_string : "", lctx->buffer);

			if (!buffered) {
				fflush(FILE_STREAM(channel));
			}

			/*
			 * If the file now exceeds its maximum size
			 * threshold, note it so that it will not be
			 * logged to any more.
			 */
			if (FILE_MAXSIZE(channel) > 0) {
				INSIST(channel->type == ISC_LOG_TOFILE);

				/* XXXDCL NT fstat/fileno */
				/* XXXDCL complain if fstat fails? */
				if (fstat(fileno(FILE_STREAM(channel)),
					  &statbuf) >= 0 &&
				    statbuf.st_size > FILE_MAXSIZE(channel))
				{
					FILE_MAXREACHED(channel) = true;
				}
			}

			break;

		case ISC_LOG_TOSYSLOG:
			if (level > 0) {
				syslog_level = LOG_DEBUG;
			} else if (level < ISC_LOG_CRITICAL) {
				syslog_level = LOG_CRIT;
			} else {
				syslog_level = syslog_map[-level];
			}

			(void)syslog(
				FACILITY(channel) | syslog_level,
				"%s%s%s%s%s%s%s%s%s%s",
				printtime ? time_string : "",
				printtime ? " " : "", printtag ? lcfg->tag : "",
				printcolon ? ": " : "",
				printcategory ? category->name : "",
				printcategory ? ": " : "",
				printmodule ? (module != NULL ? module->name
							      : "no_module")
					    : "",
				printmodule ? ": " : "",
				printlevel ? level_string : "", lctx->buffer);
			break;

		case ISC_LOG_TONULL:
			break;
		}
	} while (1);

unlock:
	UNLOCK(&lctx->lock);
	RDUNLOCK(&lctx->lcfg_rwl);
}
