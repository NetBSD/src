#	@(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.12 2024/05/10 09:27:27 rillig Exp $

PROG=	error
SRCS=	main.c input.c pi.c subr.c filter.c touch.c

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

LINTFLAGS+=	-w		# treat warnings as errors
LINTFLAGS+=	${HAVE_LLVM:U-T:D}	# strict bool mode

.include <bsd.prog.mk>
