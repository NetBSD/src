#	$NetBSD: Makefile,v 1.7 1997/05/31 21:21:23 cjs Exp $

LIB=		g++

SRCS=	ACG.cc AllocRing.cc Binomial.cc BitSet.cc BitString.cc CursesW.cc \
DLList.cc DiscUnif.cc Erlang.cc Fix.cc Fix16.cc Fix24.cc \
Geom.cc GetOpt.cc HypGeom.cc Intdouble.cc Integer.cc LogNorm.cc \
MLCG.cc NegExp.cc Normal.cc Obstack.cc Poisson.cc RNG.cc \
Random.cc Rational.cc Regex.cc RndInt.cc SLList.cc SmplHist.cc \
SmplStat.cc String.cc Uniform.cc Weibull.cc bitand.c bitany.c \
bitblt.c bitclear.c bitcopy.c bitcount.c bitinvert.c bitlcomp.c \
bitset1.c bitxor.c builtin.cc compare.cc error.cc fmtq.cc \
gcd.cc hash.cc ioob.cc lg.cc pow.cc sqrt.cc \
timer.c 

CXXFLAGS+=	-nostdinc++ -I$(.CURDIR)/../../include \
		  -I$(.CURDIR)/../../libio -I$(.CURDIR)/../../libstdc++
CFLAGS+=	-I$(.CURDIR) -I$(.CURDIR)/../../libio
.if (${MACHINE} == "alpha")
LDADD=		-lcurses
DPADD=		${LIBCURSES}
.else
LDADD=		${DESTDIR}/usr/lib/c++rt0.o -lcurses
DPADD=		${DESTDIR}/usr/lib/c++rt0.o ${LIBCURSES}
.endif
NOMAN=		noman
NOLINT=		nolint
LIBCURSES!=	printf "xxx: .MAKE\n\t@echo \$${LIBCURSES}\n.include <bsd.prog.mk>" |\
			$(MAKE) -s -f- xxx

.include <bsd.lib.mk>
