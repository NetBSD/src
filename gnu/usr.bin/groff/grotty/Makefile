#	$NetBSD: Makefile,v 1.8.2.2 1999/02/03 07:09:45 cgd Exp $

PROG=		grotty
SRCS=		tty.cc
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

# gcc 2.7.2.2 c++ optimisation broken on vax. (tty.cc)
# egcs fixes this, but for now disable optimisation to get working groff.
.if (${MACHINE_ARCH} == "vax" && ${CXXFLAGS} == "-O")
CXXFLAGS=
.endif

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
