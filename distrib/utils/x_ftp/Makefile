# $NetBSD: Makefile,v 1.7 2000/05/04 03:27:09 itojun Exp $
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
.if (${MACHINE_ARCH} == "i386") || (${MACHINE_ARCH} == "hpcmips")
CPPFLAGS+= -DINET6
.endif

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
