/*	$NetBSD: pic.h,v 1.1.4.4 2004/09/21 13:24:37 skrll Exp $	*/

#ifndef _XEN_PIC_H_
#define	_XEN_PIC_H_

#include <x86/pic.h>

#define	PIC_XEN		4

extern struct pic xen_pic;

#endif /* _XEN_PIC_H_ */
