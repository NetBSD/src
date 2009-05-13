#	$NetBSD: Makefile,v 1.83.4.1 2009/05/13 19:15:51 jym Exp $
#	@(#)Makefile	8.4 (Berkeley) 5/5/95

.include <bsd.own.mk>

YHEADER=1
PROG=	sh
SHSRCS=	alias.c cd.c echo.c error.c eval.c exec.c expand.c \
	histedit.c input.c jobs.c mail.c main.c memalloc.c miscbltin.c \
	mystring.c options.c parser.c redir.c show.c trap.c output.c var.c \
	test.c kill.c syntax.c
GENSRCS=arith.c arith_lex.c builtins.c init.c nodes.c
GENHDRS=arith.h builtins.h nodes.h token.h
SRCS=	${SHSRCS} ${GENSRCS}

DPSRCS+=${GENHDRS}

LDADD+=	-ll -ledit -ltermcap
DPADD+=	${LIBL} ${LIBEDIT} ${LIBTERMCAP}

LFLAGS=	-8	# 8-bit lex scanner for arithmetic
YFLAGS=	-d

# Environment for scripts executed during build.
SCRIPT_ENV= \
	AWK=${TOOL_AWK:Q} \
	SED=${TOOL_SED:Q}

# The .depend file can get references to these temporary files
.OPTIONAL: lex.yy.c y.tab.c

.ifdef CRUNCHEDPROG
LFLAGS+=-L
YFLAGS+=-l
.endif

CPPFLAGS+=-DSHELL -I. -I${.CURDIR} -DYY_NO_INPUT
#XXX: For testing only.
#CPPFLAGS+=-DDEBUG=1
#CFLAGS+=-funsigned-char
#TARGET_CHARFLAG?= -DTARGET_CHAR="unsigned char" -funsigned-char

.ifdef SMALLPROG
CPPFLAGS+=-DSMALL
.else
SRCS+=printf.c
.endif

.PATH:	${.CURDIR}/bltin ${NETBSDSRCDIR}/bin/test \
	${NETBSDSRCDIR}/usr.bin/printf \
	${NETBSDSRCDIR}/bin/kill

CLEANFILES+= ${GENSRCS} ${GENHDRS} y.tab.h
CLEANFILES+= trace

token.h: mktokens
	${_MKTARGET_CREATE}
	${SCRIPT_ENV} ${HOST_SH} ${.ALLSRC}

builtins.h: builtins.c
	${_MKTARGET_CREATE}

builtins.c: mkbuiltins shell.h builtins.def
	${_MKTARGET_CREATE}
	${SCRIPT_ENV} ${HOST_SH} ${.ALLSRC} ${.OBJDIR}
	[ -f builtins.h ]

init.c: mkinit.sh ${SHSRCS}
	${_MKTARGET_CREATE}
	${SCRIPT_ENV} ${HOST_SH} ${.ALLSRC}

nodes.h: nodes.c

nodes.c: mknodes.sh nodetypes nodes.c.pat
	${_MKTARGET_CREATE}
	${SCRIPT_ENV} ${HOST_SH} ${.ALLSRC} ${.OBJDIR}
	[ -f nodes.h ]

.if ${USETOOLS} == "yes"
COMPATOBJDIR!=	cd ${NETBSDSRCDIR}/tools/compat && ${PRINTOBJDIR}
NBCOMPATLIB=	-L${COMPATOBJDIR} -lnbcompat
.endif

.include <bsd.prog.mk>
