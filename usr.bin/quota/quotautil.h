/*	$NetBSD: quotautil.h,v 1.2 2011/03/06 23:26:16 christos Exp $ */

const char *qfextension[MAXQUOTAS];
const char *qfname;
struct fstab;
int hasquota(char *, size_t, struct fstab *, int);
int alldigits(const char *);
int oneof(const char *, char *[], int);
