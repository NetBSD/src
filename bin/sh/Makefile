#	$NetBSD: Makefile,v 1.66 2003/05/15 10:13:36 dsl Exp $
#	@(#)Makefile	8.4 (Berkeley) 5/5/95

.include <bsd.own.mk>

YHEADER=1
PROG=	sh
SHSRCS=	alias.c cd.c echo.c error.c eval.c exec.c expand.c \
	histedit.c input.c jobs.c mail.c main.c memalloc.c miscbltin.c \
	mystring.c options.c parser.c redir.c show.c trap.c output.c var.c \
	test.c printf.c kill.c
GENSRCS=arith.c arith.h arith_lex.c builtins.c builtins.h init.c nodes.c \
	nodes.h syntax.c syntax.h token.h
SRCS=	${SHSRCS} ${GENSRCS}

LDADD+=	-ll -ledit -ltermcap
DPADD+=	${LIBL} ${LIBEDIT} ${LIBTERMCAP}

LFLAGS=	-8	# 8-bit lex scanner for arithmetic
YFLAGS=	-d

.ifdef CRUNCHEDPROG
LFLAGS+=-L
YFLAGS+=-l
.endif

CPPFLAGS+=-DSHELL -I. -I${.CURDIR}
#XXX: For testing only.
#CPPFLAGS+=-DDEBUG=1
#CFLAGS+=-funsigned-char
#TARGET_CHARFLAG?= -DTARGET_CHAR="unsigned char" -funsigned-char

.ifdef SMALLPROG
CPPFLAGS+=-DSMALL
.endif

.PATH:	${.CURDIR}/bltin ${NETBSDSRCDIR}/bin/test \
	${NETBSDSRCDIR}/usr.bin/printf \
	${NETBSDSRCDIR}/bin/kill

CLEANFILES+= mkinit mknodes mksyntax
CLEANFILES+= ${GENSRCS} y.tab.h

token.h: mktokens
	${HOST_SH} ${.ALLSRC}

builtins.h: builtins.c
	[ -f ${.TARGET} ]

builtins.c: mkbuiltins shell.h builtins.def
	${HOST_SH} ${.ALLSRC} ${.OBJDIR}
	[ -f builtins.h ]
	touch builtins.h

init.c: mkinit ${SHSRCS}
	./${.ALLSRC}

nodes.h: nodes.c
	[ -f ${.TARGET} ]

nodes.c: mknodes nodetypes nodes.c.pat
	./${.ALLSRC}
	[ -f nodes.h ]
	touch nodes.h

syntax.h: syntax.c
	[ -f ${.TARGET} ]

syntax.c: mksyntax
	./${.ALLSRC}
	[ -f syntax.h ]
	touch syntax.h

mkinit: mkinit.c
	${HOST_LINK.c} -o mkinit ${.IMPSRC}

mknodes: mknodes.c
	${HOST_LINK.c} -o mknodes ${.IMPSRC}

.if	(${MACHINE_ARCH} == "powerpc") || \
	(${MACHINE_CPU} == "arm")
TARGET_CHARFLAG?= -DTARGET_CHAR="unsigned char"
.else
TARGET_CHARFLAG?= -DTARGET_CHAR="signed char"
.endif

mksyntax: mksyntax.c
	${HOST_LINK.c} ${TARGET_CHARFLAG} -o mksyntax ${.IMPSRC}

.include <bsd.prog.mk>

${OBJS}: builtins.h nodes.h syntax.h token.h
