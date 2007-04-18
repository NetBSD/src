/*      $NetBSD: psbuf.c,v 1.4 2007/04/18 15:35:02 pooka Exp $        */

/*
 * Copyright (c) 2006  Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: psbuf.c,v 1.4 2007/04/18 15:35:02 pooka Exp $");
#endif /* !lint */

/*
 * buffering functions for network input/output.  slightly different
 * from the average joe buffer routines, as is usually the case ...
 * these use efuns for now.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/vnode.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <util.h>
#include <unistd.h>

#include "psshfs.h"
#include "sftp_proto.h"

#define FAILRV(x, rv) do { if (!(x)) return (rv); } while (/*CONSTCOND*/0)

int
psbuf_read(struct psshfs_ctx *pctx, struct psbuf *pb)
{
	ssize_t n;

	assert(pb->state != PSBUF_PUT);

 again:
	n = read(pctx->sshfd, pb->buf+pb->offset, pb->remain);
	switch (n) {
	case 0:
		errno = EIO;
		return -1;
	case -1:
		if (errno == EAGAIN)
			return 0;
		return -1;
	default:
		pb->offset += n;
		pb->remain -= n;
	}

	if (pb->remain != 0)
		return 0;

	/* ok, at least there's something to do */
	assert(pb->state == PSBUF_GETLEN || pb->state == PSBUF_GETDATA);

	if (pb->state == PSBUF_GETLEN) {
		memcpy(&pb->len, pb->buf, 4);
		pb->len = ntohl(pb->len);
		pb->remain = pb->len;
		pb->offset = 0;

		free(pb->buf); /* XXX */
		pb->buf = emalloc(pb->len);
		pb->state = PSBUF_GETDATA;
		goto again;

	} else if (pb->state == PSBUF_GETDATA) {
		pb->remain = pb->offset;
		pb->offset = 0;

		pb->state = PSBUF_GETREADY;

		/* sloppy */
		if (!psbuf_get_1(pb, &pb->type))
			errx(1, "invalid server response, no type");
		if (!psbuf_get_4(pb, &pb->reqid))
			errx(1, "invalid server response, no reqid");

		return 1;
	}

	return -1; /* XXX: impossible */
}

int
psbuf_write(struct psshfs_ctx *pctx, struct psbuf *pb)
{
	ssize_t n;

	if (pb->state == PSBUF_PUT) {
		uint32_t len;

		len = htonl(pb->offset - sizeof(len));
		memcpy(pb->buf, &len, sizeof(len));

		pb->remain = pb->offset;
		pb->offset = 0;

		pb->state = PSBUF_PUTDONE;
	}

	assert(pb->state == PSBUF_PUTDONE);

	n = write(pctx->sshfd, pb->buf + pb->offset, pb->remain);
	if (n == 0) {
		errno = EIO;
		return -1;
	}

	if (n == -1) {
		if (errno == EAGAIN)
			return 0;
		else
			return -1;
	}

	pb->offset += n;
	pb->remain -= n;

	if (pb->remain == 0)
		return 1;
	else
		return 0;
}

struct psbuf *
psbuf_make(int incoming)
{
	struct psbuf *pb;

	pb = emalloc(sizeof(struct psbuf));
	memset(pb, 0, sizeof(struct psbuf));
	pb->buf = emalloc(PSDEFALLOC);
	pb->len = PSDEFALLOC;

	psbuf_recycle(pb, incoming);

	return pb;
}

void
psbuf_destroy(struct psbuf *pb)
{

	free(pb->buf);
	free(pb);
}

void
psbuf_recycle(struct psbuf *pb, int incoming)
{

	if (incoming) {
		pb->offset = 0;
		pb->remain = 4;
		pb->state = PSBUF_GETLEN;
	} else {
		/* save space for len */
		pb->remain = pb->len - 4;
		pb->offset = 4;

		pb->state = PSBUF_PUT;
	}
}

static void
psbuf_putspace(struct psbuf *pb, uint32_t space)
{
	uint32_t morespace;
	uint8_t *nb;

	if (pb->remain >= space)
		return;

	for (morespace = PSDEFALLOC; morespace < space; morespace += PSDEFALLOC)
		if (morespace > PSBUFMAX)	
			err(1, "too much memory");

	nb = erealloc(pb->buf, pb->len + morespace);
	pb->len += morespace;
	pb->remain += morespace;
	pb->buf = nb;
}

int
psbuf_put_1(struct psbuf *pb, uint8_t val)
{

	assert(pb->state == PSBUF_PUT);

	psbuf_putspace(pb, 1);
	memcpy(pb->buf + pb->offset, &val, 1);
	pb->offset += 1;
	pb->remain -= 1;

	return 1;
}

int
psbuf_put_2(struct psbuf *pb, uint16_t val)
{

	assert(pb->state == PSBUF_PUT);

	psbuf_putspace(pb, 2);
	val = htons(val);
	memcpy(pb->buf + pb->offset, &val, 2);
	pb->offset += 2;
	pb->remain -= 2;

	return 1;
}

int
psbuf_put_4(struct psbuf *pb, uint32_t val)
{

	assert(pb->state == PSBUF_PUT);

	psbuf_putspace(pb, 4);
	val = htonl(val);
	memcpy(pb->buf + pb->offset, &val, 4);
	pb->offset += 4;
	pb->remain -= 4;

	return 1;
}

int
psbuf_put_8(struct psbuf *pb, uint64_t val)
{

	assert(pb->state == PSBUF_PUT);

	psbuf_putspace(pb, 8);
#if BYTE_ORDER == LITTLE_ENDIAN
	val = bswap64(val);
#endif
	memcpy(pb->buf + pb->offset, &val, 8);
	pb->offset += 8;
	pb->remain -= 8;

	return 1;
}

int
psbuf_put_data(struct psbuf *pb, const void *data, uint32_t dlen)
{

	assert(pb->state == PSBUF_PUT);

	psbuf_put_4(pb, dlen);

	psbuf_putspace(pb, dlen);
	memcpy(pb->buf + pb->offset, data, dlen);
	pb->offset += dlen;
	pb->remain -= dlen;

	return 1;
}

int
psbuf_put_str(struct psbuf *pb, const char *str)
{

	return psbuf_put_data(pb, str, strlen(str));
}

int
psbuf_put_vattr(struct psbuf *pb, const struct vattr *va)
{
	uint32_t flags;
	flags = 0;

	if (va->va_size != PUFFS_VNOVAL)
		flags |= SSH_FILEXFER_ATTR_SIZE;
	if (va->va_uid != PUFFS_VNOVAL)
		flags |= SSH_FILEXFER_ATTR_UIDGID;
	if (va->va_mode != PUFFS_VNOVAL)
		flags |= SSH_FILEXFER_ATTR_PERMISSIONS;

	if (va->va_atime.tv_sec != PUFFS_VNOVAL)
		flags |= SSH_FILEXFER_ATTR_ACCESSTIME;

	psbuf_put_4(pb, flags);
	if (flags & SSH_FILEXFER_ATTR_SIZE)
		psbuf_put_8(pb, va->va_size);
	if (flags & SSH_FILEXFER_ATTR_UIDGID) {
		psbuf_put_4(pb, va->va_uid);
		psbuf_put_4(pb, va->va_gid);
	}
	if (flags & SSH_FILEXFER_ATTR_PERMISSIONS)
		psbuf_put_4(pb, va->va_mode);

	/* XXX: this is totally wrong for protocol v3, see OpenSSH */
	if (flags & SSH_FILEXFER_ATTR_ACCESSTIME) {
		psbuf_put_4(pb, va->va_atime.tv_sec);
		psbuf_put_4(pb, va->va_mtime.tv_sec);
	}

	return 1;
}


int
psbuf_get_1(struct psbuf *pb, uint8_t *val)
{

	assert(pb->state == PSBUF_GETREADY);

	if (pb->remain < 1)
		return 0;

	memcpy(val, pb->buf + pb->offset, 1);
	pb->offset += 1;
	pb->remain -= 1;

	return 1;
}

int
psbuf_get_2(struct psbuf *pb, uint16_t *val)
{
	uint16_t v;

	assert(pb->state == PSBUF_GETREADY);

	if (pb->remain < 2)
		return 0;

	memcpy(&v, pb->buf + pb->offset, 2);
	pb->offset += 2;
	pb->remain -= 2;

	*val = ntohs(v);

	return 1;
}

int
psbuf_get_4(struct psbuf *pb, uint32_t *val)
{
	uint32_t v;

	assert(pb->state == PSBUF_GETREADY);

	if (pb->remain < 4)
		return 0;

	memcpy(&v, pb->buf + pb->offset, 4);
	pb->offset += 4;
	pb->remain -= 4;

	*val = ntohl(v);

	return 1;
}

int
psbuf_get_8(struct psbuf *pb, uint64_t *val)
{
	uint64_t v;

	assert(pb->state == PSBUF_GETREADY);

	if (pb->remain < 8)
		return 0;

	memcpy(&v, pb->buf + pb->offset, 8);
	pb->offset += 8;
	pb->remain -= 8;

#if BYTE_ORDER == LITTLE_ENDIAN
	v = bswap64(v);
#endif
	*val = v;

	return 1;

}

int
psbuf_get_str(struct psbuf *pb, char **strp, uint32_t *strlenp)
{
	char *str;
	uint32_t len;

	assert(pb->state == PSBUF_GETREADY);

	FAILRV(psbuf_get_4(pb, &len), 0);

	if (pb->remain < len)
		return 0;

	str = emalloc(len+1);
	memcpy(str, pb->buf + pb->offset, len);
	str[len] = '\0';
	*strp = str;

	pb->offset += len;
	pb->remain -= len;

	if (strlenp)
		*strlenp = len;

	return 1;
}

int
psbuf_get_vattr(struct psbuf *pb, struct vattr *vap)
{
	uint32_t flags;
	uint32_t val;

	puffs_vattr_null(vap);

	FAILRV(psbuf_get_4(pb, &flags), 0);

	if (flags & SSH_FILEXFER_ATTR_SIZE) {
		FAILRV(psbuf_get_8(pb, &vap->va_size), 0);
		vap->va_bytes = vap->va_size;
	}
	if (flags & SSH_FILEXFER_ATTR_UIDGID) {
		FAILRV(psbuf_get_4(pb, &vap->va_uid), 0);
		FAILRV(psbuf_get_4(pb, &vap->va_gid), 0);
	}
	if (flags & SSH_FILEXFER_ATTR_PERMISSIONS) {
		FAILRV(psbuf_get_4(pb, &vap->va_mode), 0);
		vap->va_type = puffs_mode2vt(vap->va_mode);
	}
	if (flags & SSH_FILEXFER_ATTR_ACCESSTIME) {
		/*
		 * XXX: this is utterly wrong if we want to speak
		 * protocol version 3, but it seems like the
		 * "internet standard" for doing this
		 */
		FAILRV(psbuf_get_4(pb, &val), 0);
		vap->va_atime.tv_sec = val;
		FAILRV(psbuf_get_4(pb, &val), 0);
		vap->va_mtime.tv_sec = val;
		/* make ctime the same as mtime */
		vap->va_ctime.tv_sec = val;

		vap->va_atime.tv_nsec = 0;
		vap->va_ctime.tv_nsec = 0;
		vap->va_mtime.tv_nsec = 0;
	}

	return 1;
}

/*
 * Buffer content helpers.  Caller frees all data.
 */

/*
 * error mapping.. most are not expected for a file system, but
 * should help with diagnosing a possible error
 */
static int emap[] = {
	0,			/* OK			*/
	0,			/* EOF			*/
	ENOENT,			/* NO_SUCH_FILE		*/
	EPERM,			/* PERMISSION_DENIED	*/
	EIO,			/* FAILURE		*/
	EBADMSG,		/* BAD_MESSAGE		*/
	ENOTCONN,		/* NO_CONNECTION	*/
	ECONNRESET,		/* CONNECTION_LOST	*/
	EOPNOTSUPP,		/* OP_UNSUPPORTED	*/
	EINVAL,			/* INVALID_HANDLE	*/
	ENXIO,			/* NO_SUCH_PATH		*/
	EEXIST,			/* FILE_ALREADY_EXISTS	*/
	ENODEV			/* WRITE_PROTECT	*/
};
#define NERRORS (sizeof(emap) / sizeof(emap[0]))

static int
sftperr_to_errno(int error)
{

	if (!error)
		return 0;

	if (error >= NERRORS || error < 0)
		return EPROTO;

	return emap[error];
}

#define INVALRESPONSE EPROTO

static int
expectcode(struct psbuf *pb, int value)
{
	uint32_t error;

	if (pb->type == value)
		return 0;

	if (pb->type != SSH_FXP_STATUS)
		return INVALRESPONSE;

	FAILRV(psbuf_get_4(pb, &error), INVALRESPONSE);

	return sftperr_to_errno(error);
}

#define CHECKCODE(pb,val)						\
do {									\
	int rv;								\
	rv = expectcode(pb, val);					\
	if (rv)								\
		return rv;						\
} while (/*CONSTCOND*/0)

int
psbuf_expect_status(struct psbuf *pb)
{
	uint32_t error;

	if (pb->type != SSH_FXP_STATUS)
		return INVALRESPONSE;

	FAILRV(psbuf_get_4(pb, &error), INVALRESPONSE);
	
	return sftperr_to_errno(error);
}

int
psbuf_expect_handle(struct psbuf *pb, char **hand, uint32_t *handlen)
{

	CHECKCODE(pb, SSH_FXP_HANDLE);
	FAILRV(psbuf_get_str(pb, hand, handlen), INVALRESPONSE);

	return 0;
}

/* no memory allocation, direct copy */
int
psbuf_do_data(struct psbuf *pb, uint8_t *data, uint32_t *dlen)
{
	uint32_t len;

	if (pb->type != SSH_FXP_DATA) {
		uint32_t val;

		if (pb->type != SSH_FXP_STATUS)
			return INVALRESPONSE;

		if (!psbuf_get_4(pb, &val))
			return INVALRESPONSE;

		if (val != SSH_FX_EOF)
			return sftperr_to_errno(val);

		*dlen = 0;
		return 0;
	}
	if (!psbuf_get_4(pb, &len))
		return INVALRESPONSE;

	if (*dlen < len)
		return EINVAL;

	memcpy(data, pb->buf + pb->offset, len);

	pb->offset += len;
	pb->remain -= len;

	*dlen = len;

	return 0;
}

int
psbuf_expect_name(struct psbuf *pb, uint32_t *count)
{

	CHECKCODE(pb, SSH_FXP_NAME);
	FAILRV(psbuf_get_4(pb, count), INVALRESPONSE);

	return 0;
}

int
psbuf_expect_attrs(struct psbuf *pb, struct vattr *vap)
{

	CHECKCODE(pb, SSH_FXP_ATTRS);
	FAILRV(psbuf_get_vattr(pb, vap), INVALRESPONSE);

	return 0;
}

/*
 * More helpers: larger-scale put functions
 */

int
psbuf_req_data(struct psbuf *pb, int type, uint32_t reqid, const void *data,
	uint32_t dlen)
{

	psbuf_put_1(pb, type);
	psbuf_put_4(pb, reqid);
	psbuf_put_data(pb, data, dlen);

	return 1;
}

int
psbuf_req_str(struct psbuf *pb, int type, uint32_t reqid, const char *str)
{

	return psbuf_req_data(pb, type, reqid, str, strlen(str));
}
