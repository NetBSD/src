#	$Id: Makefile,v 1.4 1993/08/02 17:30:31 mycroft Exp $

PROG=           cc1obj
BINDIR=         /usr/libexec
SRCS=		objc-parse.c objc-act.c  \
		c-lex.c c-pragma.c c-decl.c c-typeck.c c-convert.c\
		c-aux-info.c c-iterate.c \
		$(COMMON_SRC)
CFLAGS+=        -I. -I$(.CURDIR) -I$(.CURDIR)/../common \
                -I$(.CURDIR)/.. -I$(.CURDIR)/../$(MACHINE)
YFLAGS=
NOMAN=          noman
.PATH:          $(.CURDIR)/../common $(.CURDIR)/../cc1 \
                $(.CURDIR)/../$(MACHINE)

.include <bsd.prog.mk>
