# $NetBSD: Makefile,v 1.2 2023/06/03 09:09:14 lukem Exp $

.include "../Makefile.inc"
.include "../drmkms/Makefile.inc"

KMOD=	drmkms_ttm

.PATH:	${S}/external/bsd/drm2/ttm
.PATH:	${S}/external/bsd/drm2/dist/drm/ttm

CPPFLAGS+=	-DCONFIG_AGP=1

WARNS=	3

COPTS.ttm_bo.c+=	${CC_WNO_IMPLICIT_FALLTHROUGH}

CWARNFLAGS+=		-Wno-missing-field-initializers
CWARNFLAGS+=		-Wno-shadow

SRCS+=	ttm_agp_backend.c
SRCS+=	ttm_memory.c
SRCS+=	ttm_tt.c
SRCS+=	ttm_bo.c
SRCS+=	ttm_bo_util.c
SRCS+=	ttm_bo_vm.c
SRCS+=	ttm_module.c
#SRCS+=	ttm_object.c
#SRCS+=	ttm_lock.c
SRCS+=	ttm_execbuf_util.c
#SRCS+=	ttm_page_alloc.c
SRCS+=	ttm_bo_manager.c
#SRCS+=	ttm_page_alloc_dma.c
SRCS+=	ttm_bus_dma.c

.include <bsd.kmodule.mk>
