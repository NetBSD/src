# $NetBSD: Makefile,v 1.12 2001/10/05 01:17:30 jmc Exp $
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
.if ${MACHINE_ARCH} != "sparc64"
DBG= -Os
.endif
LDSTATIC?=-static

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
