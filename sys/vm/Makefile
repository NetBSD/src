#	$NetBSD: Makefile,v 1.2 1998/08/29 17:06:12 mrg Exp $

KDIR=	/sys/vm
INCSDIR= /usr/include/vm

INCS=	device_pager.h pglist.h pmap.h swap_pager.h vm.h vm_conf.h \
	vm_extern.h vm_inherit.h vm_kern.h vm_map.h vm_object.h vm_page.h \
	vm_pageout.h vm_pager.h vm_param.h vm_prot.h vnode_pager.h

# XXX remove after NetBSD 1.4 (or earlier if you want :)
INCS+=	vm_swap.h 

.include <bsd.kinc.mk>
