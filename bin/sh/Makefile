#	$Id: Makefile,v 1.9 1993/12/28 10:59:05 davidb Exp $

PROG=	sh
SRCS=	builtins.c cd.c dirent.c echo.c error.c eval.c exec.c expand.c \
	input.c jobs.c mail.c main.c memalloc.c miscbltin.c \
	mystring.c nodes.c options.c parser.c redir.c show.c \
	syntax.c trap.c output.c var.c
OBJS+=	init.o
CFLAGS+=-DSHELL -I. -I${.CURDIR}
.PATH:	${.CURDIR}/bltin
CLEANFILES+=\
	builtins.c builtins.h init.c mkinit mknodes mksyntax \
	nodes.c nodes.h syntax.c syntax.h token.def

.depend parser.o: token.def
token.def: mktokens
	sh ${.CURDIR}/mktokens

builtins.h builtins.c: ${.CURDIR}/mkbuiltins ${.CURDIR}/builtins
	cd ${.CURDIR}; sh mkbuiltins ${.OBJDIR}

init.c: mkinit ${SRCS}
	./mkinit '${CC} -c ${CFLAGS} init.c' ${.ALLSRC}
	touch ${.TARGET}

nodes.c nodes.h: mknodes ${.CURDIR}/nodetypes ${.CURDIR}/nodes.c.pat
	./mknodes ${.CURDIR}/nodetypes ${.CURDIR}/nodes.c.pat

syntax.c syntax.h: mksyntax
	./mksyntax

.include <bsd.prog.mk>

mkinit: ${LIBCRT0} ${.CURDIR}/mkinit.c ${LIBC} ${DPADD}
	${CC} ${CFLAGS} ${LDFLAGS} ${LDSTATIC} ${.CURDIR}/mkinit.c -o $@ ${LDADD}

mknodes: ${LIBCRT0} ${.CURDIR}/mknodes.c ${LIBC} ${DPADD}
	${CC} ${CFLAGS} ${LDFLAGS} ${LDSTATIC} ${.CURDIR}/mknodes.c -o $@ ${LDADD}

mksyntax: ${LIBCRT0} ${.CURDIR}/mksyntax.c ${.CURDIR}/parser.h ${LIBC} ${DPADD}
	${CC} ${CFLAGS} ${LDFLAGS} ${LDSTATIC} ${.CURDIR}/mksyntax.c -o $@ ${LDADD}
