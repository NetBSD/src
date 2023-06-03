#	$NetBSD: Makefile,v 1.10 2023/06/03 09:09:19 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

USE_FORT?= yes	# network client

PROG=	talk
DPADD=	${LIBCURSES} ${LIBTERMINFO} ${LIBUTIL}
LDADD=	-lcurses -lterminfo -lutil
SRCS=	ctl.c ctl_transact.c display.c get_addrs.c get_names.c \
	init_disp.c invite.c io.c look_up.c msgs.c talk.c

COPTS.get_names.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
