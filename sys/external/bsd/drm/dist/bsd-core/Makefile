SHARED=		../shared-core

SUBDIR = drm mach64 mga r128 radeon savage sis tdfx i915 #nouveau

.include <bsd.obj.mk>

depend: drm_pciids.h
all: drm_pciids.h

drm_pciids.h: ${SHARED}/drm_pciids.txt
	sh ../scripts/create_bsd_pci_lists.sh < ${SHARED}/drm_pciids.txt
