#	$NetBSD: Makefile,v 1.19 1995/03/21 09:08:36 cgd Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

PROG=	sh
SRCS=	alias.c builtins.c cd.c echo.c error.c eval.c exec.c expand.c \
	histedit.c input.c jobs.c mail.c main.c memalloc.c miscbltin.c \
	mystring.c nodes.c options.c parser.c redir.c show.c \
	syntax.c trap.c output.c ulimit.c var.c
OBJS+=	init.o arith.o arith_lex.o
LDADD+=	-ll -ledit -ltermcap
DPADD+=	${LIBL} ${LIBEDIT} ${LIBTERMCAP}
LFLAGS= -8	# 8-bit lex scanner for arithmetic
CFLAGS+=-DSHELL -I. -I${.CURDIR}
.PATH:	${.CURDIR}/bltin ${.CURDIR}/../../usr.bin/printf
CLEANFILES+=\
	builtins.c builtins.h init.c mkinit mknodes mksyntax \
	nodes.c nodes.h syntax.c syntax.h token.def \
	y.tab.h

.depend parser.o: token.def
token.def: mktokens
	sh ${.CURDIR}/mktokens

builtins.h builtins.c: ${.CURDIR}/mkbuiltins ${.CURDIR}/builtins.def
	cd ${.CURDIR}; sh mkbuiltins ${.OBJDIR}

init.c: mkinit ${SRCS}
	./mkinit '${CC} -c ${CFLAGS} init.c' ${.ALLSRC:S/^mkinit$//}

mkinit: ${.CURDIR}/mkinit.c
	${CC} ${CFLAGS} ${.CURDIR}/mkinit.c -o $@

nodes.c nodes.h: mknodes ${.CURDIR}/nodetypes ${.CURDIR}/nodes.c.pat
	./mknodes ${.CURDIR}/nodetypes ${.CURDIR}/nodes.c.pat

mknodes: ${.CURDIR}/mknodes.c
	${CC} ${CFLAGS} ${.CURDIR}/mknodes.c -o $@

syntax.c syntax.h: mksyntax
	./mksyntax

mksyntax: ${.CURDIR}/mksyntax.c ${.CURDIR}/parser.h
	${CC} ${CFLAGS} ${.CURDIR}/mksyntax.c -o $@

.include <bsd.prog.mk>
