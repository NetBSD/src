#	$NetBSD: Makefile,v 1.16 2002/09/18 03:54:27 lukem Exp $

NOLINKLIB=	# defined

.include <bsd.own.mk>

LIB=	amu

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/libamu
.PATH:	${DIST} ${IDIST}/conf/mtab ${IDIST}/conf/transp ${IDIST}/conf/umount

SRCS=	hasmntopt.c misc_rpc.c mount_fs.c mtab.c mtab_bsd.c \
	nfs_prot_xdr.c transp_sockets.c ualarm.c umount_bsd44.c util.c wire.c \
	xdr_func.c xutil.c

# some host specific compile time settings amd wants to have compiled in
config_local.h: mkconf
	@rm -f ${.TARGET}
	sh ${.ALLSRC} `sh ${NETBSDSRCDIR}/sys/conf/osrelease.sh` > ${.TARGET}

CLEANFILES+=	config_local.h

.include <bsd.lib.mk>

${SRCS}: config_local.h
