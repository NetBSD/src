/*	$NetBSD: iconv.h,v 1.1.4.2 2002/02/28 04:15:18 nathanw Exp $	*/

/* Public domain */

#ifndef _SYS_ICONV_H_
#define _SYS_ICONV_H_

#ifdef _KERNEL

int iconv_open(const char *to, const char *from, void *);
int iconv_close(void *);
int iconv_conv(void *handle, const char **inbuf,
	size_t *inbytesleft, char **outbuf, size_t *outbytesleft);
char* iconv_convstr(void *handle, char *dst, const char *src);
void* iconv_convmem(void *handle, void *dst, const void *src, int size);

int iconv_lookupcp(const char **cpp, const char *s);

#endif /* !_KERNEL */

#endif /* _SYS_ICONV_H_ */
