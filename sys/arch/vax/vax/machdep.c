/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: Utah Hdr: machdep.c 1.63 91/04/24
 *	from: @(#)machdep.c	7.16 (Berkeley) 6/3/91
 *	$Id: machdep.c,v 1.1 1994/08/02 20:22:02 ragge Exp $
 */

/* All bugs are subject to removal without further notice */

#include "sys/param.h"
#include "vax/include/sid.h"
#include "sys/map.h"
#include "buf.h"
#include "mbuf.h"
#include "vax/include/pte.h"
#include "uba.h"
#include "reboot.h"
#include "sys/callout.h"
#include "sys/device.h"
#include "conf.h"
#include "sys/proc.h"
#include "sys/user.h"
#include "sys/time.h"
#include "sys/kernel.h"
#include "vax/include/mtpr.h"
#include "vax/include/cpu.h"
#include "vm/vm.h"
#include "vm/vm_kern.h"
#include "vm/vm_page.h"
#include "vax/include/macros.h"
#include "vax/include/nexus.h"

/*
 * We do these external declarations here, maybe they should be done 
 * somewhere else...
 */
int nmcr, nmba, numuba, cold=1;
caddr_t	mcraddr[MAXNMCR];
int     astpending;
int     want_resched;
char	machine[]="VAX";
char	cpu_model[100];
int	msgbufmapped=0;
struct	msgbuf *msgbufp;
int	physmem=0;
struct	cfdriver nexuscd;
int	todrstopped;

/*   */
#define valloc(name, type, num) \
		(name) = (type *)v; v = (caddr_t)((name)+(num))
#define valloclim(name, type, num, lim) \
		(name) = (type *)v; v = (caddr_t)((lim) = ((name)+(num)))

#ifdef  BUFPAGES
int     bufpages = BUFPAGES;
#else
int     bufpages = 0;
#endif
int     nswbuf = 0;
#ifdef  NBUF
int     nbuf = NBUF;
#else
int     nbuf = 0;
#endif

cpu_startup() {
	caddr_t v,tempaddr;
	extern char version[];
	int base, residual,i;
	vm_offset_t minaddr, maxaddr;
	vm_size_t size;
	extern int cpu_type,boothowto;
	extern char *panicstr;

	printf("%s\n", version);
	printf("RealMem = 0x%s%x\n", PRINT_HEXF(mem_size), mem_size);
	panicstr=NULL;
	mtpr(AST_NO,PR_ASTLVL);
	spl0();

/*	printf("avail mem = %d\n", ptoa(vm_page_free_count)); */
	boothowto=0; /* XXX Vi l}ser s} att vi alltid f}r root-fr}{ga */

        /*
         * Allocate space for system data structures.
	 * First the sizes of the buffers are calculated and then 
	 * memory is allocated for those buffers. The first
	 * available kernel virtual address is then saved to "v".
	 * Last of all, the pointers to the buffers are set to point
	 * to their respective memory area.
	 *
         * An index into the kernel page table corresponding to the
         * virtual memory address maintained in "v" is kept in "mapaddr".
         */

	bufpages = bufpages ? bufpages : mem_size/NBPG/CLSIZE/10;
	nbuf     = nbuf     ? nbuf     : max(bufpages, 16);
	nswbuf   = nswbuf   ? nswbuf   : min((nbuf/2)&~1, 256);

/*
	printf("cpu_startup(): Mapping 0x%x bytes from kernel_map into v\n",
	       (nswbuf+nbuf)*sizeof(struct buf));
	printf("               Must allocate 0x%x bytes for the operation.\n",
	       round_page((nswbuf+nbuf)*sizeof(struct buf)));
*/

        v = (caddr_t) kmem_alloc(kernel_map,(nswbuf+nbuf)*sizeof(struct buf));

        if (v == 0) {
          panic("cpu_startup(): Cannot allocate physical memory for buffers");
        }
        swbuf = (struct buf *) v;
        buf   = swbuf + nswbuf;
        v    += round_page((nswbuf+nbuf)*sizeof(struct buf));

/*	printf("avail mem = %d\n", ptoa(vm_page_free_count)); */

/*	printf("cpu_startup(), after allocation:\n");              XXX */
/*	printf("swbuf=0x%x, buf=0x%x, v=0x%x\n", swbuf, buf, v);   XXX */
/*	printf("nswbuf=0x%x, nbuf=0x%x\n", nswbuf, nbuf);          XXX */

        /*
         * XXX Now allocate buffers proper.  They are different than the above
         * in that they usually occupy more virtual memory than physical.
         */

        size = MAXBSIZE * nbuf;
        buffer_map = kmem_suballoc(kernel_map, (vm_offset_t) &buffers,
                                   &maxaddr, size, FALSE);
        minaddr = (vm_offset_t) buffers;
        if (vm_map_find(buffer_map, vm_object_allocate(size), (vm_offset_t)0,
                        &minaddr, size, FALSE) != KERN_SUCCESS)
                panic("cpu_startup(): Cannot allocate buffers");

/*	printf("cpu_startup(): avail mem = %d\n",
	       ptoa(vm_page_free_count));                         /* XXX */

        base = bufpages / nbuf;
        residual = bufpages % nbuf;

	/*
	 * The first (residual) buffers get (base+1) number of
	 * physical pages allocated from the beginning.
	 * The other buffers only get (base) number of physical
	 * pages allocated. The remaining virtual space of the
	 * buffers stays unmapped.
	 */
        for (i = 0; i < nbuf; i++) {
	  vm_size_t   curbufsize;
	  vm_offset_t curbuf;
	  curbuf = (vm_offset_t) buffers + i * MAXBSIZE;
	  curbufsize = CLBYTES * (i < residual ? base+1 : base);
	  vm_map_pageable(buffer_map, curbuf, curbuf + curbufsize, FALSE);
	  vm_map_simplify(buffer_map, curbuf);
        }
	/* We found it cleaner to allocate the i/o buffers in autoconf.c
	 * instead of here, as the previous (horrid) code did.
	 * Look in autoconf.c for its specification.   (Aqua)
	 */

/*	printf("cpu_startup(): avail mem = %d\n",
	       ptoa(vm_page_free_count));                         /* XXX */

        /*
	 * Finally, allocate mbuf pool.  Since mclrefcnt is an off-size
         * we use the more space efficient malloc in place of kmem_alloc.
         */

        mclrefcnt = (char *)malloc(NMBCLUSTERS+CLBYTES/MCLBYTES,
                                   M_MBUF, M_NOWAIT);
        bzero(mclrefcnt, NMBCLUSTERS+CLBYTES/MCLBYTES);
        mb_map = kmem_suballoc(kernel_map, (vm_offset_t)&mbutl, &maxaddr,
                               VM_MBUF_SIZE, FALSE);
/*	printf("Available physical memory = %d.\n", ptoa(vm_page_free_count));*/
	printf("Using %d buffers containing %d bytes of memory.\n",
	       nbuf, bufpages * CLBYTES);

        /*
         * Set up buffers, so they can be used to read disk labels.
         */

        bufinit();
        /*
         * Initialize callouts
         */
	callout=(struct callout *)
		kmem_alloc(kernel_map, ncallout*sizeof(struct callout));

        callfree = callout;
        for (i = 1; i < ncallout; i++)
                callout[i-1].c_next = &callout[i];
	/*
	 * XXX Det h{r skall inte alls g|ras s} h{r! (och inte h{r :)
	 */
	swapmap=(struct map *)
		kmem_alloc(kernel_map, (sizeof(struct map)*512)); /* XXX */
	nswapmap=512;
        /*
         * Configure the system.
         */
	switch(MACHID(cpu_type)){

#ifdef VAX750
	case VAX_750:
	strcpy(cpu_model,"VAX 11/750");
		conf_750();
		break;
#endif
	default:
		printf("Cpu type %d not configured.\n",MACHID(cpu_type));
		asm("halt");
	}
#if GENERIC
        if ((boothowto & RB_ASKNAME) == 0)
                setroot();
        setconf();
#else
        setroot();
#endif
        /*
         * Configure swap area and related system
         * parameter based on device(s) used.
         */
        swapconf();
	cold=0;
	lastinit();
        return;
	
}

inittodr(time_t disktid){
/*
 * disktid contains time from superblock of the root filesystem.
 * We compare this with the time in mfpr(PR_TODR) which updates
 * 100 times/second. Because todr is only 32 bits, it is simplest 
 * to start counting from 1/1 00.00 and reset todr an year later.
 * One year is about 3153600000 in todr.
 */
	int todrtid,nytid;

	if(todrstopped){
		printf(
		"TODR clock not started - time taken from file system.\n");
		nytid=disktid;
	} else {
		/* XXX This is ugly time counting :( */
		todrtid=(disktid/(3600*24*365))+1970+(mfpr(PR_TODR)/100);
		if(disktid>todrtid){
			/* Old well known message :) */
			printf(
		"WARNING: todr too small -- CHECK AND RESET THE DATE!\n");
			/* Use filesystem time anyway */
			nytid=disktid;
		} else {
			nytid=todrtid;
		}
	}
	time.tv_sec=nytid;
}

resettodr(){
	printf("Time reset routine resettodr() not implemented yet.\n");
}

dumpconf(){
	printf("dumpconf() not implemented - yet!\n");
}

cpu_initclocks(){
	todrstopped=clock_750();
}

cpu_sysctl(){
	printf("cpu_sysctl:\n");
	return(EOPNOTSUPP);
}

setstatclockrate(){
	printf("setstatclockrate\n");
	asm("halt");
}

struct queue {
        struct queue *q_next, *q_prev;
};

/*
 * insert an element into a queue
 */
void
_insque(elem, head)
        register struct queue *elem, *head;
{
        register struct queue *next;

        next = head->q_next;
        elem->q_next = next;
        head->q_next = elem;
        elem->q_prev = head;
        next->q_prev = elem;
}

/*
 * remove an element from a queue
 */
void
_remque(elem)
        register struct queue *elem;
{
        register struct queue *next, *prev;

        next = elem->q_next;
        prev = elem->q_prev;
        next->q_prev = prev;
        prev->q_next = next;
        elem->q_prev = 0;
}

lastinit()
{
	caddr_t	plats;

	plats=malloc(4096,M_PCB,M_NOWAIT);
	mtpr(plats,PR_P0BR);
	mtpr(1,PR_P0LR);
	mtpr(plats+4096-0x800000,PR_P1BR);
	mtpr(0x1ffffe,PR_P1LR);
	mtpr(0x7ffffffc,PR_USP);
	printf("lastinit: plats %x\n",plats);
	*(int *)plats=0xa0002800;
	*(int *)(plats+4092)=0xa0002801;
}
