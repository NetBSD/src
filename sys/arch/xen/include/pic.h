/*	$NetBSD: pic.h,v 1.1 2004/03/11 21:44:08 cl Exp $	*/

#ifndef _XEN_PIC_H_
#define	_XEN_PIC_H_

#include <x86/pic.h>

#define	PIC_XEN		4

extern struct pic xen_pic;

#endif /* _XEN_PIC_H_ */
