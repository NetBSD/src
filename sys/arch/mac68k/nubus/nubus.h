/*-
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: nubus.h,v 1.2 1993/11/29 00:32:52 briggs Exp $
 *
 */

#define NUBUS_VIDEO		3
#define NUBUS_NETWORK		4
#define NUBUS_MOTHERBOARD	0x0a
#define NUBUS_MAXSLOTS		16

struct imagedata{
	long whatTheHellIsThis;
	long offset;
	short rowbytes;
	short top;
	short left;
	short right;
	short bottom;
	short version;
	short packType;
	short packSize;
	long hRes;
	long vRes;
	short pixelType;
	short pixelSize;	
};


/* this is the main data structure that points to good stuff */
struct header {
	long offset;
	long length;
	long crc;
	char romrev;
	char format;
	long tst;
	char reserved;
	char bytelane;
} ;


/* this is what the directory entries contain */
struct dir {
	unsigned char rsrc;
	unsigned long offset;
	unsigned long base;
};

/* describe a single slot */
struct slot {
	int size;
	struct header head;
	struct dir mainDir[15];
	long type;
	char name[40];
	char manufacturer[40];
};

struct nubus_hw{
   int found;			/* If there is a card there	*/
   caddr_t addr;		/* Phys addr of start of card	*/
   caddr_t rom;			/* Phys addr of start of ROM	*/
   int claimed;			/* TRUE if a driver claims this */
   struct slot Slot;		/* MF NUBUS STUFF */
   /* any other Nubus stuff we can think of when we get */
   /*  the NuBus documentation */
};
