/*	$NetBSD: dmeventd.c,v 1.1.1.1 2008/12/12 11:42:06 haad Exp $	*/

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

/*
 * dmeventd - dm event daemon to monitor active mapped devices
 */

#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#include "configure.h"
#include "libdevmapper.h"
#include "libdevmapper-event.h"
#include "dmeventd.h"
//#include "libmultilog.h"
#include "dm-logging.h"

#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>		/* for htonl, ntohl */

#ifdef linux
#  include <malloc.h>

#  define OOM_ADJ_FILE "/proc/self/oom_adj"

/* From linux/oom.h */
#  define OOM_DISABLE (-17)
#  define OOM_ADJUST_MIN (-16)

#endif

/* FIXME We use syslog for now, because multilog is not yet implemented */
#include <syslog.h>

static volatile sig_atomic_t _exit_now = 0;	/* set to '1' when signal is given to exit */
static volatile sig_atomic_t _thread_registries_empty = 1;	/* registries are empty initially */
static int _debug = 0;

/* List (un)link macros. */
#define	LINK(x, head)		dm_list_add(head, &(x)->list)
#define	LINK_DSO(dso)		LINK(dso, &_dso_registry)
#define	LINK_THREAD(thread)	LINK(thread, &_thread_registry)

#define	UNLINK(x)		dm_list_del(&(x)->list)
#define	UNLINK_DSO(x)		UNLINK(x)
#define	UNLINK_THREAD(x)	UNLINK(x)

#define DAEMON_NAME "dmeventd"

/*
  Global mutex for thread list access. Has to be held when:
  - iterating thread list
  - adding or removing elements from thread list
  - changing or reading thread_status's fields:
    processing, status, events
  Use _lock_mutex() and _unlock_mutex() to hold/release it
*/
static pthread_mutex_t _global_mutex;

/*
  There are three states a thread can attain (see struct
  thread_status, field int status):

  - DM_THREAD_RUNNING: thread has started up and is either working or
  waiting for events... transitions to either SHUTDOWN or DONE
  - DM_THREAD_SHUTDOWN: thread is still doing something, but it is
  supposed to terminate (and transition to DONE) as soon as it
  finishes whatever it was doing at the point of flipping state to
  SHUTDOWN... the thread is still on the thread list
  - DM_THREAD_DONE: thread has terminated and has been moved over to
  unused thread list, cleanup pending
 */
#define DM_THREAD_RUNNING  0
#define DM_THREAD_SHUTDOWN 1
#define DM_THREAD_DONE     2

#define THREAD_STACK_SIZE (300*1024)

#define DEBUGLOG(fmt, args...) _debuglog(fmt, ## args)

/* Data kept about a DSO. */
struct dso_data {
	struct dm_list list;

	char *dso_name;		/* DSO name (eg, "evms", "dmraid", "lvm2"). */

	void *dso_handle;	/* Opaque handle as returned from dlopen(). */
	unsigned int ref_count;	/* Library reference count. */

	/*
	 * Event processing.
	 *
	 * The DSO can do whatever appropriate steps if an event
	 * happens such as changing the mapping in case a mirror
	 * fails, update the application metadata etc.
	 *
	 * This function gets a dm_task that is a result of
	 * DM_DEVICE_WAITEVENT ioctl (results equivalent to
	 * DM_DEVICE_STATUS). It should not destroy it.
	 * The caller must dispose of the task.
	 */
	void (*process_event)(struct dm_task *dmt, enum dm_event_mask event, void **user);

	/*
	 * Device registration.
	 *
	 * When an application registers a device for an event, the DSO
	 * can carry out appropriate steps so that a later call to
	 * the process_event() function is sane (eg, read metadata
	 * and activate a mapping).
	 */
	int (*register_device)(const char *device, const char *uuid, int major,
			       int minor, void **user);

	/*
	 * Device unregistration.
	 *
	 * In case all devices of a mapping (eg, RAID10) are unregistered
	 * for events, the DSO can recognize this and carry out appropriate
	 * steps (eg, deactivate mapping, metadata update).
	 */
	int (*unregister_device)(const char *device, const char *uuid,
				 int major, int minor, void **user);
};
static DM_LIST_INIT(_dso_registry);

/* Structure to keep parsed register variables from client message. */
struct message_data {
	char *id;
	char *dso_name;		/* Name of DSO. */
	char *device_uuid;	/* Mapped device path. */
	union {
		char *str;	/* Events string as fetched from message. */
		enum dm_event_mask field;	/* Events bitfield. */
	} events;
	union {
		char *str;
		uint32_t secs;
	} timeout;
	struct dm_event_daemon_message *msg;	/* Pointer to message buffer. */
};

/*
 * Housekeeping of thread+device states.
 *
 * One thread per mapped device which can block on it until an event
 * occurs and the event processing function of the DSO gets called.
 */
struct thread_status {
	struct dm_list list;

	pthread_t thread;

	struct dso_data *dso_data;	/* DSO this thread accesses. */

	struct {
		char *uuid;
		char *name;
		int major, minor;
	} device;
	uint32_t event_nr;	/* event number */
	int processing;		/* Set when event is being processed */

	int status;		/* see DM_THREAD_{RUNNING,SHUTDOWN,DONE}
				   constants above */
	enum dm_event_mask events;	/* bitfield for event filter. */
	enum dm_event_mask current_events;	/* bitfield for occured events. */
	struct dm_task *current_task;
	time_t next_time;
	uint32_t timeout;
	struct dm_list timeout_list;
	void *dso_private; /* dso per-thread status variable */
};
static DM_LIST_INIT(_thread_registry);
static DM_LIST_INIT(_thread_registry_unused);

static int _timeout_running;
static DM_LIST_INIT(_timeout_registry);
static pthread_mutex_t _timeout_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t _timeout_cond = PTHREAD_COND_INITIALIZER;

static void _debuglog(const char *fmt, ...)
{
        time_t P;
        va_list ap;
 
        if (!_debug)
                return;
 
        va_start(ap,fmt);

        time(&P);
        fprintf(stderr, "dmeventd[%p]: %.15s ", (void *) pthread_self(), ctime(&P)+4 );
        vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");

        va_end(ap);
}

/* Allocate/free the status structure for a monitoring thread. */
static struct thread_status *_alloc_thread_status(struct message_data *data,
						  struct dso_data *dso_data)
{
	struct thread_status *ret = (typeof(ret)) dm_malloc(sizeof(*ret));

	if (!ret)
		return NULL;

	memset(ret, 0, sizeof(*ret));
	if (!(ret->device.uuid = dm_strdup(data->device_uuid))) {
		dm_free(ret);
		return NULL;
	}

	ret->current_task = NULL;
	ret->device.name = NULL;
	ret->device.major = ret->device.minor = 0;
	ret->dso_data = dso_data;
	ret->events = data->events.field;
	ret->timeout = data->timeout.secs;
	dm_list_init(&ret->timeout_list);

	return ret;
}

static void _free_thread_status(struct thread_status *thread)
{
	if (thread->current_task)
		dm_task_destroy(thread->current_task);
	dm_free(thread->device.uuid);
	dm_free(thread->device.name);
	dm_free(thread);
}

/* Allocate/free DSO data. */
static struct dso_data *_alloc_dso_data(struct message_data *data)
{
	struct dso_data *ret = (typeof(ret)) dm_malloc(sizeof(*ret));

	if (!ret)
		return NULL;

	memset(ret, 0, sizeof(*ret));
	if (!(ret->dso_name = dm_strdup(data->dso_name))) {
		dm_free(ret);
		return NULL;
	}

	return ret;
}

/* Create a device monitoring thread. */
static int _pthread_create_smallstack(pthread_t *t, void *(*fun)(void *), void *arg)
{
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	/*
	 * We use a smaller stack since it gets preallocated in its entirety
	 */
	pthread_attr_setstacksize(&attr, THREAD_STACK_SIZE);
	return pthread_create(t, &attr, fun, arg);
}

static void _free_dso_data(struct dso_data *data)
{
	dm_free(data->dso_name);
	dm_free(data);
}

/*
 * Fetch a string off src and duplicate it into *ptr.
 * Pay attention to zero-length strings.
 */
/* FIXME? move to libdevmapper to share with the client lib (need to
   make delimiter a parameter then) */
static int _fetch_string(char **ptr, char **src, const int delimiter)
{
	int ret = 0;
	char *p;
	size_t len;

	if ((p = strchr(*src, delimiter)))
		*p = 0;

	if ((*ptr = dm_strdup(*src))) {
		if ((len = strlen(*ptr)))
			*src += len;
		else {
			dm_free(*ptr);
			*ptr = NULL;
		}

		(*src)++;
		ret = 1;
	}

	if (p)
		*p = delimiter;

	return ret;
}

/* Free message memory. */
static void _free_message(struct message_data *message_data)
{
	if (message_data->id)
		dm_free(message_data->id);
	if (message_data->dso_name)
		dm_free(message_data->dso_name);

	if (message_data->device_uuid)
		dm_free(message_data->device_uuid);

}

/* Parse a register message from the client. */
static int _parse_message(struct message_data *message_data)
{
	int ret = 0;
	char *p = message_data->msg->data;
	struct dm_event_daemon_message *msg = message_data->msg;

	if (!msg->data)
		return 0;

	/*
	 * Retrieve application identifier, mapped device
	 * path and events # string from message.
	 */
	if (_fetch_string(&message_data->id, &p, ' ') &&
	    _fetch_string(&message_data->dso_name, &p, ' ') &&
	    _fetch_string(&message_data->device_uuid, &p, ' ') &&
	    _fetch_string(&message_data->events.str, &p, ' ') &&
	    _fetch_string(&message_data->timeout.str, &p, ' ')) {
		if (message_data->events.str) {
			enum dm_event_mask i = atoi(message_data->events.str);

			/*
			 * Free string representaion of events.
			 * Not needed an more.
			 */
			dm_free(message_data->events.str);
			message_data->events.field = i;
		}
		if (message_data->timeout.str) {
			uint32_t secs = atoi(message_data->timeout.str);
			dm_free(message_data->timeout.str);
			message_data->timeout.secs = secs ? secs :
			    DM_EVENT_DEFAULT_TIMEOUT;
		}

		ret = 1;
	}

	dm_free(msg->data);
	msg->data = NULL;
	msg->size = 0;
	return ret;
};

/* Global mutex to lock access to lists et al. See _global_mutex
   above. */
static int _lock_mutex(void)
{
	return pthread_mutex_lock(&_global_mutex);
}

static int _unlock_mutex(void)
{
	return pthread_mutex_unlock(&_global_mutex);
}

/* Store pid in pidfile. */
static int _storepid(int lf)
{
	int len;
	char pid[8];

	if ((len = snprintf(pid, sizeof(pid), "%u\n", getpid())) < 0)
		return 0;

	if (len > (int) sizeof(pid))
		len = (int) sizeof(pid);

	if (write(lf, pid, (size_t) len) != len)
		return 0;

	fsync(lf);

	return 1;
}

/* Check, if a device exists. */
static int _fill_device_data(struct thread_status *ts)
{
	struct dm_task *dmt;
	struct dm_info dmi;

	if (!ts->device.uuid)
		return 0;

	ts->device.name = NULL;
	ts->device.major = ts->device.minor = 0;

	dmt = dm_task_create(DM_DEVICE_INFO);
	if (!dmt)
		return 0;

	dm_task_set_uuid(dmt, ts->device.uuid);
	if (!dm_task_run(dmt))
		goto fail;

	ts->device.name = dm_strdup(dm_task_get_name(dmt));
	if (!ts->device.name)
		goto fail;

	if (!dm_task_get_info(dmt, &dmi))
		goto fail;

	ts->device.major = dmi.major;
	ts->device.minor = dmi.minor;

	dm_task_destroy(dmt);
	return 1;

      fail:
	dm_task_destroy(dmt);
	dm_free(ts->device.name);
	return 0;
}

/*
 * Find an existing thread for a device.
 *
 * Mutex must be held when calling this.
 */
static struct thread_status *_lookup_thread_status(struct message_data *data)
{
	struct thread_status *thread;

	dm_list_iterate_items(thread, &_thread_registry)
	    if (!strcmp(data->device_uuid, thread->device.uuid))
		return thread;

	return NULL;
}

/* Cleanup at exit. */
static void _exit_dm_lib(void)
{
	dm_lib_release();
	dm_lib_exit();
}

static void _exit_timeout(void *unused __attribute((unused)))
{
	_timeout_running = 0;
	pthread_mutex_unlock(&_timeout_mutex);
}

/* Wake up monitor threads every so often. */
static void *_timeout_thread(void *unused __attribute((unused)))
{
	struct timespec timeout;
	time_t curr_time;

	timeout.tv_nsec = 0;
	pthread_cleanup_push(_exit_timeout, NULL);
	pthread_mutex_lock(&_timeout_mutex);

	while (!dm_list_empty(&_timeout_registry)) {
		struct thread_status *thread;

		timeout.tv_sec = 0;
		curr_time = time(NULL);

		dm_list_iterate_items_gen(thread, &_timeout_registry, timeout_list) {
			if (thread->next_time <= curr_time) {
				thread->next_time = curr_time + thread->timeout;
				pthread_kill(thread->thread, SIGALRM);
			}

			if (thread->next_time < timeout.tv_sec || !timeout.tv_sec)
				timeout.tv_sec = thread->next_time;
		}

		pthread_cond_timedwait(&_timeout_cond, &_timeout_mutex,
				       &timeout);
	}

	pthread_cleanup_pop(1);

	return NULL;
}

static int _register_for_timeout(struct thread_status *thread)
{
	int ret = 0;

	pthread_mutex_lock(&_timeout_mutex);

	thread->next_time = time(NULL) + thread->timeout;

	if (dm_list_empty(&thread->timeout_list)) {
		dm_list_add(&_timeout_registry, &thread->timeout_list);
		if (_timeout_running)
			pthread_cond_signal(&_timeout_cond);
	}

	if (!_timeout_running) {
		pthread_t timeout_id;

		if (!(ret = -_pthread_create_smallstack(&timeout_id, _timeout_thread, NULL)))
			_timeout_running = 1;
	}

	pthread_mutex_unlock(&_timeout_mutex);

	return ret;
}

static void _unregister_for_timeout(struct thread_status *thread)
{
	pthread_mutex_lock(&_timeout_mutex);
	if (!dm_list_empty(&thread->timeout_list)) {
		dm_list_del(&thread->timeout_list);
		dm_list_init(&thread->timeout_list);
	}
	pthread_mutex_unlock(&_timeout_mutex);
}

static void _no_intr_log(int level, const char *file, int line,
			const char *f, ...)
{
	va_list ap;

	if (errno == EINTR)
		return;
	if (level > _LOG_WARN)
		return;

	va_start(ap, f);

	if (level < _LOG_WARN)
		vfprintf(stderr, f, ap);
	else
		vprintf(f, ap);

	va_end(ap);

	if (level < _LOG_WARN)
		fprintf(stderr, "\n");
	else
		fprintf(stdout, "\n");
}

static sigset_t _unblock_sigalrm(void)
{
	sigset_t set, old;

	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	pthread_sigmask(SIG_UNBLOCK, &set, &old);
	return old;
}

#define DM_WAIT_RETRY 0
#define DM_WAIT_INTR 1
#define DM_WAIT_FATAL 2

/* Wait on a device until an event occurs. */
static int _event_wait(struct thread_status *thread, struct dm_task **task)
{
	sigset_t set;
	int ret = DM_WAIT_RETRY;
	struct dm_task *dmt;
	struct dm_info info;

	*task = 0;

	if (!(dmt = dm_task_create(DM_DEVICE_WAITEVENT)))
		return DM_WAIT_RETRY;

	thread->current_task = dmt;

	if (!dm_task_set_uuid(dmt, thread->device.uuid) ||
	    !dm_task_set_event_nr(dmt, thread->event_nr))
		goto out;

	/*
	 * This is so that you can break out of waiting on an event,
	 * either for a timeout event, or to cancel the thread.
	 */
	set = _unblock_sigalrm();
	dm_log_init(_no_intr_log);
	errno = 0;
	if (dm_task_run(dmt)) {
		thread->current_events |= DM_EVENT_DEVICE_ERROR;
		ret = DM_WAIT_INTR;

		if ((ret = dm_task_get_info(dmt, &info)))
			thread->event_nr = info.event_nr;
	} else if (thread->events & DM_EVENT_TIMEOUT && errno == EINTR) {
		thread->current_events |= DM_EVENT_TIMEOUT;
		ret = DM_WAIT_INTR;
	} else if (thread->status == DM_THREAD_SHUTDOWN && errno == EINTR) {
		ret = DM_WAIT_FATAL;
	} else {
		syslog(LOG_NOTICE, "dm_task_run failed, errno = %d, %s",
		       errno, strerror(errno));
		if (errno == ENXIO) {
			syslog(LOG_ERR, "%s disappeared, detaching",
			       thread->device.name);
			ret = DM_WAIT_FATAL;
		}
	}

	pthread_sigmask(SIG_SETMASK, &set, NULL);
	dm_log_init(NULL);

      out:
	if (ret == DM_WAIT_FATAL || ret == DM_WAIT_RETRY) {
		dm_task_destroy(dmt);
		thread->current_task = NULL;
	} else
		*task = dmt;

	return ret;
}

/* Register a device with the DSO. */
static int _do_register_device(struct thread_status *thread)
{
	return thread->dso_data->register_device(thread->device.name,
						 thread->device.uuid,
						 thread->device.major,
						 thread->device.minor,
						 &(thread->dso_private));
}

/* Unregister a device with the DSO. */
static int _do_unregister_device(struct thread_status *thread)
{
	return thread->dso_data->unregister_device(thread->device.name,
						   thread->device.uuid,
						   thread->device.major,
						   thread->device.minor,
						   &(thread->dso_private));
}

/* Process an event in the DSO. */
static void _do_process_event(struct thread_status *thread, struct dm_task *task)
{
	thread->dso_data->process_event(task, thread->current_events, &(thread->dso_private));
}

/* Thread cleanup handler to unregister device. */
static void _monitor_unregister(void *arg)
{
	struct thread_status *thread = arg, *thread_iter;

	if (!_do_unregister_device(thread))
		syslog(LOG_ERR, "%s: %s unregister failed\n", __func__,
		       thread->device.name);
	if (thread->current_task)
		dm_task_destroy(thread->current_task);
	thread->current_task = NULL;

	_lock_mutex();
	if (thread->events & DM_EVENT_TIMEOUT) {
		/* _unregister_for_timeout locks another mutex, we
		   don't want to deadlock so we release our mutex for
		   a bit */
		_unlock_mutex();
		_unregister_for_timeout(thread);
		_lock_mutex();
	}
	/* we may have been relinked to unused registry since we were
	   called, so check that */
	dm_list_iterate_items(thread_iter, &_thread_registry_unused)
		if (thread_iter == thread) {
			thread->status = DM_THREAD_DONE;
			_unlock_mutex();
			return;
		}
	thread->status = DM_THREAD_DONE;
	UNLINK_THREAD(thread);
	LINK(thread, &_thread_registry_unused);
	_unlock_mutex();
}

static struct dm_task *_get_device_status(struct thread_status *ts)
{
	struct dm_task *dmt = dm_task_create(DM_DEVICE_STATUS);

	if (!dmt)
		return NULL;

	dm_task_set_uuid(dmt, ts->device.uuid);

	if (!dm_task_run(dmt)) {
		dm_task_destroy(dmt);
		return NULL;
	}

	return dmt;
}

/* Device monitoring thread. */
static void *_monitor_thread(void *arg)
{
	struct thread_status *thread = arg;
	int wait_error = 0;
	struct dm_task *task;

	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
	pthread_cleanup_push(_monitor_unregister, thread);

	/* Wait for do_process_request() to finish its task. */
	_lock_mutex();
	thread->status = DM_THREAD_RUNNING;
	_unlock_mutex();

	/* Loop forever awaiting/analyzing device events. */
	while (1) {
		thread->current_events = 0;

		wait_error = _event_wait(thread, &task);
		if (wait_error == DM_WAIT_RETRY)
			continue;

		if (wait_error == DM_WAIT_FATAL)
			break;

		/* Timeout occurred, task is not filled properly.
		 * We get device status here for processing it in DSO.
		 */
		if (wait_error == DM_WAIT_INTR &&
		    thread->current_events & DM_EVENT_TIMEOUT) {
			dm_task_destroy(task);
			task = _get_device_status(thread);
			/* FIXME: syslog fail here ? */
			if (!(thread->current_task = task))
				continue;
		}

		/*
		 * We know that wait succeeded and stored a
		 * pointer to dm_task with device status into task.
		 */

		/*
		 * Check against filter.
		 *
		 * If there's current events delivered from _event_wait() AND
		 * the device got registered for those events AND
		 * those events haven't been processed yet, call
		 * the DSO's process_event() handler.
		 */
		_lock_mutex();
		if (thread->status == DM_THREAD_SHUTDOWN) {
			_unlock_mutex();
			break;
		}
		_unlock_mutex();

		if (thread->events & thread->current_events) {
			_lock_mutex();
			thread->processing = 1;
			_unlock_mutex();

			_do_process_event(thread, task);
			dm_task_destroy(task);
			thread->current_task = NULL;

			_lock_mutex();
			thread->processing = 0;
			_unlock_mutex();
		} else {
			dm_task_destroy(task);
			thread->current_task = NULL;
		}
	}

	pthread_cleanup_pop(1);

	return NULL;
}

/* Create a device monitoring thread. */
static int _create_thread(struct thread_status *thread)
{
	return _pthread_create_smallstack(&thread->thread, _monitor_thread, thread);
}

static int _terminate_thread(struct thread_status *thread)
{
	return pthread_kill(thread->thread, SIGALRM);
}

/* DSO reference counting. Call with _global_mutex locked! */
static void _lib_get(struct dso_data *data)
{
	data->ref_count++;
}

static void _lib_put(struct dso_data *data)
{
	if (!--data->ref_count) {
		dlclose(data->dso_handle);
		UNLINK_DSO(data);
		_free_dso_data(data);
	}
}

/* Find DSO data. */
static struct dso_data *_lookup_dso(struct message_data *data)
{
	struct dso_data *dso_data, *ret = NULL;

	dm_list_iterate_items(dso_data, &_dso_registry)
	    if (!strcmp(data->dso_name, dso_data->dso_name)) {
		_lib_get(dso_data);
		ret = dso_data;
		break;
	}

	return ret;
}

/* Lookup DSO symbols we need. */
static int _lookup_symbol(void *dl, void **symbol, const char *name)
{
	if ((*symbol = dlsym(dl, name)))
		return 1;

	return 0;
}

static int lookup_symbols(void *dl, struct dso_data *data)
{
	return _lookup_symbol(dl, (void *) &data->process_event,
			     "process_event") &&
	    _lookup_symbol(dl, (void *) &data->register_device,
			  "register_device") &&
	    _lookup_symbol(dl, (void *) &data->unregister_device,
			  "unregister_device");
}

/* Load an application specific DSO. */
static struct dso_data *_load_dso(struct message_data *data)
{
	void *dl;
	struct dso_data *ret = NULL;

	if (!(dl = dlopen(data->dso_name, RTLD_NOW))) {
		const char *dlerr = dlerror();
		syslog(LOG_ERR, "dmeventd %s dlopen failed: %s", data->dso_name,
		       dlerr);
		data->msg->size =
		    dm_asprintf(&(data->msg->data), "%s %s dlopen failed: %s",
				data->id, data->dso_name, dlerr);
		return NULL;
	}

	if (!(ret = _alloc_dso_data(data))) {
		dlclose(dl);
		return NULL;
	}

	if (!(lookup_symbols(dl, ret))) {
		_free_dso_data(ret);
		dlclose(dl);
		return NULL;
	}

	/*
	 * Keep handle to close the library once
	 * we've got no references to it any more.
	 */
	ret->dso_handle = dl;
	_lib_get(ret);

	_lock_mutex();
	LINK_DSO(ret);
	_unlock_mutex();

	return ret;
}

/* Return success on daemon active check. */
static int _active(struct message_data *message_data)
{
	return 0;
}

/*
 * Register for an event.
 *
 * Only one caller at a time here, because we use
 * a FIFO and lock it against multiple accesses.
 */
static int _register_for_event(struct message_data *message_data)
{
	int ret = 0;
	struct thread_status *thread, *thread_new = NULL;
	struct dso_data *dso_data;

	if (!(dso_data = _lookup_dso(message_data)) &&
	    !(dso_data = _load_dso(message_data))) {
		stack;
#ifdef ELIBACC
		ret = -ELIBACC;
#else
		ret = -ENODEV;
#endif
		goto out;
	}

	/* Preallocate thread status struct to avoid deadlock. */
	if (!(thread_new = _alloc_thread_status(message_data, dso_data))) {
		stack;
		ret = -ENOMEM;
		goto out;
	}

	if (!_fill_device_data(thread_new)) {
		stack;
		ret = -ENODEV;
		goto out;
	}

	_lock_mutex();

	/* If creation of timeout thread fails (as it may), we fail
	   here completely. The client is responsible for either
	   retrying later or trying to register without timeout
	   events. However, if timeout thread cannot be started, it
	   usually means we are so starved on resources that we are
	   almost as good as dead already... */
	if (thread_new->events & DM_EVENT_TIMEOUT) {
		ret = -_register_for_timeout(thread_new);
		if (ret) {
		    _unlock_mutex();
		    goto out;
		}
	}

	if (!(thread = _lookup_thread_status(message_data))) {
		_unlock_mutex();

		if (!(ret = _do_register_device(thread_new)))
			goto out;

		thread = thread_new;
		thread_new = NULL;

		/* Try to create the monitoring thread for this device. */
		_lock_mutex();
		if ((ret = -_create_thread(thread))) {
			_unlock_mutex();
			_do_unregister_device(thread);
			_free_thread_status(thread);
			goto out;
		} else
			LINK_THREAD(thread);
	}

	/* Or event # into events bitfield. */
	thread->events |= message_data->events.field;

	_unlock_mutex();

      out:
	/*
	 * Deallocate thread status after releasing
	 * the lock in case we haven't used it.
	 */
	if (thread_new)
		_free_thread_status(thread_new);

	return ret;
}

/*
 * Unregister for an event.
 *
 * Only one caller at a time here as with register_for_event().
 */
static int _unregister_for_event(struct message_data *message_data)
{
	int ret = 0;
	struct thread_status *thread;

	/*
	 * Clear event in bitfield and deactivate
	 * monitoring thread in case bitfield is 0.
	 */
	_lock_mutex();

	if (!(thread = _lookup_thread_status(message_data))) {
		_unlock_mutex();
		ret = -ENODEV;
		goto out;
	}

	if (thread->status == DM_THREAD_DONE) {
		/* the thread has terminated while we were not
		   watching */
		_unlock_mutex();
		return 0;
	}

	thread->events &= ~message_data->events.field;

	if (!(thread->events & DM_EVENT_TIMEOUT))
		_unregister_for_timeout(thread);
	/*
	 * In case there's no events to monitor on this device ->
	 * unlink and terminate its monitoring thread.
	 */
	if (!thread->events) {
		UNLINK_THREAD(thread);
		LINK(thread, &_thread_registry_unused);
	}
	_unlock_mutex();

      out:
	return ret;
}

/*
 * Get registered device.
 *
 * Only one caller at a time here as with register_for_event().
 */
static int _registered_device(struct message_data *message_data,
			     struct thread_status *thread)
{
	struct dm_event_daemon_message *msg = message_data->msg;

	const char *fmt = "%s %s %s %u";
	const char *id = message_data->id;
	const char *dso = thread->dso_data->dso_name;
	const char *dev = thread->device.uuid;
	unsigned events = ((thread->status == DM_THREAD_RUNNING)
			   && (thread->events)) ? thread->events : thread->
	    events | DM_EVENT_REGISTRATION_PENDING;

	if (msg->data)
		dm_free(msg->data);

	msg->size = dm_asprintf(&(msg->data), fmt, id, dso, dev, events);

	_unlock_mutex();

	return 0;
}

static int _want_registered_device(char *dso_name, char *device_uuid,
				  struct thread_status *thread)
{
	/* If DSO names and device paths are equal. */
	if (dso_name && device_uuid)
		return !strcmp(dso_name, thread->dso_data->dso_name) &&
		    !strcmp(device_uuid, thread->device.uuid) &&
			(thread->status == DM_THREAD_RUNNING ||
			 (thread->events & DM_EVENT_REGISTRATION_PENDING));

	/* If DSO names are equal. */
	if (dso_name)
		return !strcmp(dso_name, thread->dso_data->dso_name) &&
			(thread->status == DM_THREAD_RUNNING ||
			 (thread->events & DM_EVENT_REGISTRATION_PENDING));

	/* If device paths are equal. */
	if (device_uuid)
		return !strcmp(device_uuid, thread->device.uuid) &&
			(thread->status == DM_THREAD_RUNNING ||
			 (thread->events & DM_EVENT_REGISTRATION_PENDING));

	return 1;
}

static int _get_registered_dev(struct message_data *message_data, int next)
{
	struct thread_status *thread, *hit = NULL;

	_lock_mutex();

	/* Iterate list of threads checking if we want a particular one. */
	dm_list_iterate_items(thread, &_thread_registry)
		if (_want_registered_device(message_data->dso_name,
					    message_data->device_uuid,
					    thread)) {
			hit = thread;
			break;
		}

	/*
	 * If we got a registered device and want the next one ->
	 * fetch next conforming element off the list.
	 */
	if (hit && !next) {
		_unlock_mutex();
		return _registered_device(message_data, hit);
	}

	if (!hit)
		goto out;

	thread = hit;

	while (1) {
		if (dm_list_end(&_thread_registry, &thread->list))
			goto out;

		thread = dm_list_item(thread->list.n, struct thread_status);
		if (_want_registered_device(message_data->dso_name, NULL, thread)) {
			hit = thread;
			break;
		}
	}

	_unlock_mutex();
	return _registered_device(message_data, hit);

      out:
	_unlock_mutex();
	
	return -ENOENT;
}

static int _get_registered_device(struct message_data *message_data)
{
	return _get_registered_dev(message_data, 0);
}

static int _get_next_registered_device(struct message_data *message_data)
{
	return _get_registered_dev(message_data, 1);
}

static int _set_timeout(struct message_data *message_data)
{
	struct thread_status *thread;

	_lock_mutex();
	if ((thread = _lookup_thread_status(message_data)))
		thread->timeout = message_data->timeout.secs;
	_unlock_mutex();

	return thread ? 0 : -ENODEV;
}

static int _get_timeout(struct message_data *message_data)
{
	struct thread_status *thread;
	struct dm_event_daemon_message *msg = message_data->msg;

	if (msg->data)
		dm_free(msg->data);

	_lock_mutex();
	if ((thread = _lookup_thread_status(message_data))) {
		msg->size =
		    dm_asprintf(&(msg->data), "%s %" PRIu32, message_data->id,
				thread->timeout);
	} else {
		msg->data = NULL;
		msg->size = 0;
	}
	_unlock_mutex();

	return thread ? 0 : -ENODEV;
}

/* Initialize a fifos structure with path names. */
static void _init_fifos(struct dm_event_fifos *fifos)
{
	memset(fifos, 0, sizeof(*fifos));

	fifos->client_path = DM_EVENT_FIFO_CLIENT;
	fifos->server_path = DM_EVENT_FIFO_SERVER;
}

/* Open fifos used for client communication. */
static int _open_fifos(struct dm_event_fifos *fifos)
{
	/* Create fifos */
	if (((mkfifo(fifos->client_path, 0600) == -1) && errno != EEXIST) ||
	    ((mkfifo(fifos->server_path, 0600) == -1) && errno != EEXIST)) {
		syslog(LOG_ERR, "%s: Failed to create a fifo.\n", __func__);
		stack;
		return -errno;
	}

	struct stat st;

	/* Warn about wrong permissions if applicable */
	if ((!stat(fifos->client_path, &st)) && (st.st_mode & 0777) != 0600)
		syslog(LOG_WARNING, "Fixing wrong permissions on %s",
		       fifos->client_path);

	if ((!stat(fifos->server_path, &st)) && (st.st_mode & 0777) != 0600)
		syslog(LOG_WARNING, "Fixing wrong permissions on %s",
		       fifos->server_path);

	/* If they were already there, make sure permissions are ok. */
	if (chmod(fifos->client_path, 0600)) {
		syslog(LOG_ERR, "Unable to set correct file permissions on %s",
		       fifos->client_path);
		return -errno;
	}

	if (chmod(fifos->server_path, 0600)) {
		syslog(LOG_ERR, "Unable to set correct file permissions on %s",
		       fifos->server_path);
		return -errno;
	}

	/* Need to open read+write or we will block or fail */
	if ((fifos->server = open(fifos->server_path, O_RDWR)) < 0) {
		stack;
		return -errno;
	}

	/* Need to open read+write for select() to work. */
	if ((fifos->client = open(fifos->client_path, O_RDWR)) < 0) {
		stack;
		close(fifos->server);
		return -errno;
	}

	return 0;
}

/*
 * Read message from client making sure that data is available
 * and a complete message is read.  Must not block indefinitely.
 */
static int _client_read(struct dm_event_fifos *fifos,
		       struct dm_event_daemon_message *msg)
{
	struct timeval t;
	unsigned bytes = 0;
	int ret = 0;
	fd_set fds;
	int header = 1;
	size_t size = 2 * sizeof(uint32_t);	/* status + size */
	char *buf = alloca(size);

	msg->data = NULL;

	errno = 0;
	while (bytes < size && errno != EOF) {
		/* Watch client read FIFO for input. */
		FD_ZERO(&fds);
		FD_SET(fifos->client, &fds);
		t.tv_sec = 1;
		t.tv_usec = 0;
		ret = select(fifos->client + 1, &fds, NULL, NULL, &t);

		if (!ret && !bytes)	/* nothing to read */
			return 0;

		if (!ret)	/* trying to finish read */
			continue;

		if (ret < 0)	/* error */
			return 0;

		ret = read(fifos->client, buf + bytes, size - bytes);
		bytes += ret > 0 ? ret : 0;
		if (bytes == 2 * sizeof(uint32_t) && header) {
			msg->cmd = ntohl(*((uint32_t *) buf));
			msg->size = ntohl(*((uint32_t *) buf + 1));
			buf = msg->data = dm_malloc(msg->size);
			size = msg->size;
			bytes = 0;
			header = 0;
		}
	}

	if (bytes != size) {
		if (msg->data)
			dm_free(msg->data);
		msg->data = NULL;
		msg->size = 0;
	}

	return bytes == size;
}

/*
 * Write a message to the client making sure that it is ready to write.
 */
static int _client_write(struct dm_event_fifos *fifos,
			struct dm_event_daemon_message *msg)
{
	unsigned bytes = 0;
	int ret = 0;
	fd_set fds;

	size_t size = 2 * sizeof(uint32_t) + msg->size;
	char *buf = alloca(size);

	*((uint32_t *)buf) = htonl(msg->cmd);
	*((uint32_t *)buf + 1) = htonl(msg->size);
	if (msg->data)
		memcpy(buf + 2 * sizeof(uint32_t), msg->data, msg->size);

	errno = 0;
	while (bytes < size && errno != EIO) {
		do {
			/* Watch client write FIFO to be ready for output. */
			FD_ZERO(&fds);
			FD_SET(fifos->server, &fds);
		} while (select(fifos->server + 1, NULL, &fds, NULL, NULL) !=
			 1);

		ret = write(fifos->server, buf + bytes, size - bytes);
		bytes += ret > 0 ? ret : 0;
	}

	return bytes == size;
}

/*
 * Handle a client request.
 *
 * We put the request handling functions into
 * a list because of the growing number.
 */
static int _handle_request(struct dm_event_daemon_message *msg,
			  struct message_data *message_data)
{
	static struct {
		unsigned int cmd;
		int (*f)(struct message_data *);
	} requests[] = {
		{ DM_EVENT_CMD_REGISTER_FOR_EVENT, _register_for_event},
		{ DM_EVENT_CMD_UNREGISTER_FOR_EVENT, _unregister_for_event},
		{ DM_EVENT_CMD_GET_REGISTERED_DEVICE, _get_registered_device},
		{ DM_EVENT_CMD_GET_NEXT_REGISTERED_DEVICE,
			_get_next_registered_device},
		{ DM_EVENT_CMD_SET_TIMEOUT, _set_timeout},
		{ DM_EVENT_CMD_GET_TIMEOUT, _get_timeout},
		{ DM_EVENT_CMD_ACTIVE, _active},
	}, *req;

	for (req = requests; req < requests + sizeof(requests); req++)
		if (req->cmd == msg->cmd)
			return req->f(message_data);

	return -EINVAL;
}

/* Process a request passed from the communication thread. */
static int _do_process_request(struct dm_event_daemon_message *msg)
{
	int ret;
	char *answer;
	static struct message_data message_data;

	/* Parse the message. */
	memset(&message_data, 0, sizeof(message_data));
	message_data.msg = msg;
	if (msg->cmd == DM_EVENT_CMD_HELLO)  {
		ret = 0;
		answer = msg->data;
		if (answer) {
			msg->size = dm_asprintf(&(msg->data), "%s HELLO", answer);
			dm_free(answer);
		} else {
			msg->size = 0;
			msg->data = NULL;
		}
	} else if (msg->cmd != DM_EVENT_CMD_ACTIVE && !_parse_message(&message_data)) {
		stack;
		ret = -EINVAL;
	} else
		ret = _handle_request(msg, &message_data);

	msg->cmd = ret;
	if (!msg->data)
		msg->size = dm_asprintf(&(msg->data), "%s %s", message_data.id, strerror(-ret));

	_free_message(&message_data);

	return ret;
}

/* Only one caller at a time. */
static void _process_request(struct dm_event_fifos *fifos)
{
	struct dm_event_daemon_message msg;

	memset(&msg, 0, sizeof(msg));

	/*
	 * Read the request from the client (client_read, client_write
	 * give true on success and false on failure).
	 */
	if (!_client_read(fifos, &msg))
		return;

	/* _do_process_request fills in msg (if memory allows for
	   data, otherwise just cmd and size = 0) */
	_do_process_request(&msg);

	if (!_client_write(fifos, &msg))
		stack;

	if (msg.data)
		dm_free(msg.data);
}

static void _cleanup_unused_threads(void)
{
	int ret;
	struct dm_list *l;
	struct thread_status *thread;

	_lock_mutex();
	while ((l = dm_list_first(&_thread_registry_unused))) {
		thread = dm_list_item(l, struct thread_status);
		if (thread->processing)
			break;	/* cleanup on the next round */

		if (thread->status == DM_THREAD_RUNNING) {
			thread->status = DM_THREAD_SHUTDOWN;
			break;
		}

		if (thread->status == DM_THREAD_SHUTDOWN) {
			if (!thread->events) {
				/* turn codes negative -- should we be returning this? */
				ret = _terminate_thread(thread);

				if (ret == ESRCH) {
					thread->status = DM_THREAD_DONE;
				} else if (ret) {
					syslog(LOG_ERR,
					       "Unable to terminate thread: %s\n",
					       strerror(-ret));
					stack;
				}
				break;
			}

			dm_list_del(l);
			syslog(LOG_ERR,
			       "thread can't be on unused list unless !thread->events");
			thread->status = DM_THREAD_RUNNING;
			LINK_THREAD(thread);

			continue;
		}

		if (thread->status == DM_THREAD_DONE) {
			dm_list_del(l);
			pthread_join(thread->thread, NULL);
			_lib_put(thread->dso_data);
			_free_thread_status(thread);
		}
	}

	_unlock_mutex();
}

static void _sig_alarm(int signum __attribute((unused)))
{
	pthread_testcancel();
}

/* Init thread signal handling. */
static void _init_thread_signals(void)
{
	sigset_t my_sigset;
	struct sigaction act;

	memset(&act, 0, sizeof(act));
	act.sa_handler = _sig_alarm;
	sigaction(SIGALRM, &act, NULL);
	sigfillset(&my_sigset);

	/* These are used for exiting */
	sigdelset(&my_sigset, SIGTERM);
	sigdelset(&my_sigset, SIGINT);
	sigdelset(&my_sigset, SIGHUP);
	sigdelset(&my_sigset, SIGQUIT);

	pthread_sigmask(SIG_BLOCK, &my_sigset, NULL);
}

/*
 * exit_handler
 * @sig
 *
 * Set the global variable which the process should
 * be watching to determine when to exit.
 */
static void _exit_handler(int sig __attribute((unused)))
{
	/*
	 * We exit when '_exit_now' is set.
	 * That is, when a signal has been received.
	 *
	 * We can not simply set '_exit_now' unless all
	 * threads are done processing.
	 */
	if (!_thread_registries_empty) {
		syslog(LOG_ERR, "There are still devices being monitored.");
		syslog(LOG_ERR, "Refusing to exit.");
	} else
		_exit_now = 1;

}

static int _lock_pidfile(void)
{
	int lf;
	char pidfile[] = DMEVENTD_PIDFILE;

	if ((lf = open(pidfile, O_CREAT | O_RDWR, 0644)) < 0)
		exit(EXIT_OPEN_PID_FAILURE);

	if (flock(lf, LOCK_EX | LOCK_NB) < 0)
		exit(EXIT_LOCKFILE_INUSE);

	if (!_storepid(lf))
		exit(EXIT_FAILURE);

	return 0;
}

#ifdef linux
/*
 * Protection against OOM killer if kernel supports it
 */
static int _set_oom_adj(int val)
{
	FILE *fp;

	struct stat st;

	if (stat(OOM_ADJ_FILE, &st) == -1) {
		if (errno == ENOENT)
			DEBUGLOG(OOM_ADJ_FILE " not found");
		else
			perror(OOM_ADJ_FILE ": stat failed");
		return 1;
	}

	if (!(fp = fopen(OOM_ADJ_FILE, "w"))) {
		perror(OOM_ADJ_FILE ": fopen failed");
		return 0;
	}

	fprintf(fp, "%i", val);
	if (dm_fclose(fp))
		perror(OOM_ADJ_FILE ": fclose failed");

	return 1;
}
#endif

static void _daemonize(void)
{
	int child_status;
	int fd;
	pid_t pid;
	struct rlimit rlim;
	struct timeval tval;
	sigset_t my_sigset;

	sigemptyset(&my_sigset);
	if (sigprocmask(SIG_SETMASK, &my_sigset, NULL) < 0) {
		fprintf(stderr, "Unable to restore signals.\n");
		exit(EXIT_FAILURE);
	}
	signal(SIGTERM, &_exit_handler);

	switch (pid = fork()) {
	case -1:
		perror("fork failed:");
		exit(EXIT_FAILURE);

	case 0:		/* Child */
		break;

	default:
		/* Wait for response from child */
		while (!waitpid(pid, &child_status, WNOHANG) && !_exit_now) {
			tval.tv_sec = 0;
			tval.tv_usec = 250000;	/* .25 sec */
			select(0, NULL, NULL, NULL, &tval);
		}

		if (_exit_now)	/* Child has signaled it is ok - we can exit now */
			exit(EXIT_SUCCESS);

		/* Problem with child.  Determine what it is by exit code */
		switch (WEXITSTATUS(child_status)) {
		case EXIT_LOCKFILE_INUSE:
			fprintf(stderr, "Another dmeventd daemon is already running\n");
			break;
		case EXIT_DESC_CLOSE_FAILURE:
		case EXIT_DESC_OPEN_FAILURE:
		case EXIT_OPEN_PID_FAILURE:
		case EXIT_FIFO_FAILURE:
		case EXIT_CHDIR_FAILURE:
		default:
			fprintf(stderr, "Child exited with code %d\n", WEXITSTATUS(child_status));
			break;
		}

		exit(WEXITSTATUS(child_status));
	}

	if (chdir("/"))
		exit(EXIT_CHDIR_FAILURE);

	if (getrlimit(RLIMIT_NOFILE, &rlim) < 0)
		fd = 256;	/* just have to guess */
	else
		fd = rlim.rlim_cur;

	for (--fd; fd >= 0; fd--)
		close(fd);

	if ((open("/dev/null", O_RDONLY) < 0) ||
	    (open("/dev/null", O_WRONLY) < 0) ||
	    (open("/dev/null", O_WRONLY) < 0))
		exit(EXIT_DESC_OPEN_FAILURE);

	setsid();
}

static void usage(char *prog, FILE *file)
{
	fprintf(file, "Usage:\n");
	fprintf(file, "%s [Vhd]\n", prog);
	fprintf(file, "\n");
	fprintf(file, "   -V       Show version of dmeventd\n");
	fprintf(file, "   -h       Show this help information\n");
	fprintf(file, "   -d       Don't fork, run in the foreground\n");
	fprintf(file, "\n");
}

int main(int argc, char *argv[])
{
	int ret;
	signed char opt;
	struct dm_event_fifos fifos;
	//struct sys_log logdata = {DAEMON_NAME, LOG_DAEMON};

	opterr = 0;
	optind = 0;

	while ((opt = getopt(argc, argv, "?hVd")) != EOF) {
		switch (opt) {
		case 'h':
			usage(argv[0], stdout);
			exit(0);
		case '?':
			usage(argv[0], stderr);
			exit(0);
		case 'd':
			_debug++;
			break;
		case 'V':
			printf("dmeventd version: %s\n", DM_LIB_VERSION);
			exit(1);
			break;
		}
	}

	if (!_debug)
		_daemonize();

	openlog("dmeventd", LOG_PID, LOG_DAEMON);

	_lock_pidfile();		/* exits if failure */

	/* Set the rest of the signals to cause '_exit_now' to be set */
	signal(SIGINT, &_exit_handler);
	signal(SIGHUP, &_exit_handler);
	signal(SIGQUIT, &_exit_handler);

#ifdef linux
	if (!_set_oom_adj(OOM_DISABLE) && !_set_oom_adj(OOM_ADJUST_MIN))
		syslog(LOG_ERR, "Failed to set oom_adj to protect against OOM killer");
#endif

	_init_thread_signals();

	//multilog_clear_logging();
	//multilog_add_type(std_syslog, &logdata);
	//multilog_init_verbose(std_syslog, _LOG_DEBUG);
	//multilog_async(1);

	_init_fifos(&fifos);

	pthread_mutex_init(&_global_mutex, NULL);

#ifdef MCL_CURRENT
	if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1)
		exit(EXIT_FAILURE);
#endif

	if ((ret = _open_fifos(&fifos)))
		exit(EXIT_FIFO_FAILURE);

	/* Signal parent, letting them know we are ready to go. */
	kill(getppid(), SIGTERM);
	syslog(LOG_NOTICE, "dmeventd ready for processing.");

	while (!_exit_now) {
		_process_request(&fifos);
		_cleanup_unused_threads();
		if (!dm_list_empty(&_thread_registry)
		    || !dm_list_empty(&_thread_registry_unused))
			_thread_registries_empty = 0;
		else
			_thread_registries_empty = 1;
	}

	_exit_dm_lib();

#ifdef MCL_CURRENT
	munlockall();
#endif
	pthread_mutex_destroy(&_global_mutex);

	syslog(LOG_NOTICE, "dmeventd shutting down.");
	closelog();

	exit(EXIT_SUCCESS);
}
