#	$NetBSD: Makefile,v 1.18 2000/02/25 16:53:22 pk Exp $

STRIPFLAG=
PROGSOURCE=	boot.c net.c netif_sun.c conf.c openfirm.c bootinfo.c vers.c
NEWVERSWHAT=	"Secondary Boot"
FILES=		boot.net ${RELOCS:S/^/boot./g}
CLEANFILES:=	vers.c ${FILES}
LINKS=		${BINDIR}/boot.${RELOC_DEFAULT} ${BINDIR}/boot

INCLUDE_LIBZ=	yes
SAMISCMAKEFLAGS= SA_USE_CREAD=yes

.include "../Makefile.buildboot"

.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif
.include <bsd.own.mk>
.include <bsd.obj.mk>

OBJS=${SRCS:N*.h:N*.sh:N*.fth:R:S/$/.o/g}

OBJCOPY?=	objcopy

LINKFLAGS=-N -e start

.MAIN: all
all: ${FILES}


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

clean cleandir: cleanprog

cleanprog:
	rm -f ${OBJS} ${CLEANFILES}

.include <bsd.files.mk>
.include <bsd.links.mk>
.include <bsd.sys.mk>
