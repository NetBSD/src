/* $NetBSD: xenbus_xs.c,v 1.5.10.6 2008/01/21 09:40:38 yamt Exp $ */
/******************************************************************************
 * xenbus_xs.c
 *
 * This is the kernel equivalent of the "xs" library.  We don't need everything
 * and we use xenbus_comms for communication.
 *
 * Copyright (C) 2005 Rusty Russell, IBM Corporation
 * 
 * This file may be distributed separately from the Linux kernel, or
 * incorporated into other software packages, subject to the following license:
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xenbus_xs.c,v 1.5.10.6 2008/01/21 09:40:38 yamt Exp $");

#if 0
#define DPRINTK(fmt, args...) \
    printf("xenbus_xs (%s:%d) " fmt ".\n", __func__, __LINE__, ##args)
#else
#define DPRINTK(fmt, args...) ((void)0)
#endif 

#include <sys/types.h>
#include <sys/null.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/mutex.h>
#include <sys/kthread.h>
#include <sys/simplelock.h>

#include <machine/stdarg.h>

#include <xen/xenbus.h>
#include "xenbus_comms.h"

#define streq(a, b) (strcmp((a), (b)) == 0)

struct xs_stored_msg {
	SIMPLEQ_ENTRY(xs_stored_msg) msg_next;

	struct xsd_sockmsg hdr;

	union {
		/* Queued replies. */
		struct {
			char *body;
		} reply;

		/* Queued watch events. */
		struct {
			struct xenbus_watch *handle;
			char **vec;
			unsigned int vec_size;
		} watch;
	} u;
};

struct xs_handle {
	/* A list of replies. Currently only one will ever be outstanding. */
	SIMPLEQ_HEAD(, xs_stored_msg) reply_list;
	struct simplelock reply_lock;
	kmutex_t xs_lock; /* serialize access to xenstore */
	int suspend_spl;

};

static struct xs_handle xs_state;

/* List of registered watches, and a lock to protect it. */
static SLIST_HEAD(, xenbus_watch) watches =
    SLIST_HEAD_INITIALIZER(watches);
static struct simplelock watches_lock = SIMPLELOCK_INITIALIZER;

/* List of pending watch callback events, and a lock to protect it. */
static SIMPLEQ_HEAD(, xs_stored_msg) watch_events =
    SIMPLEQ_HEAD_INITIALIZER(watch_events);
static struct simplelock watch_events_lock = SIMPLELOCK_INITIALIZER;

static int
get_error(const char *errorstring)
{
	unsigned int i;

	for (i = 0; !streq(errorstring, xsd_errors[i].errstring); i++) {
		if (i == (sizeof(xsd_errors) / sizeof(xsd_errors[0]) - 1)) {
			printf(
			       "XENBUS xen store gave: unknown error %s",
			       errorstring);
			return EINVAL;
		}
	}
	return xsd_errors[i].errnum;
}

static void *
read_reply(enum xsd_sockmsg_type *type, unsigned int *len)
{
	struct xs_stored_msg *msg;
	char *body;
	int s;

	simple_lock(&xs_state.reply_lock);
	s = spltty();

	while (SIMPLEQ_EMPTY(&xs_state.reply_list)) {
		ltsleep(&xs_state.reply_list, PRIBIO, "rplq", 0,
		    &xs_state.reply_lock);
	}

	msg = SIMPLEQ_FIRST(&xs_state.reply_list);
	SIMPLEQ_REMOVE_HEAD(&xs_state.reply_list, msg_next);

	splx(s);
	simple_unlock(&xs_state.reply_lock);

	*type = msg->hdr.type;
	if (len)
		*len = msg->hdr.len;
	body = msg->u.reply.body;
	DPRINTK("read_reply: type %d body %s\n",
	    msg->hdr.type, body);

	free(msg, M_DEVBUF);

	return body;
}

#if 0
/* Emergency write. */
void
xenbus_debug_write(const char *str, unsigned int count)
{
	struct xsd_sockmsg msg = { 0 };

	msg.type = XS_DEBUG;
	msg.len = sizeof("print") + count + 1;

	xb_write(&msg, sizeof(msg));
	xb_write("print", sizeof("print"));
	xb_write(str, count);
	xb_write("", 1);
}
#endif

int
xenbus_dev_request_and_reply(struct xsd_sockmsg *msg, void**reply)
{
	int err = 0, s;

	s = spltty();
	mutex_enter(&xs_state.xs_lock);
	err = xb_write(msg, sizeof(*msg) + msg->len);
	if (err) {
		msg->type = XS_ERROR;
		*reply = NULL;
	} else {
		*reply = read_reply(&msg->type, &msg->len);
	}
	mutex_exit(&xs_state.xs_lock);
	splx(s);

	return err;
}

/* Send message to xs, get kmalloc'ed reply.  ERR_PTR() on error. */
static int
xs_talkv(struct xenbus_transaction *t,
		      enum xsd_sockmsg_type type,
		      const struct iovec *iovec,
		      unsigned int num_vecs,
		      unsigned int *len,
		      char **retbuf)
{
	struct xsd_sockmsg msg;
	unsigned int i;
	int err, s;
	void *ret;

	msg.tx_id = (uint32_t)(unsigned long)t;
	msg.req_id = 0;
	msg.type = type;
	msg.len = 0;
	for (i = 0; i < num_vecs; i++)
		msg.len += iovec[i].iov_len;

	s = spltty();
	mutex_enter(&xs_state.xs_lock);

	DPRINTK("write msg");
	err = xb_write(&msg, sizeof(msg));
	DPRINTK("write msg err %d", err);
	if (err) {
		mutex_exit(&xs_state.xs_lock);
		splx(s);
		return (err);
	}

	for (i = 0; i < num_vecs; i++) {
		DPRINTK("write iovect");
		err = xb_write(iovec[i].iov_base, iovec[i].iov_len);;
		DPRINTK("write iovect err %d", err);
		if (err) {
			mutex_exit(&xs_state.xs_lock);
			splx(s);
			return (err);
		}
	}

	DPRINTK("read");
	ret = read_reply(&msg.type, len);
	DPRINTK("read done");

	mutex_exit(&xs_state.xs_lock);
	splx(s);

	if (msg.type == XS_ERROR) {
		err = get_error(ret);
		free(ret, M_DEVBUF);
		return (err);
	}

	KASSERT(msg.type == type);
	if (retbuf != NULL)
		*retbuf = ret;
	else
		free(ret, M_DEVBUF);
	return 0;
}

/* Simplified version of xs_talkv: single message. */
static int
xs_single(struct xenbus_transaction *t,
		       enum xsd_sockmsg_type type,
		       const char *string,
		       unsigned int *len,
		       char **ret)
{
	struct iovec iovec;

	/* xs_talkv only reads iovec */
	iovec.iov_base = __UNCONST(string);
	iovec.iov_len = strlen(string) + 1;
	return xs_talkv(t, type, &iovec, 1, len, ret);
}

static unsigned int
count_strings(const char *strings, unsigned int len)
{
	unsigned int num;
	const char *p;

	for (p = strings, num = 0; p < strings + len; p += strlen(p) + 1)
		num++;

	return num;
}

/* Return the path to dir with /name appended. Buffer must be kfree()'ed. */ 
static char *
join(const char *dir, const char *name)
{
	char *buffer;

	buffer = malloc(strlen(dir) + strlen("/") + strlen(name) + 1,
			 M_DEVBUF, M_NOWAIT);
	if (buffer == NULL)
		return NULL;

	strcpy(buffer, dir);
	if (!streq(name, "")) {
		strcat(buffer, "/");
		strcat(buffer, name);
	}

	return buffer;
}

static char **
split(char *strings, unsigned int len, unsigned int *num)
{
	char *p, **ret;

	/* Count the strings. */
	*num = count_strings(strings, len);

	/* Transfer to one big alloc for easy freeing. */
	ret = malloc(*num * sizeof(char *) + len, M_DEVBUF, M_NOWAIT);
	if (!ret) {
		free(strings, M_DEVBUF);
		return NULL;
	}
	memcpy(&ret[*num], strings, len);
	free(strings, M_DEVBUF);

	strings = (char *)&ret[*num];
	for (p = strings, *num = 0; p < strings + len; p += strlen(p) + 1)
		ret[(*num)++] = p;

	return ret;
}

int
xenbus_directory(struct xenbus_transaction *t,
			const char *dir, const char *node, unsigned int *num,
			char ***retbuf)
{
	char *strings, *path;
	unsigned int len;
	int err;

	path = join(dir, node);
	if (path == NULL) 
		return ENOMEM;

	err = xs_single(t, XS_DIRECTORY, path, &len, &strings);
	DPRINTK("xs_single %d %d", err, len);
	free(path, M_DEVBUF);
	if (err)
		return err;

	DPRINTK("xs_single strings %s", strings);
	*retbuf = split(strings, len, num);
	if (*retbuf == NULL)
		return ENOMEM;
	return 0;
}

/* Check if a path exists. Return 1 if it does. */
int
xenbus_exists(struct xenbus_transaction *t,
		  const char *dir, const char *node)
{
	char **d;
	int dir_n, err;

	err = xenbus_directory(t, dir, node, &dir_n, &d);
	if (err)
		return 0;
	free(d, M_DEVBUF);
	return 1;
}

/* Get the value of a single file.
 * Returns a kmalloced value: call free() on it after use.
 * len indicates length in bytes.
 */
int
xenbus_read(struct xenbus_transaction *t,
		  const char *dir, const char *node, unsigned int *len,
		  char **ret)
{
	char *path;
	int err;

	path = join(dir, node);
	if (path == NULL)
		return ENOMEM;

	err = xs_single(t, XS_READ, path, len, ret);
	free(path, M_DEVBUF);
	return err;
}

/* Read a node and convert it to unsigned long. */
int
xenbus_read_ul(struct xenbus_transaction *t,
		  const char *dir, const char *node, unsigned long *val,
		  int base)
{
	char *string, *ep;
	int err;

	err = xenbus_read(t, dir, node, NULL, &string);
	if (err)
		return err;
	*val = strtoul(string, &ep, base);
	if (*ep != '\0') {
		free(string, M_DEVBUF);
		return EFTYPE;
	}
	free(string, M_DEVBUF);
	return 0;
}

/* Write the value of a single file.
 * Returns -err on failure.
 */
int
xenbus_write(struct xenbus_transaction *t,
		 const char *dir, const char *node, const char *string)
{
	const char *path;
	struct iovec iovec[2];
	int ret;

	path = join(dir, node);
	if (path == NULL)
		return ENOMEM;

	/* xs_talkv only reads iovec */
	iovec[0].iov_base = __UNCONST(path);
	iovec[0].iov_len = strlen(path) + 1;
	iovec[1].iov_base = __UNCONST(string);
	iovec[1].iov_len = strlen(string);

	ret = xs_talkv(t, XS_WRITE, iovec, 2, NULL, NULL);
	return ret;
}

/* Create a new directory. */
int
xenbus_mkdir(struct xenbus_transaction *t,
		 const char *dir, const char *node)
{
	char *path;
	int ret;

	path = join(dir, node);
	if (path == NULL)
		return ENOMEM;

	ret = xs_single(t, XS_MKDIR, path, NULL, NULL);
	free(path, M_DEVBUF);
	return ret;
}

/* Destroy a file or directory (directories must be empty). */
int xenbus_rm(struct xenbus_transaction *t, const char *dir, const char *node)
{
	char *path;
	int ret;

	path = join(dir, node);
	if (path == NULL)
		return ENOMEM;

	ret = xs_single(t, XS_RM, path, NULL, NULL);
	free(path, M_DEVBUF);
	return ret;
}

/* Start a transaction: changes by others will not be seen during this
 * transaction, and changes will not be visible to others until end.
 * MUST BE CALLED AT IPL_TTY !
 */
struct xenbus_transaction *
xenbus_transaction_start(void)
{
	char *id_str;
	unsigned long id, err;

	err = xs_single(NULL, XS_TRANSACTION_START, "", NULL, &id_str);
	if (err) {
		return NULL;
	}

	id = strtoul(id_str, NULL, 0);
	free(id_str, M_DEVBUF);

	return (struct xenbus_transaction *)id;
}

/* End a transaction.
 * If abandon is true, transaction is discarded instead of committed.
 * MUST BE CALLED AT IPL_TTY !
 */
int xenbus_transaction_end(struct xenbus_transaction *t, int abort)
{
	char abortstr[2];
	int err;

	if (abort)
		strcpy(abortstr, "F");
	else
		strcpy(abortstr, "T");

	err = xs_single(t, XS_TRANSACTION_END, abortstr, NULL, NULL);

	return err;
}

/* Single read and scanf: returns -errno or num scanned. */
int
xenbus_scanf(struct xenbus_transaction *t,
		 const char *dir, const char *node, const char *fmt, ...)
{
	va_list ap;
	int ret;
	char *val;

	ret = xenbus_read(t, dir, node, NULL, &val);
	if (ret)
		return ret;

	va_start(ap, fmt);
	//ret = vsscanf(val, fmt, ap);
	ret = ENXIO;
	printf("xb_scanf format %s in %s\n", fmt, val);
	va_end(ap);
	free(val, M_DEVBUF);
	return ret;
}

/* Single printf and write: returns -errno or 0. */
int
xenbus_printf(struct xenbus_transaction *t,
		  const char *dir, const char *node, const char *fmt, ...)
{
	va_list ap;
	int ret;
#define PRINTF_BUFFER_SIZE 4096
	char *printf_buffer;

	printf_buffer = malloc(PRINTF_BUFFER_SIZE, M_DEVBUF, M_NOWAIT);
	if (printf_buffer == NULL)
		return ENOMEM;

	va_start(ap, fmt);
	ret = vsnprintf(printf_buffer, PRINTF_BUFFER_SIZE, fmt, ap);
	va_end(ap);

	KASSERT(ret < PRINTF_BUFFER_SIZE);
	ret = xenbus_write(t, dir, node, printf_buffer);

	free(printf_buffer, M_DEVBUF);

	return ret;
}

/* Takes tuples of names, scanf-style args, and void **, NULL terminated. */
int
xenbus_gather(struct xenbus_transaction *t, const char *dir, ...)
{
	va_list ap;
	const char *name;
	int ret = 0;

	va_start(ap, dir);
	while (ret == 0 && (name = va_arg(ap, char *)) != NULL) {
		const char *fmt = va_arg(ap, char *);
		void *result = va_arg(ap, void *);
		char *p;

		ret = xenbus_read(t, dir, name, NULL, &p);
		if (ret)
			break;
		if (fmt) {
			// XXX if (sscanf(p, fmt, result) == 0)
				ret = -EINVAL;
			free(p, M_DEVBUF);
		} else
			*(char **)result = p;
	}
	va_end(ap);
	return ret;
}

static int
xs_watch(const char *path, const char *token)
{
	struct iovec iov[2];

	/* xs_talkv only reads iovec */
	iov[0].iov_base = __UNCONST(path);
	iov[0].iov_len = strlen(path) + 1;
	iov[1].iov_base = __UNCONST(token);
	iov[1].iov_len = strlen(token) + 1;

	return xs_talkv(NULL, XS_WATCH, iov, 2, NULL, NULL);
}

static int
xs_unwatch(const char *path, const char *token)
{
	struct iovec iov[2];

	/* xs_talkv only reads iovec */
	iov[0].iov_base = __UNCONST(path);
	iov[0].iov_len = strlen(path) + 1;
	iov[1].iov_base = __UNCONST(token);
	iov[1].iov_len = strlen(token) + 1;

	return xs_talkv(NULL, XS_UNWATCH, iov, 2, NULL, NULL);
}

static struct xenbus_watch *
find_watch(const char *token)
{
	struct xenbus_watch *i, *cmp;

	cmp = (void *)strtoul(token, NULL, 16);

	SLIST_FOREACH(i, &watches, watch_next) {
		if (i == cmp)
			return i;
	}

	return NULL;
}

/* Register callback to watch this node. */
int
register_xenbus_watch(struct xenbus_watch *watch)
{
	/* Pointer in ascii is the token. */
	char token[sizeof(watch) * 2 + 1];
	int err;
	int s;

	sprintf(token, "%lX", (long)watch);

	s = spltty();

	simple_lock(&watches_lock);
	KASSERT(find_watch(token) == 0);
	SLIST_INSERT_HEAD(&watches, watch, watch_next);
	simple_unlock(&watches_lock);

	err = xs_watch(watch->node, token);

	/* Ignore errors due to multiple registration. */
	if ((err != 0) && (err != EEXIST)) {
		simple_lock(&watches_lock);
		SLIST_REMOVE(&watches, watch, xenbus_watch, watch_next);
		simple_unlock(&watches_lock);
	}

	splx(s);

	return err;
}

void
unregister_xenbus_watch(struct xenbus_watch *watch)
{
	struct xs_stored_msg *msg, *next_msg;
	char token[sizeof(watch) * 2 + 1];
	int err, s;

	sprintf(token, "%lX", (long)watch);

	s = spltty();

	simple_lock(&watches_lock);
	KASSERT(find_watch(token));
	SLIST_REMOVE(&watches, watch, xenbus_watch, watch_next);
	simple_unlock(&watches_lock);

	err = xs_unwatch(watch->node, token);
	if (err)
		printf(
		       "XENBUS Failed to release watch %s: %i\n",
		       watch->node, err);

	splx(s);

	/* Cancel pending watch events. */
	simple_lock(&watch_events_lock);
	for (msg = SIMPLEQ_FIRST(&watch_events); msg != NULL; msg = next_msg) {
		next_msg = SIMPLEQ_NEXT(msg, msg_next);
		if (msg->u.watch.handle != watch)
			continue;
		SIMPLEQ_REMOVE(&watch_events, msg, xs_stored_msg, msg_next);
		free(msg->u.watch.vec, M_DEVBUF);
		free(msg, M_DEVBUF);
	}
	simple_unlock(&watch_events_lock);

}

void
xs_suspend(void)
{
	xs_state.suspend_spl = spltty();
}

void
xs_resume(void)
{
	struct xenbus_watch *watch;
	char token[sizeof(watch) * 2 + 1];
	/* No need for watches_lock: the suspend_mutex is sufficient. */
	SLIST_FOREACH(watch, &watches, watch_next) {
		sprintf(token, "%lX", (long)watch);
		xs_watch(watch->node, token);
	}

	splx(xs_state.suspend_spl);
}

static void
xenwatch_thread(void *unused)
{
	struct xs_stored_msg *msg;
	int s;

	for (;;) {
		tsleep(&watch_events, PRIBIO, "evtsq", 0);
		s = spltty(); /* to block IPL_CTRL */
		while (!SIMPLEQ_EMPTY(&watch_events)) {
			msg = SIMPLEQ_FIRST(&watch_events);

			simple_lock(&watch_events_lock);
			SIMPLEQ_REMOVE_HEAD(&watch_events, msg_next);
			simple_unlock(&watch_events_lock);
			DPRINTK("xenwatch_thread: got event\n");

			msg->u.watch.handle->xbw_callback(
				msg->u.watch.handle,
				(const char **)msg->u.watch.vec,
				msg->u.watch.vec_size);
			free(msg->u.watch.vec, M_DEVBUF);
			free(msg, M_DEVBUF);
		}

		splx(s);
	}
}

static int
process_msg(void)
{
	struct xs_stored_msg *msg;
	char *body;
	int err, s;

	msg = malloc(sizeof(*msg), M_DEVBUF, M_NOWAIT);
	if (msg == NULL)
		return ENOMEM;

	err = xb_read(&msg->hdr, sizeof(msg->hdr));
	DPRINTK("xb_read hdr %d", err);
	if (err) {
		free(msg, M_DEVBUF);
		return err;
	}

	body = malloc(msg->hdr.len + 1, M_DEVBUF, M_NOWAIT);
	if (body == NULL) {
		free(msg, M_DEVBUF);
		return ENOMEM;
	}

	err = xb_read(body, msg->hdr.len);
	DPRINTK("xb_read body %d", err);
	if (err) {
		free(body, M_DEVBUF);
		free(msg, M_DEVBUF);
		return err;
	}
	body[msg->hdr.len] = '\0';

	if (msg->hdr.type == XS_WATCH_EVENT) {
		DPRINTK("process_msg: XS_WATCH_EVENT\n");
		msg->u.watch.vec = split(body, msg->hdr.len,
					 &msg->u.watch.vec_size);
		if (msg->u.watch.vec == NULL) {
			free(msg, M_DEVBUF);
			return ENOMEM;
		}

		simple_lock(&watches_lock);
		s = spltty();
		msg->u.watch.handle = find_watch(
			msg->u.watch.vec[XS_WATCH_TOKEN]);
		if (msg->u.watch.handle != NULL) {
			simple_lock(&watch_events_lock);
			SIMPLEQ_INSERT_TAIL(&watch_events, msg, msg_next);
			wakeup(&watch_events);
			simple_unlock(&watch_events_lock);
		} else {
			free(msg->u.watch.vec, M_DEVBUF);
			free(msg, M_DEVBUF);
		}
		splx(s);
		simple_unlock(&watches_lock);
	} else {
		DPRINTK("process_msg: type %d body %s\n", msg->hdr.type, body);
		    
		msg->u.reply.body = body;
		simple_lock(&xs_state.reply_lock);
		s = spltty();
		SIMPLEQ_INSERT_TAIL(&xs_state.reply_list, msg, msg_next);
		splx(s);
		simple_unlock(&xs_state.reply_lock);
		wakeup(&xs_state.reply_list);
	}

	return 0;
}

static void
xenbus_thread(void *unused)
{
	int err;

	for (;;) {
		err = process_msg();
		if (err)
			printk("XENBUS error %d while reading message\n", err);
	}
}

int
xs_init(void)
{
	int err;

	SIMPLEQ_INIT(&xs_state.reply_list);
	simple_lock_init(&xs_state.reply_lock);
	mutex_init(&xs_state.xs_lock, MUTEX_DEFAULT, IPL_NONE);

	err = kthread_create(PRI_NONE, 0, NULL, xenwatch_thread,
	    NULL, NULL, "xenwatch");
	if (err)
		printf("kthread_create(xenwatch): %d\n", err);

	err = kthread_create(PRI_NONE, 0, NULL, xenbus_thread,
	    NULL, NULL, "xenbus");
	if (err)
		printf("kthread_create(xenbus): %d\n", err);

	return 0;
}

/*
 * Local variables:
 *  c-file-style: "linux"
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
