#	$NetBSD: Makefile,v 1.9.2.1 2000/02/29 23:40:26 he Exp $

PROG=		grotty
SRCS=		tty.cc
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

# 1.4.1 vax groff (breaks on tail and rwhod manpages without this). (tty.cc)
.if (${MACHINE_ARCH} == "vax" && ${CXXFLAGS} == "-O")
CXXFLAGS=
.endif

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
