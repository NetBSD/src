#	$NetBSD: Makefile,v 1.13 1998/07/25 11:21:24 mycroft Exp $

# XXX Temporary for USE_EGCS
.include <bsd.own.mk>

SUBDIR+= libmalloc
.ifdef USE_EGCS
SUBDIR+= libf2c libstdc++
.else
SUBDIR+= libg++
.endif

SUBDIR+= bfd

.include <bsd.subdir.mk>
