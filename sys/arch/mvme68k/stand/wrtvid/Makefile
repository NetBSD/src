#	$NetBSD: Makefile,v 1.7 2001/08/05 06:10:25 matt Exp $

S=${.CURDIR}/../../../../
HOSTPROG=	wrtvid
MKMAN=	no
LIBSA=${.CURDIR}/../../../../lib/libsa
COPTS+= -Wall -Wstrict-prototypes -Wmissing-prototypes
HOST_CPPFLAGS+= -I. -I${LIBSA}
DPADD+=	machine m68k

.PATH.c: ${LIBSA}

SRCS=	wrtvid.c loadfile.c
CLEANFILES+= machine m68k

# only needed during build
proginstall::
beforedepend:	machine m68k

machine::
	-rm -f ${.TARGET}
	ln -s $S/arch/mvme68k/include machine

m68k::
	-rm -f ${.TARGET}
	ln -s $S/arch/m68k/include m68k

.include <bsd.hostprog.mk>
