#	$NetBSD: Makefile,v 1.5 1998/02/18 22:50:33 jtc Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	talk
DPADD=	${LIBCURSES}
LDADD=	-lcurses
SRCS=	ctl.c ctl_transact.c display.c get_addrs.c get_names.c \
	init_disp.c invite.c io.c look_up.c msgs.c talk.c

.include <bsd.prog.mk>
