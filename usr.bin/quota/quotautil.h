/*	$NetBSD: quotautil.h,v 1.4 2012/01/30 06:14:43 dholland Exp $ */

extern const char *qfextension[MAXQUOTAS];
extern const char *qfname;
struct fstab;
int hasquota(char *, size_t, struct fstab *, int);
int oneof(const char *, char *[], int);
