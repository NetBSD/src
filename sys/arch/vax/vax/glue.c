/*	$NetBSD: glue.c,v 1.2 1994/10/26 08:03:06 cgd Exp $	*/

/* This glue file will go away SOON! */

#include "vax/include/pte.h"
#include "sys/param.h"
#include "sys/buf.h"
#include "sys/map.h"

#include "vax/uba/ubavar.h"


#define C (caddr_t)

extern struct uba_driver udadriver;
extern udaintr();
extern struct uba_driver dedriver;
extern deintr();

struct uba_ctlr ubminit[] = {
	{ &udadriver,0,'?',0,udaintr, C 0172150 },
	0
};

struct uba_device ubdinit[] = {
	{ &udadriver,0,0,'?',0,0,C 00,1,0x0},
	{ &dedriver,0,-1,'?',-1,deintr,C 0174510,0,0x0},
	0
};
