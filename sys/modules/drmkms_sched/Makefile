#	$NetBSD: Makefile,v 1.2 2022/07/28 10:24:46 riastradh Exp $

.include "../Makefile.inc"
.include "../drmkms/Makefile.inc"

.PATH:	${S}/external/bsd/drm2/dist/drm/scheduler
.PATH:	${S}/external/bsd/drm2/drm

KMOD=	drmkms_sched
MKLDSCRIPT=yes

SRCS+=	sched_entity.c
SRCS+=	sched_fence.c
SRCS+=	sched_main.c
SRCS+=	sched_module.c

WARNS=	3

.include <bsd.kmodule.mk>
