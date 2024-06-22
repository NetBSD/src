#	$NetBSD: Makefile,v 1.56.6.1 2024/06/22 10:57:10 martin Exp $
#

.include <bsd.own.mk>

PROG=	installboot
MAN=	installboot.8
SRCS=	installboot.c sum.c machines.c fstypes.c

ARCH_XLAT= amd64-i386.c news68k-news.c newsmips-news.c
ARCH_XLAT+= sun2-sun68k.c sun3-sun68k.c

.if !defined(SMALLPROG) && !defined(ARCH_FILES)
ARCH_FILES=  alpha.c amiga.c
ARCH_FILES+= emips.c evbarm.c evbmips.c ews4800mips.c
ARCH_FILES+= hp300.c hppa.c
ARCH_FILES+= i386.c
ARCH_FILES+= landisk.c
ARCH_FILES+= macppc.c
ARCH_FILES+= news.c next68k.c
ARCH_FILES+= pmax.c
ARCH_FILES+= sparc.c sparc64.c sun68k.c
ARCH_FILES+= vax.c
ARCH_FILES+= x68k.c
.else
ARCH_FILES?= ${ARCH_XLAT:M${MACHINE}-*:S/${MACHINE}-//}
.if empty(ARCH_FILES)
ARCH_FILES= ${MACHINE}.c
.endif
COPTS.machines.c+=	-DSINGLE_ARCH=ib_mach_${MACHINE}
.endif

SRCS+=${ARCH_FILES}

.if !empty(ARCH_FILES:C/(evbarm|evbmips)/evboard/:Mevboard.c)
SRCS+=evboards.c
.endif

.if !empty(ARCH_FILES:C/(evbarm|evbmips)/fdt/:Mfdt.c)
FDTDIR=		${.CURDIR}/../../sys/external/bsd/libfdt/dist
.PATH:		${FDTDIR}
CPPFLAGS+=	-DSUPPORT_FDT -I${FDTDIR}
SRCS+=fdt.c fdt_ro.c fdt_strerror.c
# XXX libfdt has some sign-comparison issues
COPTS.fdt.c+=		-Wno-error=sign-compare
COPTS.fdt_ro.c+=	-Wno-error=sign-compare
COPTS.fdt_strerror.c+=	-Wno-error=sign-compare
.endif


.if !defined(HOSTPROGNAME)
.if !empty(ARCH_FILES:C/(evbarm|evbmips)/ofw/:Mofw.c)
CPPFLAGS+=	-DSUPPORT_OPENFIRMWARE
.endif
.endif

.if empty(ARCH_FILES:C/(macppc|news|sparc|sun68k|x68k)/stg2/:Mstg2.c)
CPPFLAGS+=	-DNO_STAGE2
.else
SRCS+= bbinfo.c

# fstypes are only needed for 'stage2' and then only from bbinfo.
SRCS+= ffs.c
.if SMALLPROG
CPPFLAGS+=	-DNO_FFS_SWAP
.else
SRCS+= ffs_bswap.c
.endif
#SRCS+= ext2fs.c ext2fs_bswap.c
.endif

.if !empty(ARCH_FILES:C/(hp300|macppc)/cd9660/:Mcd9660.c) && \
    !defined(SMALLPROG)
CPPFLAGS+=	-DSUPPORT_CD9660
SRCS+= cd9660.c cd9660_util.c

.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN:
	-rm -rf fs
	${HOST_INSTALL_DIR} fs
	${HOST_INSTALL_DIR} fs/cd9660
	${HOST_LN} -s ${NETBSDSRCDIR}/sys/fs/unicode.h fs
	${HOST_LN} -s ${NETBSDSRCDIR}/sys/fs/cd9660/iso.h fs/cd9660
	${HOST_LN} -s ${NETBSDSRCDIR}/sys/fs/cd9660/cd9660_extern.h fs/cd9660
.endif

cleandir distclean: cleaninc

cleaninc:
	-rm -rf fs
.endif

UFSSRC=		${NETBSDSRCDIR}/sys/ufs
CD9660SRC=	${NETBSDSRCDIR}/sys/fs/cd9660
CPPFLAGS+=	-I${.CURDIR} -I.
.PATH:		${.CURDIR}/arch ${UFSSRC}/ffs ${UFSSRC}/ext2fs ${CD9660SRC}

.if !defined(HOSTPROGNAME)
.for f in i386 macppc
COPTS.${f}.c+=  -Wno-pointer-sign
.endfor

.if !empty(SRCS:Mevboards.c)
LDADD+=	-lprop
DPADD+=	${LIBPROP}
.endif

LDADD+= -lutil
DPADD+= ${LIBUTIL}
.endif

CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member
CWARNFLAGS.gcc+=	${GCC_NO_ADDR_OF_PACKED_MEMBER}

.include <bsd.prog.mk>
