#	$NetBSD: Makefile,v 1.38 2008/02/02 17:01:03 tsutsui Exp $
#

.include <bsd.own.mk>

PROG=	installboot
MAN=	installboot.8
SRCS=	installboot.c sum.c machines.c fstypes.c


ARCH_XLAT= amd64-i386.c news68k-news.c newsmips-news.c
ARCH_XLAT+= sun2-sun68k.c sun3-sun68k.c

.if !defined(SMALLPROG) && !defined(ARCH_FILES)
ARCH_FILES=  alpha.c amiga.c ews4800mips.c hp300.c hp700.c i386.c
ARCH_FILES+= landisk.c macppc.c news.c next68k.c pmax.c
ARCH_FILES+= sparc.c sparc64.c sun68k.c vax.c x68k.c
.else
ARCH_FILES?= ${ARCH_XLAT:M${MACHINE}-*:S/${MACHINE}-//}
.if empty(ARCH_FILES)
ARCH_FILES= ${MACHINE}.c
.endif
.endif

SRCS+=${ARCH_FILES}

.if empty(ARCH_FILES:C/(macppc|news|sparc|sun68k|x68k)/stg2/:Mstg2.c)
CPPFLAGS	+= -DNO_STAGE2
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

WARNS?=	3

UFSSRC=		${NETBSDSRCDIR}/sys/ufs
CPPFLAGS+=	-I${.CURDIR} -I.
.PATH:		${.CURDIR}/arch ${UFSSRC}/ffs ${UFSSRC}/ext2fs

# XXX
.if ${MACHINE_ARCH} == "m68000"
COPTS.ffs.c=	-O0
.endif

.if !defined(HOSTPROGNAME)
.if ${HAVE_GCC} == 4
.for f in i386 macppc
COPTS.${f}.c+=  -Wno-pointer-sign
.endfor
.endif
.endif

.include <bsd.prog.mk>
