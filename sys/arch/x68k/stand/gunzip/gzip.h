/*
 *	Interface for extracting gzip'ed data
 *
 *	Written by Yasha (ITOH Yasufumi)
 *	This code is in the public domain.
 */

#ifndef GZIP_H
#define GZIP_H

#ifdef __STDC__
# define OF(p) p
#else
# define const
# define OF(p) ()
#endif

typedef unsigned int uint32;
typedef unsigned char uch;
typedef unsigned short ush;
typedef unsigned long ulg;
typedef void *voidp;

/* magic number for gzip */
#define GZIP_MAGIC1		037
#define GZIP_MAGIC2		0213
#define GZIP_MAGIC2_OLD		0236	/* gzip 0.5 */

/* check gzip magic number */
#define check_gzip_magic(top)		\
	({ register const uch *t = top;	\
	 *t++ == GZIP_MAGIC1 && (*t == GZIP_MAGIC2 || *t == GZIP_MAGIC2_OLD);})

/* gzip compression method */
#define GZIP_METHOD_DEFLATED	8

/* gzip flags */
#define GZIP_FLAG_ASCII		0001
#define GZIP_FLAG_MULTIPART	0002
#define GZIP_FLAG_EXTRA		0004
#define GZIP_FLAG_FILE_NAME	0010
#define GZIP_FLAG_COMMENT	0020
#define GZIP_FLAG_ENCRYPTED	0040
#define GZIP_FLAGS_RESERVED	0300

/* the window size */
#define WSIZE 32768

extern const uch *csrc;		/* compressed data */
extern uch *udst;		/* uncompressed data */
extern long udst_cnt;
extern unsigned outcnt;
extern uch window[WSIZE];

int unzip OF((const uch *src, uch *dst));
#ifdef TEST
int inflate OF((void));
#endif
void flush_window OF((void));
void debug_print OF((const char *));
#define error(m) debug_print(m)

#define Tracecv(a, b)
#define Tracevv(a)

#ifndef NULL
# define NULL 0
#endif

#endif /* GZIP_H */
