/*	$NetBSD: report.h,v 1.5 2002/07/14 00:26:18 wiz Exp $	*/

/* report.h */

extern void report_init(int);
extern void report(int, const char *, ...)
     __attribute__((__format__(__printf__, 2, 3)));
extern const char *get_errmsg(void);
