#	$NetBSD: Makefile,v 1.5 1997/06/01 03:39:25 mrg Exp $

PROG=		boot
PROGSOURCE=	boot.c net.c netif_sun.c conf.c
NEWVERSWHAT=	"Secondary Boot"

INCLUDE_LIBZ=	yes
SAMISCMAKEFLAGS= SA_USE_CREAD=yes

.include "../Makefile.buildboot"

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	sh ${.CURDIR}/../common/newvers.sh ${.CURDIR}/version ${NEWVERSWHAT}
	${COMPILE.c} vers.c
	${LD} -o ${.TARGET} -N -T ${RELOC} -e start ${OBJS} \
	    vers.o ${LIBSA} ${LIBZ} ${LIBKERN}
	@size ${.TARGET}

.include <bsd.prog.mk>
