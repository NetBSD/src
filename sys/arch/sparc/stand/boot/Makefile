#	$NetBSD: Makefile,v 1.20 2000/03/13 03:25:20 mycroft Exp $

STRIPFLAG=
PROGSOURCE=	boot.c net.c netif_sun.c conf.c openfirm.c bootinfo.c vers.c
NEWVERSWHAT=	"Secondary Boot"
FILES=		boot.net ${RELOCS:S/^/boot./g}
CLEANFILES:=	vers.c ${FILES}
LINKS=		${BINDIR}/boot.${RELOC_DEFAULT} ${BINDIR}/boot

INCLUDE_LIBZ=	yes
SAMISCMAKEFLAGS= SA_USE_CREAD=yes

.include "../Makefile.buildboot"

.include <bsd.own.mk>

OBJS=${SRCS:N*.h:N*.sh:N*.fth:R:S/$/.o/g}

OBJCOPY?=	objcopy

LINKFLAGS=-N -e start

.MAIN: all
realall: ${FILES}


vers.c:
	sh ${.CURDIR}/../common/newvers.sh ${.CURDIR}/version ${NEWVERSWHAT}

.for RELOC in ${RELOCS}
boot.${RELOC}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${LD} -o ${.TARGET}.tmp ${LINKFLAGS} -Ttext ${RELOC} ${OBJS} \
	    ${LIBSA} ${LIBZ} ${LIBKERN}
	${SIZE} ${.TARGET}.tmp
	${OBJCOPY} -O binary ${.TARGET}.tmp ${.TARGET}
	rm -f ${.TARGET}.tmp
.endfor
 

# conjure up a magic header that is accepted by all Sun PROMS;
# see installboot.c for details.
boot.net: boot.${RELOC_DEFAULT}
	(printf '\01\03\01\07\060\200\0\07\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0'; cat ${.ALLSRC} ) > ${.TARGET}

.include <bsd.prog.mk>
