#	$NetBSD: Makefile,v 1.1.1.1 1996/03/09 00:01:20 phil Exp $

LIB=		g++

SRCS=	ACG.cc AllocRing.cc Binomial.cc BitSet.cc BitString.cc CursesW.cc \
DLList.cc DiscUnif.cc Erlang.cc Fix.cc Fix16.cc Fix24.cc \
Geom.cc GetOpt.cc HypGeom.cc Intdouble.cc Integer.cc LogNorm.cc \
MLCG.cc NegExp.cc Normal.cc Obstack.cc Poisson.cc RNG.cc \
Random.cc Rational.cc Regex.cc RndInt.cc SLList.cc SmplHist.cc \
SmplStat.cc String.cc Uniform.cc Weibull.cc bitand.c bitany.c \
bitblt.c bitclear.c bitcopy.c bitcount.c bitinvert.c bitlcomp.c \
bitset1.c bitxor.c builtin.cc chr.cc compare.cc error.cc \
fmtq.cc gcd.cc hash.cc ioob.cc lg.cc pow.cc \
sqrt.cc str.cc timer.c 

CXXFLAGS+=	-nostdinc++ -I$(.CURDIR)/../../include \
		  -I$(.CURDIR)/../../libio -I$(.CURDIR)/../../libstdc++
CFLAGS+=	-I$(.CURDIR) -I$(.CURDIR)/../../libio
LDADD=		${DESTDIR}/usr/lib/c++rt0.o -lcurses
DPADD=		${DESTDIR}/usr/lib/c++rt0.o ${LIBCURSES}
NOMAN=		noman
LIBCURSES!=	printf "xxx:\n\techo \$${LIBCURSES}\n.include <bsd.prog.mk>" |\
			$(MAKE) -r -s -f - xxx | grep curses

.include <bsd.lib.mk>
