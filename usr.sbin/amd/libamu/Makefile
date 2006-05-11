#	$NetBSD: Makefile,v 1.20 2006/05/11 09:08:09 mrg Exp $

NOLINKLIB=	# defined

.include <bsd.own.mk>

LIB=	amu

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/libamu
.PATH:	${DIST} ${IDIST}/conf/mtab ${IDIST}/conf/transp ${IDIST}/conf/umount

SRCS=	hasmntopt.c misc_rpc.c mount_fs.c mtab.c mtab_bsd.c \
	nfs_prot_xdr.c strutil.c transp_sockets.c ualarm.c umount_bsd44.c \
	wire.c xdr_func.c xutil.c

# XXX
.if ${HAVE_GCC} == 4
COPTS.xdr_func.c+=	-fno-strict-aliasing
.endif

# some host specific compile time settings amd wants to have compiled in
config_local.h: mkconf
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	${HOST_SH} ${.ALLSRC} \
	    `${HOST_SH} ${NETBSDSRCDIR}/sys/conf/osrelease.sh` > ${.TARGET}

CLEANFILES+=	config_local.h

.include <bsd.lib.mk>

${SRCS}: config_local.h
