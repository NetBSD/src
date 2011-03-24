/*	$NetBSD: getvfsquota.h,v 1.4 2011/03/24 17:05:46 bouyer Exp $ */

int getvfsquota(const char *, struct ufs_quota_entry *, int8_t *,
    uint32_t, int, int, int);
