# $NetBSD: Makefile,v 1.1 1997/11/17 01:35:57 cjs Exp $
# Build a smaller ftp (i.e. for boot media)

PROG=	ftp
NOMAN=	

SRCDIR= ${.CURDIR}/../../../usr.bin/ftp

SRCS=	cmds.c cmdtab.c complete.c domacro.c fetch.c ftp.c main.c ruserpass.c \
	util.c

CPPFLAGS+= -DSMALL -I${SRCDIR}

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
