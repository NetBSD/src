/* Copyright (c) 1994 Ludd, University of Lule}, Sweden.
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Changed for the VAX port (and for readability) /IC
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
 *	from: machdep.c,v 1.12 1993/10/13 09:36:43 cgd Exp $
 *	$Id: machdep.c,v 1.2 1994/08/16 23:47:34 ragge Exp $
 */

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
int	physmem;
struct	cfdriver nexuscd;
int	todrstopped,glurg;

caddr_t allocsys __P((caddr_t));

/*   */
#if 0
#define valloc(name, type, num) \
		(name) = (type *)v; v = (caddr_t)((name)+(num))
#endif
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
	int base, residual,i,sz;
	vm_offset_t minaddr, maxaddr;
	vm_size_t size;
	extern int cpu_type,boothowto,startpmapdebug;
	extern char *panicstr;

	printf("%s\n", version);
	printf("realmem = %d\n", mem_size);
	physmem=btoc(mem_size); 
	panicstr=NULL;
	mtpr(AST_NO,PR_ASTLVL);
	spl0();

	boothowto=0; /* XXX Vi l}ser s} att vi alltid f}r root-fr}{ga */


    /*
     * Find out how much space we need, allocate it,
     * and then give everything true virtual addresses.
     */
    sz = (int)allocsys((caddr_t)0);
    if ((v = (caddr_t)kmem_alloc(kernel_map, round_page(sz))) == 0)
        panic("startup: no room for tables");
    if (allocsys(v) - v != sz)
        panic("startup: table size inconsistency");

    /*
     * Now allocate buffers proper.  They are different than the above
     * in that they usually occupy more virtual memory than physical.
     */
    size = MAXBSIZE * nbuf;
    buffer_map = kmem_suballoc(kernel_map, (vm_offset_t *)&buffers,
                               &maxaddr, size, TRUE);
    minaddr = (vm_offset_t)buffers;
    if (vm_map_find(buffer_map, vm_object_allocate(size), (vm_offset_t)0,
                    &minaddr, size, FALSE) != KERN_SUCCESS)
        panic("startup: cannot allocate buffers");
    if ((bufpages / nbuf) >= btoc(MAXBSIZE)) {
        /* don't want to alloc more physical mem than needed */
        bufpages = btoc(MAXBSIZE) * nbuf;
    }
    base = bufpages / nbuf;
    residual = bufpages % nbuf;
    for (i = 0; i < nbuf; i++) {
        vm_size_t curbufsize;
        vm_offset_t curbuf;

        /*
         * First <residual> buffers get (base+1) physical pages
         * allocated for them.  The rest get (base) physical pages.
         *
         * The rest of each buffer occupies virtual space,
         * but has no physical memory allocated for it.
         */
        curbuf = (vm_offset_t)buffers + i * MAXBSIZE;
        curbufsize = CLBYTES * (i < residual ? base+1 : base);
        vm_map_pageable(buffer_map, curbuf, curbuf+curbufsize, FALSE);
        vm_map_simplify(buffer_map, curbuf);
    }

    /*
     * Allocate a submap for exec arguments.  This map effectively
     * limits the number of processes exec'ing at any time.
     */
    exec_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
                             16*NCARGS, TRUE);







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
#if 0
	bufpages = bufpages ? bufpages : mem_size/NBPG/CLSIZE/10;
	nbuf     = nbuf     ? nbuf     : max(bufpages, 16);
	nswbuf   = nswbuf   ? nswbuf   : min((nbuf/2)&~1, 256);

        v = (caddr_t) kmem_alloc(kernel_map,(nswbuf+nbuf)*sizeof(struct buf));

        if (v == 0) {
          panic("cpu_startup(): Cannot allocate physical memory for buffers");
        }
        swbuf = (struct buf *) v;
        buf   = swbuf + nswbuf;
        v    += round_page((nswbuf+nbuf)*sizeof(struct buf));
#endif
        /*
         * XXX Now allocate buffers proper.  They are different than the above
         * in that they usually occupy more virtual memory than physical.
         */
#if 0
        size = MAXBSIZE * nbuf;
        buffer_map = kmem_suballoc(kernel_map, (vm_offset_t) &buffers,
                                   &maxaddr, size, FALSE);
        minaddr = (vm_offset_t) buffers;
        if (vm_map_find(buffer_map, vm_object_allocate(size), (vm_offset_t)0,
                        &minaddr, size, FALSE) != KERN_SUCCESS)
                panic("cpu_startup(): Cannot allocate buffers");

        base = bufpages / nbuf;
        residual = bufpages % nbuf;
#endif
	/*
	 * The first (residual) buffers get (base+1) number of
	 * physical pages allocated from the beginning.
	 * The other buffers only get (base) number of physical
	 * pages allocated. The remaining virtual space of the
	 * buffers stays unmapped.
	 */
#if 0
        for (i = 0; i < nbuf; i++) {
	  vm_size_t   curbufsize;
	  vm_offset_t curbuf;
glurg=i;
	  curbuf = (vm_offset_t) buffers + i * MAXBSIZE;
	  curbufsize = CLBYTES * (i < residual ? base+1 : base);
	  vm_map_pageable(buffer_map, curbuf, curbuf + curbufsize, FALSE);
	  vm_map_simplify(buffer_map, curbuf);
        }
#endif
	/* We found it cleaner to allocate the i/o buffers in autoconf.c
	 * instead of here, as the previous (horrid) code did.
	 * Look in autoconf.c for its specification.   (Aqua)
	 */

        /*
	 * Finally, allocate mbuf pool.  Since mclrefcnt is an off-size
         * we use the more space efficient malloc in place of kmem_alloc.
         */

        mclrefcnt = (char *)malloc(NMBCLUSTERS+CLBYTES/MCLBYTES,
                                   M_MBUF, M_NOWAIT);
        bzero(mclrefcnt, NMBCLUSTERS+CLBYTES/MCLBYTES);
        mb_map = kmem_suballoc(kernel_map, (vm_offset_t*)&mbutl, &maxaddr,
                               VM_MBUF_SIZE, FALSE);

        /*
         * Initialize callouts
         */
#if 0
	callout=(struct callout *)
		kmem_alloc(kernel_map, ncallout*sizeof(struct callout));
#endif

        callfree = callout;
        for (i = 1; i < ncallout; i++)
                callout[i-1].c_next = &callout[i];
	callout[i-1].c_next = NULL;

	printf("avail mem = %d\n", ptoa(cnt.v_free_count));
	printf("Using %d buffers containing %d bytes of memory.\n",
	       nbuf, bufpages * CLBYTES);

        /*
         * Set up buffers, so they can be used to read disk labels.
         */

        bufinit();
	/*
	 * XXX Det h{r skall inte alls g|ras s} h{r! (och inte h{r :)
	 */
#if 0
	swapmap=(struct map *)
		kmem_alloc(kernel_map, (sizeof(struct map)*512)); /* XXX */
	nswapmap=512;
#endif
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
        return;
}
/*
 * Allocate space for system data structures.  We are given
 * a starting virtual address and we return a final virtual
 * address; along the way we set each data structure pointer.
 *
 * We call allocsys() with 0 to find out how much space we want,
 * allocate that much and fill it with zeroes, and then call
 * allocsys() again with the correct base virtual address.
 */
caddr_t
allocsys(v)
        register caddr_t v;
{

#define valloc(name, type, num) \
            v = (caddr_t)(((name) = (type *)v) + (num))

#ifdef REAL_CLISTS
        valloc(cfree, struct cblock, nclist);
#endif
        valloc(callout, struct callout, ncallout);
        valloc(swapmap, struct map, nswapmap = maxproc * 2);
#ifdef SYSVSHM
        valloc(shmsegs, struct shmid_ds, shminfo.shmmni);
#endif
#ifdef SYSVSEM
        valloc(sema, struct semid_ds, seminfo.semmni);
        valloc(sem, struct sem, seminfo.semmns);
        /* This is pretty disgusting! */
        valloc(semu, int, (seminfo.semmnu * seminfo.semusz) / sizeof(int));
#endif
#ifdef SYSVMSG
        valloc(msgpool, char, msginfo.msgmax);
        valloc(msgmaps, struct msgmap, msginfo.msgseg);
        valloc(msghdrs, struct msg, msginfo.msgtql);
        valloc(msqids, struct msqid_ds, msginfo.msgmni);
#endif

        /*
         * Determine how many buffers to allocate (enough to
         * hold 5% of total physical memory, but at least 16).
         * Allocate 1/2 as many swap buffer headers as file i/o buffers.
         */
#ifdef DEBUG
printf("physmem: %x, btoc %x, CLSIZE %x\n",physmem,btoc(2*1024*1024),CLSIZE);
#endif
        if (bufpages == 0)
            if (physmem < btoc(2 * 1024 * 1024))
                bufpages = (physmem / 10) / CLSIZE;
            else
                bufpages = (physmem / 20) / CLSIZE;
        if (nbuf == 0) {
                nbuf = bufpages;
                if (nbuf < 16)
                        nbuf = 16;
        }
        if (nswbuf == 0) {
                nswbuf = (nbuf / 2) &~ 1;       /* force even */
                if (nswbuf > 256)
                        nswbuf = 256;           /* sanity */
        }
        valloc(swbuf, struct buf, nswbuf);
        valloc(buf, struct buf, nbuf);
        return v;
}

/*
 * disktid contains time from superblock of the root filesystem.
 * We compare this with the time in mfpr(PR_TODR) which updates
 * 100 times/second. Because todr is only 32 bits, it is simplest 
 * to start counting from 1/1 00.00 and reset todr an year later.
 * (And that was the way they did it in old BSD Unix... :)
 * One year is about 3153600000 in todr.
 */
inittodr(time_t disktid){
	int todrtid,nytid;

	if(todrstopped){
		printf(
		"TODR clock not started - time taken from file system.\n");
		nytid=disktid;
	} else {
		/* XXX This is ugly time counting :( */
		todrtid=(disktid/(3600*24*365))+1970+(mfpr(PR_TODR)/100);
		if(disktid>todrtid){
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

microtime(){
	printf("Microtime...\n");
}
