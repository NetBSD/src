#	$NetBSD: Makefile,v 1.18.6.1 2007/02/24 12:17:34 bouyer Exp $

NOLINKLIB=	# defined

.include <bsd.own.mk>

LIB=	amu

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/libamu
.PATH:	${DIST} ${IDIST}/conf/mtab ${IDIST}/conf/transp ${IDIST}/conf/umount

SRCS=	hasmntopt.c misc_rpc.c mount_fs.c mtab.c mtab_bsd.c \
	nfs_prot_xdr.c strutil.c transp_sockets.c ualarm.c umount_bsd44.c \
	wire.c xdr_func.c xutil.c

# some host specific compile time settings amd wants to have compiled in
config_local.h: mkconf
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	${HOST_SH} ${.ALLSRC} \
	    `${HOST_SH} ${NETBSDSRCDIR}/sys/conf/osrelease.sh` > ${.TARGET}

CLEANFILES+=	config_local.h

.include <bsd.lib.mk>

${SRCS}: config_local.h
