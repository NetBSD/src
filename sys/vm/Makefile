#	$NetBSD: Makefile,v 1.3 1999/03/30 13:08:55 mycroft Exp $

KDIR=	/sys/vm
INCSDIR= /usr/include/vm

INCS=	pglist.h pmap.h vm.h vm_conf.h \
	vm_extern.h vm_inherit.h vm_kern.h vm_map.h vm_object.h vm_page.h \
	vm_pageout.h vm_pager.h vm_param.h vm_prot.h

# XXX remove after NetBSD 1.4 (or earlier if you want :)
INCS+=	vm_swap.h 

.include <bsd.kinc.mk>
