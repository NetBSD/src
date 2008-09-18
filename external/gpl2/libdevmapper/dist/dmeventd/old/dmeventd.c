/*
 * Copyright (C) 2005 Red Hat, Inc. All rights reserved.
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
 *
 * Author - Heinz Mauelshagen, Red Hat GmbH.
 */

#include "libdevmapper.h"
#include "libdm-event.h"
#include "list.h"

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <libdevmapper.h>
#include <libgen.h>
#include <malloc.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "libmultilog.h"


#define	dbg_malloc(x...)	malloc(x)
#define	dbg_strdup(x...)	strdup(x)
#define	dbg_free(x...)		free(x)

/* List (un)link macros. */
#define	LINK(x, head)		list_add(&(x)->list, head)
#define	LINK_DSO(dso)		LINK(dso, &dso_registry)
#define	LINK_THREAD(thread)	LINK(thread, &thread_registry)

#define	UNLINK(x)		list_del(&(x)->list)
#define	UNLINK_DSO(x)		UNLINK(x)
#define	UNLINK_THREAD(x)	UNLINK(x)

#define DAEMON_NAME  "dmeventd"

/* Global mutex for list accesses. */
static pthread_mutex_t mutex;

/* Data kept about a DSO. */
struct dso_data {
	struct list list;

	char *dso_name; /* DSO name (eg, "evms", "dmraid", "lvm2"). */

	void *dso_handle; /* Opaque handle as returned from dlopen(). */
	unsigned int ref_count; /* Library reference count. */

	/*
	 * Event processing.
	 *
	 * The DSO can do whatever appropriate steps if an event happens
	 * such as changing the mapping in case a mirror fails, update
	 * the application metadata etc.
	 */
	void (*process_event)(char *device, enum event_type event);

	/*
	 * Device registration.
	 *
	 * When an application registers a device for an event, the DSO
	 * can carry out appropriate steps so that a later call to
	 * the process_event() function is sane (eg, read metadata
	 * and activate a mapping).
	 */
	int (*register_device)(char *device);

	/*
	 * Device unregistration.
	 *
	 * In case all devices of a mapping (eg, RAID10) are unregistered
	 * for events, the DSO can recognize this and carry out appropriate
	 * steps (eg, deactivate mapping, metadata update).
	 */
	int (*unregister_device)(char *device);
};
static LIST_INIT(dso_registry);

/* Structure to keep parsed register variables from client message. */
struct message_data {
	char *dso_name;		/* Name of DSO. */
	char *device_path;	/* Mapped device path. */
	union {
		char *str;	/* Events string as fetched from message. */
		enum event_type field;	/* Events bitfield. */
	} events;
	struct daemon_message *msg;	/* Pointer to message buffer. */
};

/*
 * Housekeeping of thread+device states.
 *
 * One thread per mapped device which can block on it until an event
 * occurs and the event processing function of the DSO gets called.
 */
struct thread_status {
	struct list	list;

	pthread_t	thread;

	struct dso_data *dso_data;/* DSO this thread accesses. */
	
	char *device_path;	/* Mapped device path. */
	enum event_type events;	/* bitfield for event filter. */
	enum event_type current_events;/* bitfield for occured events. */
	enum event_type processed_events;/* bitfield for processed events. */
};
static LIST_INIT(thread_registry);

/* Allocate/free the status structure for a monitoring thread. */
static struct thread_status *alloc_thread_status(struct message_data *data,
						 struct dso_data *dso_data)
{
	struct thread_status *ret = (typeof(ret)) dbg_malloc(sizeof(*ret));

	if (ret) {
		memset(ret, 0, sizeof(*ret));
		if ((ret->device_path = dbg_strdup(data->device_path))) {
			ret->dso_data = dso_data;
			ret->events   = data->events.field;
		} else {
			dbg_free(ret);
			ret = NULL;
		}
	}

	return ret;
}

static void free_thread_status(struct thread_status *thread)
{
	dbg_free(thread->device_path);
	dbg_free(thread);
}

/* Allocate/free DSO data. */
static struct dso_data *alloc_dso_data(struct message_data *data)
{
	struct dso_data *ret = (typeof(ret)) dbg_malloc(sizeof(*ret));

	if (ret) {
		memset(ret, 0, sizeof(*ret));
		if (!(ret->dso_name = dbg_strdup(data->dso_name))) {
			dbg_free(ret);
			ret = NULL;
		}
	}

	return ret;
}

static void free_dso_data(struct dso_data *data)
{
	dbg_free(data->dso_name);
	dbg_free(data);
}

/* Fetch a string off src and duplicate it into *dest. */
/* FIXME: move to separate module to share with the client lib. */
static const char delimiter = ' ';
static char *fetch_string(char **src)
{
	char *p, *ret;

	if ((p = strchr(*src, delimiter)))
		*p = 0;

	if ((ret = strdup(*src)))
		*src += strlen(ret) + 1;

	if (p)
		*p = delimiter;

	return ret;
}

/* Free message memory. */
static void free_message(struct message_data *message_data)
{
	if (message_data->dso_name)
		dbg_free(message_data->dso_name);

	if (message_data->device_path)
		dbg_free(message_data->device_path);
}

/* Parse a register message from the client. */
static int parse_message(struct message_data *message_data)
{
	char *p = message_data->msg->msg;

log_print("%s: here\n", __func__);
fflush(stdout);

	/*
	 * Retrieve application identifier, mapped device
	 * path and events # string from message.
	 */
	if ((message_data->dso_name    = fetch_string(&p)) &&
	    (message_data->device_path = fetch_string(&p)) &&
	    (message_data->events.str  = fetch_string(&p))) {
log_print("%s: %s %s %s\n", __func__, message_data->dso_name, message_data->device_path, message_data->events.str);
		if (message_data->events.str) {
			enum event_type i = atoi(message_data->events.str);

			/*
			 * Free string representaion of events.
			 * Not needed an more.
			 */
			dbg_free(message_data->events.str);
			message_data->events.field = i;
		}

		return 1;
	}

	free_message(message_data);

	return 0;
};

/* Global mutex to lock access to lists et al. */
static int lock_mutex(void)
{
	return pthread_mutex_lock(&mutex);
}

static int unlock_mutex(void)
{
	return pthread_mutex_unlock(&mutex);
}

/* Store pid in pidfile. */
static int storepid(int lf)
{
	int len;
	char pid[8];

	if ((len = snprintf(pid, sizeof(pid), "%u\n", getpid())) < 0)
		return 0;

	if (len > sizeof(pid))
		len = sizeof(pid);

	if (write(lf, pid, len) != len)
		return 0;

	fsync(lf);

	return 1;
}

/*
 * create+flock file.
 *
 * Used to synchronize daemon startups.
 */
static int lf = -1;
static char pidfile[] = "/var/run/dmeventd.pid";

/* Store pid in pidfile. */
static int lock(void)
{
	/* Already locked. */
	if (lf > -1)
		return 1;

	if ((lf = open(pidfile, O_CREAT | O_RDWR, 0644)) == -1) {
		log_err("opening pid file\n");
		return 0;
	}

	if (flock(lf, LOCK_EX | LOCK_NB) == -1) {
		log_err("lock pid file\n");
		close(lf);
		lf = -1;
		return 0;
	}

	return 1;
}

static void unlock(void)
{
	/* Not locked! */
	if (lf == -1)
		return;

	if (flock(lf, LOCK_UN))
		log_err("flock unlock %s\n", pidfile);

	if (close(lf))
		log_err("close %s\n", pidfile);

	lf = -1;
}

/* Check, if a device exists. */
static int device_exists(char *device)
{
	struct stat st_buf;

	return !stat(device, &st_buf) && S_ISBLK(st_buf.st_mode);
}

/*
 * Find an existing thread for a device.
 *
 * Mutex must be hold when calling this.
 */
static struct thread_status *lookup_thread_status(struct message_data *data)
{
	struct thread_status *thread;

	list_iterate_items(thread, &thread_registry) {
		if (!strcmp(data->device_path, thread->device_path))
			return thread;
	}

	return NULL;
}


/* Cleanup at exit. */
static void exit_dm_lib(void)
{
	dm_lib_release();
	dm_lib_exit();
}

/* Derive error case from target parameter string. */
static int error_detected(struct thread_status *thread, char *params)
{
	size_t len;

	if ((len = strlen(params)) &&
	    params[len - 1] == 'F') {
		thread->current_events |= DEVICE_ERROR;
		return 1;
	}

	return 0;
}

/* Wait on a device until an event occurs. */
static int event_wait(struct thread_status *thread)
{
	int ret = 0;
	void *next = NULL;
	char *params, *target_type;
	uint64_t start, length;
	struct dm_task *dmt;

	if (!(dmt = dm_task_create(DM_DEVICE_WAITEVENT)))
		return 0;

	if ((ret = dm_task_set_name(dmt, basename(thread->device_path))) &&
	    (ret = dm_task_set_event_nr(dmt, 0)) &&
	    (ret = dm_task_run(dmt))) {
		do {
			/* Retrieve next target. */
			params = NULL;
			next = dm_get_next_target(dmt, next, &start, &length,
						  &target_type, &params);

log_print("%s: %s\n", __func__, params);
			if ((ret = error_detected(thread, params)))
				break;
		} while(next);
	}

	dm_task_destroy(dmt);

	return ret;
}

/* Register a device with the DSO. */
static int do_register_device(struct thread_status *thread)
{
	return thread->dso_data->register_device(thread->device_path);
}

/* Unregister a device with the DSO. */
static int do_unregister_device(struct thread_status *thread)
{
	return thread->dso_data->unregister_device(thread->device_path);
}

/* Process an event the DSO. */
static void do_process_event(struct thread_status *thread)
{
	thread->dso_data->process_event(thread->device_path,
					thread->current_events);
}

/* Thread cleanup handler to unregister device. */
static void monitor_unregister(void *arg)
{
	struct thread_status *thread = arg;

	if (!do_unregister_device(thread))
		log_err("%s: %s unregister failed\n", __func__,
			thread->device_path);
}

/* Device monitoring thread. */
static void *monitor_thread(void *arg)
{
	struct thread_status *thread = arg;

	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
	pthread_cleanup_push(monitor_unregister, thread);

	/* Wait for comm_thread() to finish its task. */
	lock_mutex();
	unlock_mutex();

	/* Loop forever awaiting/analyzing device events. */
	while (1) {
		thread->current_events = 0;

		if (!event_wait(thread))
			continue;

/* REMOVEME: */
log_print("%s: cycle on %s\n", __func__, thread->device_path);

		/*
		 * Check against filter.
		 *
		 * If there's current events delivered from event_wait() AND
		 * the device got registered for those events AND
		 * those events haven't been processed yet, call
		 * the DSO's process_event() handler.
		 */
		if (thread->events &
		    thread->current_events &
		    ~thread->processed_events) {
			do_process_event(thread);
			thread->processed_events |= thread->current_events;
		}
	}

	pthread_cleanup_pop(0);
}

/* Create a device monitoring thread. */
/* FIXME: call this with mutex hold ? */
static int create_thread(struct thread_status *thread)
{
	return pthread_create(&thread->thread, NULL, monitor_thread, thread);
}

static int terminate_thread(struct thread_status *thread)
{
	return pthread_cancel(thread->thread);
}

/* DSO reference counting. */
static void lib_get(struct dso_data *data)
{
	data->ref_count++;
}

static void lib_put(struct dso_data *data)
{
	if (!--data->ref_count) {
		dlclose(data->dso_handle);
		UNLINK_DSO(data);
		free_dso_data(data);
	}
}

/* Find DSO data. */
static struct dso_data *lookup_dso(struct message_data *data)
{
	struct dso_data *dso_data, *ret = NULL;

	lock_mutex();

	list_iterate_items(dso_data, &dso_registry) {
		if (!strcmp(data->dso_name, dso_data->dso_name)) {
			lib_get(dso_data);
			ret = dso_data;
			break;
		}
	}

	unlock_mutex();

	return ret;
}

/* Lookup DSO symbols we need. */
static int lookup_symbol(void *dl, struct dso_data *data,
			 void **symbol, const char *name)
{
	if ((*symbol = dlsym(dl, name)))
		return 1;

	log_err("looking up %s symbol in %s\n", name, data->dso_name);

	return 0;
}

static int lookup_symbols(void *dl, struct dso_data *data)
{
	return lookup_symbol(dl, data, (void*) &data->process_event,
			     "process_event") &&
	       lookup_symbol(dl, data, (void*) &data->register_device,
			     "register_device") &&
	       lookup_symbol(dl, data, (void*) &data->unregister_device,
			     "unregister_device");
}

/* Create a DSO file name based on its name. */
static char *create_dso_file_name(char *dso_name)
{
	char *ret;
	static char prefix[] = "libdmeventd";
	static char suffix[] = ".so";

	if ((ret = dbg_malloc(strlen(prefix) +
			      strlen(dso_name) +
			      strlen(suffix) + 1)))
		sprintf(ret, "%s%s%s", prefix, dso_name, suffix);

log_print("%s: \"%s\"\n", __func__, ret);

	return ret;
}

/* Load an application specific DSO. */
static struct dso_data *load_dso(struct message_data *data)
{
	void *dl;
	struct dso_data *ret = NULL;
	char *dso_file;

log_print("%s: \"%s\"\n", __func__, data->dso_name);

	if (!(dso_file = create_dso_file_name(data->dso_name)))
		return NULL;

	if (!(dl = dlopen(dso_file, RTLD_NOW)))
		goto free_dso_file;

	if (!(ret = alloc_dso_data(data)))
		goto close;

	if (!(lookup_symbols(dl, ret)))
		goto free_all;

	/*
	 * Keep handle to close the library once
	 * we've got no references to it any more.
	 */
	ret->dso_handle = dl;
	lib_get(ret);

	lock_mutex();
	LINK_DSO(ret);
	unlock_mutex();

	goto free_dso_file;

   free_all:
	free_dso_data(ret);

   close:
	dlclose(dl);

   free_dso_file:
	dbg_free(dso_file);

	return ret;
}


/*
 * Register for an event.
 *
 * Only one caller at a time here, because we use a FIFO and lock
 * it against multiple accesses.
 */
static int register_for_event(struct message_data *message_data)
{
	int ret = 0;
	struct thread_status *thread, *thread_new;
	struct dso_data *dso_data;

	if (!device_exists(message_data->device_path)) {
		stack;
		ret = -ENODEV;
		goto out;
	}
log_print("%s\n", __func__);
fflush(stdout);

	if (!(dso_data = lookup_dso(message_data)) &&
	    !(dso_data = load_dso(message_data))) {
		stack;
		ret = -ELIBACC;
		goto out;
	}
		
	/* Preallocate thread status struct to avoid deadlock. */
	if (!(thread_new = alloc_thread_status(message_data, dso_data))) {
		stack;
		ret = -ENOMEM;
		goto out;
	}

	if (!(ret = do_register_device(thread_new)))
			goto out;

	lock_mutex();

	if ((thread = lookup_thread_status(message_data)))
		ret = -EPERM;
	else {
		thread = thread_new;
		thread_new = NULL;

		/* Try to create the monitoring thread for this device. */
		if ((ret = -create_thread(thread))) {
			unlock_mutex();
			free_thread_status(thread);
			goto out;
		} else
			LINK_THREAD(thread);
	}

	/* Or event # into events bitfield. */
	thread->events |= message_data->events.field;

	unlock_mutex();

	/*
	 * Deallocate thread status after releasing
	 * the lock in case we haven't used it.
	 */
	if (thread_new)
		free_thread_status(thread_new);

   out:
	free_message(message_data);

	return ret;
}

/*
 * Unregister for an event.
 *
 * Only one caller at a time here as with register_for_event().
 */
static int unregister_for_event(struct message_data *message_data)
{
	int ret = 0;
	struct thread_status *thread;

	/*
	 * Clear event in bitfield and deactivate
	 * monitoring thread in case bitfield is 0.
	 */
	lock_mutex();

	if (!(thread = lookup_thread_status(message_data))) {
		unlock_mutex();
		ret = -ESRCH;
		goto out;
	}

	thread->events &= ~message_data->events.field;

	/*
	 * In case there's no events to monitor on this
	 * device -> unlink and terminate its monitoring thread.
	 */
	if (!thread->events)
		UNLINK_THREAD(thread);

	unlock_mutex();

	if (!thread->events) {
		/* turn codes negative */
		if ((ret = -terminate_thread(thread))) {
			stack;
		} else {
			pthread_join(thread->thread, NULL);
			lib_put(thread->dso_data);
			free_thread_status(thread);

			lock_mutex();
			if (list_empty(&thread_registry))
				exit_dm_lib();
			unlock_mutex();
		}
	}


   out:
	free_message(message_data);

	return ret;
}

/*
 * Get next registered device.
 *
 * Only one caller at a time here as with register_for_event().
 */
static int registered_device(struct message_data *message_data,
			     struct thread_status *thread)
{
	struct daemon_message *msg = message_data->msg;

	snprintf(msg->msg, sizeof(msg->msg), "%s %s %u", 
		 thread->dso_data->dso_name, thread->device_path,
		 thread->events);

	unlock_mutex();

	return 0;
}

static int get_registered_device(struct message_data *message_data, int next)
{
	int dev, dso, hit = 0;
	struct thread_status *thread;

	lock_mutex();

	thread = list_item(thread_registry.n, struct thread_status);

	if (!message_data->dso_name &&
	    !message_data->device_path)
		goto out;
		

	list_iterate_items(thread, &thread_registry) {
		dev = dso = 0;

log_print("%s: working %s %s %u\n", __func__, thread->dso_data->dso_name, thread->device_path, thread->events);
		/* If DSO name equals. */
		if (message_data->dso_name &&
		    !strcmp(message_data->dso_name,
			    thread->dso_data->dso_name))
			dso = 1;

		/* If dev path equals. */
		if (message_data->device_path &&
		    !strcmp(message_data->device_path,
			    thread->device_path))
			dev = 1;

		/* We've got both DSO name and device patch or either. */
		/* FIXME: wrong logic! */
		if (message_data->dso_name && message_data->device_path &&
		    dso && dev)
			hit = 1;
		else if (message_data->dso_name && dso)
			hit = 1;
		else if (message_data->device_path &&
			 dev)
			hit = 1;

		if (hit)
{log_print("%s: HIT %s %s %u\n", __func__, thread->dso_data->dso_name, thread->device_path, thread->events);
			break;
}
	}

	/*
	 * If we got a registered device and want the next one ->
	 * fetch next element off the list.
	 */
	if (hit && next)
		thread = list_item(&thread->list.n, struct thread_status);

   out:
	if (list_empty(&thread->list) ||
	    &thread->list == &thread_registry) {
		unlock_mutex();
		return -ENOENT;
	}

	unlock_mutex();

	return registered_device(message_data, thread);
}

/* Initialize a fifos structure with path names. */
static void init_fifos(struct fifos *fifos)
{
	memset(fifos, 0, sizeof(*fifos));
	fifos->client_path = FIFO_CLIENT;
	fifos->server_path = FIFO_SERVER;
}

/* Open fifos used for client communication. */
static int open_fifos(struct fifos *fifos)
{
	/* Blocks until client is ready to write. */
	if ((fifos->server = open(fifos->server_path, O_WRONLY)) == -1) {
		stack;
		return 0;
	}

	/* Need to open read+write for select() to work. */
        if ((fifos->client = open(fifos->client_path, O_RDWR)) == -1) {
		stack;
		close(fifos->server);
		return 0;
	}

	return 1;
}

/*
 * Read message from client making sure that data is available
 * and a complete message is read.
 */
static int client_read(struct fifos *fifos, struct daemon_message *msg)
{
	int bytes = 0, ret = 0;
	fd_set fds;

	errno = 0;
	while (bytes < sizeof(*msg) && errno != EOF) {
		do {
			/* Watch client read FIFO for input. */
			FD_ZERO(&fds);
			FD_SET(fifos->client, &fds);
		} while (select(fifos->client+1, &fds, NULL, NULL, NULL) != 1);

		ret = read(fifos->client, msg, sizeof(*msg) - bytes);
		bytes += ret > 0 ? ret : 0;
	}

	return bytes == sizeof(*msg);
}

/*
 * Write a message to the client making sure that it is ready to write.
 */
static int client_write(struct fifos *fifos, struct daemon_message *msg)
{
	int bytes = 0, ret = 0;
	fd_set fds;

	errno = 0;
	while (bytes < sizeof(*msg) && errno != EIO) {
		do {
			/* Watch client write FIFO to be ready for output. */
			FD_ZERO(&fds);
			FD_SET(fifos->server, &fds);
		} while (select(fifos->server +1, NULL, &fds, NULL, NULL) != 1);

		ret = write(fifos->server, msg, sizeof(*msg) - bytes);
		bytes += ret > 0 ? ret : 0;
	}

	return bytes == sizeof(*msg);
}

/* Process a request passed from the communication thread. */
static int do_process_request(struct daemon_message *msg)
{
	int ret;
	static struct message_data message_data;

log_print("%s: \"%s\"\n", __func__, msg->msg);
	/* Parse the message. */
	message_data.msg = msg;
	if (msg->opcode.cmd != CMD_ACTIVE &&
	    !parse_message(&message_data)) {
		stack;
fflush(stdout);
		return -EINVAL;
	}

fflush(stdout);

	/* Check the request type. */
	switch (msg->opcode.cmd) {
	case CMD_ACTIVE:
		ret = 0;
		break;
	case CMD_REGISTER_FOR_EVENT:
		ret = register_for_event(&message_data);
		break;
	case CMD_UNREGISTER_FOR_EVENT:
		ret = unregister_for_event(&message_data);
		break;
	case CMD_GET_REGISTERED_DEVICE:
		ret = get_registered_device(&message_data, 0);
		break;
	case CMD_GET_NEXT_REGISTERED_DEVICE:
		ret = get_registered_device(&message_data, 1);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

/* Only one caller at a time. */
static void process_request(struct fifos *fifos, struct daemon_message *msg)
{
	/* Read the request from the client. */
	memset(msg, 0, sizeof(*msg));
	if (!client_read(fifos, msg)) {
		stack;
		return;
	}

	msg->opcode.status = do_process_request(msg);

	memset(&msg->msg, 0, sizeof(msg->msg));
	if (!client_write(fifos, msg))
		stack;
}

/* Communication thread. */
static void comm_thread(struct fifos *fifos)
{
	struct daemon_message msg;

	/* Open fifos (must be created by client). */
	if (!open_fifos(fifos)) {
		stack;
		return;
	}
	
	/* Exit after last unregister. */
	do {
		process_request(fifos, &msg);
	} while (!list_empty(&thread_registry));
}

/* Fork into the background and detach from our parent process. */
static int daemonize(void)
{
	pid_t pid;

	if ((pid = fork()) == -1) {
		log_err("%s: fork", __func__);
		return 0;
	} else if (pid > 0) /* Parent. */
		return 2;

log_print("daemonizing 2nd...\n");

	setsid();
	if (chdir("/")) {
		log_err("%s: chdir /", __func__);
		return 0;
	}

/* REMOVEME: */
	return 1;

	log_print("daemonizing 3rd...\n");

	/* Detach ourself. */
	if (close(STDIN_FILENO) == -1 ||
	    close(STDOUT_FILENO) == -1 ||
	    close(STDERR_FILENO) == -1)
		return 0;

log_print("daemonized\n");

	return 1;
}

/* Init thread signal handling. */
#define HANGUP	SIGHUP
static void init_thread_signals(int hup)
{
	sigset_t sigset;
	
	sigfillset(&sigset);

	if (hup)
		sigdelset(&sigset, HANGUP);

	pthread_sigmask(SIG_SETMASK, &sigset, NULL);
}

int main(void)
{
	struct fifos fifos;
	struct sys_log logdata = {DAEMON_NAME, LOG_DAEMON};
	/* Make sure, parent accepts HANGUP signal. */
	init_thread_signals(1);

	switch (daemonize()) {
	case 1: /* Child. */
		/* Try to lock pidfile. */
		if (!lock()) {
			fprintf(stderr, "daemon already running\n");
			break;
		}

		init_thread_signals(0);
		kill(getppid(), HANGUP);

		multilog_clear_logging();
		multilog_add_type(std_syslog, &logdata);
		multilog_init_verbose(std_syslog, _LOG_DEBUG);
		multilog_async(1);

		init_fifos(&fifos);
		pthread_mutex_init(&mutex, NULL);

		if (!storepid(lf)) {
			stack;
			exit(EXIT_FAILURE);
		}

		if (mlockall(MCL_FUTURE) == -1) {
			stack;
			exit(EXIT_FAILURE);
		}

		/* Communication thread runs forever... */
		comm_thread(&fifos);

		/* We should never get here. */
		munlockall();
		pthread_mutex_destroy(&mutex);

	case 0: /* Error (either on daemonize() or on comm_thread() return. */
		unlock();
		exit(EXIT_FAILURE);
		break;

	case 2: /* Parent. */
		wait(NULL);
		break;
	}

	exit(EXIT_SUCCESS);
}

/*
 * Overrides for Emacs so that we follow Linus's tabbing style.
 * Emacs will notice this stuff at the end of the file and automatically
 * adjust the settings for this buffer only.  This must remain at the end
 * of the file.
 * ---------------------------------------------------------------------------
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
