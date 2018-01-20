# $NetBSD: Makefile,v 1.1.2.2 2018/01/20 19:36:29 perseant Exp $
# $FreeBSD: head/usr.bin/localedef/Makefile 290562 2015-11-08 22:23:21Z bapt $

.ifndef HOSTPROG
.include <bsd.own.mk>
.endif

.PATH: ${NETBSDSRCDIR}/lib/libc/locale ${NETBSDSRCDIR}/lib/libc/citrus ${NETBSDSRCDIR}/lib/libc/stdio

PROG=	localedef
SRCS+=	parser.y \
	charmap.c \
	collate.c \
	ctype.c \
	localedef.c \
	messages.c \
	monetary.c \
	numeric.c \
	scanner.c \
	time.c \
	wide.c
SRCS+=	iso2022.c mklocaledb.c citrus_db_factory.c citrus_trie.c

WARNS=	3
YHEADER=1

parser.h: parser.y

IGNORE_PRAGMA=	yes

CPPFLAGS+=	-I. -I${.CURDIR}
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/locale
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/citrus
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/stdtime
#CPPFLAGS+=	-I${NETBSDSRCDIR}/include
#CPPFLAGS+=	-I${NETBSDSRCDIR}/sys
CPPFLAGS+=	-DHOSTPROG

CPPFLAGS.iso2022.c+= -I../../lib/libc/citrus -I../../lib/libc/locale -Wno-cast-qual -Wno-pointer-sign
CPPFLAGS.mklocaledb.c+= -I../../lib/libc/citrus -I../../lib/libc/locale -Wno-cast-qual -Wno-pointer-sign
CPPFLAGS.messages.c+= -I../../lib/libc/locale

CLEANFILES+=	parser.c parser.h

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
