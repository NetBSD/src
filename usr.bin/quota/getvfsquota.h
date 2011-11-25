/*	$NetBSD: getvfsquota.h,v 1.5 2011/11/25 16:55:05 dholland Exp $ */

int getvfsquota(const char *, struct quotaval *, int8_t *,
    uint32_t, int, int, int);
