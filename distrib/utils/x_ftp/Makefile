# $NetBSD: Makefile,v 1.6 1999/06/25 01:07:56 cgd Exp $
# Build a smaller ftp (i.e. for boot media)

PROG=	ftp
MKMAN=	no

# Need -lutil for: fparseln
DPADD= ${LIBUTIL}
LDADD= -lutil

SRCDIR= ${.CURDIR}/../../../usr.bin/ftp

SRCS=	cmds.c cmdtab.c complete.c domacro.c fetch.c ftp.c main.c ruserpass.c \
	util.c

CPPFLAGS+= -DNO_EDITCOMPLETE -DNO_ABOUT -I${SRCDIR}

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
