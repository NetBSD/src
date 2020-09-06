#	$NetBSD: Makefile,v 1.8 2020/09/06 07:20:32 mrg Exp $

WARNS?=	1	# XXX -Wsign-compare -Wshadow

USE_FORT?= yes	# network server

PROG=	ndbootd
SRCS=	ndbootd.c
MAN=	ndbootd.8

CPPFLAGS+=-DHAVE_CONFIG_H -I${.CURDIR}

.if ${MACHINE_ARCH} == "m68000"
COPTS.ndbootd.c+=	-fno-tree-ter -fno-tree-lrs
.endif

CWARNFLAGS.clang+=	-Wno-tautological-compare
CWARNFLAGS.gcc+=        ${GCC_NO_ADDR_OF_PACKED_MEMBER}

.include <bsd.own.mk>
.include <bsd.prog.mk>
