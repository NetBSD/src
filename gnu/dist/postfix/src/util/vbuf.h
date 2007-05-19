/*	$NetBSD: vbuf.h,v 1.1.1.4 2007/05/19 16:28:49 heas Exp $	*/

#ifndef _VBUF_H_INCLUDED_
#define _VBUF_H_INCLUDED_

/*++
/* NAME
/*	vbuf 3h
/* SUMMARY
/*	generic buffer
/* SYNOPSIS
/*	#include <vbuf.h>
/* DESCRIPTION
/* .nf

 /*
  * The VBUF buffer is defined by 1) its structure, by 2) the VBUF_GET() and
  * 3) VBUF_PUT() operations that automatically handle buffer empty and
  * buffer full conditions, and 4) by the VBUF_SPACE() operation that allows
  * the user to reserve buffer space ahead of time, to allow for situations
  * where calling VBUF_PUT() is not possible or desirable.
  *
  * The VBUF buffer does not specify primitives for memory allocation or
  * deallocation. The purpose is to allow different applications to have
  * different strategies: a memory-resident buffer; a memory-mapped file; or
  * a stdio-like window to an open file. Each application provides its own
  * get(), put() and space() methods that perform the necessary magic.
  *
  * This interface is pretty normal. With one exception: the number of bytes
  * left to read is negated. This is done so that we can change direction
  * between reading and writing on the fly. The alternative would be to use
  * separate read and write counters per buffer.
  */
typedef struct VBUF VBUF;
typedef int (*VBUF_GET_READY_FN) (VBUF *);
typedef int (*VBUF_PUT_READY_FN) (VBUF *);
typedef int (*VBUF_SPACE_FN) (VBUF *, ssize_t);

struct VBUF {
    int     flags;			/* status, see below */
    unsigned char *data;		/* variable-length buffer */
    ssize_t len;			/* buffer length */
    ssize_t cnt;			/* bytes left to read/write */
    unsigned char *ptr;			/* read/write position */
    VBUF_GET_READY_FN get_ready;	/* read buffer empty action */
    VBUF_PUT_READY_FN put_ready;	/* write buffer full action */
    VBUF_SPACE_FN space;		/* request for buffer space */
};

 /*
  * Typically, an application will embed a VBUF structure into a larger
  * structure that also contains application-specific members. This approach
  * gives us the best of both worlds. The application can still use the
  * generic VBUF primitives for reading and writing VBUFs. The macro below
  * transforms a pointer from VBUF structure to the structure that contains
  * it.
  */
#define VBUF_TO_APPL(vbuf_ptr,app_type,vbuf_member) \
    ((app_type *) (((char *) (vbuf_ptr)) - offsetof(app_type,vbuf_member)))

 /*
  * Buffer status management.
  */
#define	VBUF_FLAG_ERR	(1<<0)		/* some I/O error */
#define VBUF_FLAG_EOF	(1<<1)		/* end of data */
#define VBUF_FLAG_TIMEOUT (1<<2)	/* timeout error */
#define VBUF_FLAG_BAD	(VBUF_FLAG_ERR | VBUF_FLAG_EOF | VBUF_FLAG_TIMEOUT)
#define VBUF_FLAG_FIXED	(1<<3)		/* fixed-size buffer */

#define vbuf_error(v)	((v)->flags & (VBUF_FLAG_ERR | VBUF_FLAG_TIMEOUT))
#define vbuf_eof(v)	((v)->flags & VBUF_FLAG_EOF)
#define vbuf_timeout(v)	((v)->flags & VBUF_FLAG_TIMEOUT)
#define vbuf_clearerr(v) ((v)->flags &= ~VBUF_FLAG_BAD)

 /*
  * Buffer I/O-like operations and results.
  */
#define VBUF_GET(v)	((v)->cnt < 0 ? ++(v)->cnt, \
				(int) *(v)->ptr++ : vbuf_get(v))
#define VBUF_PUT(v,c)	((v)->cnt > 0 ? --(v)->cnt, \
				(int) (*(v)->ptr++ = (c)) : vbuf_put((v),(c)))
#define VBUF_SPACE(v,n) ((v)->space((v),(n)))

#define VBUF_EOF	(-1)		/* no more space or data */

extern int vbuf_get(VBUF *);
extern int vbuf_put(VBUF *, int);
extern int vbuf_unget(VBUF *, int);
extern ssize_t vbuf_read(VBUF *, char *, ssize_t);
extern ssize_t vbuf_write(VBUF *, const char *, ssize_t);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

#endif
