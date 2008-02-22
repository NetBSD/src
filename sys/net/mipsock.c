/*	$NetBSD: mipsock.c,v 1.1.2.1 2008/02/22 02:53:33 keiichi Exp $	*/
/* $Id: mipsock.c,v 1.1.2.1 2008/02/22 02:53:33 keiichi Exp $ */

/*
 * Copyright (C) 2004 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifdef __FreeBSD__
#include "opt_mip6.h"
#endif /* __FreeBSD__ */
#include "mip.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#ifdef __FreeBSD__
#include <sys/malloc.h>
#endif /* __FreeBSD__ */
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/syslog.h>
#include <sys/proc.h>
#if defined(__NetBSD__) && __NetBSD_Version__ >= 400000000
#include <sys/kauth.h>
#endif /* __NetBSD__ && __NetBSD_Version__ >= 400000000 */

#include <net/if.h>
#include <net/mipsock.h>
#ifdef __FreeBSD__
#include <net/if_var.h>
#endif /* __FreeBSD__ */
#include <net/if_mip.h>
#include <net/raw_cb.h>
#include <net/route.h>


#include <netinet/in.h>
#include <netinet/ip6mh.h>
#include <netinet6/in6_var.h>
#include <netinet6/mip6.h>
#include <netinet6/mip6_var.h>

#ifdef __APPLE__
#include <machine/spl.h>
#define thread proc
#endif /* __APPLE__ */

#if defined(__NetBSD__) && __NetBSD_Version__ >= 400000000
DOMAIN_DEFINE(mipdomain);	/* foward declare and add to link set */
#endif /* __NetBSD__ && __NetBSD_Version__ >= 400000000 */

static struct	sockaddr mips_dst = { .sa_len = 2, .sa_family = PF_MOBILITY, };
static struct	sockaddr mips_src = { .sa_len = 2, .sa_family = PF_MOBILITY, };
static struct	sockproto mips_proto = { .sp_family = PF_MOBILITY, };

#if defined(__FreeBSD__) || defined(__APPLE__)
static int mips_abort(struct socket *);
static int mips_attach(struct socket *, int, struct thread *);
static int mips_bind(struct socket *, struct sockaddr *, struct thread *);
static int mips_connect(struct socket *, struct sockaddr *, struct thread *);
static int mips_send(struct socket *, int, struct mbuf *, struct sockaddr *,
    struct mbuf *, struct thread *);
static int mips_detach(struct socket *);
static int mips_disconnect(struct socket *);
static int mips_peeraddr(struct socket *, struct sockaddr **);
static int mips_shutdown(struct socket *);
static int mips_sockaddr(struct socket *, struct sockaddr **);
#endif
#if defined(__FreeBSD__) || defined(__APPLE__)
static int mips_output(struct mbuf *, struct socket *);
#else
static int mips_output(struct mbuf *, ...);
#endif
static struct mbuf *mips_msg1(int type, int len);

#if defined(__FreeBSD__) || defined(__APPLE__)
/*
 * It really doesn't make any sense at all for this code to share much
 * with raw_usrreq.c, since its functionality is so restricted.  XXX
 */
static int
mips_abort(so)
	struct socket *so;
{
	int s, error;
	s = splnet();
	error = raw_usrreqs.pru_abort(so);
	splx(s);
	return error;
}

/* pru_accept is EOPNOTSUPP */

static int
mips_attach(so, proto, p)
	struct socket *so;
	int proto;
	struct thread *p;
{
	struct rawcb *rp;
	int s, error;

	if (sotorawcb(so) != 0)
		return EISCONN;	/* XXX panic? */
	MALLOC(rp, struct rawcb *, sizeof *rp, M_PCB, M_WAITOK|M_ZERO);
	if (rp == 0)
		return ENOBUFS;
	/*
	 * The splnet() is necessary to block protocols from sending
	 * error notifications (like RTM_REDIRECT or RTM_LOSING) while
	 * this PCB is extant but incompletely initialized.
	 * Probably we should try to do more of this work beforehand and
	 * eliminate the spl.
	 */
	s = splnet();
	so->so_pcb = (void *)rp;
	error = raw_attach(so, proto);
	rp = sotorawcb(so);
	if (error) {
		splx(s);
#ifdef __APPLE__
		FREE(rp, M_PCB);
#else
		free(rp, M_PCB);
#endif
		return error;
	}
	switch(rp->rcb_proto.sp_protocol) {
	case AF_INET:
		break;
	case AF_INET6:
		break;
	}
	rp->rcb_faddr = &mips_src;
	soisconnected(so);
	so->so_options |= SO_USELOOPBACK;
	splx(s);
	return 0;
}

static int
mips_bind(so, nam, p)
	struct socket *so;
	struct sockaddr *nam;
	struct thread *p;
{
	int s, error;

	s = splnet();
	error = raw_usrreqs.pru_bind(so, nam, p); /* xxx just EINVAL */
	splx(s);
	return error;
}

static int
mips_connect(so, nam, p)
	struct socket *so;
	struct sockaddr *nam;
	struct thread *p;
{
	int s, error;

	s = splnet();
	error = raw_usrreqs.pru_connect(so, nam, p); /* XXX just EINVAL */
	splx(s);
	return error;
}

/* pru_connect2 is EOPNOTSUPP */
/* pru_control is EOPNOTSUPP */

static int
mips_detach(so)
	struct socket *so;
{
	struct rawcb *rp = sotorawcb(so);
	int s, error;

	s = splnet();
	if (rp != 0) {
		switch(rp->rcb_proto.sp_protocol) {
		case AF_INET:
			break;
		case AF_INET6:
			break;
		}
	}
	error = raw_usrreqs.pru_detach(so);
	splx(s);
	return error;
}

static int
mips_disconnect(so)
	struct socket *so;
{
	int s, error;

	s = splnet();
	error = raw_usrreqs.pru_disconnect(so);
	splx(s);
	return error;
}

/* pru_listen is EOPNOTSUPP */

static int
mips_peeraddr(so, nam)
	struct socket *so;
	struct sockaddr **nam;
{
	int s, error;

	s = splnet();
	error = raw_usrreqs.pru_peeraddr(so, nam);
	splx(s);
	return error;
}

/* pru_rcvd is EOPNOTSUPP */
/* pru_rcvoob is EOPNOTSUPP */

static int
mips_send(so, flags, m, nam, control, p)
	struct socket *so;
	int flags;
	struct mbuf *m;
	struct sockaddr *nam;
	struct mbuf *control;
	struct thread *p;
{
	int s, error;

	s = splnet();
	error = raw_usrreqs.pru_send(so, flags, m, nam, control, p);
	splx(s);
	return error;
}

/* pru_sense is null */

static int
mips_shutdown(so)
	struct socket *so;
{
	int s, error;

	s = splnet();
	error = raw_usrreqs.pru_shutdown(so);
	splx(s);
	return error;
}

static int
mips_sockaddr(so, nam)
	struct socket *so;
	struct sockaddr **nam;
{
	int s, error;

	s = splnet();
	error = raw_usrreqs.pru_sockaddr(so, nam);
	splx(s);
	return error;
}

static struct pr_usrreqs mip_usrreqs = {
	mips_abort, pru_accept_notsupp, mips_attach, mips_bind, mips_connect,
	pru_connect2_notsupp, pru_control_notsupp, mips_detach, mips_disconnect,
	pru_listen_notsupp, mips_peeraddr, pru_rcvd_notsupp, pru_rcvoob_notsupp,
	mips_send, pru_sense_null, mips_shutdown, mips_sockaddr,
	sosend, soreceive, sopoll
};
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__)
/*ARGSUSED*/
int
#ifdef __NetBSD__
mips_usrreq(so, req, m, nam, control, p)
#else
mips_usrreq(so, req, m, nam, control)
#endif
	struct socket *so;
	int req;
	struct mbuf *m;
	struct mbuf *nam;
	struct mbuf *control;
#ifdef __NetBSD__
#if __NetBSD_Version__ >= 400000000
	struct lwp *p;
#else
	struct proc *p;
#endif /* __NetBSD_Version__ >= 400000000 */
#else
#define p curproc
#endif /* __NetBSD__ */
{
	int error = 0;
	struct rawcb *rp = sotorawcb(so);
	int s;

	if (req == PRU_ATTACH) {
		MALLOC(rp, struct rawcb *, sizeof(*rp), M_PCB, M_WAITOK);
		if ((so->so_pcb = rp) != NULL)
			memset(so->so_pcb, 0, sizeof(*rp));

	}
#if 0
	if (req == PRU_DETACH && rp)
		rt_adjustcount(rp->rcb_proto.sp_protocol, -1);
#endif
	s = splsoftnet();

	/*
	 * Don't call raw_usrreq() in the attach case, because
	 * we want to allow non-privileged processes to listen on
	 * and send "safe" commands to the routing socket.
	 */
	if (req == PRU_ATTACH) {
		if (p == 0)
			error = EACCES;
		else
			error = raw_attach(so, (int)(long)nam);
	} else
#ifdef __OpenBSD__
		error = raw_usrreq(so, req, m, nam, control);
#else
		error = raw_usrreq(so, req, m, nam, control, p);
#endif

	rp = sotorawcb(so);
	if (req == PRU_ATTACH && rp) {
		if (error) {
			free((void *)rp, M_PCB);
			splx(s);
			return (error);
		}
		/*		rt_adjustcount(rp->rcb_proto.sp_protocol, 1);*/
		rp->rcb_laddr = &mips_src;
		rp->rcb_faddr = &mips_dst;
		soisconnected(so);
		so->so_options |= SO_USELOOPBACK;
	}
	splx(s);
	return (error);
}
#ifdef __OpenBSD__
#endif
#endif

/*ARGSUSED*/
static int
#if defined(__FreeBSD__) || defined(__APPLE__)
mips_output(m, so)
	register struct mbuf *m;
	struct socket *so;
#else
#if __STDC__
mips_output(struct mbuf *m, ...)
#else
mips_output(m, va_alist)
	struct mbuf *m;
	va_dcl
#endif
#endif
{
	int len, error = 0;
	struct mip_msghdr *miph = NULL;
	struct mipm_bc_info *mipc = NULL;
	struct mipm_nodetype_info *mipmni = NULL;
#if NMIP > 0
	struct mipm_bul_info *mipu = NULL;
	struct mip6_bul_internal *mbul = NULL;
        struct mipm_md_info *mipmd = NULL;
#endif
	struct mipm_dad *mipmdad = NULL;
	struct sockaddr_storage hoa, coa, cnaddr;
	u_int16_t bid = 0;

#define senderr(e) do { error = e; goto flush;} while (/*CONSTCOND*/ 0)
	if (m == 0 || ((m->m_len < sizeof(int32_t)) &&
	    (m = m_pullup(m, sizeof(int32_t))) == 0))
		return (ENOBUFS);
	if ((m->m_flags & M_PKTHDR) == 0)
		panic("mips_output");
	len = m->m_pkthdr.len;
	if (len < sizeof(struct mip_msghdr) ||
	    len != mtod(m, struct mip_msghdr *)->miph_msglen) {
		senderr(EINVAL);
	}
	miph = mtod(m, struct mip_msghdr *);

	/*
	 * Perform permission checking, only privileged sockets
	 * may perform operations other than RTM_GET
	 */
	if (
#if defined(__APPLE__)
		(so->so_state & SS_PRIV == 0) && (error = EPERM)
#elif defined(__FreeBSD__)
		(error = suser(curthread)) != 0
#elif defined(__NetBSD__)
#if __NetBSD_Version__ >= 400000000
		/* XXX is KAUTH_NETWORK_ROUTE correct? */
		(kauth_authorize_network(curlwp->l_cred, KAUTH_NETWORK_ROUTE,
		    0, miph, NULL, NULL) != 0) && (error = EACCES)
#else
		(suser(curproc->p_ucred, &curproc->p_acflag) != 0)
		&& (error = EACCES)
#endif /* __NetBSD_Version >= 400000000 */
#endif         
		) {
		senderr(error);
	}

#ifndef __APPLE__
	miph->miph_pid = curproc->p_pid;
#else
	miph->miph_pid = proc_selfpid();
#endif /* !__APPLE__ */

	switch (miph->miph_type) {
	case MIPM_BC_ADD:
/*	case MIPM_BC_UPDATE:*/
		mipc = (struct mipm_bc_info *)miph;
		bcopy(MIPC_CNADDR(mipc), &cnaddr, MIPC_CNADDR(mipc)->sa_len);
		bcopy(MIPC_HOA(mipc), &hoa, MIPC_HOA(mipc)->sa_len);
		bcopy(MIPC_COA(mipc), &coa, MIPC_COA(mipc)->sa_len);
		switch (hoa.ss_family) {
		case AF_INET6:
#ifdef MOBILE_IPV6_MCOA
			bid = ((struct sockaddr_in6 *)&coa)->sin6_port;
#endif /* MOBILE_IPV6_MCOA */
			error = mip6_bce_update((struct sockaddr_in6 *)&cnaddr,
						(struct sockaddr_in6 *)&hoa,
						(struct sockaddr_in6 *)&coa,
						mipc->mipmci_flags, bid);
			break;
		default:
			error = EPFNOSUPPORT;
			break;
		}
		break;

	case MIPM_BC_REMOVE:
		mipc = (struct mipm_bc_info *)miph;
		bcopy(MIPC_CNADDR(mipc), &cnaddr, MIPC_CNADDR(mipc)->sa_len);
		bcopy(MIPC_HOA(mipc), &hoa, MIPC_HOA(mipc)->sa_len);
		bcopy(MIPC_COA(mipc), &coa, MIPC_COA(mipc)->sa_len);
		switch (hoa.ss_family) {
		case AF_INET6:
#ifdef MOBILE_IPV6_MCOA
			bid = ((struct sockaddr_in6 *)&coa)->sin6_port;
#endif /* MOBILE_IPV6_MCOA */
			error = mip6_bce_remove_addr((struct sockaddr_in6 *)&cnaddr,
						     (struct sockaddr_in6 *)&hoa,
						     (struct sockaddr_in6 *)&coa,
						     mipc->mipmci_flags, bid);
			break;
		default:
			error = EPFNOSUPPORT;
			break;
		}
		break;

	case MIPM_BC_FLUSH:
		mip6_bce_remove_all();
		break;

	case MIPM_NODETYPE_INFO:
		mipmni = (struct mipm_nodetype_info *)miph;
		if (mipmni->mipmni_enable) {
			if (MIP6_IS_MN
			    && (mipmni->mipmni_nodetype
				& MIP6_NODETYPE_HOME_AGENT))
				error = EINVAL;
			if (MIP6_IS_HA
			    && ((mipmni->mipmni_nodetype
				    & MIP6_NODETYPE_MOBILE_NODE)
				|| (mipmni->mipmni_nodetype
				    & MIP6_NODETYPE_MOBILE_ROUTER)))
				error = EINVAL;
			mip6_nodetype |= mipmni->mipmni_nodetype;
		} else {
			if (mipmni->mipmni_nodetype
			    == MIP6_NODETYPE_NONE)
				error = EINVAL;
			mip6_nodetype &= ~mipmni->mipmni_nodetype;
		}
		break;

#if NMIP > 0
	case MIPM_BUL_ADD:
/*	case MIPM_BUL_UPDATE: */
		mipu = (struct mipm_bul_info *)miph;
		/* Non IPv6 address is not supported (only for MIP6) */
		bcopy(MIPU_PEERADDR(mipu), &cnaddr, MIPU_PEERADDR(mipu)->sa_len);
		bcopy(MIPU_HOA(mipu), &hoa, MIPU_HOA(mipu)->sa_len);
		bcopy(MIPU_COA(mipu), &coa, MIPU_COA(mipu)->sa_len);
		switch (hoa.ss_family) {
		case AF_INET6:
			if ((MIPU_PEERADDR(mipu))->sa_family != AF_INET6 ||
			    (MIPU_COA(mipu))->sa_family != AF_INET6) {
				error = EPFNOSUPPORT; /* XXX ? */
				break;
			}
#ifdef MOBILE_IPV6_MCOA
			bid = ((struct sockaddr_in6 *)&coa)->sin6_port;
#endif /* MOBILE_IPV6_MCOA */
			error = mip6_bul_add(&((struct sockaddr_in6 *)&cnaddr)->sin6_addr,
					     &((struct sockaddr_in6 *)&hoa)->sin6_addr,
					     &((struct sockaddr_in6 *)&coa)->sin6_addr,
					     mipu->mipmui_hoa_ifindex,
					     mipu->mipmui_flags,
					     mipu->mipmui_state, bid);
			break;
		default:
			error = EPFNOSUPPORT;
			break;
		}
		break;

	case MIPM_BUL_REMOVE:
		mipu = (struct mipm_bul_info *)miph;
		bcopy(MIPU_PEERADDR(mipu), &cnaddr, MIPU_PEERADDR(mipu)->sa_len);
		bcopy(MIPU_HOA(mipu), &hoa, MIPU_HOA(mipu)->sa_len);
#ifdef MOBILE_IPV6_MCOA
		bcopy(&((struct sockaddr_in6 *)MIPU_COA(mipu))->sin6_port,
		      &bid, sizeof(bid));
#endif /* MOBILE_IPV6_MCOA */
		mbul = mip6_bul_get(&((struct sockaddr_in6 *)&hoa)->sin6_addr, &((struct sockaddr_in6 *)&cnaddr)->sin6_addr, bid);
		if (mbul == NULL) 
			return (ENOENT);

		mip6_bul_remove(mbul);
		break;

	case MIPM_BUL_FLUSH:
		mip6_bul_remove_all();
		break;

	case MIPM_HOME_HINT:
	case MIPM_MD_INFO:
                mipmd = (struct mipm_md_info *)miph;
                if (mipmd->mipmmi_command == MIPM_MD_SCAN) {
                        mip6_md_scan(mipmd->mipmmi_ifindex);
                } 
                break;
#endif /* NMIP > 0 */

		/* MIPM_DAD_DO/STOP are issued from userland */
		/* MIPM_DAD_SUCCESS or MIPM_DAD_FAIL is returned as a result of the DAD */
	case MIPM_DAD:
		mipmdad = (struct mipm_dad *)miph;
		if (mipmdad->mipmdad_message == MIPM_DAD_DO) {
			mip6_do_dad(&mipmdad->mipmdad_addr6, mipmdad->mipmdad_ifindex);
		} else if (mipmdad->mipmdad_message == MIPM_DAD_STOP) {
			mip6_stop_dad(&mipmdad->mipmdad_addr6, mipmdad->mipmdad_ifindex);
		} else if (mipmdad->mipmdad_message == MIPM_DAD_LINKLOCAL) {
			mip6_do_dad_lladdr(mipmdad->mipmdad_ifindex);
		}
		break;

	default:
		return (0);
	}

 flush:
	if (miph) {
		if (error)
			miph->miph_errno = error;
/*
		else
			miph->miph_flags |= RTF_DONE;
*/
	}

#ifdef __APPLE__
	socket_unlock(so, 0);
#endif /* __APPLE__ */
	raw_input(m, &mips_proto, &mips_src, &mips_dst);
#ifdef __APPLE__
	socket_lock(so, 0);
#endif /* __APPLE__ */
	return (error);
}

static struct mbuf *
mips_msg1(type, len)
	int type;
	int len;
{
	register struct mip_msghdr *miph;
	register struct mbuf *m;

	if (len > MCLBYTES)
		panic("mips_msg1");
	m = m_gethdr(M_DONTWAIT, MT_DATA);
	if (m && (size_t)len > MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_free(m);
			m = NULL;
		}
	}
	if (m == NULL)
		return (m);
	m->m_pkthdr.len = m->m_len = len;
	m->m_pkthdr.rcvif = 0;
	miph = mtod(m, struct mip_msghdr *);
	bzero((void *)miph, len);
	if (m->m_pkthdr.len != len) {
		m_freem(m);
		return (NULL);
	}
	miph->miph_msglen = len;
	miph->miph_version = MIP_VERSION;
	miph->miph_type = type;
	return (m);
}


void
mips_notify_home_hint(ifindex, prefix, prefixlen) 
	u_int16_t ifindex;
	struct sockaddr *prefix;
	u_int16_t prefixlen;
{
	struct mipm_home_hint *hint;
	struct mbuf *m;
	int len = sizeof(struct mipm_home_hint) + prefix->sa_len;

	m = mips_msg1(MIPM_HOME_HINT, len);
	if (m == NULL)
		return;

        hint = mtod(m, struct mipm_home_hint *);

	hint->mipmhh_seq = 0;
	hint->mipmhh_ifindex = ifindex;
	hint->mipmhh_prefixlen = prefixlen;
	bcopy(prefix, (char *) hint->mipmhh_prefix, prefix->sa_len);

	raw_input(m, &mips_proto, &mips_src, &mips_dst);
}

/*
 * notify bi-directional tunneling event so that a moblie node can
 * initiate RR procedure.
 */
void
mips_notify_rr_hint(hoa, peeraddr)
	struct sockaddr *hoa;
	struct sockaddr *peeraddr;
{
	struct mipm_rr_hint *rr_hint;
	struct mbuf *m;
	u_short len = sizeof(struct mipm_rr_hint)
	    + hoa->sa_len + peeraddr->sa_len;

	m = mips_msg1(MIPM_RR_HINT, len);
	if (m == NULL)
		return;
	rr_hint = mtod(m, struct mipm_rr_hint *);
	rr_hint->mipmrh_seq = 0;

	bcopy(hoa, (void *)MIPMRH_HOA(rr_hint), hoa->sa_len);
	bcopy(peeraddr, (void *)MIPMRH_PEERADDR(rr_hint), peeraddr->sa_len);

	raw_input(m, &mips_proto, &mips_src, &mips_dst);
}

/*
 * notify a hint to send a binding error message.  this message is
 * usually sent when an invalid home address is received.
 */
void
mips_notify_be_hint(src, coa, hoa, status)
	struct sockaddr *src;
	struct sockaddr *coa;
	struct sockaddr *hoa;
	u_int8_t status;
{
	struct mipm_be_hint *be_hint;
	struct mbuf *m;
	u_short len = sizeof(struct mipm_be_hint)
	    + src->sa_len + coa->sa_len + hoa->sa_len;

	m = mips_msg1(MIPM_BE_HINT, len);
	if (m == NULL)
		return;
	be_hint = mtod(m, struct mipm_be_hint *);
	be_hint->mipmbeh_seq = 0;

	be_hint->mipmbeh_status = status;

	bcopy(src, (void *)MIPMBEH_PEERADDR(be_hint), src->sa_len);
	bcopy(coa, (void *)MIPMBEH_COA(be_hint), coa->sa_len);
	bcopy(hoa, (void *)MIPMBEH_HOA(be_hint), hoa->sa_len);

	raw_input(m, &mips_proto, &mips_src, &mips_dst);
}

void
mips_notify_dad_result(message, addr, ifindex)
	int message;
	struct in6_addr *addr;
	int ifindex;
{
	struct mipm_dad *mipmdad;
	struct mbuf *m;
	
	m = mips_msg1(MIPM_DAD, sizeof(struct mipm_dad));
	if (m == NULL)
		return;
	mipmdad = mtod(m, struct mipm_dad *);
	mipmdad->mipmdad_message = message;
	mipmdad->mipmdad_ifindex = ifindex;
	bcopy(addr, &mipmdad->mipmdad_addr6, sizeof(struct in6_addr));

	raw_input(m, &mips_proto, &mips_src, &mips_dst);
}

/*
 * Definitions of protocols supported in the MOBILITY domain.
 */

extern struct domain mipdomain;		/* or at least forward */

static struct protosw mipsw[] = {
{ SOCK_RAW,	&mipdomain,	0,		PR_ATOMIC|PR_ADDR,
  0,		mips_output,	raw_ctlinput,	0,
#if defined(__FreeBSD__) || defined(__APPLE__)
  0,
#else
  mips_usrreq,
#endif
  raw_init,	0,		0,		0,
#ifdef __APPLE__
  0,
#endif
#if defined(__NetBSD__)
#elif !defined(__FreeBSD__) && !defined(__APPLE__)
  0/*sysctl_rtable*/
#else
  &mip_usrreqs
#endif
#ifdef __APPLE__
  ,0,			0,		0,
  { 0, 0 }, 	0,	{ 0 }
#endif
}
};

struct domain mipdomain = {
	.dom_family = PF_MOBILITY,
	.dom_name = "mip",
	.dom_protosw = mipsw,
#ifdef __APPLE__
	0, 0, 0, 0, 0, 0, 0, 0,
	{0, 0}
#else
	.dom_protoswNPROTOSW = &mipsw[sizeof(mipsw)/sizeof(mipsw[0])],
#endif /* __APPLE__ */
};

#if defined(__FreeBSD__) || defined(__APPLE__)
DOMAIN_SET(mip);
#endif
