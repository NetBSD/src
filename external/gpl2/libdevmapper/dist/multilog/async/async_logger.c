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

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <sys/param.h>
#include <sys/select.h>
#include "libmultilog.h"
#include "async_logger.h"
#include "multilog_internal.h"

#include <unistd.h>

#define CBUFSIZE 1024 * 8
#define MAX_FILELEN 256
#define MAX_LINELEN 10

struct circ_buf {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	char buf[CBUFSIZE];
	int read_pos;
	int write_pos;
	int should_sig:1;
	int initialized:1;
};

/*
 * Need a circular buffer to hold the messages - this will
 * have to be global so all threads can see it.
 */
static struct circ_buf cbuf = {
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.cond = PTHREAD_COND_INITIALIZER,
	.buf = {0},
	.read_pos = 0,
	.write_pos = 0,
	.should_sig = 0,
	.initialized = 0,
};

static int will_overrun(int read_pos, int write_pos,
			int first_len, int second_len) {

	return (((read_pos > write_pos) &&
		 (write_pos + first_len >= read_pos)) ||
		((read_pos < write_pos) &&
		 (second_len >= read_pos)));
}

/*
 * Messages are encoded in the buffer with the following format:
 *
 * priority:file:line:msg
 */
void write_to_buf(int priority, const char *file, int line,
		  const char *string)
{
	char buf[CBUFSIZE];
	char *curpos = buf;
	int written_len;
	int buf_len;
	int first_len;
	int second_len;
	int ret = 0;

	/* The first character in a message is its priority */
	sprintf(curpos, "%u", priority);
	curpos++;

	written_len = snprintf(curpos, CBUFSIZE-1, ":%s:%d:",
			       file, line);

	/*
	 * file, func, line were too long for the buffer,
	 * so leave them blank.
	 */
	if (written_len >= (CBUFSIZE-1)) {
		sprintf(curpos, ":::");
		written_len = 3;
	}

	curpos += written_len;

	/*
	 * write message to circular buffer - what kind of delimiter
	 * do we want? Is NULL ok?
	 */
	strncpy(curpos, string, CBUFSIZE - written_len);

	/* FIXME: should we warn somehow if the buffer was truncated? */
	buf[CBUFSIZE-1] = '\0';

	/*
	 * FIXME - if the message is too long for the buffer, should
	 * we retry without the filename, function and line number?
	 */
	buf_len = strlen(buf) + 1;

	pthread_mutex_lock(&cbuf.mutex);

	/* Need to split the write into parts */
	if ((cbuf.write_pos + buf_len) >= CBUFSIZE) {
		first_len = CBUFSIZE - (cbuf.write_pos);
		second_len = buf_len - first_len;

		/* Protect from overrun */
		if (!will_overrun(cbuf.read_pos, cbuf.write_pos,
				  first_len, second_len)) {
			strncpy(&cbuf.buf[cbuf.write_pos], buf, first_len);
			strncpy(&cbuf.buf[0], buf + first_len, second_len);
			cbuf.write_pos = second_len;
			ret = 1;
		} else
			ret = 0;
	} else {
		/* Protect from overrun */
		if (!will_overrun(cbuf.read_pos, cbuf.write_pos, buf_len, -1)) {
			strncpy(&cbuf.buf[cbuf.write_pos], buf, buf_len);
			cbuf.write_pos += buf_len;
			ret = 1;
		} else
			ret = 0;
	}

	/*
	 * We don't need to signal the syslog thread
	 * when it's writing to syslog.
	 */
	if (cbuf.should_sig)
		pthread_cond_signal(&cbuf.cond);

	pthread_mutex_unlock(&cbuf.mutex);
}

static int read_from_buf(int *priority, char *file, int *line,
			 char *msg, int len)
{
	char *pos;
	int first_len;
	char pbuf[MAX_LINELEN];
	char tmpbuf[CBUFSIZE] = {0};
	char *curpos = &cbuf.buf[cbuf.read_pos];

	/*
	 * Pull the entire message into a temporary
	 * buffer first to handle wraparound cleanly
	 */
	first_len = CBUFSIZE - cbuf.read_pos;
	strncpy(tmpbuf, &cbuf.buf[cbuf.read_pos], first_len);

	if (tmpbuf[first_len - 1] != '\0') {
		/* FIXME: Make sure we're null terminated */
		strncpy(tmpbuf+first_len, &cbuf.buf[0], cbuf.read_pos);
		cbuf.read_pos = strlen(&cbuf.buf[0]) + 1;
	} else
		cbuf.read_pos += strlen(tmpbuf) + 1;

	/* Parse the message for various fields */
	curpos = tmpbuf;

	if (!(pos = strchr(curpos, ':')))
		return 0;

	pbuf[0] = curpos[0];
	pbuf[1] = '\0';
	sscanf(pbuf, "%d", priority);
	curpos = pos;

	if (!(pos = strchr(curpos+1, ':')))
		return 0;

	if (pos != curpos + 1) {
		strncpy(file, curpos + 1, MAX_FILELEN - 1);
		file[pos - (curpos + 1)] = '\0';
	}

	curpos = pos;

	if (!(pos = strchr(curpos+1, ':')))
		return 0;

	if (pos != curpos + 1) {
		strncpy(pbuf, curpos + 1, MIN(MAX_LINELEN, pos - (curpos + 1)));
		sscanf(pbuf, "%d", line);
	} else
		*line = -1;

	curpos = pos + 1;
	strncpy(msg, curpos, MIN(len, (CBUFSIZE - (curpos - tmpbuf))));

	//cbuf.read_pos += strlen(msg) + (curpos - tmpbuf) + 1;

	return 1;
}

static void finish_processing(void *arg)
{
	int priority;
	char file[256];
	int line;
	char mybuf[CBUFSIZE] = {0};

	while(cbuf.write_pos != cbuf.read_pos) {
		/* Call read_from_buf, unlock mutex, and write to syslog. */
		read_from_buf(&priority, file, &line, mybuf, CBUFSIZE);

		/*
		 * Unlock the mutex before writing to syslog
		 * so we don't block the writer threads - but
		 * then how to we handle pthread_cond_signal?
		 */
		pthread_mutex_unlock(&cbuf.mutex);
		logit(priority, file, line, mybuf);
		pthread_mutex_lock(&cbuf.mutex);
	}

	pthread_mutex_unlock(&cbuf.mutex);
}

/*
 * Handle logging to syslog through circular buffer so that syslog
 * blocking doesn't hold anyone up.
 */
static void *process_log(void *arg)
{
	char mybuf[CBUFSIZE] = {0};
	int priority;
	char file[256];
	int line;

	/* We can lock this because cbuf is initialized at creation time */
	pthread_mutex_lock(&cbuf.mutex);

	/* FIXME: Should I return an error code or something here? */
	/* Prevent multiple process_syslog threads from starting. */
	if (cbuf.initialized)
		pthread_exit(NULL);

	cbuf.initialized = 1;
	pthread_cleanup_push(finish_processing, NULL);

	while (1) {
		/* check write_pos & read_pos */
		if (cbuf.write_pos != cbuf.read_pos) {
			/*
			 * Call read_from_buf, unlock mutex,
			 * and write to syslog.
			 */
			read_from_buf(&priority, file, &line, mybuf, CBUFSIZE);

			/*
			 * Unlock the mutex before writing to syslog
			 * so we don't block the writer threads.
			 */
			pthread_mutex_unlock(&cbuf.mutex);
			logit(priority, file, line, mybuf);
/*			syslog(priority, "%s", mybuf);*/
			pthread_mutex_lock(&cbuf.mutex);
		} else {
			/*
			 * Set the should_sig var before waiting so
			 * the threads using the log know to signal it
			 * to wake up after adding to the log.
			 */
			cbuf.should_sig = 1;
			pthread_cond_wait(&cbuf.cond, &cbuf.mutex);
			cbuf.should_sig = 0;
		}
        }

	pthread_cleanup_pop(0);

	return NULL;
}

int start_syslog_thread(pthread_t *thread, long usecs)
{
	struct timeval ts = {0, usecs};

	/* FIXME: should i protect this with the mutex? */
	if(cbuf.initialized)
		return 1;

	pthread_create(thread, NULL, process_log, NULL);
	select(0, NULL, NULL, NULL, &ts);

	/*
	 * Not grabbing the mutex before I check this - mainly
	 * because it better be atomic - either 0 or 1, and if
	 * it changes from 0 -> 1 while i'm reading it, I
	 * don't really care.
	 */
	return cbuf.initialized ? 1 : 0;
}

int stop_syslog_thread(pthread_t thread)
{
	pthread_cancel(thread);
	pthread_join(thread, NULL);
	/* FIXME: should i protect this with the mutex? */
	cbuf.initialized = 0;
	return 1;
}
