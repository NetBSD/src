/*	$NetBSD: natm.c,v 1.42 2014/08/05 05:24:27 rtr Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: natm.c,v 1.42 2014/08/05 05:24:27 rtr Exp $");

#include <sys/param.h>
#include <sys/kmem.h>
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
	int error = 0;
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
	return error;
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

static int
natm_accept(struct socket *so, struct mbuf *nam)
{
	KASSERT(solocked(so));

	return EOPNOTSUPP;
}

static int
natm_bind(struct socket *so, struct mbuf *nam, struct lwp *l)
{
	KASSERT(solocked(so));

	return EOPNOTSUPP;
}

static int
natm_listen(struct socket *so, struct lwp *l)
{
	KASSERT(solocked(so));

	return EOPNOTSUPP;
}

static int
natm_connect(struct socket *so, struct mbuf *nam, struct lwp *l)
{
	int error = 0, s2;
	struct natmpcb *npcb;
	struct sockaddr_natm *snatm;
	struct atm_pseudoioctl api;
	struct atm_pseudohdr *aph;
	struct ifnet *ifp;
	int proto = so->so_proto->pr_protocol;

	KASSERT(solocked(so));

	/*
	 * validate nam and npcb
	 */

	if (nam->m_len != sizeof(*snatm))
		return EINVAL;
	snatm = mtod(nam, struct sockaddr_natm *);
	if (snatm->snatm_len != sizeof(*snatm) ||
	    (npcb->npcb_flags & NPCB_FREE) == 0)
		return EINVAL;
	if (snatm->snatm_family != AF_NATM)
		return EAFNOSUPPORT;

	snatm->snatm_if[IFNAMSIZ-1] = '\0';  /* XXX ensure null termination
						since ifunit() uses strcmp */

	/*
	 * convert interface string to ifp, validate.
	 */

	ifp = ifunit(snatm->snatm_if);
	if (ifp == NULL || (ifp->if_flags & IFF_RUNNING) == 0) {
		return ENXIO;
	}
	if (ifp->if_output != atm_output) {
		return EAFNOSUPPORT;
	}

	/*
	 * register us with the NATM PCB layer
	 */

	if (npcb_add(npcb, ifp, snatm->snatm_vci, snatm->snatm_vpi) != npcb)
		return EADDRINUSE;

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
		return EIO;
	}
	splx(s2);

	soisconnected(so);
	return error;
}

static int
natm_disconnect(struct socket *so)
{
	struct natmpcb *npcb = (struct natmpcb *)so->so_pcb;

	KASSERT(solocked(so));
	KASSERT(npcb != NULL);

	if ((npcb->npcb_flags & NPCB_CONNECTED) == 0) {
		printf("natm: disconnected check\n");
		return EIO;
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
	return 0;
}

static int
natm_shutdown(struct socket *so)
{
	int s;

	KASSERT(solocked(so));

	s = splsoftnet();
	socantsendmore(so);
	splx(s);

	return 0;
}

static int
natm_abort(struct socket *so)
{
	KASSERT(solocked(so));

	return EOPNOTSUPP;
}

static int
natm_ioctl(struct socket *so, u_long cmd, void *nam, struct ifnet *ifp)
{
  int error = 0, s;
  struct natmpcb *npcb;
  struct atm_rawioctl ario;

  s = SPLSOFTNET();

  npcb = (struct natmpcb *) so->so_pcb;

  /*
   * raw atm ioctl.   comes in as a SIOCRAWATM.   we convert it to
   * SIOCXRAWATM and pass it to the driver.
   */
  if (cmd == SIOCRAWATM) {
    if (npcb->npcb_ifp == NULL) {
      error = ENOTCONN;
      goto done;
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

    goto done;
  }

  error = EOPNOTSUPP;

done:
  splx(s);
  return(error);
}

static int
natm_stat(struct socket *so, struct stat *ub)
{
  KASSERT(solocked(so));

  return 0;
}

static int
natm_peeraddr(struct socket *so, struct mbuf *nam)
{
  struct natmpcb *npcb = (struct natmpcb *) so->so_pcb;
  struct sockaddr_natm *snatm;

  KASSERT(solocked(so));
  KASSERT(pcb != NULL);
  KASSERT(nam != NULL);

  snatm = mtod(nam, struct sockaddr_natm *);
  memset(snatm, 0, sizeof(*snatm));
  nam->m_len = snatm->snatm_len = sizeof(*snatm);
  snatm->snatm_family = AF_NATM;
  memcpy(snatm->snatm_if, npcb->npcb_ifp->if_xname, sizeof(snatm->snatm_if));
  snatm->snatm_vci = npcb->npcb_vci;
  snatm->snatm_vpi = npcb->npcb_vpi;
  return 0;
}

static int
natm_sockaddr(struct socket *so, struct mbuf *nam)
{
  KASSERT(solocked(so));

  return EOPNOTSUPP;
}

static int
natm_recvoob(struct socket *so, struct mbuf *m, int flags)
{
  KASSERT(solocked(so));

  return EOPNOTSUPP;
}

static int
natm_sendoob(struct socket *so, struct mbuf *m, struct mbuf *control)
{
  KASSERT(solocked(so));

  return EOPNOTSUPP;
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
  struct ifnet *ifp;
  int proto = so->so_proto->pr_protocol;

  KASSERT(req != PRU_ATTACH);
  KASSERT(req != PRU_DETACH);
  KASSERT(req != PRU_ACCEPT);
  KASSERT(req != PRU_BIND);
  KASSERT(req != PRU_LISTEN);
  KASSERT(req != PRU_CONNECT);
  KASSERT(req != PRU_DISCONNECT);
  KASSERT(req != PRU_SHUTDOWN);
  KASSERT(req != PRU_ABORT);
  KASSERT(req != PRU_CONTROL);
  KASSERT(req != PRU_SENSE);
  KASSERT(req != PRU_PEERADDR);
  KASSERT(req != PRU_SOCKADDR);
  KASSERT(req != PRU_RCVOOB);
  KASSERT(req != PRU_SENDOOB);

  s = SPLSOFTNET();

  npcb = (struct natmpcb *) so->so_pcb;

  if (npcb == NULL) {
    error = EINVAL;
    goto done;
  }

  switch (req) {
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

    case PRU_CONNECT2:			/* connect two sockets */
    case PRU_RCVD:			/* have taken data; more room now */
    case PRU_FASTTIMO:			/* 200ms timeout */
    case PRU_SLOWTIMO:			/* 500ms timeout */
    case PRU_PROTORCV:			/* receive from below */
    case PRU_PROTOSEND:			/* send to below */
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

PR_WRAP_USRREQS(natm)
#define	natm_attach	natm_attach_wrapper
#define	natm_detach	natm_detach_wrapper
#define	natm_accept	natm_accept_wrapper
#define	natm_bind	natm_bind_wrapper
#define	natm_listen	natm_listen_wrapper
#define	natm_connect	natm_connect_wrapper
#define	natm_disconnect	natm_disconnect_wrapper
#define	natm_shutdown	natm_shutdown_wrapper
#define	natm_abort	natm_abort_wrapper
#define	natm_ioctl	natm_ioctl_wrapper
#define	natm_stat	natm_stat_wrapper
#define	natm_peeraddr	natm_peeraddr_wrapper
#define	natm_sockaddr	natm_sockaddr_wrapper
#define	natm_recvoob	natm_recvoob_wrapper
#define	natm_sendoob	natm_sendoob_wrapper
#define	natm_usrreq	natm_usrreq_wrapper

const struct pr_usrreqs natm_usrreqs = {
	.pr_attach	= natm_attach,
	.pr_detach	= natm_detach,
	.pr_accept	= natm_accept,
	.pr_bind	= natm_bind,
	.pr_listen	= natm_listen,
	.pr_connect	= natm_connect,
	.pr_disconnect	= natm_disconnect,
	.pr_shutdown	= natm_shutdown,
	.pr_abort	= natm_abort,
	.pr_ioctl	= natm_ioctl,
	.pr_stat	= natm_stat,
	.pr_peeraddr	= natm_peeraddr,
	.pr_sockaddr	= natm_sockaddr,
	.pr_recvoob	= natm_recvoob,
	.pr_sendoob	= natm_sendoob,
	.pr_generic	= natm_usrreq,
};
