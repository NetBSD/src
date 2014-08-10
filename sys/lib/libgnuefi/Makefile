#	$NetBSD: Makefile,v 1.1.6.2 2014/08/10 06:56:02 tls Exp $

LIB=	gnuefi
LIBISPRIVATE?= yes

GNUEFI_USE_CREAD?= no		# Read compressed kernels
GNUEFI_INCLUDE_NET?= yes		# Netboot via TFTP, NFS
GNUEFI_USE_LOADFILE?= no		# Generic executable loading support
GNUEFI_ENABLE_LS_OP?= no		# Filesystems ls operation

CPPFLAGS=	-I${GNUEFIDIR} ${GNUEFICPPFLAGS} ${GNUEFIMISCCPPFLAGS}

.if ${MACHINE_ARCH} == "i386"
GNUEFIARCH=ia32
.else
GNUEFIARCH=${MACHINE_ARCH}
.endif

.if defined(GNUEFI_EXTRADIR)
.-include "${GNUEFI_EXTRADIR}/Makefile.inc"
.endif

.include <bsd.own.mk>

.PATH.c: ${GNUEFIDIR} ${GNUEFIDIR}/../../external/bsd/gnu-efi/dist/lib/${GNUEFIARCH} ${GNUEFIDIR}/../../external/bsd/gnu-efi/dist/lib ${GNUEFIDIR}/../../external/bsd/gnu-efi/dist/lib/runtime

SRCS+=	boxdraw.c smbios.c console.c crc.c data.c debug.c dpath.c \
	error.c event.c guid.c hand.c hw.c init.c lock.c \
	misc.c print.c sread.c str.c \
	rtlock.c efirtlib.c rtstr.c vm.c rtdata.c \
	initplat.c math.c

COPTS.boxdraw.c= -Wno-missing-field-initializers
COPTS.guid.c= -Wno-missing-field-initializers
COPTS.smbios.c= -Wno-int-to-pointer-cast
COPTS.dpath.c= -Wno-missing-prototypes
COPTS.print.c= -Wno-missing-prototypes
COPTS.hw.c= -Wno-shadow

.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.o
	@${LD} -r -o lib${LIB}.o `lorder ${OBJS} | tsort`

CPPFLAGS+=	-Wno-pointer-sign
