#	$NetBSD: Makefile,v 1.6.4.2 2002/01/10 19:46:40 thorpej Exp $

S=${.CURDIR}/../../../../
HOSTPROG=	wrtvid
NOMAN=	# defined
LIBSA=${.CURDIR}/../../../../lib/libsa
COPTS+= -Wall -Wstrict-prototypes -Wmissing-prototypes
HOST_CPPFLAGS+= -I. -I${LIBSA}
DPADD+=	machine m68k

.PATH.c: ${LIBSA}

SRCS=	wrtvid.c byteorder.c loadfile.c loadfile_aout.c loadfile_elf32.c
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
