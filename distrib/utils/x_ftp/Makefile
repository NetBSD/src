# $NetBSD: Makefile,v 1.4 1999/05/19 04:17:50 gwr Exp $
# Build a smaller ftp (i.e. for boot media)

PROG=	ftp
MKMAN=	no

# Need -lutil for: fparseln
DPADD= ${LIBUTIL}
LDADD= -lutil

SRCDIR= ${BSDSRCDIR}/usr.bin/ftp

SRCS=	cmds.c cmdtab.c complete.c domacro.c fetch.c ftp.c main.c ruserpass.c \
	util.c

CPPFLAGS+= -DSMALL -I${SRCDIR}

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
