#	$NetBSD: Makefile,v 1.1 1999/03/06 16:36:04 ragge Exp $

S=	${.CURDIR}/../../../../

PROG=	boot
NOMAN=	1
DEVS=	hp.c ctu.c ra.c tmscp.c mfm.c if_qe.c if_le.c if_ze.c
SRCS=	srt0.s boot.c devopen.c conf.c autoconf.c netio.c rom.c romread.c \
	urem.c udiv.c consio.c str.c ${DEVS}

CLEANFILES+=${PROG} ${PROG}.mop
CPPFLAGS+=-DSUPPORT_BOOTPARAMS -DSUPPORT_BOOTP -DSUPPORT_DHCP -D_STANDALONE
BINDIR=	/

SA_AS=	library
SAMISCMAKEFLAGS=SA_USE_CREAD=yes SA_INCLUDE_NET=yes
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=	${SALIB}

Z_AS=	library
.include "${S}/lib/libz/Makefile.inc"
LIBZ=	${ZLIB}


${PROG}: ${OBJS} ${LIBSA} ${LIBZ}
	ld -N -Ttext ${RELOC} -e nisse -o ${PROG} ${OBJS} \
	    ${LIBSA} ${LIBZ} ${LIBSA}
	/usr/sbin/mopa.out ${PROG} ${PROG}.mop
	strip ${PROG}
	size ${PROG}

.include <bsd.prog.mk>
