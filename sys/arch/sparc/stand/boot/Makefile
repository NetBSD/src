#	$NetBSD: Makefile,v 1.12 1999/04/28 15:22:25 christos Exp $

STRIPFLAG=
PROG=		boot
PROGSOURCE=	boot.c net.c netif_sun.c conf.c openfirm.c
FILES=		boot.net
NEWVERSWHAT=	"Secondary Boot"
CLEANFILES+=	vers.c vers.o boot.net

INCLUDE_LIBZ=	yes
SAMISCMAKEFLAGS= SA_USE_CREAD=yes

.include "../Makefile.buildboot"

RELOC=		0x300000

.include <bsd.own.mk>

LINKFLAGS=-N -Ttext ${RELOC} -e start

all: ${FILES}

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	sh ${.CURDIR}/../common/newvers.sh ${.CURDIR}/version ${NEWVERSWHAT}
	${COMPILE.c} vers.c
	${LD} -o ${.TARGET}.tmp ${LINKFLAGS} ${OBJS} \
	    vers.o ${LIBSA} ${LIBZ} ${LIBKERN}
	@size ${.TARGET}.tmp
	@objcopy -O binary ${.TARGET}.tmp ${.TARGET}
	@rm -f ${.TARGET}.tmp
 
# conjure up a magic header that is accepted by all Sun PROMS;
# see installboot.c for details.
${FILES}: ${PROG}
	(printf '\01\03\01\07\060\200\0\07'; cat ${.ALLSRC} ) > ${.TARGET}

.include <bsd.prog.mk>
