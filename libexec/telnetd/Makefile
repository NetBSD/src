#	$NetBSD: Makefile,v 1.10 1997/10/16 06:49:11 mikel Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93

PROG=	telnetd
CFLAGS+=-DLINEMODE -DKLUDGELINEMODE -DUSE_TERMIO -DDIAGNOSTICS
CFLAGS+=-DOLD_ENVIRON -DENV_HACK -I${.CURDIR}
#CFLAGS+=-DAUTHENTICATION -I${.CURDIR}/../../lib
SRCS=	authenc.c global.c slc.c state.c sys_term.c telnetd.c \
	termstat.c utility.c
DPADD=	${LIBUTIL} ${LIBTERMCAP} ${LIBTELNET}
LDADD+=	-lutil -ltermcap -ltelnet
MAN=	telnetd.8


.include <bsd.prog.mk>

nocrypt:
	@echo "Encryption code already removed."
