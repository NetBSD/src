#	from: @(#)Makefile	6.1 (Berkeley) 3/3/91
#	$Id: Makefile,v 1.2 1993/08/01 05:42:28 mycroft Exp $

PROG=		as
SRCS=		app.c append.c as.c atof-generic.c bignum-copy.c \
		expr.c flonum-const.c flonum-copy.c flonum-mult.c \
		frags.c hash.c hex-value.c input-file.c input-scrub.c \
		messages.c obstack.c output-file.c read.c subsegs.c \
		symbols.c version.c write.c xmalloc.c xrealloc.c
CFLAGS+=	-I$(.CURDIR) -I$(.CURDIR)/config \
		-DSIGTY=void -Derror=as_fatal
.PATH: $(.CURDIR)/config

.include "config/Makefile.$(MACHINE)"

.include <bsd.prog.mk>
