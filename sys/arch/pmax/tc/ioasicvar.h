/*	$NetBSD: ioasicvar.h,v 1.2 1998/01/05 07:03:17 perry Exp $	*/

/*
 * Copyright 1995, 1996 Jonathan Stone
 * All rights reserved
 */
#ifndef __IOASICVAR_H__
#define __IOASICVAR_H__
extern struct cfdriver ioasiccd;	/* FIXME: in header file */
extern caddr_t ioasic_cvtaddr __P((struct confargs *ca));
#endif /*__IOASICVAR_H__*/
