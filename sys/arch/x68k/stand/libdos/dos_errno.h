/*
 *	dos_errno.h
 *	Human68k DOS call errors
 *
 *	written by Yasha (ITOH Yasufumi)
 *	public domain
 *
 *	$NetBSD: dos_errno.h,v 1.1 1998/09/01 19:53:26 itohy Exp $
 */

#ifndef X68K_DOS_ERRNO_H
#define X68K_DOS_ERRNO_H

#ifndef __ASSEMBLER__
#include <sys/cdefs.h>

extern int dos_errno;
extern __const int dos_nerr;

__const char * __pure dos_strerror __P((int)) __attribute__((__const__));
#endif

/*
 * The following block is used by  makstrerror.awk .
 * The error number shall be contiguous.
 */

/* dos_errlist begin */
#define DOS_ENOERROR	0	/* Undefined error: 0 */
#define DOS_EINVAL	1	/* Invalid argument */
#define DOS_ENOENT	2	/* No such file or directory */
#define DOS_ENOTDIR	3	/* Not a directory */
#define DOS_EMFILE	4	/* Too many open files */
#define DOS_EDIRVOL	5	/* Is a directory or volume label */
#define DOS_EBADF	6	/* Bad file descriptor */
#define DOS_EMCBCORRUPT	7	/* Memory control block corrupt */
#define DOS_ENOMEM	8	/* Cannot allocate memory */
#define DOS_EMCB	9	/* Illegal memory control block */
#define DOS_EENV	10	/* Invalid environment */
#define DOS_ENOEXEC	11	/* Exec format error */
#define DOS_EOPEN	12	/* Invalid access mode */
#define DOS_EFILENAME	13	/* Bad file name */
#define DOS_ERANGE	14	/* Argument out of range */
#define DOS_ENXIO	15	/* Device not configured */
#define DOS_EISCURDIR	16	/* Is a current directory */
#define DOS_EIOCTRL	17	/* No ioctrl for device */
#define DOS_EDIREND	18	/* No more files */
#define DOS_EROFILE	19	/* File is read-only */
#define DOS_EMKDIR	20	/* Can't mkdir -- file exists */
#define DOS_ENOTEMPTY	21	/* Directory not empty */
#define DOS_ERENAME	22	/* Can't rename -- file exists */
#define DOS_ENOSPC	23	/* No space left on device */
#define DOS_EDIRFUL	24	/* No more files in the directory */
#define DOS_ESEEK	25	/* Illegal seek */
#define DOS_ESUPER	26	/* Already in supervisor */
#define DOS_EPRNAME	27	/* Duplicated thread name */
#define DOS_EROBUF	28	/* Buffer is read-only */
#define DOS_EPROCFULL	29	/* No more processes */
#define DOS_E30		30	/* Unknown error: 30 */
#define DOS_E31		31	/* Unknown error: 31 */
#define DOS_ENOLCK	32	/* No locks available */
#define DOS_ELOCKED	33	/* File is locked */
#define DOS_EBUSY	34	/* Device busy */
#define DOS_ELOOP	35	/* Too many levels of symbolic links */
#define DOS_EEXIST	36	/* File exists */
#define DOS_EBUFOVER	37	/* Buffer overflow */
#define DOS_E38		38	/* Unknown error: 38 */
#define DOS_E39		39	/* Unknown error: 39 */
#define DOS_ESRCH	40	/* No such process */
/* dos_errlist end */

#define	DOS_ELAST	40	/* largest DOS errno */

#endif /* X68K_DOS_ERRNO_H */
