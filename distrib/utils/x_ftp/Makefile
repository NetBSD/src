# $NetBSD: Makefile,v 1.1.2.2 1998/12/14 04:31:55 cgd Exp $
# Build a smaller ftp (i.e. for boot media)

PROG=	ftp
NOMAN=	

SRCDIR= ${.CURDIR}/../../../usr.bin/ftp

SRCS=	cmds.c cmdtab.c complete.c domacro.c fetch.c ftp.c main.c ruserpass.c \
	util.c fparseln.c

CPPFLAGS+= -DSMALL -I${SRCDIR}

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
