#	$NetBSD: Makefile,v 1.6 1997/01/18 00:59:47 cgd Exp $

S=	${.CURDIR}/../../../..

.PATH: ${.CURDIR}/.. ${.CURDIR}/../boot

BOOT_PROG = netboot
BOOT_RELOC = ${PRIMARY_LOAD_ADDRESS}

BOOT_SRCS = start.S boot.c prom.c prom_disp.S OSFpal.c prom_swpal.S
BOOT_SRCS+= rpcc.S

BOOT_SRCS+= dev_net.c conf.c devopen.c

BOOT_SRCS+= if_prom.c loadfile.c getsecs.c

BOOT_OBJS = ${BOOT_SRCS:N*.h:R:S/$/.o/g}

HEADERSIZE_PROG = headersize

### find out what to use for libkern
KERN_AS=	library
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN=	${KERNLIB}

### find out what to use for libz
Z_AS=		library
.include "${S}/lib/libz/Makefile.inc"
LIBZ=		${ZLIB}

### find out what to use for libsa
SA_AS=		library
SAREL=
#SA_USE_CREAD=	yes
EXTRACFLAGS=	-mno-fp-regs
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=		${SALIB}

AFLAGS += -DASSEMBLER
CPPFLAGS += -I${.CURDIR}/../.. -I${.CURDIR}/../../../.. \
	-DALPHA_BOOT_ECOFF -DALPHA_BOOT_ELF \
	-DPRIMARY_BOOTBLOCK
CFLAGS = -Werror -mno-fp-regs -g

CLEANFILES+= vers.c vers.o ${BOOT_PROG}.sym ${BOOT_PROG}.nosym \
	${HEADERSIZE_PROG}

all: ${BOOT_PROG}

${BOOT_PROG}.sym: ${BOOT_OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	sh ${.CURDIR}/newvers.sh ${.CURDIR}/version
	${COMPILE.c} vers.c
	${LD} -Ttext ${BOOT_RELOC} -N -e start -o ${BOOT_PROG}.sym \
	    ${BOOT_OBJS} vers.o ${LIBSA} ${LIBZ} ${LIBSA} ${LIBKERN}
	size ${BOOT_PROG}.sym

${BOOT_PROG}.nosym: ${BOOT_PROG}.sym
	cp ${BOOT_PROG}.sym ${BOOT_PROG}.nosym
	strip ${BOOT_PROG}.nosym

${BOOT_PROG}: ${BOOT_PROG}.nosym ${HEADERSIZE_PROG}
	dd if=${BOOT_PROG}.nosym of=${BOOT_PROG} \
	    bs=`./${HEADERSIZE_PROG} ${BOOT_RELOC} ${BOOT_PROG}.nosym` skip=1

install:
	${INSTALL} -c -o bin -g bin -m 444 ${BOOT_PROG} \
	    ${DESTDIR}${BINDIR}/${BOOT_PROG}

clean::
	rm -f a.out [Ee]rrs mklog core *.core \
	    ${BOOT_PROG} ${BOOT_OBJS} ${CLEANFILES}

cleandir: clean

.include "${.CURDIR}/../Makefile.inc"
.include <bsd.obj.mk>
.include <bsd.dep.mk>
