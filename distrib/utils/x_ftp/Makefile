# $NetBSD: Makefile,v 1.3 1999/02/13 02:54:19 lukem Exp $
# Build a smaller ftp (i.e. for boot media)

PROG=	ftp
MKMAN=	no

# Need -lutil for: fparseln
DPADD= ${LIBUTIL}
LDADD= -lutil

SRCDIR= ${.CURDIR}/../../../usr.bin/ftp

SRCS=	cmds.c cmdtab.c complete.c domacro.c fetch.c ftp.c main.c ruserpass.c \
	util.c

CPPFLAGS+= -DSMALL -I${SRCDIR}

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
