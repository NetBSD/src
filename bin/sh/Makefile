#	$NetBSD: Makefile,v 1.56 2002/02/12 06:39:10 ross Exp $
#	@(#)Makefile	8.4 (Berkeley) 5/5/95

.include <bsd.own.mk>

YHEADER=1
PROG=	sh
SHSRCS=	alias.c cd.c echo.c error.c eval.c exec.c expand.c \
	histedit.c input.c jobs.c mail.c main.c memalloc.c miscbltin.c \
	mystring.c options.c parser.c redir.c show.c trap.c output.c var.c \
	test.c
GENSRCS=arith.c arith.h arith_lex.c builtins.c builtins.h init.c nodes.c \
	nodes.h syntax.c syntax.h token.h
SRCS=	${SHSRCS} ${GENSRCS}

LDADD+=	-ll -ledit -ltermcap
DPADD+=	${LIBL} ${LIBEDIT} ${LIBTERMCAP}

LFLAGS= -8	# 8-bit lex scanner for arithmetic
YFLAGS=	-d

CPPFLAGS+=-DSHELL -I. -I${.CURDIR}

.ifdef SMALLPROG
CPPFLAGS+=-DSMALL
.endif

.PATH:	${.CURDIR}/bltin ${.CURDIR}/../../usr.bin/printf ${.CURDIR}/../test

CLEANFILES+= mkinit mknodes mksyntax
CLEANFILES+= ${GENSRCS} y.tab.h

token.h: mktokens
	sh ${.ALLSRC}

builtins.c builtins.h: mkbuiltins shell.h builtins.def
	sh ${.ALLSRC} ${.OBJDIR}

init.c: mkinit ${SHSRCS}
	./${.ALLSRC}

nodes.c nodes.h: mknodes nodetypes nodes.c.pat
	./${.ALLSRC}

syntax.c syntax.h: mksyntax
	./${.ALLSRC}

mkinit: mkinit.c
	${HOST_LINK.c} -o mkinit ${.IMPSRC}

mknodes: mknodes.c
	${HOST_LINK.c} -o mknodes ${.IMPSRC}

.if	(${MACHINE_ARCH} == "powerpc") || \
	(${MACHINE_CPU} == "arm")
TARGET_CHARFLAG= -DTARGET_CHAR="u_int8_t"
.else
TARGET_CHARFLAG= -DTARGET_CHAR="int8_t"
.endif

mksyntax: mksyntax.c
	${HOST_LINK.c} ${TARGET_CHARFLAG} -o mksyntax ${.IMPSRC}

.include <bsd.prog.mk>

${OBJS}: builtins.h nodes.h syntax.h token.h
