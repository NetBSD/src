/*	$NetBSD: iconv.c,v 1.6 2003/02/19 12:01:37 martin Exp $	*/

/* Public domain */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iconv.c,v 1.6 2003/02/19 12:01:37 martin Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/malloc.h>

#include <netsmb/iconv.h>

int
iconv_open(const char *to, const char *from, void **handle)
{
	return 0;
}

int
iconv_close(void *handle)
{
	return 0;
}

int
iconv_conv(void *handle, const char **inbuf,
	size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
{
	if (*inbytesleft > *outbytesleft)
		return(E2BIG);

	if (inbuf == NULL)
		return(0); /* initial shift state */

	memcpy((void *)*inbuf, (void *)*outbuf, *inbytesleft);

	*outbytesleft -= *inbytesleft;

	*inbuf += *inbytesleft;
	*outbuf += *inbytesleft;

	*inbytesleft = 0;

	return 0;
}

char *
iconv_convstr(void *handle, char *dst, const char *src)
{
	char *p = dst;
	size_t inlen, outlen;
	int error;

	if (handle == NULL) {
		strcpy(dst, src);
		return dst;
	}
	inlen = outlen = strlen(src);
	error = iconv_conv(handle, NULL, NULL, &p, &outlen);
	if (error)
		return NULL;
	error = iconv_conv(handle, &src, &inlen, &p, &outlen);
	if (error)
		return NULL;
	*p = 0;
	return dst;
}

void *
iconv_convmem(void *handle, void *dst, const void *src, int size)
{
	const char *s = src;
	char *d = dst;
	size_t inlen, outlen;
	int error;

	if (size == 0)
		return dst;
	if (handle == NULL) {
		memcpy(dst, src, size);
		return dst;
	}
	inlen = outlen = size;
	error = iconv_conv(handle, NULL, NULL, &d, &outlen);
	if (error)
		return NULL;
	error = iconv_conv(handle, &s, &inlen, &d, &outlen);
	if (error)
		return NULL;
	return dst;
}

int
iconv_lookupcp(const char **cpp, const char *s)
{
	for (; *cpp; cpp++)
		if (strcmp(*cpp, s) == 0)
			return 0;
	return ENOENT;
}
