/*	$NetBSD: iommu.h,v 1.2 1998/01/05 07:03:35 perry Exp $	*/

void iommu_init();
void set_iommupde(u_int32_t va, u_int32_t pa);
