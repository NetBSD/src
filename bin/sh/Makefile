#	$NetBSD: Makefile,v 1.26 1997/04/17 08:13:26 thorpej Exp $
#	@(#)Makefile	8.4 (Berkeley) 5/5/95

PROG=	sh
SHSRCS=	alias.c cd.c echo.c error.c eval.c exec.c expand.c \
	histedit.c input.c jobs.c mail.c main.c memalloc.c miscbltin.c \
	mystring.c options.c parser.c redir.c show.c trap.c output.c var.c
GENSRCS= arith.c arith_lex.c builtins.c init.c nodes.c syntax.c
SRCS= ${SHSRCS} ${GENSRCS}

LDADD+=	-ll -ledit -ltermcap
DPADD+=	${LIBL} ${LIBEDIT} ${LIBTERMCAP}

LFLAGS= -8	# 8-bit lex scanner for arithmetic

CFLAGS+=-DSHELL -I. -I${.CURDIR}

# XXX
.if (${MACHINE_ARCH} == "powerpc")
CFLAGS+= -w
.endif

.PATH:	${.CURDIR}/bltin ${.CURDIR}/../../usr.bin/printf

CLEANFILES+= builtins.h mkinit mknodes mksyntax \
	nodes.h syntax.h token.h y.tab.h
CLEANFILES+= ${GENSRCS}

beforedepend: token.h

token.h: mktokens
	sh ${.CURDIR}/mktokens

builtins.h builtins.c: mkbuiltins builtins.def
	cd ${.CURDIR}; sh mkbuiltins ${.OBJDIR}

init.c: mkinit ${SHSRCS}
	./mkinit ${.ALLSRC:S/^mkinit$//}

nodes.c nodes.h: mknodes nodetypes nodes.c.pat
	./mknodes ${.CURDIR}/nodetypes ${.CURDIR}/nodes.c.pat

syntax.c syntax.h: mksyntax
	./mksyntax

parser.o: token.h

mkinit: mkinit.c
	${HOST_COMPILE.c} ${.CURDIR}/mkinit.c
	${HOST_LINK.c} -o ${.TARGET} mkinit.o

mknodes: mknodes.c
	${HOST_COMPILE.c} ${.CURDIR}/mknodes.c
	${HOST_LINK.c} -o ${.TARGET} mknodes.o

mksyntax: mksyntax.c
	${HOST_COMPILE.c} ${.CURDIR}/mksyntax.c
	${HOST_LINK.c} -o ${.TARGET} mksyntax.o

.include <bsd.prog.mk>

${OBJS}: builtins.h nodes.h syntax.h
