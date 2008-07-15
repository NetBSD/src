/*
 * Copyright (C) 2005-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of the device-mapper userspace tools.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <dlfcn.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/param.h>
#include <sys/select.h>
#include "list.h"
#include "libmultilog.h"
#include "multilog_internal.h"

#include <unistd.h>

#define DEFAULT_VERBOSITY 4

struct threaded_log {
	pthread_t thread;
	void *dlh;
	int (*start_log) (pthread_t *t, long usecs);
	int (*stop_log) (pthread_t t);
	void (*log) (int priority, const char *file,
		     int line, const char *string);
	int enabled;
};


struct custom_log {
	void (*destructor)(void *data);
	void *custom;
};

union log_info {
	FILE *logfile;
	struct custom_log cl;
};

struct log_data {
	int verbose_level;
	union log_info info;
};

struct log_list {
	struct list list;
	enum log_type type;
	multilog_fn log;
	struct log_data data;
};

/* FIXME: probably shouldn't do it this way, but... */
static LIST_INIT(logs);

static struct threaded_log tl;

/* locking for log accesss */
static void* (*init_lock_fn)(void) = NULL;
static int (*lock_fn)(void *) = NULL;
static int (*unlock_fn)(void *) = NULL;
static void (*destroy_lock_fn)(void *) = NULL;
void *lock_dlh = NULL;
void *lock_handle = NULL;

static void *init_lock(void)
{
	return init_lock_fn ? init_lock_fn() : NULL;
}

static int lock_list(void *handle)
{
	return lock_fn ? !(*lock_fn)(handle) : 0;
}

static int unlock_list(void *handle)
{
	return unlock_fn ? !(*unlock_fn)(handle) : 0;
}

static void destroy_lock(void *handle)
{
	if (destroy_lock_fn)
		(*destroy_lock_fn)(handle);
}

static void *init_nop_lock(void)
{
	return NULL;
}

static int nop_lock(void *handle)
{
	return 0;
}

static int nop_unlock(void *handle)
{
	return 0;
}

static void destroy_nop_lock(void *handle)
{
	return;
}

static int load_lock_syms(void)
{
	void *dlh;

	if (!(dlh = dlopen("libmultilog_pthread_lock.so", RTLD_NOW))) {
		if (strstr(dlerror(), "undefined symbol: pthread")) {
			fprintf(stderr, "pthread library not linked in - "
				"using nop locking\n");
			init_lock_fn = init_nop_lock;
			lock_fn = nop_lock;
			unlock_fn = nop_unlock;
			destroy_lock_fn = destroy_nop_lock;
			return 1;
		} else
			return 0;
	}

	lock_dlh = dlh;

	return ((init_lock_fn    = dlsym(dlh, "init_locking")) &&
		(lock_fn         = dlsym(dlh, "lock_fn")) &&
		(unlock_fn       = dlsym(dlh, "unlock_fn")) &&
		(destroy_lock_fn = dlsym(dlh, "destroy_locking")));
}

/* Noop logging until the custom log fxn gets registered */
static void nop_log(void *data, int priority, const char *file, int line,
		    const char *string)
{
	return;
}

static void init_file_log(void *data, struct file_log *fld)
{
	if (!(((struct log_data *) data)->info.logfile =
	      fopen(fld->filename, fld->append ? "a" : "w")))
		log_sys_error("fopen", fld->filename);
}

static void file_log(void *data, int priority, const char *file, int line,
			 const char *string)
{
	fprintf(((struct log_data *) data)->info.logfile,
		"%s:%d %s\n", file, line, string);
}

static void destroy_file_log(void *data)
{
	fclose(((struct log_data *)data)->info.logfile);
}

static void init_sys_log(struct sys_log *sld)
{
	openlog(sld->ident, LOG_NDELAY, sld->facility);
}

static void sys_log(void *data, int priority, const char *file, int line,
			 const char *string)
{
	syslog(priority, "%s", string);
}

static void destroy_sys_log(void)
{
	closelog();
}

static void standard_log(void *data, int priority, const char *file, int line,
			 const char *string)
{
	struct log_data *ldata = (struct log_data *) data;
	/* FIXME: stack allocation of large buffer. */
	char locn[512];

	if (ldata->verbose_level > _LOG_DEBUG)
		snprintf(locn, sizeof(locn), "#%s:%d ", file, line);
	else
		*locn = '\0';

	switch (ldata->verbose_level) {
	case _LOG_DEBUG:
		if (strcmp("<backtrace>", string) &&
		    ldata->verbose_level >= _LOG_DEBUG)
			fprintf(stderr, "%s%s\n", locn, string);

		break;
	case _LOG_INFO:
		if (ldata->verbose_level >= _LOG_INFO)
			fprintf(stderr, "%s%s\n", locn, string);

		break;
	case _LOG_NOTICE:
		if (ldata->verbose_level >= _LOG_NOTICE)
			fprintf(stderr, "%s%s\n", locn, string);

		break;
	case _LOG_WARN:
		if (ldata->verbose_level >= _LOG_WARN)
			printf("%s\n", string);

		break;
	case _LOG_ERR:
		if (ldata->verbose_level >= _LOG_ERR)
			fprintf(stderr, "%s%s\n", locn, string);

		break;
	case _LOG_FATAL:
	default:
		if (ldata->verbose_level >= _LOG_FATAL)
			fprintf(stderr, "%s%s\n", locn, string);

		break;
	};
}

static int start_threaded_log(void)
{
	/*
	 * We set this immediately so that even if the threaded
	 * logging can't load, we don't log in a blocking manner - the
	 * non-blocking behavior overrides the fact that we won't see
	 * any logs if the async logging can't load
	 */
	tl.enabled = 1;

	if (!(tl.dlh = dlopen("libmultilog_async.so", RTLD_NOW))) {
		fprintf(stderr, "%s\n", dlerror());
		return 0;
	}

	if (!(tl.start_log = dlsym(tl.dlh, "start_syslog_thread")) ||
	    !(tl.stop_log  = dlsym(tl.dlh, "stop_syslog_thread")) ||
	    !(tl.log       = dlsym(tl.dlh, "write_to_buf"))) {
		fprintf(stderr, "Unable to load all fxns\n");
		dlclose(tl.dlh);
		tl.dlh = NULL;
		return 0;
	}

	/* FIXME: the timeout here probably can be tweaked */
	/* FIXME: Probably want to do something if this fails */
	return tl.start_log(&tl.thread, 100000);
}

static int stop_threaded_log(void)
{
	if (tl.enabled) {
		tl.enabled = 0;

		if (tl.dlh) {
			tl.stop_log(tl.thread);
			dlclose(tl.dlh);
			tl.dlh = NULL;
		}
	}

	return 1;
}

int multilog_add_type(enum log_type type, void *data)
{
	struct log_list *logl, *ll;

	/* FIXME: Potential race here */
	/* attempt to load locking protocol */
	if (!init_lock_fn) {
		if (!load_lock_syms()) {
			fprintf(stderr, "Unable to load locking\n");
			return 0;
		}

		lock_handle = init_lock();
	}

	/*
	 * Preallocate because we don't want to sleep holding a lock.
	 */
	if (!(logl = malloc(sizeof(*logl))))
		return 0;

	memset(logl, 0, sizeof(*logl));

	/*
	 * If the type has already been registered, it doesn't need to
	 * be registered again.  This means the caller will need to
	 * explicitly unregister to change registration.
	 */
	lock_list(lock_handle);

	list_iterate_items(ll, &logs) {
		if (ll->type == type) {
			unlock_list(lock_handle);
			free(logl);
			return 1;
		}
	}

	logl->type = type;

	memset(&logl->data, 0, sizeof(logl->data));

	logl->data.verbose_level = DEFAULT_VERBOSITY;
	logl->log = nop_log;
	list_add(&logs, &logl->list);
	unlock_list(lock_handle);

	switch (type) {
	case standard:
		logl->log = standard_log;
		break;
	case logfile:
		init_file_log(&logl->data, (struct file_log *) data);
		logl->log = file_log;
		break;
	case std_syslog:
		if (data)
			init_sys_log((struct sys_log *) data);

		logl->log = sys_log;
		break;
	case custom:
		/* Caller should use multilog_custom to set their logging fxn */
		logl->log = nop_log;
		break;
	}

	return 1;
}

/* Resets the logging handle to no logging */
void multilog_clear_logging(void)
{
	enum log_type i;

	for (i = standard; i <= custom; i++)
		multilog_del_type(i);
}

/* FIXME: Might want to have this return an error if we can't find the type */
void multilog_del_type(enum log_type type)
{
	struct list *tmp, *next;
	struct log_list *logl, *ll = NULL;

	/* First delete type from list safely. */
	lock_list(lock_handle);

	list_iterate_safe(tmp, next, &logs) {
		logl = list_item(tmp, struct log_list);

		if (logl->type == type) {
			ll = logl;
			list_del(tmp);
			break;
		}
	}

	unlock_list(lock_handle);

	if (ll) {
		if (ll->type == custom &&
		    ll->data.info.cl.destructor)
			ll->data.info.cl.destructor(ll->data.info.cl.custom);

		free(ll);
	}

	if (list_empty(&logs)) {
		/* FIXME: Not sure the destroy_lock call is really necessary */
		destroy_lock(lock_handle);

		if(lock_dlh) {
			dlclose(lock_dlh);
			init_lock_fn = NULL;
			lock_fn = NULL;
			unlock_fn = NULL;
			destroy_lock_fn = NULL;
		}
	}
}

void multilog_custom(multilog_fn fn, void (*destructor_fn)(void *data),
		     void *data)
{
	struct log_list *logl;

	/*
	 * FIXME: Should we present an error if
	 * we can't find a suitable target?
	 */
	lock_list(lock_handle);

	list_iterate_items(logl, &logs) {
		if (logl->type == custom && logl->log == nop_log) {
			logl->log = fn;
			logl->data.info.cl.destructor = destructor_fn;
			logl->data.info.cl.custom = data;
		}
	}

	unlock_list(lock_handle);
}


void multilog(int priority, const char *file, int line, const char *format, ...)
{
	/* FIXME: stack allocation of large buffer. */
	char buf[4096];

	va_list args;
	va_start(args, format);
	vsnprintf(buf, 4096, format, args);
	va_end(args);

	if (tl.enabled) {
		/* send to async code */
		if (tl.dlh)
			tl.log(priority, file, line, buf);
	} else
		/* Log directly */
		logit(priority, file, line, buf);

}


void multilog_init_verbose(enum log_type type, int level)
{
	struct log_list *ll;

	lock_list(lock_handle);

	list_iterate_items(ll, &logs) {
		if (ll->type == type)
			ll->data.verbose_level = level;
	}

	unlock_list(lock_handle);
}

/* Toggle asynchronous logging */
int multilog_async(int enabled)
{
	return enabled ? start_threaded_log() : stop_threaded_log();
}


/* Internal function */
void logit(int priority, const char *file, int line, char *buf)
{
	struct log_list *logl;

	lock_list(lock_handle);

	list_iterate_items(logl, &logs) {
		/* Custom logging types do just get the custom data
		 * they setup initially - multilog doesn't do any
		 * handling of custom loggers data */
		logl->log(logl->type == custom ?
			  logl->data.info.cl.custom : &logl->data,
			  priority, file, line, buf);
	}

	unlock_list(lock_handle);

}
