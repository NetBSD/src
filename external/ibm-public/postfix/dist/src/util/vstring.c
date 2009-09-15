/*	$NetBSD: vstring.c,v 1.1.1.1.2.2 2009/09/15 06:04:05 snj Exp $	*/

/*++
/* NAME
/*	vstring 3
/* SUMMARY
/*	arbitrary-length string manager
/* SYNOPSIS
/*	#include <vstring.h>
/*
/*	VSTRING	*vstring_alloc(len)
/*	ssize_t	len;
/*
/*	vstring_ctl(vp, type, value, ..., VSTRING_CTL_END)
/*	VSTRING	*vp;
/*	int	type;
/*
/*	VSTRING	*vstring_free(vp)
/*	VSTRING	*vp;
/*
/*	char	*vstring_str(vp)
/*	VSTRING	*vp;
/*
/*	ssize_t	VSTRING_LEN(vp)
/*	VSTRING	*vp;
/*
/*	char	*vstring_end(vp)
/*	VSTRING	*vp;
/*
/*	void	VSTRING_ADDCH(vp, ch)
/*	VSTRING	*vp;
/*	int	ch;
/*
/*	int	VSTRING_SPACE(vp, len)
/*	VSTRING	*vp;
/*	ssize_t	len;
/*
/*	ssize_t	vstring_avail(vp)
/*	VSTRING	*vp;
/*
/*	VSTRING	*vstring_truncate(vp, len)
/*	VSTRING	*vp;
/*	ssize_t	len;
/*
/*	void	VSTRING_RESET(vp)
/*	VSTRING	*vp;
/*
/*	void	VSTRING_TERMINATE(vp)
/*	VSTRING	*vp;
/*
/*	void	VSTRING_SKIP(vp)
/*	VSTRING	*vp;
/*
/*	VSTRING	*vstring_strcpy(vp, src)
/*	VSTRING	*vp;
/*	const char *src;
/*
/*	VSTRING	*vstring_strncpy(vp, src, len)
/*	VSTRING	*vp;
/*	const char *src;
/*	ssize_t	len;
/*
/*	VSTRING	*vstring_strcat(vp, src)
/*	VSTRING	*vp;
/*	const char *src;
/*
/*	VSTRING	*vstring_strncat(vp, src, len)
/*	VSTRING	*vp;
/*	const char *src;
/*	ssize_t	len;
/*
/*	VSTRING	*vstring_memcpy(vp, src, len)
/*	VSTRING	*vp;
/*	const char *src;
/*	ssize_t	len;
/*
/*	VSTRING	*vstring_memcat(vp, src, len)
/*	VSTRING	*vp;
/*	const char *src;
/*	ssize_t	len;
/*
/*	char	*vstring_memchr(vp, ch)
/*	VSTRING	*vp;
/*	int	ch;
/*
/*	VSTRING	*vstring_insert(vp, start, src, len)
/*	VSTRING	*vp;
/*	ssize_t	start;
/*	const char *src;
/*	ssize_t	len;
/*
/*	VSTRING	*vstring_prepend(vp, src, len)
/*	VSTRING	*vp;
/*	const char *src;
/*	ssize_t	len;
/*
/*	VSTRING	*vstring_sprintf(vp, format, ...)
/*	VSTRING	*vp;
/*	const char *format;
/*
/*	VSTRING	*vstring_sprintf_append(vp, format, ...)
/*	VSTRING	*vp;
/*	const char *format;
/*
/*	VSTRING	*vstring_sprintf_prepend(vp, format, ...)
/*	VSTRING	*vp;
/*	const char *format;
/*
/*	VSTRING	*vstring_vsprintf(vp, format, ap)
/*	VSTRING	*vp;
/*	const char *format;
/*	va_list	ap;
/*
/*	VSTRING	*vstring_vsprintf_append(vp, format, ap)
/*	VSTRING	*vp;
/*	const char *format;
/*	va_list	ap;
/* AUXILIARY FUNCTIONS
/*	char	*vstring_export(vp)
/*	VSTRING	*vp;
/*
/*	VSTRING	*vstring_import(str)
/*	char	*str;
/* DESCRIPTION
/*	The functions and macros in this module implement arbitrary-length
/*	strings and common operations on those strings. The strings do not
/*	need to be null terminated and may contain arbitrary binary data.
/*	The strings manage their own memory and grow automatically when full.
/*	The optional string null terminator does not add to the string length.
/*
/*	vstring_alloc() allocates storage for a variable-length string
/*	of at least "len" bytes. The minimal length is 1. The result
/*	is a null-terminated string of length zero.
/*
/*	vstring_ctl() gives additional control over VSTRING behavior.
/*	The function takes a VSTRING pointer and a list of zero
/*	or more (name,value) pairs. The expected value type
/*	depends on the specified name. The value name codes are:
/* .IP "VSTRING_CTL_MAXLEN (ssize_t)"
/*	Specifies a hard upper limit on a string's length. When the
/*	length would be exceeded, the program simulates a memory
/*	allocation problem (i.e. it terminates through msg_fatal()).
/*	This fuctionality is currently unimplemented.
/* .IP "VSTRING_CTL_END (no value)"
/*	Specifies the end of the argument list. Forgetting to terminate
/*	the argument list may cause the program to crash.
/* .PP
/*	VSTRING_SPACE() ensures that the named string has room for
/*	"len" more characters. VSTRING_SPACE() is an unsafe macro
/*	that either returns zero or never returns.
/*
/*	vstring_avail() returns the number of bytes that can be placed
/*	into the buffer before the buffer would need to grow.
/*
/*	vstring_free() reclaims storage for a variable-length string.
/*	It conveniently returns a null pointer.
/*
/*	vstring_str() is a macro that returns the string value
/*	of a variable-length string. It is a safe macro that
/*	evaluates its argument only once.
/*
/*	VSTRING_LEN() is a macro that returns the current length of
/*	its argument (i.e. the distance from the start of the string
/*	to the current write position). VSTRING_LEN() is an unsafe macro
/*	that evaluates its argument more than once.
/*
/*	vstring_end() is a macro that returns the current write position of
/*	its argument. It is a safe macro that evaluates its argument only once.
/*
/*	VSTRING_ADDCH() adds a character to a variable-length string
/*	and extends the string if it fills up.  \fIvs\fP is a pointer
/*	to a VSTRING structure; \fIch\fP the character value to be written.
/*	The result is the written character.
/*	Note that VSTRING_ADDCH() is an unsafe macro that evaluates some
/*	arguments more than once. The result is NOT null-terminated.
/*
/*	vstring_truncate() truncates the named string to the specified
/*	length. The operation has no effect when the string is shorter.
/*	The string is not null-terminated.
/*
/*	VSTRING_RESET() is a macro that resets the write position of its
/*	string argument to the very beginning. Note that VSTRING_RESET()
/*	is an unsafe macro that evaluates some arguments more than once.
/*	The result is NOT null-terminated.
/*
/*	VSTRING_TERMINATE() null-terminates its string argument.
/*	VSTRING_TERMINATE() is an unsafe macro that evaluates some
/*	arguments more than once.
/*	VSTRING_TERMINATE() does not return an interesting result.
/*
/*	VSTRING_SKIP() is a macro that moves the write position to the first
/*	null byte after the current write position. VSTRING_SKIP() is an unsafe
/*	macro that evaluates some arguments more than once.
/*
/*	vstring_strcpy() copies a null-terminated string to a variable-length
/*	string. \fIsrc\fP provides the data to be copied; \fIvp\fP is the
/*	target and result value.  The result is null-terminated.
/*
/*	vstring_strncpy() copies at most \fIlen\fR characters. Otherwise it is
/*	identical to vstring_strcpy().
/*
/*	vstring_strcat() appends a null-terminated string to a variable-length
/*	string. \fIsrc\fP provides the data to be copied; \fIvp\fP is the
/*	target and result value.  The result is null-terminated.
/*
/*	vstring_strncat() copies at most \fIlen\fR characters. Otherwise it is
/*	identical to vstring_strcat().
/*
/*	vstring_memcpy() copies \fIlen\fR bytes to a variable-length string.
/*	\fIsrc\fP provides the data to be copied; \fIvp\fP is the
/*	target and result value.  The result is not null-terminated.
/*
/*	vstring_memcat() appends \fIlen\fR bytes to a variable-length string.
/*	\fIsrc\fP provides the data to be copied; \fIvp\fP is the
/*	target and result value.  The result is not null-terminated.
/*
/*	vstring_memchr() locates a byte in a variable-length string.
/*
/*	vstring_insert() inserts a buffer content into a variable-length
/*	string at the specified start position. The result is
/*	null-terminated.
/*
/*	vstring_prepend() prepends a buffer content to a variable-length
/*	string. The result is null-terminated.
/*
/*	vstring_sprintf() produces a formatted string according to its
/*	\fIformat\fR argument. See vstring_vsprintf() for details.
/*
/*	vstring_sprintf_append() is like vstring_sprintf(), but appends
/*	to the end of the result buffer.
/*
/*	vstring_sprintf_append() is like vstring_sprintf(), but prepends
/*	to the beginning of the result buffer.
/*
/*	vstring_vsprintf() returns a null-terminated string according to
/*	the \fIformat\fR argument. It understands the s, c, d, u,
/*	o, x, X, p, e, f and g format types, the l modifier, field width
/*	and precision, sign, and null or space padding. This module
/*	can format strings as large as available memory permits.
/*
/*	vstring_vsprintf_append() is like vstring_vsprintf(), but appends
/*	to the end of the result buffer.
/*
/*	In addition to stdio-like format specifiers, vstring_vsprintf()
/*	recognizes %m and expands it to the corresponding errno text.
/*
/*	vstring_export() extracts the string value from a VSTRING.
/*	The VSTRING is destroyed. The result should be passed to myfree().
/*
/*	vstring_import() takes a `bare' string and converts it to
/*	a VSTRING. The string argument must be obtained from mymalloc().
/*	The string argument is not copied.
/* DIAGNOSTICS
/*	Fatal errors: memory allocation failure.
/* BUGS
/*	Auto-resizing may change the address of the string data in
/*	a vstring structure. Beware of dangling pointers.
/* HISTORY
/* .ad
/* .fi
/*	A vstring module appears in the UNPROTO software by Wietse Venema.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System libraries. */

#include <sys_defs.h>
#include <stddef.h>
#include <stdlib.h>			/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>
#include <string.h>

/* Utility library. */

#include "mymalloc.h"
#include "msg.h"
#include "vbuf_print.h"
#include "vstring.h"

/* vstring_extend - variable-length string buffer extension policy */

static void vstring_extend(VBUF *bp, ssize_t incr)
{
    size_t  used = bp->ptr - bp->data;
    ssize_t new_len;

    /*
     * Note: vp->vbuf.len is the current buffer size (both on entry and on
     * exit of this routine). We round up the increment size to the buffer
     * size to avoid silly little buffer increments. With really large
     * strings we might want to abandon the length doubling strategy, and go
     * to fixed increments.
     * 
     * The length overflow tests here and in vstring_alloc() should protect us
     * against all length overflow problems within vstring library routines.
     * (The tests are redundant as long as mymalloc() and myrealloc() reject
     * negative length parameters).
     */
    new_len = bp->len + (bp->len > incr ? bp->len : incr);
    if (new_len < 0)
	msg_fatal("vstring_extend: length overflow");
    bp->data = (unsigned char *) myrealloc((char *) bp->data, new_len);
    bp->len = new_len;
    bp->ptr = bp->data + used;
    bp->cnt = bp->len - used;
}

/* vstring_buf_get_ready - vbuf callback for read buffer empty condition */

static int vstring_buf_get_ready(VBUF *unused_buf)
{
    msg_panic("vstring_buf_get: write-only buffer");
}

/* vstring_buf_put_ready - vbuf callback for write buffer full condition */

static int vstring_buf_put_ready(VBUF *bp)
{
    vstring_extend(bp, 0);
    return (0);
}

/* vstring_buf_space - vbuf callback to reserve space */

static int vstring_buf_space(VBUF *bp, ssize_t len)
{
    ssize_t need;

    if (len < 0)
	msg_panic("vstring_buf_space: bad length %ld", (long) len);
    if ((need = len - bp->cnt) > 0)
	vstring_extend(bp, need);
    return (0);
}

/* vstring_alloc - create variable-length string */

VSTRING *vstring_alloc(ssize_t len)
{
    VSTRING *vp;

    if (len < 1)
	msg_panic("vstring_alloc: bad length %ld", (long) len);
    vp = (VSTRING *) mymalloc(sizeof(*vp));
    vp->vbuf.flags = 0;
    vp->vbuf.len = 0;
    vp->vbuf.data = (unsigned char *) mymalloc(len);
    vp->vbuf.len = len;
    VSTRING_RESET(vp);
    vp->vbuf.data[0] = 0;
    vp->vbuf.get_ready = vstring_buf_get_ready;
    vp->vbuf.put_ready = vstring_buf_put_ready;
    vp->vbuf.space = vstring_buf_space;
    vp->maxlen = 0;
    return (vp);
}

/* vstring_free - destroy variable-length string */

VSTRING *vstring_free(VSTRING *vp)
{
    if (vp->vbuf.data)
	myfree((char *) vp->vbuf.data);
    myfree((char *) vp);
    return (0);
}

/* vstring_ctl - modify memory management policy */

void    vstring_ctl(VSTRING *vp,...)
{
    va_list ap;
    int     code;

    va_start(ap, vp);
    while ((code = va_arg(ap, int)) != VSTRING_CTL_END) {
	switch (code) {
	default:
	    msg_panic("vstring_ctl: unknown code: %d", code);
	case VSTRING_CTL_MAXLEN:
	    vp->maxlen = va_arg(ap, ssize_t);
	    if (vp->maxlen < 0)
		msg_panic("vstring_ctl: bad max length %ld", (long) vp->maxlen);
	    break;
	}
    }
    va_end(ap);
}

/* vstring_truncate - truncate string */

VSTRING *vstring_truncate(VSTRING *vp, ssize_t len)
{
    if (len < 0)
	msg_panic("vstring_truncate: bad length %ld", (long) len);
    if (len < VSTRING_LEN(vp))
	VSTRING_AT_OFFSET(vp, len);
    return (vp);
}

/* vstring_strcpy - copy string */

VSTRING *vstring_strcpy(VSTRING *vp, const char *src)
{
    VSTRING_RESET(vp);

    while (*src) {
	VSTRING_ADDCH(vp, *src);
	src++;
    }
    VSTRING_TERMINATE(vp);
    return (vp);
}

/* vstring_strncpy - copy string of limited length */

VSTRING *vstring_strncpy(VSTRING *vp, const char *src, ssize_t len)
{
    VSTRING_RESET(vp);

    while (len-- > 0 && *src) {
	VSTRING_ADDCH(vp, *src);
	src++;
    }
    VSTRING_TERMINATE(vp);
    return (vp);
}

/* vstring_strcat - append string */

VSTRING *vstring_strcat(VSTRING *vp, const char *src)
{
    while (*src) {
	VSTRING_ADDCH(vp, *src);
	src++;
    }
    VSTRING_TERMINATE(vp);
    return (vp);
}

/* vstring_strncat - append string of limited length */

VSTRING *vstring_strncat(VSTRING *vp, const char *src, ssize_t len)
{
    while (len-- > 0 && *src) {
	VSTRING_ADDCH(vp, *src);
	src++;
    }
    VSTRING_TERMINATE(vp);
    return (vp);
}

/* vstring_memcpy - copy buffer of limited length */

VSTRING *vstring_memcpy(VSTRING *vp, const char *src, ssize_t len)
{
    VSTRING_RESET(vp);

    VSTRING_SPACE(vp, len);
    memcpy(vstring_str(vp), src, len);
    VSTRING_AT_OFFSET(vp, len);
    return (vp);
}

/* vstring_memcat - append buffer of limited length */

VSTRING *vstring_memcat(VSTRING *vp, const char *src, ssize_t len)
{
    VSTRING_SPACE(vp, len);
    memcpy(vstring_end(vp), src, len);
    len += VSTRING_LEN(vp);
    VSTRING_AT_OFFSET(vp, len);
    return (vp);
}

/* vstring_memchr - locate byte in buffer */

char   *vstring_memchr(VSTRING *vp, int ch)
{
    unsigned char *cp;

    for (cp = (unsigned char *) vstring_str(vp); cp < (unsigned char *) vstring_end(vp); cp++)
	if (*cp == ch)
	    return ((char *) cp);
    return (0);
}

/* vstring_insert - insert text into string */

VSTRING *vstring_insert(VSTRING *vp, ssize_t start, const char *buf, ssize_t len)
{
    ssize_t new_len;

    /*
     * Sanity check.
     */
    if (start < 0 || start >= VSTRING_LEN(vp))
	msg_panic("vstring_insert: bad start %ld", (long) start);
    if (len < 0)
	msg_panic("vstring_insert: bad length %ld", (long) len);

    /*
     * Move the existing content and copy the new content.
     */
    new_len = VSTRING_LEN(vp) + len;
    VSTRING_SPACE(vp, len);
    memmove(vstring_str(vp) + start + len, vstring_str(vp) + start,
	    VSTRING_LEN(vp) - start);
    memcpy(vstring_str(vp) + start, buf, len);
    VSTRING_AT_OFFSET(vp, new_len);
    VSTRING_TERMINATE(vp);
    return (vp);
}

/* vstring_prepend - prepend text to string */

VSTRING *vstring_prepend(VSTRING *vp, const char *buf, ssize_t len)
{
    ssize_t new_len;

    /*
     * Sanity check.
     */
    if (len < 0)
	msg_panic("vstring_prepend: bad length %ld", (long) len);

    /*
     * Move the existing content and copy the new content.
     */
    new_len = VSTRING_LEN(vp) + len;
    VSTRING_SPACE(vp, len);
    memmove(vstring_str(vp) + len, vstring_str(vp), VSTRING_LEN(vp));
    memcpy(vstring_str(vp), buf, len);
    VSTRING_AT_OFFSET(vp, new_len);
    VSTRING_TERMINATE(vp);
    return (vp);
}

/* vstring_export - VSTRING to bare string */

char   *vstring_export(VSTRING *vp)
{
    char   *cp;

    cp = (char *) vp->vbuf.data;
    vp->vbuf.data = 0;
    myfree((char *) vp);
    return (cp);
}

/* vstring_import - bare string to vstring */

VSTRING *vstring_import(char *str)
{
    VSTRING *vp;
    ssize_t len;

    vp = (VSTRING *) mymalloc(sizeof(*vp));
    len = strlen(str);
    vp->vbuf.data = (unsigned char *) str;
    vp->vbuf.len = len + 1;
    VSTRING_AT_OFFSET(vp, len);
    vp->maxlen = 0;
    return (vp);
}

/* vstring_sprintf - formatted string */

VSTRING *vstring_sprintf(VSTRING *vp, const char *format,...)
{
    va_list ap;

    va_start(ap, format);
    vp = vstring_vsprintf(vp, format, ap);
    va_end(ap);
    return (vp);
}

/* vstring_vsprintf - format string, vsprintf-like interface */

VSTRING *vstring_vsprintf(VSTRING *vp, const char *format, va_list ap)
{
    VSTRING_RESET(vp);
    vbuf_print(&vp->vbuf, format, ap);
    VSTRING_TERMINATE(vp);
    return (vp);
}

/* vstring_sprintf_append - append formatted string */

VSTRING *vstring_sprintf_append(VSTRING *vp, const char *format,...)
{
    va_list ap;

    va_start(ap, format);
    vp = vstring_vsprintf_append(vp, format, ap);
    va_end(ap);
    return (vp);
}

/* vstring_vsprintf_append - format + append string, vsprintf-like interface */

VSTRING *vstring_vsprintf_append(VSTRING *vp, const char *format, va_list ap)
{
    vbuf_print(&vp->vbuf, format, ap);
    VSTRING_TERMINATE(vp);
    return (vp);
}

/* vstring_sprintf_prepend - format + prepend string, vsprintf-like interface */

VSTRING *vstring_sprintf_prepend(VSTRING *vp, const char *format,...)
{
    va_list ap;
    ssize_t old_len = VSTRING_LEN(vp);
    ssize_t result_len;

    /* Construct: old|new|free */
    va_start(ap, format);
    vp = vstring_vsprintf_append(vp, format, ap);
    va_end(ap);
    result_len = VSTRING_LEN(vp);

    /* Construct: old|new|old|free */
    VSTRING_SPACE(vp, old_len);
    vstring_memcat(vp, vstring_str(vp), old_len);

    /* Construct: new|old|free */
    memmove(vstring_str(vp), vstring_str(vp) + old_len, result_len);
    VSTRING_AT_OFFSET(vp, result_len);
    VSTRING_TERMINATE(vp);
    return (vp);
}

#ifdef TEST

 /*
  * Test program - concatenate all command-line arguments into one string.
  */
#include <stdio.h>

int     main(int argc, char **argv)
{
    VSTRING *vp = vstring_alloc(1);

    while (argc-- > 0) {
	vstring_strcat(vp, *argv++);
	vstring_strcat(vp, ".");
    }
    printf("argv concatenated: %s\n", vstring_str(vp));
    vstring_free(vp);
    return (0);
}

#endif
