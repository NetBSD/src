#ifndef _VSTREAM_H_INCLUDED_
#define _VSTREAM_H_INCLUDED_

/*++
/* NAME
/*	vstream 3h
/* SUMMARY
/*	simple buffered I/O package
/* SYNOPSIS
/*	#include <vstream.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <time.h>
#include <fcntl.h>
#include <stdarg.h>
#include <setjmp.h>
#include <unistd.h>

 /*
  * Utility library.
  */
#include <vbuf.h>

 /*
  * Simple buffered stream. The members of this structure are not part of the
  * official interface and can change without prior notice.
  */
typedef int (*VSTREAM_FN) (int, void *, unsigned, int, void *);
typedef int (*VSTREAM_WAITPID_FN) (pid_t, WAIT_STATUS_T *, int);

typedef struct VSTREAM {
    VBUF    buf;			/* generic intelligent buffer */
    int     fd;				/* file handle, no 256 limit */
    VSTREAM_FN read_fn;			/* buffer fill action */
    VSTREAM_FN write_fn;		/* buffer fill action */
    void   *context;			/* application context */
    off_t   offset;			/* cached seek info */
    char   *path;			/* give it at least try */
    int     read_fd;			/* read channel (double-buffered) */
    int     write_fd;			/* write channel (double-buffered) */
    VBUF    read_buf;			/* read buffer (double-buffered) */
    VBUF    write_buf;			/* write buffer (double-buffered) */
    pid_t   pid;			/* vstream_popen/close() */
    VSTREAM_WAITPID_FN waitpid_fn;	/* vstream_popen/close() */
    int     timeout;			/* read/write timout */
    jmp_buf *jbuf;			/* exception handling */
    time_t  iotime;			/* time of last fill/flush */
} VSTREAM;

extern VSTREAM vstream_fstd[];		/* pre-defined streams */

#define VSTREAM_IN		(&vstream_fstd[0])
#define VSTREAM_OUT		(&vstream_fstd[1])
#define VSTREAM_ERR		(&vstream_fstd[2])

#define	VSTREAM_FLAG_ERR	VBUF_FLAG_ERR	/* some I/O error */
#define VSTREAM_FLAG_EOF	VBUF_FLAG_EOF	/* end of file */
#define VSTREAM_FLAG_TIMEOUT	VBUF_FLAG_TIMEOUT	/* timeout error */
#define VSTREAM_FLAG_FIXED	VBUF_FLAG_FIXED	/* fixed-size buffer */
#define VSTREAM_FLAG_BAD	VBUF_FLAG_BAD

#define VSTREAM_FLAG_READ	(1<<8)	/* read buffer */
#define VSTREAM_FLAG_WRITE	(1<<9)	/* write buffer */
#define VSTREAM_FLAG_SEEK	(1<<10)	/* seek info valid */
#define VSTREAM_FLAG_NSEEK	(1<<11)	/* can't seek this file */
#define VSTREAM_FLAG_DOUBLE	(1<<12)	/* double buffer */

#define VSTREAM_BUFSIZE		4096

extern VSTREAM *vstream_fopen(const char *, int, int);
extern int vstream_fclose(VSTREAM *);
extern off_t vstream_fseek(VSTREAM *, off_t, int);
extern off_t vstream_ftell(VSTREAM *);
extern int vstream_fflush(VSTREAM *);
extern int vstream_fputs(const char *, VSTREAM *);
extern VSTREAM *vstream_fdopen(int, int);

#define vstream_fread(v, b, n)	vbuf_read(&(v)->buf, (b), (n))
#define vstream_fwrite(v, b, n)	vbuf_write(&(v)->buf, (b), (n))

#define VSTREAM_PUTC(ch, vp)	VBUF_PUT(&(vp)->buf, (ch))
#define VSTREAM_GETC(vp)	VBUF_GET(&(vp)->buf)
#define vstream_ungetc(vp, ch)	vbuf_unget(&(vp)->buf, (ch))
#define VSTREAM_EOF		VBUF_EOF

#define VSTREAM_PUTCHAR(ch)	VSTREAM_PUTC((ch), VSTREAM_OUT)
#define VSTREAM_GETCHAR()	VSTREAM_GETC(VSTREAM_IN)

#define vstream_fileno(vp)	((vp)->fd)
#define vstream_context(vp)	((vp)->context)
#define vstream_ferror(vp)	vbuf_error(&(vp)->buf)
#define vstream_feof(vp)	vbuf_eof(&(vp)->buf)
#define vstream_ftimeout(vp)	vbuf_timeout(&(vp)->buf)
#define vstream_clearerr(vp)	vbuf_clearerr(&(vp)->buf)
#define VSTREAM_PATH(vp)	((vp)->path ? (vp)->path : "unknown_stream")
#define vstream_ftime(vp)	((vp)->iotime)

extern void vstream_control(VSTREAM *, int,...);

#define VSTREAM_CTL_END		0
#define VSTREAM_CTL_READ_FN	1
#define VSTREAM_CTL_WRITE_FN	2
#define VSTREAM_CTL_PATH	3
#define VSTREAM_CTL_DOUBLE	4
#define VSTREAM_CTL_READ_FD	5
#define VSTREAM_CTL_WRITE_FD	6
#define VSTREAM_CTL_WAITPID_FN	7
#define VSTREAM_CTL_TIMEOUT	8
#define VSTREAM_CTL_EXCEPT	9
#define VSTREAM_CTL_CONTEXT	10

extern VSTREAM *PRINTFLIKE(1, 2) vstream_printf(const char *,...);
extern VSTREAM *PRINTFLIKE(2, 3) vstream_fprintf(VSTREAM *, const char *,...);

extern VSTREAM *vstream_popen(int,...);
extern int vstream_pclose(VSTREAM *);

#define vstream_ispipe(vp)	((vp)->pid != 0)

#define VSTREAM_POPEN_END	0	/* terminator */
#define VSTREAM_POPEN_COMMAND	1	/* command is string */
#define VSTREAM_POPEN_ARGV	2	/* command is array */
#define VSTREAM_POPEN_UID	3	/* privileges */
#define VSTREAM_POPEN_GID	4	/* privileges */
#define VSTREAM_POPEN_ENV	5	/* extra environment */
#define VSTREAM_POPEN_SHELL	6	/* alternative shell */
#define VSTREAM_POPEN_WAITPID_FN 7	/* child catcher, waitpid() compat. */
#define VSTREAM_POPEN_EXPORT	8	/* exportable environment */

extern VSTREAM *vstream_vfprintf(VSTREAM *, const char *, va_list);

extern int vstream_peek(VSTREAM *);

 /*
  * Exception handling. We use pointer to jmp_buf to avoid a lot of unused
  * baggage for streams that don't need this functionality.
  */
#define vstream_setjmp(stream)		setjmp((stream)->jbuf[0])
#define vstream_longjmp(stream, val)	longjmp((stream)->jbuf[0], (val))

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
