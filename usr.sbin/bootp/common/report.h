/*	$NetBSD: report.h,v 1.3.2.1 2000/10/19 17:05:04 he Exp $	*/

/* report.h */

#ifdef	__STDC__
#define P(args) args
#else
#define P(args) ()
#endif

extern void report_init P((int nolog));
extern void report P((int, const char *, ...))
     __attribute__((__format__(__printf__, 2, 3)));
extern const char *get_errmsg P((void));

#undef P
