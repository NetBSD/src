#	$NetBSD: Makefile,v 1.9 2020/02/08 12:53:29 fox Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

USE_FORT?= yes	# network client

PROG=	talk
DPADD=	${LIBCURSES} ${LIBTERMINFO} ${LIBUTIL}
LDADD=	-lcurses -lterminfo -lutil
SRCS=	ctl.c ctl_transact.c display.c get_addrs.c get_names.c \
	init_disp.c invite.c io.c look_up.c msgs.c talk.c

COPTS.get_names.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
