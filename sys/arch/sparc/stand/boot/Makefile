#	$NetBSD: Makefile,v 1.33 2005/07/10 08:40:21 martin Exp $

STRIPFLAG=
PROGSOURCE=	boot.c net.c netif_sun.c conf.c openfirm.c bootinfo.c \
		mmu.c prompatch.c vers.c
NEWVERSWHAT=	"Secondary Boot"
FILES=		boot.net bootjs.net ${RELOCS:S/^/boot./g}
CLEANFILES:=	vers.c ${FILES}
LINKS=		${BINDIR}/boot.${RELOC_DEFAULT} ${BINDIR}/boot

CPPFLAGS+=	-DBOOT_AOUT -DBOOT_ELF32 -DSUPPORT_DHCP
CFLAGS+=	-freestanding

INCLUDE_LIBZ=	yes
SAMISCMAKEFLAGS= SA_USE_CREAD=yes

.include "../Makefile.buildboot"

.if ${MACHINE} == sparc64
CFLAGS+=	-m32
LDFLAGS+=	-m elf32_sparc
AFLAGS+=	-Wa,-32
.endif

.include <bsd.own.mk>

OBJS=${SRCS:N*.h:N*.sh:N*.fth:R:S/$/.o/g}

LINKFLAGS=-N -e start

.MAIN: all
realall: ${FILES}

.PHONY: vers.c
vers.c: version
	${HOST_SH} ${S}/conf/newvers_stand.sh ${.CURDIR}/version "sparc" ${NEWVERSWHAT}

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

SUN_MAGIC_HEADER='\01\03\01\07\060\200\0\07\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0'

boot.net: boot.${RELOC_DEFAULT}
	(printf ${SUN_MAGIC_HEADER}; cat ${.ALLSRC} ) > ${.TARGET}

bootjs.net: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${LD} -o ${.TARGET} ${LINKFLAGS} -Ttext 300000 ${OBJS} \
	    ${LIBSA} ${LIBZ} ${LIBKERN}
	${SIZE} ${.TARGET}

.include <bsd.prog.mk>
