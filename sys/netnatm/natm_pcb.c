/*	$NetBSD: natm_pcb.c,v 1.16.4.1 2016/04/22 15:44:18 skrll Exp $	*/

/*
 * Copyright (c) 1996 Charles D. Cranor and Washington University.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * atm_pcb.c: manage atm protocol control blocks and keep IP and NATM
 * from trying to use each other's VCs.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: natm_pcb.c,v 1.16.4.1 2016/04/22 15:44:18 skrll Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/mbuf.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>

#include <netnatm/natm.h>

/*
 * npcb_alloc: allocate a npcb [in the free state]
 */

struct natmpcb *
npcb_alloc(bool wait)
{
  struct natmpcb *npcb;

  npcb = kmem_intr_zalloc(sizeof(*npcb), wait ? KM_SLEEP : KM_NOSLEEP);
  if (npcb) {
    npcb->npcb_flags = NPCB_FREE;
  }
  return(npcb);
}


/*
 * npcb_free: free a npcb
 */

void
npcb_free(struct natmpcb *npcb, int op)
{
  int s = splnet();

  if ((npcb->npcb_flags & NPCB_FREE) == 0) {
    LIST_REMOVE(npcb, pcblist);
    npcb->npcb_flags = NPCB_FREE;
  }
  if (op == NPCB_DESTROY) {
    if (npcb->npcb_inq) {
      npcb->npcb_flags = NPCB_DRAIN;	/* flag for destruction */
    } else {
      kmem_intr_free(npcb, sizeof(*npcb));
    }
  }

  splx(s);
}


/*
 * npcb_add: add or remove npcb from main list
 *   returns npcb if ok
 */

struct natmpcb *
npcb_add(struct natmpcb *npcb, struct ifnet *ifp,
	u_int16_t vci, u_int8_t vpi)
{
  struct natmpcb *cpcb = NULL;		/* current pcb */
  int s = splnet();


  /*
   * lookup required
   */

  for (cpcb = natm_pcbs.lh_first ; cpcb != NULL ;
					cpcb = cpcb->pcblist.le_next) {
    if (ifp == cpcb->npcb_ifp && vci == cpcb->npcb_vci && vpi == cpcb->npcb_vpi)
      break;
  }

  /*
   * add & something already there?
   */

  if (cpcb) {
    cpcb = NULL;
    goto done;					/* fail */
  }

  /*
   * need to allocate a pcb?
   */

  if (npcb == NULL) {
    cpcb = npcb_alloc(false);	/* could be called from lower half */
    if (cpcb == NULL)
      goto done;			/* fail */
  } else {
    cpcb = npcb;
  }

  cpcb->npcb_ifp = ifp;
  cpcb->ipaddr.s_addr = 0;
  cpcb->npcb_vci = vci;
  cpcb->npcb_vpi = vpi;
  cpcb->npcb_flags = NPCB_CONNECTED;

  LIST_INSERT_HEAD(&natm_pcbs, cpcb, pcblist);

done:
  splx(s);
  return(cpcb);
}



#ifdef DDB

int npcb_dump(void);

int npcb_dump(void)

{
  struct natmpcb *cpcb;

  printf("npcb dump:\n");
  for (cpcb = natm_pcbs.lh_first ; cpcb != NULL ;
					cpcb = cpcb->pcblist.le_next) {
    printf("if=%s, vci=%d, vpi=%d, IP=0x%x, sock=%p, flags=0x%x, inq=%d\n",
	cpcb->npcb_ifp->if_xname, cpcb->npcb_vci, cpcb->npcb_vpi,
	cpcb->ipaddr.s_addr, cpcb->npcb_socket,
	cpcb->npcb_flags, cpcb->npcb_inq);
  }
  printf("done\n");
  return(0);
}

#endif
