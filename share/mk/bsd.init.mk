#	$NetBSD: bsd.init.mk,v 1.1 2001/11/02 05:21:50 tv Exp $

# <bsd.init.mk> includes Makefile.inc and <bsd.own.mk>; this is used at the
# top of all <bsd.*.mk> files which actually "build something".

.if !target(__initialized__)
__initialized__:
.-include "${.CURDIR}/../Makefile.inc"
.include <bsd.own.mk>
.MAIN:		all
.endif
