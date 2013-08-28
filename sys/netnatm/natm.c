/*	$NetBSD: natm.c,v 1.24.18.1 2013/08/28 15:21:49 rmind Exp $	*/

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
 * natm.c: native mode ATM access (both aal0 and aal5).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: natm.c,v 1.24.18.1 2013/08/28 15:21:49 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/domain.h>
#include <sys/ioctl.h>
#include <sys/protosw.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include <net/if.h>
#include <net/if_atm.h>
#include <net/netisr.h>
#include <net/route.h>

#include <netinet/in.h>

#include <netnatm/natm.h>

u_long natm5_sendspace = 16*1024;
u_long natm5_recvspace = 16*1024;

u_long natm0_sendspace = 16*1024;
u_long natm0_recvspace = 16*1024;

static int
natm_attach(struct socket *so, int proto)
{
	struct natmpcb *npcb;

	KASSERT(so->so_pcb == NULL);
	sosetlock(so);

	if (so->so_snd.sb_hiwat == 0 || so->so_rcv.sb_hiwat == 0) {
		if (proto == PROTO_NATMAAL5)
			error = soreserve(so, natm5_sendspace, natm5_recvspace);
		else
			error = soreserve(so, natm0_sendspace, natm0_recvspace);
		if (error)
			return error;
	}
	npcb = npcb_alloc(true);
	npcb->npcb_socket = so;
	so->so_pcb = npcb;
	return 0;
}

static void
natm_detach(struct socket *so)
{
	struct natmpcb *npcb = (struct natmpcb *)so->so_pcb;

	/*
	 * we turn on 'drain' *before* we sofree.
	 */

	npcb_free(npcb, NPCB_DESTROY);	/* drain */
	so->so_pcb = NULL;
	/* sofree drops the lock */
	sofree(so);
	mutex_enter(softnet_lock);
}

/*
 * user requests
 */

static int
natm_usrreq(struct socket *so, int req, struct mbuf *m, struct mbuf *nam,
    struct mbuf *control, struct lwp *l)
{
  int error = 0, s, s2;
  struct natmpcb *npcb;
  struct sockaddr_natm *snatm;
  struct atm_pseudoioctl api;
  struct atm_pseudohdr *aph;
  struct atm_rawioctl ario;
  struct ifnet *ifp;
  int proto = so->so_proto->pr_protocol;

  KASSERT(req != PRU_ATTACH);
  KASSERT(req != PRU_DETACH);

  s = SPLSOFTNET();

  npcb = (struct natmpcb *) so->so_pcb;

  if (npcb == NULL) {
    error = EINVAL;
    goto done;
  }

  switch (req) {
    case PRU_CONNECT:			/* establish connection to peer */

      /*
       * validate nam and npcb
       */

      if (nam->m_len != sizeof(*snatm)) {
        error = EINVAL;
	break;
      }
      snatm = mtod(nam, struct sockaddr_natm *);
      if (snatm->snatm_len != sizeof(*snatm) ||
		(npcb->npcb_flags & NPCB_FREE) == 0) {
	error = EINVAL;
	break;
      }
      if (snatm->snatm_family != AF_NATM) {
	error = EAFNOSUPPORT;
	break;
      }

      snatm->snatm_if[IFNAMSIZ-1] = '\0';  /* XXX ensure null termination
						since ifunit() uses strcmp */

      /*
       * convert interface string to ifp, validate.
       */

      ifp = ifunit(snatm->snatm_if);
      if (ifp == NULL || (ifp->if_flags & IFF_RUNNING) == 0) {
	error = ENXIO;
	break;
      }
      if (ifp->if_output != atm_output) {
	error = EAFNOSUPPORT;
	break;
      }


      /*
       * register us with the NATM PCB layer
       */

      if (npcb_add(npcb, ifp, snatm->snatm_vci, snatm->snatm_vpi) != npcb) {
        error = EADDRINUSE;
        break;
      }

      /*
       * enable rx
       */

      ATM_PH_FLAGS(&api.aph) = (proto == PROTO_NATMAAL5) ? ATM_PH_AAL5 : 0;
      ATM_PH_VPI(&api.aph) = npcb->npcb_vpi;
      ATM_PH_SETVCI(&api.aph, npcb->npcb_vci);
      api.rxhand = npcb;
      s2 = splnet();
      if (ifp->if_ioctl(ifp, SIOCATMENA, &api) != 0) {
	splx(s2);
	npcb_free(npcb, NPCB_REMOVE);
        error = EIO;
	break;
      }
      splx(s2);

      soisconnected(so);

      break;

    case PRU_DISCONNECT:		/* disconnect from peer */

      if ((npcb->npcb_flags & NPCB_CONNECTED) == 0) {
        printf("natm: disconnected check\n");
        error = EIO;
	break;
      }
      ifp = npcb->npcb_ifp;

      /*
       * disable rx
       */

      ATM_PH_FLAGS(&api.aph) = ATM_PH_AAL5;
      ATM_PH_VPI(&api.aph) = npcb->npcb_vpi;
      ATM_PH_SETVCI(&api.aph, npcb->npcb_vci);
      api.rxhand = npcb;
      s2 = splnet();
      ifp->if_ioctl(ifp, SIOCATMDIS, &api);
      splx(s);

      npcb_free(npcb, NPCB_REMOVE);
      soisdisconnected(so);

      break;

    case PRU_SHUTDOWN:			/* won't send any more data */
      socantsendmore(so);
      break;

    case PRU_SEND:			/* send this data */
      if (control && control->m_len) {
	m_freem(control);
	m_freem(m);
	error = EINVAL;
	break;
      }

      /*
       * send the data.   we must put an atm_pseudohdr on first
       */

      M_PREPEND(m, sizeof(*aph), M_WAITOK);
      if (m == NULL) {
        error = ENOBUFS;
	break;
      }
      aph = mtod(m, struct atm_pseudohdr *);
      ATM_PH_VPI(aph) = npcb->npcb_vpi;
      ATM_PH_SETVCI(aph, npcb->npcb_vci);
      ATM_PH_FLAGS(aph) = (proto == PROTO_NATMAAL5) ? ATM_PH_AAL5 : 0;

      error = atm_output(npcb->npcb_ifp, m, NULL, NULL);

      break;

    case PRU_SENSE:			/* return status into m */
      /* return zero? */
      break;

    case PRU_PEERADDR:			/* fetch peer's address */
      snatm = mtod(nam, struct sockaddr_natm *);
      memset(snatm, 0, sizeof(*snatm));
      nam->m_len = snatm->snatm_len = sizeof(*snatm);
      snatm->snatm_family = AF_NATM;
      memcpy(snatm->snatm_if, npcb->npcb_ifp->if_xname, sizeof(snatm->snatm_if));
      snatm->snatm_vci = npcb->npcb_vci;
      snatm->snatm_vpi = npcb->npcb_vpi;
      break;

    case PRU_CONTROL:			/* control operations on protocol */
      /*
       * raw atm ioctl.   comes in as a SIOCRAWATM.   we convert it to
       * SIOCXRAWATM and pass it to the driver.
       */
      if ((u_long)m == SIOCRAWATM) {
        if (npcb->npcb_ifp == NULL) {
          error = ENOTCONN;
          break;
        }
        ario.npcb = npcb;
        ario.rawvalue = *((int *)nam);
        error = npcb->npcb_ifp->if_ioctl(npcb->npcb_ifp, SIOCXRAWATM, &ario);
	if (!error) {
          if (ario.rawvalue)
	    npcb->npcb_flags |= NPCB_RAW;
	  else
	    npcb->npcb_flags &= ~(NPCB_RAW);
	}

        break;
      }

      error = EOPNOTSUPP;
      break;

    case PRU_BIND:			/* bind socket to address */
    case PRU_LISTEN:			/* listen for connection */
    case PRU_ACCEPT:			/* accept connection from peer */
    case PRU_CONNECT2:			/* connect two sockets */
    case PRU_ABORT:			/* abort (fast DISCONNECT, DETATCH) */
					/* (only happens if LISTEN socket) */
    case PRU_RCVD:			/* have taken data; more room now */
    case PRU_FASTTIMO:			/* 200ms timeout */
    case PRU_SLOWTIMO:			/* 500ms timeout */
    case PRU_RCVOOB:			/* retrieve out of band data */
    case PRU_SENDOOB:			/* send out of band data */
    case PRU_PROTORCV:			/* receive from below */
    case PRU_PROTOSEND:			/* send to below */
    case PRU_SOCKADDR:			/* fetch socket's address */
#ifdef DIAGNOSTIC
      printf("natm: PRU #%d unsupported\n", req);
#endif
      error = EOPNOTSUPP;
      break;

    default: panic("natm usrreq");
  }

done:
  splx(s);
  return(error);
}

/*
 * natmintr: splsoftnet interrupt
 *
 * note: we expect a socket pointer in rcvif rather than an interface
 * pointer.    we can get the interface pointer from the so's PCB if
 * we really need it.
 */

void
natmintr(void)

{
  int s;
  struct mbuf *m;
  struct socket *so;
  struct natmpcb *npcb;

  mutex_enter(softnet_lock);
next:
  s = splnet();
  IF_DEQUEUE(&natmintrq, m);
  splx(s);
  if (m == NULL) {
    mutex_exit(softnet_lock);
    return;
  }

#ifdef DIAGNOSTIC
  if ((m->m_flags & M_PKTHDR) == 0)
    panic("natmintr no HDR");
#endif

  npcb = (struct natmpcb *) m->m_pkthdr.rcvif; /* XXX: overloaded */
  so = npcb->npcb_socket;

  s = splnet();			/* could have atm devs @ different levels */
  npcb->npcb_inq--;
  splx(s);

  if (npcb->npcb_flags & NPCB_DRAIN) {
    m_freem(m);
    if (npcb->npcb_inq == 0)
      kmem_intr_free(npcb, sizeof(*npcb));
    goto next;
  }

  if (npcb->npcb_flags & NPCB_FREE) {
    m_freem(m);					/* drop */
    goto next;
  }

#ifdef NEED_TO_RESTORE_IFP
  m->m_pkthdr.rcvif = npcb->npcb_ifp;
#else
#ifdef DIAGNOSTIC
m->m_pkthdr.rcvif = NULL;	/* null it out to be safe */
#endif
#endif

  if (sbspace(&so->so_rcv) > m->m_pkthdr.len ||
     ((npcb->npcb_flags & NPCB_RAW) != 0 && so->so_rcv.sb_cc < NPCB_RAWCC) ) {
#ifdef NATM_STAT
    natm_sookcnt++;
    natm_sookbytes += m->m_pkthdr.len;
#endif
    sbappendrecord(&so->so_rcv, m);
    sorwakeup(so);
  } else {
#ifdef NATM_STAT
    natm_sodropcnt++;
    natm_sodropbytes += m->m_pkthdr.len;
#endif
    m_freem(m);
  }

  goto next;
}

PR_WRAP_USRREQ(natm_usrreq)

#define	natm_usrreq	natm_usrreq_wrapper

const struct pr_usrreqs natm_usrreqs = {
	.pr_attach	= natm_attach,
	.pr_detach	= natm_detach,
	.pr_generic	= natm_usrreq,
};
