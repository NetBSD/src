/*	$NetBSD: getvfsquota.h,v 1.1.2.1 2011/01/28 22:15:36 bouyer Exp $ */

int getvfsquota(const char *, struct quota2_entry *, long, int, int, int);

extern const char *qfextension[];
