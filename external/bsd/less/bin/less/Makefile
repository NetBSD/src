#	$NetBSD: Makefile,v 1.3 2016/12/10 13:44:51 rin Exp $

.include <bsd.own.mk>

PROG=	less

SRCS=	main.c screen.c brac.c ch.c charset.c cmdbuf.c \
	command.c cvt.c decode.c edit.c filename.c forwback.c \
	help.c ifile.c input.c jump.c line.c linenum.c \
	lsystem.c mark.c optfunc.c option.c opttbl.c os.c \
	output.c pattern.c position.c prompt.c search.c signal.c \
	tags.c ttyin.c version.c  

LDADD+=	-lterminfo
DPADD+=	${LIBTERMINFO}

MLINKS=	less.1 more.1 less.1 page.1
LINKS=	${BINDIR}/less ${BINDIR}/more ${BINDIR}/less ${BINDIR}/page

.include "bsd.prog.mk"
