# $NetBSD: Makefile,v 1.5 2023/06/19 23:56:55 mrg Exp $

USE_FORT?= yes	# network protocol library

NOLINT=		# defined

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH:		${HEIMDIST}/lib/kafs

LIB=		kafs

LIBDPLIBS+=	krb5	${.CURDIR}/../libkrb5 \
		asn1	${.CURDIR}/../libasn1 \
		roken	${.CURDIR}/../libroken

SRCS=		afssys.c afskrb5.c common.c rxkad_kdf.c

CPPFLAGS+=				\
	-I${HEIMDIST}/lib

INCS=		kafs.h
INCSDIR=	/usr/include/krb5
MAN=		kafs.3

COPTS.rxkad_kdf.c+= -Wno-error=deprecated-declarations

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.lib.mk>
