#	$NetBSD: Makefile,v 1.17 2003/10/19 06:11:38 lukem Exp $

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
	${_MKMSGCREATE}
	${_MKCMD}\
	rm -f ${.TARGET}
	${_MKCMD}\
	${HOST_SH} ${.ALLSRC} \
	    `${HOST_SH} ${NETBSDSRCDIR}/sys/conf/osrelease.sh` > ${.TARGET}

CLEANFILES+=	config_local.h

.include <bsd.lib.mk>

${SRCS}: config_local.h
