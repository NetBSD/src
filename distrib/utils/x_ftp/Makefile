# $NetBSD: Makefile,v 1.15 2002/10/17 02:05:39 lukem Exp $
# Build a smaller ftp (i.e. for boot media)

SRCDIR=		${.CURDIR}/../../../usr.bin/ftp

PROG=		ftp
NOMAN=		# defined

SRCS=		cmds.c cmdtab.c complete.c domacro.c fetch.c \
		ftp.c main.c ruserpass.c util.c

CPPFLAGS+=	-DNO_EDITCOMPLETE -DNO_ABOUT -I${SRCDIR}
.if defined(SMALLPROG_INET6)
CPPFLAGS+=	-DINET6
.endif

# Need -lutil for: fparseln
DPADD=		${LIBUTIL}
LDADD=		-lutil

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
