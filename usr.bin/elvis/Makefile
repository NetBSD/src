#	$Id: Makefile,v 1.6 1993/11/11 01:30:13 jtc Exp $

PROG=	elvis
CFLAGS+=-DBSD -DREGEX
DPADD=  ${LIBTERM}
LDADD=  -ltermcap 
SRCS=	blk.c cmd1.c cmd2.c ctype.c curses.c cut.c ex.c input.c main.c misc.c \
	modify.c move1.c move2.c move3.c move4.c move5.c opts.c recycle.c \
	redraw.c regexp.c regsub.c system.c tio.c tmp.c unix.c vars.c vcmd.c vi.c
LINKS=	${BINDIR}/elvis ${BINDIR}/vi ${BINDIR}/elvis ${BINDIR}/ex \
	${BINDIR}/elvis ${BINDIR}/view
MLINKS=	elvis.1 vi.1 elvis.1 ex.1 elvis.1 view.1

.include <bsd.prog.mk>
