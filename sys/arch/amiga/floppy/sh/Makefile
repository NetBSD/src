#	$Id: Makefile,v 1.1 1994/07/30 19:30:51 chopps Exp $

PROG=	sh
NOMAN=
SRCS=	alias.c builtins.c cd.c echo.c error.c eval.c exec.c expand.c \
	input.c jobs.c mail.c main.c memalloc.c miscbltin.c \
	mystring.c nodes.c options.c parser.c redir.c show.c \
	syntax.c trap.c output.c var.c #histedit.c
OBJS+=	init.o arith.o arith_lex.o
LDADD+=	-ll #-ledit -ltermcap
DPADD+=	${LIBL} #${LIBEDIT} ${LIBTERMCAP}
LFLAGS=	-8	# 8-bit lex scanner for arithmetic
SRCDIR=	${.CURDIR}/../../../../../bin/sh
CFLAGS+=-DSHELL -I. -I${SRCDIR} -DNO_HISTORY

BINDIR=/bin

.PATH:	${SRCDIR} ${SRCDIR}/bltin

CLEANFILES+=\
	builtins.c builtins.h init.c mkinit mknodes mksyntax \
	nodes.c nodes.h syntax.c syntax.h token.def y.tab.h

.depend parser.o: token.def
token.def: mktokens
	sh ${SRCDIR}/mktokens

builtins.h builtins.c: ${SRCDIR}/mkbuiltins ${SRCDIR}/builtins.def
	cd ${SRCDIR}; sh mkbuiltins -h ${.OBJDIR}

init.c: mkinit ${SRCS}
	./mkinit '${CC} -c ${CFLAGS} init.c' ${.ALLSRC}
	touch ${.TARGET}

mkinit: ${LIBCRT0} ${SRCDIR}/mkinit.c ${LIBC} ${DPADD}
	${CC} ${CFLAGS} ${LDFLAGS} ${LDSTATIC} ${SRCDIR}/mkinit.c -o $@ ${LDADD}

nodes.c nodes.h: mknodes ${SRCDIR}/nodetypes ${SRCDIR}/nodes.c.pat
	./mknodes ${SRCDIR}/nodetypes ${SRCDIR}/nodes.c.pat

mknodes: ${LIBCRT0} ${SRCDIR}/mknodes.c ${LIBC} ${DPADD}
	${CC} ${CFLAGS} ${LDFLAGS} ${LDSTATIC} ${SRCDIR}/mknodes.c -o $@ ${LDADD}

syntax.c syntax.h: mksyntax
	./mksyntax

mksyntax: ${LIBCRT0} ${SRCDIR}/mksyntax.c ${SRCDIR}/parser.h ${LIBC} ${DPADD}
	${CC} ${CFLAGS} ${LDFLAGS} ${LDSTATIC} ${SRCDIR}/mksyntax.c -o $@ ${LDADD}

.include <bsd.prog.mk>
