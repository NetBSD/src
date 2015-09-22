# $NetBSD: ssp.mk,v 1.1.2.2 2015/09/22 12:05:56 skrll Exp $

.if ${USE_SSP:Uno} == "yes"
COPTS.kern_ssp.c+=	-fno-stack-protector -D__SSP__
.endif

# for multi-cpu machines, cpu_hatch() straddles the init of
# __stack_chk_guard, so ensure stack protection is disabled
.if ${MACHINE} == "i386" || ${MACHINE_ARCH} == "x86_64"
COPTS.cpu.c+=		-fno-stack-protector
.endif

# The following files use alloca(3) or variable array allocations.
# Their full name is noted as documentation.
VARSTACK= \
	arch/xen/i386/gdt.c \
	dev/ic/aic79xx.c \
	dev/ic/aic7xxx.c \
	dev/ofw/ofw_subr.c \
	kern/uipc_socket.c \
	miscfs/genfs/genfs_vnops.c \
	nfs/nfs_bio.c \
	uvm/uvm_bio.c \
	uvm/uvm_pager.c \

.for __varstack in ${VARSTACK}
COPTS.${__varstack:T} += -Wno-stack-protector
.endfor
