#	$NetBSD: Makefile,v 1.8.34.1 2012/05/23 10:08:23 yamt Exp $

USE_FORT?= yes	# data-driven bugs?
NOMAN=	   # defined

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

PROG=       bzip2recover

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?= -static
.endif

DIST=	   ${NETBSDSRCDIR}/external/bsd/bzip2/dist
.PATH:	   ${DIST}

.include <bsd.prog.mk>
