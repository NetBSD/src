#	$NetBSD: Makefile,v 1.12 1998/04/09 00:05:14 tv Exp $

# XXX Temporary for USE_EGCS
.include <bsd.own.mk>

SUBDIR+= libmalloc
.ifdef USE_EGCS
SUBDIR+= libf2c libstdc++
.else
SUBDIR+= libg++
.endif

SUBDIR+= bfd readline

.include <bsd.subdir.mk>
