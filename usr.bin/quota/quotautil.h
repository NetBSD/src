/*	$NetBSD: quotautil.h,v 1.1 2011/03/06 22:36:07 christos Exp $ */

const char *qfextension[MAXQUOTAS];
const char *qfname;
struct fstab;
int hasquota(char *, size_t, struct fstab *, int);
int alldigits(const char *);
