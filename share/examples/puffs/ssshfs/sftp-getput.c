/*	$NetBSD: sftp-getput.c,v 1.3 2006/12/05 23:14:13 pooka Exp $	*/

/*	NetBSD: sftp-client.c,v 1.26 2006/09/28 21:22:15 christos Exp */
/* $OpenBSD: sftp-client.c,v 1.74 2006/08/03 03:34:42 deraadt Exp $ */
/*
 * Copyright (c) 2001-2004 Damien Miller <djm@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* hacked, mangled & maimed for ssshfs by pooka */

/* XXX: memleaks */
/* XXX: signed vs unsigned */
/* XXX: remove all logging, only return status codes */
/* XXX: copy between two remote sites */

#include "includes.h"
__RCSID("$NetBSD: sftp-getput.c,v 1.3 2006/12/05 23:14:13 pooka Exp $");

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/uio.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#include "xmalloc.h"
#include "buffer.h"
#include "log.h"
#include "atomicio.h"
#include "misc.h"

#include "sftp.h"
#include "sftp-common.h"
#include "sftp-client.h"

extern volatile sig_atomic_t interrupted;

/* Minimum amount of data to read at a time */
#define MIN_READ_SIZE	512

struct sftp_conn {
	int fd_in;
	int fd_out;
	u_int transfer_buflen;
	u_int num_requests;
	u_int version;
	u_int msg_id;
};

static void
send_msg(int fd, Buffer *m)
{
	u_char mlen[4];
	struct iovec iov[2];

	if (buffer_len(m) > SFTP_MAX_MSG_LENGTH)
		fatal("Outbound message too long %u", buffer_len(m));

	/* Send length first */
	put_u32(mlen, buffer_len(m));
	iov[0].iov_base = mlen;
	iov[0].iov_len = sizeof(mlen);
	iov[1].iov_base = buffer_ptr(m);
	iov[1].iov_len = buffer_len(m);

	if (atomiciov(writev, fd, iov, 2) != buffer_len(m) + sizeof(mlen))
		fatal("Couldn't send packet: %s", strerror(errno));

	buffer_clear(m);
}

static void
get_msg(int fd, Buffer *m)
{
	u_int msg_len;

	buffer_append_space(m, 4);
	if (atomicio(read, fd, buffer_ptr(m), 4) != 4) {
		if (errno == EPIPE)
			fatal("Connection closed");
		else
			fatal("Couldn't read packet: %s", strerror(errno));
	}

	msg_len = buffer_get_int(m);
	if (msg_len > SFTP_MAX_MSG_LENGTH)
		fatal("Received message too long %u", msg_len);

	buffer_append_space(m, msg_len);
	if (atomicio(read, fd, buffer_ptr(m), msg_len) != msg_len) {
		if (errno == EPIPE)
			fatal("Connection closed");
		else
			fatal("Read packet: %s", strerror(errno));
	}
}

static u_int
get_status(int fd, u_int expected_id)
{
	Buffer msg;
	u_int type, id, status;

	buffer_init(&msg);
	get_msg(fd, &msg);
	type = buffer_get_char(&msg);
	id = buffer_get_int(&msg);

	if (id != expected_id)
		fatal("ID mismatch (%u != %u)", id, expected_id);
	if (type != SSH2_FXP_STATUS)
		fatal("Expected SSH2_FXP_STATUS(%u) packet, got %u",
		    SSH2_FXP_STATUS, type);

	status = buffer_get_int(&msg);
	buffer_free(&msg);

	debug3("SSH2_FXP_STATUS %u", status);

	return(status);
}

static char *
get_handle(int fd, u_int expected_id, u_int *len)
{
	Buffer msg;
	u_int type, id;
	char *handle;

	buffer_init(&msg);
	get_msg(fd, &msg);
	type = buffer_get_char(&msg);
	id = buffer_get_int(&msg);

	if (id != expected_id)
		fatal("ID mismatch (%u != %u)", id, expected_id);
	if (type == SSH2_FXP_STATUS) {
		int status = buffer_get_int(&msg);

		error("Couldn't get handle: %s", fx2txt(status));
		buffer_free(&msg);
		return(NULL);
	} else if (type != SSH2_FXP_HANDLE)
		fatal("Expected SSH2_FXP_HANDLE(%u) packet, got %u",
		    SSH2_FXP_HANDLE, type);

	handle = buffer_get_string(&msg, len);
	buffer_free(&msg);

	return(handle);
}

static Attrib *
get_decode_stat(int fd, u_int expected_id, int quiet)
{
	Buffer msg;
	u_int type, id;
	Attrib *a;

	buffer_init(&msg);
	get_msg(fd, &msg);

	type = buffer_get_char(&msg);
	id = buffer_get_int(&msg);

	debug3("Received stat reply T:%u I:%u", type, id);
	if (id != expected_id)
		fatal("ID mismatch (%u != %u)", id, expected_id);
	if (type == SSH2_FXP_STATUS) {
		int status = buffer_get_int(&msg);

		if (quiet)
			debug("Couldn't stat remote file: %s", fx2txt(status));
		else
			error("Couldn't stat remote file: %s", fx2txt(status));
		buffer_free(&msg);
		return(NULL);
	} else if (type != SSH2_FXP_ATTRS) {
		fatal("Expected SSH2_FXP_ATTRS(%u) packet, got %u",
		    SSH2_FXP_ATTRS, type);
	}
	a = decode_attrib(&msg);
	buffer_free(&msg);

	return(a);
}

static void
send_read_request(int fd_out, u_int id, u_int64_t offset, u_int len,
    char *handle, u_int handle_len)
{
	Buffer msg;

	buffer_init(&msg);
	buffer_clear(&msg);
	buffer_put_char(&msg, SSH2_FXP_READ);
	buffer_put_int(&msg, id);
	buffer_put_string(&msg, handle, handle_len);
	buffer_put_int64(&msg, offset);
	buffer_put_int(&msg, len);
	send_msg(fd_out, &msg);
	buffer_free(&msg);
}

int
do_creatfile(struct sftp_conn *conn, const char *path, const Attrib *a)
{
	Buffer msg;
	char *handle;
	u_int handle_len;
	int id;

	buffer_init(&msg);
	id = conn->msg_id++;
	buffer_put_char(&msg, SSH2_FXP_OPEN);
	buffer_put_int(&msg, id);
	buffer_put_cstring(&msg, path);
	buffer_put_int(&msg, SSH2_FXF_WRITE|SSH2_FXF_CREAT|SSH2_FXF_TRUNC);
	encode_attrib(&msg, a);
	send_msg(conn->fd_out, &msg);

	handle = get_handle(conn->fd_in, id, &handle_len);
	buffer_free(&msg);

	if (handle == NULL)
		return EIO;

	do_close(conn, handle, handle_len);
	return 0;
}

int
do_readfile(struct sftp_conn *conn, char *path, uint8_t *localbuf,
    off_t getoff, size_t *getlen)
{
	Attrib junk, *a;
	Buffer msg;
	char *handle;
	int status = 0, write_error;
	int read_error, write_errno;
	u_int64_t offset, size;
	u_int handle_len, mode, type, id, buflen, num_req, max_req;
	struct request {
		u_int id;
		u_int len;
		u_int64_t offset;
		TAILQ_ENTRY(request) tq;
	};
	TAILQ_HEAD(reqhead, request) requests;
	struct request *req;

	status = -1;
	TAILQ_INIT(&requests);

	a = do_stat(conn, path, 0);
	if (a == NULL)
		return(-1);

	/* XXX: should we preserve set[ug]id? */
	if (a->flags & SSH2_FILEXFER_ATTR_PERMISSIONS)
		mode = a->perm & 0777;
	else
		mode = 0666;

	if ((a->flags & SSH2_FILEXFER_ATTR_PERMISSIONS) &&
	    (!S_ISREG(a->perm))) {
		error("Cannot download non-regular file: %s", path);
		return(-1);
	}

	if (a->flags & SSH2_FILEXFER_ATTR_SIZE)
		size = a->size;
	else
		size = 0;

	buflen = conn->transfer_buflen;
	buffer_init(&msg);

	/* Send open request */
	id = conn->msg_id++;
	buffer_put_char(&msg, SSH2_FXP_OPEN);
	buffer_put_int(&msg, id);
	buffer_put_cstring(&msg, path);
	buffer_put_int(&msg, SSH2_FXF_READ);
	attrib_clear(&junk); /* Send empty attributes */
	encode_attrib(&msg, &junk);
	send_msg(conn->fd_out, &msg);
	debug3("Sent message SSH2_FXP_OPEN I:%u P:%s", id, path);

	handle = get_handle(conn->fd_in, id, &handle_len);
	if (handle == NULL) {
		buffer_free(&msg);
		return(-1);
	}

	/* Read from remote and write to local */
	write_error = read_error = write_errno = num_req = 0;
	offset = getoff;
	max_req = 1;

	while (num_req > 0 || max_req > 0) {
		char *data;
		u_int len;

		/* Send some more requests */
		while ((num_req < max_req) && (offset < getoff + *getlen)) {
			req = xmalloc(sizeof(*req));
			req->id = conn->msg_id++;
			req->len = buflen;
			req->offset = offset;
			num_req++;

			/* adjust for tail */
			if ((req->len + req->offset) >= (getoff + *getlen)) {
				req->len = getoff+*getlen - req->offset;
				max_req = 0;
			}

			debug3("Request range %llu -> %llu (%d/%d)",
			    (unsigned long long)offset,
			    (unsigned long long)offset + req->len - 1,
			    num_req, max_req);

			offset += req->len;

			TAILQ_INSERT_TAIL(&requests, req, tq);
			send_read_request(conn->fd_out, req->id, req->offset,
			    req->len, handle, handle_len);
		}

		buffer_clear(&msg);
		get_msg(conn->fd_in, &msg);
		type = buffer_get_char(&msg);
		id = buffer_get_int(&msg);
		debug3("Received reply T:%u I:%u R:%d", type, id, max_req);

		/* Find the request in our queue */
		for (req = TAILQ_FIRST(&requests);
		    req != NULL && req->id != id;
		    req = TAILQ_NEXT(req, tq))
			;
		if (req == NULL)
			fatal("Unexpected reply %u", id);

		switch (type) {
		case SSH2_FXP_STATUS:
			status = buffer_get_int(&msg);
			if (status != SSH2_FX_EOF)
				read_error = 1;
			max_req = 0;
			TAILQ_REMOVE(&requests, req, tq);
			xfree(req);
			num_req--;
			break;
		case SSH2_FXP_DATA:
			data = buffer_get_string(&msg, &len);
			debug3("Received data %llu -> %llu",
			    (unsigned long long)req->offset,
			    (unsigned long long)req->offset + len - 1);
			if (len > req->len)
				fatal("Received more data than asked for "
				    "%u > %u", len, req->len);
			memcpy(localbuf + (req->offset - getoff), data, len);
			xfree(data);

			if (len == req->len) {
				TAILQ_REMOVE(&requests, req, tq);
				xfree(req);
				num_req--;
			} else {
				/* Resend the request for the missing data */
				debug3("Short data block, re-requesting "
				    "%llu -> %llu (%2d)",
				    (unsigned long long)req->offset + len,
				    (unsigned long long)req->offset +
				    req->len - 1, num_req);
				req->id = conn->msg_id++;
				req->len -= len;
				req->offset += len;
				send_read_request(conn->fd_out, req->id,
				    req->offset, req->len, handle, handle_len);
				/* Reduce the request size */
				if (len < buflen)
					buflen = MAX(MIN_READ_SIZE, len);
			}
			break;
		default:
			fatal("Expected SSH2_FXP_DATA(%u) packet, got %u",
			    SSH2_FXP_DATA, type);
		}
	}

	/* Sanity check */
	if (TAILQ_FIRST(&requests) != NULL)
		fatal("Transfer complete, but requests still in queue");

	if (read_error) {
		error("Couldn't read from remote file \"%s\" : %s",
		    path, fx2txt(status));
		do_close(conn, handle, handle_len);
	} else {
		status = do_close(conn, handle, handle_len);
	}
	*getlen -= (offset - getoff);
	buffer_free(&msg);
	xfree(handle);

	return(status);
}

int
do_writefile(struct sftp_conn *conn, char *path, uint8_t *localbuf,
    off_t putoff, size_t *putlen, int append)
{
	int status;
	u_int handle_len, id, type;
	u_int64_t offset;
	char *handle, *data;
	Buffer msg;
	Attrib a, savea;
	Attrib *ap;
	u_int32_t startid;
	u_int32_t ackid;
	struct outstanding_ack {
		u_int id;
		u_int len;
		u_int64_t offset;
		TAILQ_ENTRY(outstanding_ack) tq;
	};
	TAILQ_HEAD(ackhead, outstanding_ack) acks;
	struct outstanding_ack *ack = NULL;

	TAILQ_INIT(&acks);

	/* XXX: temporarily set file permission to allow writing */
	ap = do_stat(conn, path, 1);
	if (!ap)
		return -1;

	if (append)
		putoff = ap->size;

	memcpy(&savea, ap, sizeof(Attrib));
	savea.flags &= ~SSH2_FILEXFER_ATTR_SIZE;

	memset(&a, 0, sizeof(a));
	a.flags = SSH2_FILEXFER_ATTR_PERMISSIONS;
	a.perm = 0777;
	if (do_setstat(conn, path, &a)) {
		printf("lossage\n");
		return -1;
	}

	buffer_init(&msg);

	/* Send open request */
	id = conn->msg_id++;
	buffer_put_char(&msg, SSH2_FXP_OPEN);
	buffer_put_int(&msg, id);
	buffer_put_cstring(&msg, path);
	buffer_put_int(&msg, SSH2_FXF_WRITE | SSH2_FXF_CREAT);
	encode_attrib(&msg, &a);
	send_msg(conn->fd_out, &msg);
	debug3("Sent message SSH2_FXP_OPEN I:%u P:%s", id, path);

	buffer_clear(&msg);

	handle = get_handle(conn->fd_in, id, &handle_len);
	if (handle == NULL) {
		do_setstat(conn, path, &savea);
		buffer_free(&msg);
		return(-1);
	}

	startid = ackid = id + 1;
	data = xmalloc(conn->transfer_buflen);

	offset = putoff;

	for (;;) {
		size_t len;

		len = conn->transfer_buflen;
		if (offset + len > putoff + *putlen)
			len = (putoff + *putlen) - offset;

		if (len != 0) {
			memcpy(data, localbuf + (offset - putoff), len);
			ack = xmalloc(sizeof(*ack));
			ack->id = ++id;
			ack->offset = offset;
			ack->len = len;
			TAILQ_INSERT_TAIL(&acks, ack, tq);

			buffer_clear(&msg);
			buffer_put_char(&msg, SSH2_FXP_WRITE);
			buffer_put_int(&msg, ack->id);
			buffer_put_string(&msg, handle, handle_len);
			buffer_put_int64(&msg, offset);
			buffer_put_string(&msg, data, len);
			send_msg(conn->fd_out, &msg);
			debug3("Sent message SSH2_FXP_WRITE I:%u O:%llu S:%u",
			    id, (unsigned long long)offset, len);
		} else if (TAILQ_FIRST(&acks) == NULL)
			break;

		if (ack == NULL)
			fatal("Unexpected ACK %u", id);

		if (id == startid || len == 0 ||
		    id - ackid >= conn->num_requests) {
			u_int r_id;

			buffer_clear(&msg);
			get_msg(conn->fd_in, &msg);
			type = buffer_get_char(&msg);
			r_id = buffer_get_int(&msg);

			if (type != SSH2_FXP_STATUS)
				fatal("Expected SSH2_FXP_STATUS(%d) packet, "
				    "got %d", SSH2_FXP_STATUS, type);

			status = buffer_get_int(&msg);
			debug3("SSH2_FXP_STATUS %d", status);

			/* Find the request in our queue */
			for (ack = TAILQ_FIRST(&acks);
			    ack != NULL && ack->id != r_id;
			    ack = TAILQ_NEXT(ack, tq))
				;
			if (ack == NULL)
				fatal("Can't find request for ID %u", r_id);
			TAILQ_REMOVE(&acks, ack, tq);

			if (status != SSH2_FX_OK) {
				error("Couldn't write to remote file \"%s\": %s",
				    path, fx2txt(status));
				do_close(conn, handle, handle_len);
				xfree(data);
				xfree(ack);
				goto done;
			}
			debug3("In write loop, ack for %u %u bytes at %llu",
			    ack->id, ack->len, (unsigned long long)ack->offset);
			++ackid;
			xfree(ack);
		}
		offset += len;
	}
	xfree(data);

	status = do_close(conn, handle, handle_len);
	*putlen -= (offset - putoff);

done:
	do_setstat(conn, path, &savea);
	xfree(handle);
	buffer_free(&msg);
	return(status);
}
