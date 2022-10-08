/*	$NetBSD: vstream.h,v 1.4 2022/10/08 16:12:50 christos Exp $	*/

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
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <stdarg.h>
#include <setjmp.h>
#include <unistd.h>

 /*
  * Utility library.
  */
#include <vbuf.h>
#include <check_arg.h>

 /*
  * Simple buffered stream. The members of this structure are not part of the
  * official interface and can change without prior notice.
  */
typedef ssize_t (*VSTREAM_RW_FN) (int, void *, size_t, int, void *);
typedef pid_t(*VSTREAM_WAITPID_FN) (pid_t, WAIT_STATUS_T *, int);

#ifdef NO_SIGSETJMP
#define VSTREAM_JMP_BUF	jmp_buf
#else
#define VSTREAM_JMP_BUF	sigjmp_buf
#endif

typedef struct VSTREAM {
    VBUF    buf;			/* generic intelligent buffer */
    int     fd;				/* file handle, no 256 limit */
    VSTREAM_RW_FN read_fn;		/* buffer fill action */
    VSTREAM_RW_FN write_fn;		/* buffer flush action */
    ssize_t req_bufsize;		/* requested read/write buffer size */
    void   *context;			/* application context */
    off_t   offset;			/* cached seek info */
    char   *path;			/* give it at least try */
    int     read_fd;			/* read channel (double-buffered) */
    int     write_fd;			/* write channel (double-buffered) */
    VBUF    read_buf;			/* read buffer (double-buffered) */
    VBUF    write_buf;			/* write buffer (double-buffered) */
    pid_t   pid;			/* vstream_popen/close() */
    VSTREAM_WAITPID_FN waitpid_fn;	/* vstream_popen/close() */
    int     timeout;			/* read/write timeout */
    VSTREAM_JMP_BUF *jbuf;		/* exception handling */
    struct timeval iotime;		/* time of last fill/flush */
    struct timeval time_limit;		/* read/write time limit */
    int     min_data_rate;		/* min data rate for time limit */
    struct VSTRING *vstring;		/* memory-backed stream */
} VSTREAM;

extern VSTREAM vstream_fstd[];		/* pre-defined streams */

#define VSTREAM_IN		(&vstream_fstd[0])
#define VSTREAM_OUT		(&vstream_fstd[1])
#define VSTREAM_ERR		(&vstream_fstd[2])

#define VSTREAM_FLAG_RD_ERR	VBUF_FLAG_RD_ERR	/* read error */
#define VSTREAM_FLAG_WR_ERR	VBUF_FLAG_WR_ERR	/* write error */
#define VSTREAM_FLAG_RD_TIMEOUT	VBUF_FLAG_RD_TIMEOUT	/* read timeout */
#define VSTREAM_FLAG_WR_TIMEOUT	VBUF_FLAG_WR_TIMEOUT	/* write timeout */

#define	VSTREAM_FLAG_ERR	VBUF_FLAG_ERR	/* some I/O error */
#define VSTREAM_FLAG_EOF	VBUF_FLAG_EOF	/* end of file */
#define VSTREAM_FLAG_TIMEOUT	VBUF_FLAG_TIMEOUT	/* timeout error */
#define VSTREAM_FLAG_FIXED	VBUF_FLAG_FIXED	/* fixed-size buffer */
#define VSTREAM_FLAG_BAD	VBUF_FLAG_BAD

/* Flags 1<<24 and above are reserved for VSTRING. */
#define VSTREAM_FLAG_READ	(1<<8)	/* read buffer */
#define VSTREAM_FLAG_WRITE	(1<<9)	/* write buffer */
#define VSTREAM_FLAG_SEEK	(1<<10)	/* seek info valid */
#define VSTREAM_FLAG_NSEEK	(1<<11)	/* can't seek this file */
#define VSTREAM_FLAG_DOUBLE	(1<<12)	/* double buffer */
#define VSTREAM_FLAG_DEADLINE	(1<<13)	/* deadline active */
#define VSTREAM_FLAG_MEMORY	(1<<14)	/* internal stream */
#define VSTREAM_FLAG_OWN_VSTRING (1<<15)/* owns VSTRING resource */

#define VSTREAM_PURGE_READ	(1<<0)	/* flush unread data */
#define VSTREAM_PURGE_WRITE	(1<<1)	/* flush unwritten data */
#define VSTREAM_PURGE_BOTH	(VSTREAM_PURGE_READ|VSTREAM_PURGE_WRITE)

#define VSTREAM_BUFSIZE		4096

extern VSTREAM *vstream_fopen(const char *, int, mode_t);
extern int vstream_fclose(VSTREAM *);
extern off_t WARN_UNUSED_RESULT vstream_fseek(VSTREAM *, off_t, int);
extern off_t vstream_ftell(VSTREAM *);
extern int vstream_fpurge(VSTREAM *, int);
extern int vstream_fflush(VSTREAM *);
extern int vstream_fputs(const char *, VSTREAM *);
extern VSTREAM *vstream_fdopen(int, int);
extern int vstream_fdclose(VSTREAM *);

#define vstream_fread(v, b, n)	vbuf_read(&(v)->buf, (b), (n))
#define vstream_fwrite(v, b, n)	vbuf_write(&(v)->buf, (b), (n))

#define VSTREAM_PUTC(ch, vp)	VBUF_PUT(&(vp)->buf, (ch))
#define VSTREAM_GETC(vp)	VBUF_GET(&(vp)->buf)
#define vstream_ungetc(vp, ch)	vbuf_unget(&(vp)->buf, (ch))
#define VSTREAM_EOF		VBUF_EOF

#define VSTREAM_PUTCHAR(ch)	VSTREAM_PUTC((ch), VSTREAM_OUT)
#define VSTREAM_GETCHAR()	VSTREAM_GETC(VSTREAM_IN)

#define vstream_fileno(vp)	((vp)->fd)
#define vstream_req_bufsize(vp)	((const ssize_t) ((vp)->req_bufsize))
#define vstream_context(vp)	((vp)->context)
#define vstream_rd_error(vp)	vbuf_rd_error(&(vp)->buf)
#define vstream_wr_error(vp)	vbuf_wr_error(&(vp)->buf)
#define vstream_ferror(vp)	vbuf_error(&(vp)->buf)
#define vstream_feof(vp)	vbuf_eof(&(vp)->buf)
#define vstream_rd_timeout(vp)	vbuf_rd_timeout(&(vp)->buf)
#define vstream_wr_timeout(vp)	vbuf_wr_timeout(&(vp)->buf)
#define vstream_ftimeout(vp)	vbuf_timeout(&(vp)->buf)
#define vstream_clearerr(vp)	vbuf_clearerr(&(vp)->buf)
#define VSTREAM_PATH(vp)	((vp)->path ? (const char *) (vp)->path : "unknown_stream")
#define vstream_ftime(vp)	((time_t) ((vp)->iotime.tv_sec))
#define vstream_ftimeval(vp)	((vp)->iotime)

#define vstream_fstat(vp, fl)	((vp)->buf.flags & (fl))

extern ssize_t vstream_fread_buf(VSTREAM *, struct VSTRING *, ssize_t);
extern ssize_t vstream_fread_app(VSTREAM *, struct VSTRING *, ssize_t);
extern void vstream_control(VSTREAM *, int,...);

/* Legacy API: type-unchecked arguments, internal use. */
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
#ifdef F_DUPFD
#define VSTREAM_CTL_DUPFD	11
#endif
#define VSTREAM_CTL_BUFSIZE	12
#define VSTREAM_CTL_SWAP_FD	13
#define VSTREAM_CTL_START_DEADLINE 14
#define VSTREAM_CTL_STOP_DEADLINE 15
#define VSTREAM_CTL_OWN_VSTRING	16
#define VSTREAM_CTL_MIN_DATA_RATE 17

/* Safer API: type-checked arguments, external use. */
#define CA_VSTREAM_CTL_END		VSTREAM_CTL_END
#define CA_VSTREAM_CTL_READ_FN(v)	VSTREAM_CTL_READ_FN, CHECK_VAL(VSTREAM_CTL, VSTREAM_RW_FN, (v))
#define CA_VSTREAM_CTL_WRITE_FN(v)	VSTREAM_CTL_WRITE_FN, CHECK_VAL(VSTREAM_CTL, VSTREAM_RW_FN, (v))
#define CA_VSTREAM_CTL_PATH(v)		VSTREAM_CTL_PATH, CHECK_CPTR(VSTREAM_CTL, char, (v))
#define CA_VSTREAM_CTL_DOUBLE		VSTREAM_CTL_DOUBLE
#define CA_VSTREAM_CTL_READ_FD(v)	VSTREAM_CTL_READ_FD, CHECK_VAL(VSTREAM_CTL, int, (v))
#define CA_VSTREAM_CTL_WRITE_FD(v)	VSTREAM_CTL_WRITE_FD, CHECK_VAL(VSTREAM_CTL, int, (v))
#define CA_VSTREAM_CTL_WAITPID_FN(v)	VSTREAM_CTL_WAITPID_FN, CHECK_VAL(VSTREAM_CTL, VSTREAM_WAITPID_FN, (v))
#define CA_VSTREAM_CTL_TIMEOUT(v)	VSTREAM_CTL_TIMEOUT, CHECK_VAL(VSTREAM_CTL, int, (v))
#define CA_VSTREAM_CTL_EXCEPT		VSTREAM_CTL_EXCEPT
#define CA_VSTREAM_CTL_CONTEXT(v)	VSTREAM_CTL_CONTEXT, CHECK_PTR(VSTREAM_CTL, void, (v))
#ifdef F_DUPFD
#define CA_VSTREAM_CTL_DUPFD(v)		VSTREAM_CTL_DUPFD, CHECK_VAL(VSTREAM_CTL, int, (v))
#endif
#define CA_VSTREAM_CTL_BUFSIZE(v)	VSTREAM_CTL_BUFSIZE, CHECK_VAL(VSTREAM_CTL, ssize_t, (v))
#define CA_VSTREAM_CTL_SWAP_FD(v)	VSTREAM_CTL_SWAP_FD, CHECK_PTR(VSTREAM_CTL, VSTREAM, (v))
#define CA_VSTREAM_CTL_START_DEADLINE	VSTREAM_CTL_START_DEADLINE
#define CA_VSTREAM_CTL_STOP_DEADLINE	VSTREAM_CTL_STOP_DEADLINE
#define CA_VSTREAM_CTL_MIN_DATA_RATE(v)	VSTREAM_CTL_MIN_DATA_RATE, CHECK_VAL(VSTREAM_CTL, int, (v))

CHECK_VAL_HELPER_DCL(VSTREAM_CTL, ssize_t);
CHECK_VAL_HELPER_DCL(VSTREAM_CTL, int);
CHECK_VAL_HELPER_DCL(VSTREAM_CTL, VSTREAM_WAITPID_FN);
CHECK_VAL_HELPER_DCL(VSTREAM_CTL, VSTREAM_RW_FN);
CHECK_PTR_HELPER_DCL(VSTREAM_CTL, void);
CHECK_PTR_HELPER_DCL(VSTREAM_CTL, VSTREAM);
CHECK_CPTR_HELPER_DCL(VSTREAM_CTL, char);

extern VSTREAM *PRINTFLIKE(1, 2) vstream_printf(const char *,...);
extern VSTREAM *PRINTFLIKE(2, 3) vstream_fprintf(VSTREAM *, const char *,...);

extern VSTREAM *vstream_popen(int,...);
extern int vstream_pclose(VSTREAM *);

#define vstream_ispipe(vp)	((vp)->pid != 0)

/* Legacy API: type-unchecked arguments, internal use. */
#define VSTREAM_POPEN_END	0	/* terminator */
#define VSTREAM_POPEN_COMMAND	1	/* command is string */
#define VSTREAM_POPEN_ARGV	2	/* command is array */
#define VSTREAM_POPEN_UID	3	/* privileges */
#define VSTREAM_POPEN_GID	4	/* privileges */
#define VSTREAM_POPEN_ENV	5	/* extra environment */
#define VSTREAM_POPEN_SHELL	6	/* alternative shell */
#define VSTREAM_POPEN_WAITPID_FN 7	/* child catcher, waitpid() compat. */
#define VSTREAM_POPEN_EXPORT	8	/* exportable environment */

/* Safer API: type-checked arguments, external use. */
#define CA_VSTREAM_POPEN_END		VSTREAM_POPEN_END
#define CA_VSTREAM_POPEN_COMMAND(v)	VSTREAM_POPEN_COMMAND, CHECK_CPTR(VSTREAM_PPN, char, (v))
#define CA_VSTREAM_POPEN_ARGV(v)	VSTREAM_POPEN_ARGV, CHECK_PPTR(VSTREAM_PPN, char, (v))
#define CA_VSTREAM_POPEN_UID(v)		VSTREAM_POPEN_UID, CHECK_VAL(VSTREAM_PPN, uid_t, (v))
#define CA_VSTREAM_POPEN_GID(v)		VSTREAM_POPEN_GID, CHECK_VAL(VSTREAM_PPN, gid_t, (v))
#define CA_VSTREAM_POPEN_ENV(v)		VSTREAM_POPEN_ENV, CHECK_PPTR(VSTREAM_PPN, char, (v))
#define CA_VSTREAM_POPEN_SHELL(v)	VSTREAM_POPEN_SHELL, CHECK_CPTR(VSTREAM_PPN, char, (v))
#define CA_VSTREAM_POPEN_WAITPID_FN(v)	VSTREAM_POPEN_WAITPID_FN, CHECK_VAL(VSTREAM_PPN, VSTREAM_WAITPID_FN, (v))
#define CA_VSTREAM_POPEN_EXPORT(v)	VSTREAM_POPEN_EXPORT, CHECK_PPTR(VSTREAM_PPN, char, (v))

CHECK_VAL_HELPER_DCL(VSTREAM_PPN, uid_t);
CHECK_VAL_HELPER_DCL(VSTREAM_PPN, gid_t);
CHECK_VAL_HELPER_DCL(VSTREAM_PPN, VSTREAM_WAITPID_FN);
CHECK_PPTR_HELPER_DCL(VSTREAM_PPN, char);
CHECK_CPTR_HELPER_DCL(VSTREAM_PPN, char);

extern VSTREAM *vstream_vprintf(const char *, va_list);
extern VSTREAM *vstream_vfprintf(VSTREAM *, const char *, va_list);

extern ssize_t vstream_peek(VSTREAM *);
extern ssize_t vstream_bufstat(VSTREAM *, int);

#define VSTREAM_BST_FLAG_IN		(1<<0)
#define VSTREAM_BST_FLAG_OUT		(1<<1)
#define VSTREAM_BST_FLAG_PEND		(1<<2)

#define VSTREAM_BST_MASK_DIR	(VSTREAM_BST_FLAG_IN | VSTREAM_BST_FLAG_OUT)
#define VSTREAM_BST_IN_PEND	(VSTREAM_BST_FLAG_IN | VSTREAM_BST_FLAG_PEND)
#define VSTREAM_BST_OUT_PEND	(VSTREAM_BST_FLAG_OUT | VSTREAM_BST_FLAG_PEND)

#define vstream_peek(vp) vstream_bufstat((vp), VSTREAM_BST_IN_PEND)

extern const char *vstream_peek_data(VSTREAM *);

 /*
  * Exception handling. We use pointer to jmp_buf to avoid a lot of unused
  * baggage for streams that don't need this functionality.
  * 
  * XXX sigsetjmp()/siglongjmp() save and restore the signal mask which can
  * avoid surprises in code that manipulates signals, but unfortunately some
  * systems have bugs in their implementation.
  */
#ifdef NO_SIGSETJMP
#define vstream_setjmp(stream)		setjmp((stream)->jbuf[0])
#define vstream_longjmp(stream, val)	longjmp((stream)->jbuf[0], (val))
#else
#define vstream_setjmp(stream)		sigsetjmp((stream)->jbuf[0], 1)
#define vstream_longjmp(stream, val)	siglongjmp((stream)->jbuf[0], (val))
#endif

 /*
  * Tweaks and workarounds.
  */
extern int vstream_tweak_sock(VSTREAM *);
extern int vstream_tweak_tcp(VSTREAM *);

#define vstream_flags(stream) ((const int) (stream)->buf.flags)

 /*
  * Read/write VSTRING memory.
  */
#define vstream_memopen(string, flags) \
	vstream_memreopen((VSTREAM *) 0, (string), (flags))
VSTREAM *vstream_memreopen(VSTREAM *, struct VSTRING *, int);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

#endif
