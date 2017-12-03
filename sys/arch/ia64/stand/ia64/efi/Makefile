#	$NetBSD: Makefile,v 1.3.14.1 2017/12/03 11:36:21 jdolecek Exp $	

S=	${.CURDIR}/../../../../..

.include "../Makefile.booters"

PROGS=		loader.sym loader.efi

MAN.loader.sym=		# no man
MAN.loader.efi=		# no man

CPPFLAGS+=	-I${IA64_STAND_DIR}/efi/libefi/
CPPFLAGS+=	-I${IA64_STAND_DIR}/common/
CPPFLAGS+=	-I${S}
CPPFLAGS+=	-DLOADER -DLIBKERN_OPTIMISE_SPACE -DHEAP_VARIABLE

# Always add MI sources
.PATH: ${.CURDIR}/../../common
.include "${.CURDIR}/../../common/Makefile.inc"

.include "${S}/conf/newvers_stand.mk"

# need to do this after common stuff pulled in
SRCS.loader.sym:=	start.S conf.c efimd.c main.c ${SRCS}

# XXX verify
BINDIR?=	/usr/mdec

LDSCRIPT=	${.CURDIR}/ldscript.${MACHINE_ARCH}
LDFLAGS=	-Wl,-T${LDSCRIPT} -shared -symbolic -nostdlib

loader.sym: ${LDSCRIPT}

CLEANFILES+=	${OBJS}

NEWVERSWHAT=	"EFI boot" ${MACHINE_ARCH}

loader.efi: loader.sym
	if [ `${OBJDUMP} -t loader.sym | fgrep '*UND*' | wc -l` != 0 ]; then \
		${OBJDUMP} -t loader.sym | fgrep '*UND*'; \
		exit 1; \
	fi
	${OBJCOPY} -j .data -j .dynamic -j .dynstr -j .dynsym -j .hash \
	    -j .rela.dyn -j .reloc -j .sdata -j .text \
	    --target=efi-app-${MACHINE_ARCH} loader.sym ${.TARGET}
	@echo link finished.

all: loader.efi

# Pull in libefi
.include "${IA64_STAND_DIR}/efi/libefi/Makefile.inc"
LIBEFI=		${EFILIB}

DPADD=		${LIBEFI} ${LIBLIST}

# Cross dependencies between LIBEFI and LIB(SA|KERN). Need to elaborate below.
LDADD=		${LIBEFI} ${LIBLIST} ${LIBEFI} ${LIBLIST}


.include <bsd.prog.mk>
