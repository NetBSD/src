#	$NetBSD: Makefile,v 1.75 2004/01/17 11:47:30 dsl Exp $
#	@(#)Makefile	8.4 (Berkeley) 5/5/95

.include <bsd.own.mk>

YHEADER=1
PROG=	sh
SHSRCS=	alias.c cd.c echo.c error.c eval.c exec.c expand.c \
	histedit.c input.c jobs.c mail.c main.c memalloc.c miscbltin.c \
	mystring.c options.c parser.c redir.c show.c trap.c output.c var.c \
	test.c printf.c kill.c
GENSRCS=arith.c arith_lex.c builtins.c init.c nodes.c syntax.c
GENHDRS=arith.h builtins.h nodes.h syntax.h token.h
SRCS=	${SHSRCS} ${GENSRCS}

DPSRCS+=${GENHDRS}

LDADD+=	-ll -ledit -ltermcap
DPADD+=	${LIBL} ${LIBEDIT} ${LIBTERMCAP}

LFLAGS=	-8	# 8-bit lex scanner for arithmetic
YFLAGS=	-d

# The .depend file can get references to these temporary files
.OPTIONAL: lex.yy.c y.tab.c

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

CLEANFILES+= mksyntax
CLEANFILES+= ${GENSRCS} ${GENHDRS} y.tab.h
CLEANFILES+= trace

token.h: mktokens
	${_MKTARGET_CREATE}
	${HOST_SH} ${.ALLSRC}

builtins.h: builtins.c
	${_MKTARGET_CREATE}

builtins.c: mkbuiltins shell.h builtins.def
	${_MKTARGET_CREATE}
	${HOST_SH} ${.ALLSRC} ${.OBJDIR}
	[ -f builtins.h ]

init.c: mkinit.sh ${SHSRCS}
	${_MKTARGET_CREATE}
	${HOST_SH} ${.ALLSRC}

nodes.h: nodes.c

nodes.c: mknodes.sh nodetypes nodes.c.pat
	${_MKTARGET_CREATE}
	${HOST_SH} ${.ALLSRC} ${.OBJDIR}
	[ -f nodes.h ]

syntax.h: syntax.c

syntax.c: mksyntax
	${_MKTARGET_CREATE}
	./${.ALLSRC}
	[ -f syntax.h ]

.if ${USETOOLS} == "yes"
COMPATOBJDIR!=	cd ${NETBSDSRCDIR}/tools/compat && ${PRINTOBJDIR}
NBCOMPATLIB=	-L${COMPATOBJDIR} -lnbcompat
.endif

.if	(${MACHINE_ARCH} == "powerpc") || \
	(${MACHINE_CPU} == "arm")
TARGET_CHARFLAG?= -DTARGET_CHAR="unsigned char"
.else
TARGET_CHARFLAG?= -DTARGET_CHAR="signed char"
.endif

mksyntax: mksyntax.c
	${_MKTARGET_LINK}
	${HOST_LINK.c} ${TARGET_CHARFLAG} -o ${.TARGET} ${.ALLSRC} ${NBCOMPATLIB}

.include <bsd.prog.mk>
