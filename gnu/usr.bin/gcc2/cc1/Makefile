#	from: @(#)Makefile	6.2 (Berkeley) 2/2/91
#	$Id: Makefile,v 1.4 1993/08/01 05:42:56 mycroft Exp $

PROG=		cc1
BINDIR=		/usr/libexec
SRCS=		c-parse.c c-lang.c  c-lex.c c-pragma.c c-decl.c \
		c-typeck.c c-convert.c c-aux-info.c c-iterate.c \
		$(COMMON_SRC)
	
CFLAGS+=	-I. -I$(.CURDIR) -I$(.CURDIR)/../common \
                -I$(.CURDIR)/.. -I$(.CURDIR)/../$(MACHINE)
YFLAGS=
NOMAN=		noman
.PATH:		$(.CURDIR)/../common $(.CURDIR)/../$(MACHINE)
.include <bsd.prog.mk>
