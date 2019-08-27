#	$NetBSD: srcs.mk,v 1.1 2019/08/27 14:53:38 martin Exp $

# Sources and additional flags for libnv

LIBNV_SRCS=	dnvlist.c msgio.c nvlist.c nvpair.c nv_kern_netbsd.c
NVSRC_DISTPATH=	${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

.for _s in ${LIBNV_SRCS}
CPPFLAGS.${_s}	+=	-I${NVSRC_DISTPATH}
.endfor

.PATH:	${NVSRC_DISTPATH}
