#	$NetBSD: Makefile,v 1.10.2.1 1997/11/08 07:25:59 lukem Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93

PROG=	telnetd
CPPFLAGS+=-DLINEMODE -DKLUDGELINEMODE -DUSE_TERMIO -DDIAGNOSTICS
CPPFLAGS+=-DOLD_ENVIRON -DENV_HACK -I${.CURDIR}
#CPPFLAGS+=-DAUTHENTICATION -I${.CURDIR}/../../lib
SRCS=	authenc.c global.c slc.c state.c sys_term.c telnetd.c \
	termstat.c utility.c
DPADD=	${LIBUTIL} ${LIBTERMCAP} ${LIBTELNET}
LDADD+=	-lutil -ltermcap -ltelnet
MAN=	telnetd.8


.include <bsd.prog.mk>

nocrypt:
	@echo "Encryption code already removed."
