# $NetBSD: Makefile,v 1.13 2001/12/12 00:05:12 tv Exp $
# Build a smaller ftp (i.e. for boot media)

SRCDIR=		${.CURDIR}/../../../usr.bin/ftp

PROG=		ftp
NOMAN=		# defined

SRCS=		cmds.c cmdtab.c complete.c domacro.c fetch.c \
		ftp.c main.c ruserpass.c util.c

CPPFLAGS+=	-DNO_EDITCOMPLETE -DNO_ABOUT -I${SRCDIR}
.if (${MACHINE} == "i386" || ${MACHINE} == "hpcmips" || ${MACHINE} == "x68k")
CPPFLAGS+=	-DINET6
.endif

# Need -lutil for: fparseln
DPADD=		${LIBUTIL}
LDADD=		-lutil

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
