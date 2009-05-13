# $NetBSD: Makefile,v 1.6.74.1 2009/05/13 17:16:12 jym Exp $

.include <bsd.own.mk>
.include <bsd.sys.mk>	# for ${HOST_SH}

S!= cd ${.CURDIR}/../../../..; pwd

PROG= boot
MKMAN= no		# defined
STRIPFLAG=
BINMODE= 444

NETBSD_VERS!=	${HOST_SH} ${S}/conf/osrelease.sh

SRCS=	start.S
SRCS+=	boot.c bootinfo.c conf.c devopen.c disk.c getchar.c getopt.c putchar.c

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=

AFLAGS=    -x assembler-with-cpp -traditional-cpp -mno-abicalls -mips2
AFLAGS+=   -D_LOCORE -D_KERNEL
CFLAGS=    -Os -mmemcpy -G 1024
CFLAGS+=   -ffreestanding -mno-abicalls -msoft-float -mips2
CFLAGS+=   -Werror -Wall -Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith
CPPFLAGS+= -nostdinc -I. -I${S}
CPPFLAGS+= -D_STANDALONE -DNO_ABICALLS -D${MACHINE}
CPPFLAGS+= -DNETBSD_VERS='"${NETBSD_VERS}"'
#CPPFLAGS+= -DBOOT_DEBUG
LDSCRIPT=  ${S}/arch/mips/conf/stand.ldscript
TEXTADDR=  0x80f00000

# if there is a 'version' file, add rule for vers.c and add it to SRCS
# and CLEANFILES
.if exists(version)
.PHONY: vers.c
vers.c: ${.CURDIR}/version
	${HOST_SH} ${S}/conf/newvers_stand.sh ${.CURDIR}/version ${MACHINE}

SRCS+=	vers.c
CLEANFILES+= vers.c
.endif

### find out what to use for libsa
SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_USE_LOADFILE=yes
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=   ${SALIB}

### find out what to use for libkern
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN= ${KERNLIB}

### find out what to use for libz
.include "${S}/lib/libz/Makefile.inc"
LIBZ=    ${ZLIB}

LIBS=  ${LIBSA} ${LIBKERN} ${LIBZ}

cleandir distclean: cleanlibdir
cleanlibdir:
	-rm -rf lib

${PROG}: ${OBJS} ${LIBS}
	${LD} -Map ${PROG}.map -N -x -Ttext ${TEXTADDR} -T ${LDSCRIPT} \
	    -e start -o ${PROG}.elf ${OBJS} ${LIBS}
	@${SIZE} ${PROG}.elf
	${OBJCOPY} --impure -O ecoff-littlemips \
	    -R .pdr -R .mdebug.abi32 -R .comment -R .ident \
	    ${PROG}.elf ${.TARGET}

CLEANFILES+=	${PROG}.elf ${PROG}.map

.include <bsd.klinks.mk>
.include <bsd.prog.mk>
