/*	$NetBSD: getvfsquota.h,v 1.3 2011/03/06 20:47:59 christos Exp $ */

int getvfsquota(const char *, struct quota2_entry *, int8_t *,
    uint32_t, int, int, int);

extern const char *qfextension[];
