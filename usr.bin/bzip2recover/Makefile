#	$NetBSD: Makefile,v 1.9 2012/05/07 00:35:26 wiz Exp $

USE_FORT?= yes	# data-driven bugs?
NOMAN=	   # defined

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

PROG=       bzip2recover

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?= -static
.endif

DIST=	   ${NETBSDSRCDIR}/external/bsd/bzip2
.PATH:	   ${DIST}

.include <bsd.prog.mk>
