/* $NetBSD: ppcstart.c,v 1.1.2.1 1999/12/27 18:31:37 wrstuden Exp $ */

/*
 * Copyright 1999 Ignatios Souvatzis. All rights reserved.
 */

/*
 * Startit for Phase5 PPC boards. 
 */

#include <sys/types.h>
#include "libstubs.h"

#include "kickstart68.c"

void
startit(kp, ksize, entry, fmem, fmemsz, cmemsz,
	    boothowto, esym, cpuid, eclock, amiga_flags, I_flag,
	    bootpartoff, history)

	u_long kp, ksize, entry, fmem, fmemsz, cmemsz,
		boothowto, esym, cpuid, eclock, amiga_flags, I_flag,
		bootpartoff, history;
{
	int i;
	
#define ONESEC
/* #define ONESEC for (i=0; i<1000000; i++) */
	
	ONESEC *(volatile u_int16_t *)0xdff180 = 0x0f0;
	*(volatile u_int8_t *)0xf60000 = 0x10;
	ONESEC *(volatile u_int16_t *)0xdff180 = 0xf80;

	memcpy((caddr_t)0xfff00100, kickstart, kicksize);
	*(volatile u_int32_t *)0xfff000f4 = fmem;
	*(volatile u_int32_t *)0xfff000f8 = kp;
	*(volatile u_int32_t *)0xfff000fc = ksize;
	ONESEC *(volatile u_int16_t *)0xdff180 = 0xf00;
	*(volatile u_int8_t *)0xf60000 = 0x90;
	*(volatile u_int8_t *)0xf60000 = 0x08;
	/* NOTREACHED */
	while (1) {
		*(volatile u_int16_t *)0xdff180 = 0x0f0;
		*(volatile u_int16_t *)0xdff180 = 0x00f;
	}
}
