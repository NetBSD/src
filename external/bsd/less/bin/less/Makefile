#	$NetBSD: Makefile,v 1.1 2011/07/03 18:02:01 tron Exp $

.include <bsd.own.mk>

PROG=	less

SRCS=	main.c screen.c brac.c ch.c charset.c cmdbuf.c \
	command.c cvt.c decode.c edit.c filename.c forwback.c \
	help.c ifile.c input.c jump.c line.c linenum.c \
	lsystem.c mark.c optfunc.c option.c opttbl.c os.c \
	output.c pattern.c position.c prompt.c search.c signal.c \
	tags.c ttyin.c version.c  

LDADD+=	-lcurses -ltermcap
DPADD+=	${LIBCURSES}

.include "bsd.prog.mk"
