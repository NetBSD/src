S=	${.CURDIR}/../../../../..

VERSIONFILE= ${.CURDIR}/version

.include <../Makefile.booters>

PROG=		loader.sym

MAN=#		Tell bsd.prog.mk that loader.sym is not a man page.

INTERNALPROG=


SRCS=		start.S conf.c efimd.c vers.c main.c

CPPFLAGS+=	-I${IA64_STAND_DIR}/efi/include
CPPFLAGS+=	-I${IA64_STAND_DIR}/efi/include/${MACHINE_ARCH}
CPPFLAGS+=	-I${IA64_STAND_DIR}/efi/libefi/
CPPFLAGS+=	-I${IA64_STAND_DIR}/efi/libshell/
CPPFLAGS+=	-I${IA64_STAND_DIR}/common/
CPPFLAGS+=	-I${S}

CPPFLAGS+=	-DLOADER

CPPFLAGS+= 	-DLIBKERN_OPTIMISE_SPACE \
		-DHEAP_VARIABLE


# Pick up FreeBSD glue
#CPPFLAGS+= -I${.CURDIR}/../../

# Always add MI sources
.PATH: ${.CURDIR}/../../common
.include "${.CURDIR}/../../common/Makefile.inc"


FILES=		loader.efi


LDSCRIPT=	${.CURDIR}/ldscript.${MACHINE_ARCH}
LDFLAGS=	-Wl,-T${LDSCRIPT} -shared -symbolic -nostdlib

${PROG}: ${LDSCRIPT}

CLEANFILES+=	vers.c loader.efi ${OBJS}

NEWVERSWHAT=	"EFI boot" ${MACHINE_ARCH}

vers.c: ${VERSIONFILE} ${SOURCES}
	${HOST_SH} ${S}/conf/newvers_stand.sh ${.ALLSRC} ${MACHINE} ${NEWVERSWHAT}

loader.efi: ${PROG}
	if [ `${OBJDUMP} -t ${.ALLSRC} | fgrep '*UND*' | wc -l` != 0 ]; then \
		${OBJDUMP} -t ${.ALLSRC} | fgrep '*UND*'; \
		exit 1; \
	fi
	${OBJCOPY} -j .data -j .dynamic -j .dynstr -j .dynsym -j .hash \
	    -j .rela.dyn -j .reloc -j .sdata -j .text \
	    --target=efi-app-${MACHINE_ARCH} ${.ALLSRC} ${.TARGET}
	@echo link finished.

all: loader.efi

# Pull in libefi
.include "${IA64_STAND_DIR}/efi/libefi/Makefile.inc"
LIBEFI=		${EFILIB}

DPADD=		${LIBEFI} ${LIBLIST}

# Cross dependencies between LIBEFI and LIB(SA|KERN). Need to elaborate below.
LDADD=		${LIBEFI} ${LIBLIST} ${LIBEFI} ${LIBLIST}

.include <bsd.prog.mk>
