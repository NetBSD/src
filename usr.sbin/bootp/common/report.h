/*	$NetBSD: report.h,v 1.3 1998/07/26 15:33:34 mycroft Exp $	*/

/* report.h */

#ifdef	__STDC__
#define P(args) args
#else
#define P(args) ()
#endif

extern void report_init P((int nolog));
extern void report P((int, char *, ...));
extern const char *get_errmsg P((void));

#undef P
