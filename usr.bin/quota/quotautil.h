/*	$NetBSD: quotautil.h,v 1.3 2011/03/24 17:05:46 bouyer Exp $ */

const char *qfextension[MAXQUOTAS];
const char *qfname;
struct fstab;
int hasquota(char *, size_t, struct fstab *, int);
int oneof(const char *, char *[], int);
