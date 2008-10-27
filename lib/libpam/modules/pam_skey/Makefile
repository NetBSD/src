# $NetBSD: Makefile,v 1.3 2008/10/27 07:57:41 mrg Exp $

NOLINT=		# don't build a lint library
NOPROFILE=	# don't build a profile library
NOPICINSTALL=	# don't install _pic.a library

.include <bsd.own.mk>

LIB=	pam_skey
SRCS=	pam_skey.c
MAN=	pam_skey.8

LIBDPLIBS+=	skey	${.CURDIR}/../../../libskey

.include "${.CURDIR}/../mod.mk"
