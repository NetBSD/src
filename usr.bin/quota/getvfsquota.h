/*	$NetBSD: getvfsquota.h,v 1.2 2011/03/06 17:08:42 bouyer Exp $ */

int getvfsquota(const char *, struct quota2_entry *, int8_t *,
    long, int, int, int);

extern const char *qfextension[];
