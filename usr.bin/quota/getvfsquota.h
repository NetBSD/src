/*	$NetBSD: getvfsquota.h,v 1.1.2.2 2011/01/30 19:38:45 bouyer Exp $ */

int getvfsquota(const char *, struct quota2_entry *, int8_t *,
    long, int, int, int);

extern const char *qfextension[];
