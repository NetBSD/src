# $NetBSD: Makefile,v 1.1 2005/02/27 21:01:59 thorpej Exp $

NOLINT=		# don't build a lint library
NOPROFILE=	# don't build a profile library
NOPICINSTALL=	# don't install _pic.a library

.include <bsd.own.mk>

LIB=	pam_skey
SRCS=	pam_skey.c
MAN=	pam_skey.8

LIBDPLIBS=	skey	${LIB_ROOT_DIR}/libskey

.include "${.CURDIR}/../mod.mk"
