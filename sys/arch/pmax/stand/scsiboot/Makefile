#	$NetBSD: Makefile,v 1.15 1999/04/11 04:28:14 simonb Exp $
#	@(#)Makefile	8.3 (Berkeley) 2/16/94

S=	${.CURDIR}/../../../..

PROG=	bootxx
RELOC=	80700000	# Room for an almost 7MB kernel
VERS_O=			# no version info in first stage

SRCS=	start.S bootxx.c clear_cache.S printf.S strcmp.S strcpy.S \
	strlen.S

CLEANFILES+=${PROG}.aout ${PROG}.map mkboot ${ALL}
.PATH:  ${.CURDIR}/../lib

# Don't compile in sanity checks for old file systems.
SAMISCCPPFLAGS=	-DLIBSA_NO_COMPAT_UFS

BOOTDEFADD+=	-DBOOTRZ -DSMALL -DRELOC=0x${RELOC} \
		-DLIBSA_NO_FS_CLOSE -DLIBSA_NO_FS_SYMLINK -DLIBSA_NO_FS_WRITE \
		-DLIBSA_NO_DEV_CLOSE -DLIBSA_NO_DEV_IOCTL \
		-DLIBSA_NO_RAW_ACCESS -DLIBSA_NO_TWIDDLE \
		-DALLOC_FIRST_FIT \
		-DLIBSA_USE_MEMCPY -DLIBSA_USE_MEMSET
# The following will gain more space if needed (in rough order of
# preference):
# -DLIBSA_SINGLE_FILESYSTEM=ufs
# -DLIBSA_SINGLE_DEVICE=rz -D"rzioctl(x,y,z)=EINVAL" -D"rzclose(f)=0"
# -DLIBSA_FS_SINGLECOMPONENT
# -DLIBSA_NO_DISKLABEL_MSGS
# Also possible to save space:
#  + Remove disklabel support.
#  + Change alloc() to always allocate past "top", and don't worry
#    about free().  At the moment, there's only about 50kB alloc()'d.


ALL=	rzboot bootrz

all:	${ALL}

${PROG}.aout: ${PROG}
	elf2aout ${PROG} ${PROG}.aout

mkboot:	${.CURDIR}/mkboot.c
	${CC} ${CPPFLAGS} -I${DESTDIR}/usr/include -o mkboot ${.IMPSRC}

rzboot bootrz: ${PROG}.aout mkboot
	./mkboot ${PROG}.aout rzboot bootrz

proginstall:: bootrz rzboot
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.ALLSRC} ${DESTDIR}${BINDIR}

# .include <bsd.prog.mk>
.include "../Makefile.booters"
