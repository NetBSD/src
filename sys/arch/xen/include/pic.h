/*	$NetBSD: pic.h,v 1.2 2005/12/11 12:19:48 christos Exp $	*/

#ifndef _XEN_PIC_H_
#define	_XEN_PIC_H_

#include <x86/pic.h>

#define	PIC_XEN		4

extern struct pic xen_pic;

#endif /* _XEN_PIC_H_ */
