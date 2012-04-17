/*	$NetBSD: quotautil.h,v 1.3.4.1 2012/04/17 00:09:39 yamt Exp $ */

extern const char *qfextension[MAXQUOTAS];
extern const char *qfname;
struct fstab;
int hasquota(char *, size_t, struct fstab *, int);
int oneof(const char *, char *[], int);
