#	$NetBSD: Makefile,v 1.7.2.1 2000/02/29 23:38:26 he Exp $

LIB=		common

SRCS=		arparse.y arlex.l arsup.c bucomm.c version.c filemode.c \
		rddbg.c debug.c stabs.c ieee.c rdcoff.c wrstabs.c
CPPFLAGS+=	-DTARGET='"${MACHINE_GNU_ARCH}--netbsd"'
YHEADER=1

MKPROFILE=	no
MKPIC=		no
MKLINT=		no

# only needed during build
libinstall::

.include <bsd.lib.mk>

# Nasty hack to avoid internal gcc errors for 1.4.1
#
.if ${MACHINE} == "vax"
ieee.o:
	 ${CC} ${CPPFLAGS} -c ${.IMPSRC}
.endif
