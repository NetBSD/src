# $NetBSD: Makefile,v 1.9.4.1 2000/09/07 01:36:31 minoura Exp $
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
.if (${MACHINE} == "i386" || ${MACHINE} == "hpcmips" || ${MACHINE} == "x68k")
CPPFLAGS+= -DINET6
.endif
COPTS+= -Os
LDSTATIC?=-static

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
