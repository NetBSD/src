#	$NetBSD: Makefile,v 1.11.2.2 2000/02/28 19:01:15 he Exp $

PROG=		boot
PROGSOURCE=	boot.c net.c netif_sun.c conf.c openfirm.c vers.c
NEWVERSWHAT=	"Secondary Boot"
FILES=		${RELOCS:S/^/boot./g}
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

LINKFLAGS=	-N -e start

all: ${FILES}

vers.c:
	sh ${.CURDIR}/../common/newvers.sh ${.CURDIR}/version ${NEWVERSWHAT}

.for RELOC in ${RELOCS}
boot.${RELOC}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${LD} -o ${.TARGET} ${LINKFLAGS} -Ttext ${RELOC} ${OBJS} \
	    ${LIBSA} ${LIBZ} ${LIBKERN}
	@size ${.TARGET}
.endfor

afterinstall:
	# conjure up a magic header that is accepted by all Sun PROMS;
	# see installboot.c for details.
	printf '\01\03\01\07\060\200\0\07' | \
		dd of=${DESTDIR}/${BINDIR}/boot.net bs=32 conv=sync
	dd if=${DESTDIR}/${BINDIR}/boot of=${DESTDIR}/${BINDIR}/boot.net \
		bs=32 skip=1 seek=1
	chmod ${BINMODE} ${DESTDIR}/${BINDIR}/boot.net
	chown ${BINOWN}.${BINGRP} ${DESTDIR}/${BINDIR}/boot.net

clean cleandir: cleanprog

cleanprog:
	rm -f ${OBJS} ${CLEANFILES}

.include <bsd.files.mk>
.include <bsd.links.mk>
