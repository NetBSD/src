/*	$NetBSD: hpux.c,v 1.1.1.1 2004/03/28 08:55:25 martti Exp $	*/

/*
 * Copyright (C) 1993-1998 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 */
/* #pragma ident   "@(#)solaris.c	1.12 6/5/96 (C) 1995 Darren Reed"*/

/*typedef unsigned int spustate_t;*/
struct uio;

#include <sys/types.h>
#include <sys/cmn_err.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/callout.h>
#include <sys/moddefs.h>
#include <sys/spinlock.h> /* HNT */
#include <sys/io.h>
#include <sys/wsio.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/byteorder.h>
#include <sys/socket.h>
#include <sys/stropts.h>
#include <net/if.h>
#include <net/af.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/tcpip.h>
#include <netinet/ip_icmp.h>

#include "ip_compat.h"
#include "ip_fil.h"
#include "ip_nat.h"
#include "ipl.h"

#undef	untimeout
#undef	IPFDEBUG

extern	struct	filterstats	frstats[];
extern	ipfrwlock_t	ipf_mutex, ipf_nat, ipf_global;
extern	ipfmutex_t	ipf_rw, ipf_stinsert;
extern	int	fr_running;
extern	int	fr_flags;
extern	int	fr_check __P(());

extern ipnat_t *nat_list;

static	char	*ipf_devfiles[] = { IPL_NAME, IPNAT_NAME, IPSTATE_NAME,
				    IPAUTH_NAME, IPSYNC_NAME, IPLOOKUP_NAME,
				    NULL };
static	int	(*ipf_ip_inp) __P((queue_t *, mblk_t *)) = NULL;


static	int	fr_slowtimer __P((void));
struct	callout	*fr_timer_id = NULL;
struct	callout	*synctimeoutid = NULL;
#ifdef	IPFDEBUG
void	printiri __P((irinfo_t *));
#endif /* IPFDEBUG */

static	int	ipf_install(void);	/* Install Routine - Biswajit */
static	int	ipf_load(void *);
static	int	ipf_unload(void *);
static	int	ipf_write(dev_t, struct uio *);

static	int	ipf_attach(void);
static	int	ipf_detach(void);
static	int	fr_qifsync(ip_t *, int, void *, int, qif_t *, mblk_t **);


/*
 * Local functions for static linking - Biswajit
 */
static	void	ipf_linked_init(void);
static	int	(*ipf_saved_init)();	/* Used in ipf_install() - Biswajit */

/*
 * While linking  statically,  PFIL needs to be installed
 * before IPF is installed.
 */
int	pfil_install (void);

/*
 * Driver Header
 */
static drv_info_t ipf_drv_info = {
	IPL_VERSION,					/* type */
	"pseudo",					/* class */
	DRV_CHAR|DRV_PSEUDO|DRV_SAVE_CONF|DRV_MP_SAFE,	/* flags */
	-1,						/* b_major */
	-1,						/* c_major */
	NULL,						/* cdio */
	NULL,						/* gio_private */
	NULL,						/* cdio_private */
};


static drv_ops_t ipf_drv_ops = {
	iplopen,
	iplclose,
	NULL,		/* strategy */
	NULL,		/* dump */
	NULL,		/* psize */
	NULL,		/* mount */
	iplread,
	ipf_write,	/* write */
	iplioctl,	/* ioctl */
#ifdef	IPL_SELECT
	iplselect,	/* select */
#else
	NULL,		/* select */
#endif
	NULL,		/* option1 */
	NULL,		/* reserved1 */
	NULL,		/* reserved2 */
	NULL,		/* reserved3 */
	NULL,		/* reserved4 */
	0,		/* flags */
};


static wsio_drv_data_t ipf_wsio_data = {
	"pseudo_ipf",	/* drv_path */
	T_DEVICE,	/* drv_type */
	DRV_CONVERGED,	/* drv_flags */
	NULL,		/* drv_minor_build - field not used */
	NULL,		/* drv_major_build - field not used */
};

extern	struct	mod_operations	gio_mod_ops;
static	drv_info_t	ipf_drv_info;
extern	struct	mod_conf_data	ipf_conf_data;

static struct mod_type_data	ipf_drv_link = {
	IPL_VERSION, (void *)NULL
};

static	struct	modlink	ipf_mod_link[] = {
	{ &gio_mod_ops, (void *)&ipf_drv_link },
	{ NULL, (void *)NULL }
};

struct	modwrapper	ipf_wrapper = {
	MODREV,
	ipf_load,
	ipf_unload,
	(void (*)())NULL,
	(void *)&ipf_conf_data,
	ipf_mod_link
};

static wsio_drv_info_t ipf_wsio_info = {
	&ipf_drv_info,
	&ipf_drv_ops,
	&ipf_wsio_data
};


static int ipf_load(void *arg)
{
	int ipfinst;

#ifdef	IPFDEBUG
	printf("IP Filter: ipf_load(%lx)\n", arg);
#endif
	/* Use drv_info passed to us instead of static version */
	ipf_wsio_info.drv_info = (drv_info_t *)arg;

	ipfinst = wsio_install_driver(&ipf_wsio_info);
	if (ipfinst == 0) {
		cmn_err(CE_CONT,
			"IP Filter: wsio_install_driver(%lx) failed: %d\n",
			&ipf_wsio_info, ipfinst);
		return ENXIO;
	} else {
		ipfinst = ipf_attach();
#ifdef	IPFDEBUG
		printf("IP Filter: ipf_attach() = %d\n", ipfinst);
#endif
	}
#ifdef	IPFDEBUG
	printf("IP Filter: ipf_load() = %d\n", ipfinst);
#endif
	return ipfinst;
}


static int ipf_unload(void *arg)
{
	int ipfinst;

	ipfinst = wsio_uninstall_driver(&ipf_wsio_info);
	if (ipfinst != 0) {
		cmn_err(CE_CONT,
			"IP Filter: wsio_uninstall_driver(%lx) failed: %d\n",
			&ipf_wsio_info, ipfinst);
		return ENXIO;
	} else {
		ipfinst = ipf_detach();
#ifdef	IPFDEBUG
		printf("IP Filter: ipf_detach() = %d\n", ipfinst);
#endif
	}
#ifdef	IPFDEBUG
	printf("IP Filter: ipf_unload() = %d\n", ipfinst);

	/* HP Port: We don't want this to come to the cosole on boot up */
	cmn_err(CE_CONT, "IP Filter: Unloaded\n");
#endif
	return 0;
}


/*
 * INSTALL
 * Following routine is called when the dlkm module is
 * being linked statically with the kernel
 * - Biswajit
 */

static int ipf_install(void)
{
	extern int   (*dev_init) (void);
	int   ipfinst;

	/*
	 * Make sure PFIL module is installed before
	 * installing IPF
	 */

	if (pfilinterface != PFIL_INTERFACE) {
		printf("pfilinterface(%d) != %d\n", pfilinterface,
			PFIL_INTERFACE);
		return EINVAL;
	}

	/*
	 * Link my init function into the chain
	 */
	ipf_saved_init = dev_init;
	dev_init = (int (*) ()) &ipf_linked_init;

	/*
	 * Register driver with WSIO
	 */
	ipfinst = wsio_install_driver(&ipf_wsio_info);
	return (ipfinst);
}

/*
 * Device Initialization Link
 * Called only for statically linked drivers to link
 *   init routine into list
 * - Biswajit
 */
static void ipf_linked_init (void)
{
	/*
	 * Perform driver specific initialization
	 */
	(void) ipf_attach ();

	/*
	 * Call next init function in chain
	 */
	(void) (*ipf_saved_init) ();
}


static int ipf_write(dev_t dev, struct uio *uio)
{
#ifdef	IPFDEBUG
	printf("IP Filter: ipf_write(%x,%lx)\n", dev, uio);
#endif
	return 0;
}


static int ipf_attach()
{
	char *defpass;

	sync();
	/*
	 * Initialize mutex's
	 */
	if (iplattach() == -1)
		return -1;
	/*
	 * Lock people out while we set things up.
	 */
	WRITE_ENTER(&ipf_global);

	if (pfil_add_hook(fr_check, PFIL_IN|PFIL_OUT, &pfh_inet4))
		cmn_err(CE_WARN, "IP Filter: pfil_add_hook(pfh_inet4) failed");
	if (pfil_add_hook(fr_qifsync, PFIL_IN|PFIL_OUT, &pfh_sync))
		cmn_err(CE_WARN, "IP Filter: pfil_add_hook(pfh_sync) failed");
	/*
	 * Release lock
	 */
	RWLOCK_EXIT(&ipf_global);

	if (FR_ISPASS(fr_pass))
		defpass = "pass";
	else if (FR_ISBLOCK(fr_pass))
		defpass = "block";
	else
		defpass = "no-match -> block";

#ifdef	IPFDEBUG
	/* HP Port: We don't want this to come to the cosole on boot up */
	cmn_err(CE_CONT, "%s initialized.  Default = %s all, Logging = %s\n",
		ipfilter_version, defpass,
# ifdef IPFILTER_LOG
		"enabled");
# else
		"disabled");
# endif
#endif
	sync();
	if (fr_running == 0)
		fr_running = 1;
	if (fr_timer_id == NULL)
		fr_timer_id = mp_timeout(fr_slowtimer, NULL, hz/2);
	if (fr_running == 1)
		return 0;
	return -1;
}


static int ipf_detach()
{
	int i;

	if (fr_refcnt)
		return EBUSY;

	/*
	 * Make sure we're the only one's modifying things.  With
	 * this lock others should just fall out of the loop.
	 */
	MUTEX_ENTER(&ipf_rw);
	if (fr_running <= 0) {
		MUTEX_EXIT(&ipf_rw);
		return -1;
	}
	if (fr_timer_id) {
		untimeout(fr_slowtimer, NULL);
		fr_timer_id = 0;
	}
	fr_running = -1;
	MUTEX_EXIT(&ipf_rw);

	/*
	 * Undo what we did in ipf_attach, freeing resources
	 * and removing things we installed.  The system
	 * framework guarantees we are not active with this devinfo
	 * node in any other entry points at this time.
	 */
	sync();

	if (pfil_remove_hook(fr_check, PFIL_IN|PFIL_OUT, &pfh_inet4))
		cmn_err(CE_WARN,
			"IP Filter: pfil_remove_hook(pfh_inet4) failed");
	if (pfil_remove_hook(fr_qifsync, PFIL_IN|PFIL_OUT, &pfh_sync))
		cmn_err(CE_WARN,
			"IP Filter: pfil_remove_hook(pfh_sync) failed");
	while (fr_timer_id != NULL)
		sched_yield();
	i = ipldetach();
#ifdef	IPFDEBUG
	printf("IP Filter: ipldetach() = %d\n", i);
#endif
	return i;
}


#if 0
/*
 * OK, this is pretty scrappy code, but then it's essentially just here for
 * debug purposes and that's it.  Packets should not normally come through
 * here, and if they do, well, we would like to see as much information as
 * possible about them and what they claim to hold.
 */
void fr_donotip(out, qif, q, m, mt, ip, off)
int out;
qif_t *qif;
queue_t *q;
mblk_t *m, *mt;
ip_t *ip;
size_t off;
{
	u_char *s, outb[256], *t;
	int i, j;

	outb[0] = '\0';
	outb[1] = '\0';
	outb[2] = '\0';
	outb[3] = '\0';
	s = ip ? (u_char *)ip : outb;
	if (!ip && (m == mt) && m->b_cont && (MTYPE(m) != M_DATA))
		m = m->b_cont;

	printf(" !IP %s:%d %d %p %p %p %d %p/%d %p/%d %p %d %d %p\n",
		qif ? qif->qf_name : "?", out, qif->qf_hl, q,
		q ? q->q_ptr : NULL, q ? q->q_qinfo : NULL,
		mt->b_wptr - mt->b_rptr, m, MTYPE(m), mt, MTYPE(mt), m->b_rptr,
		m->b_wptr - m->b_rptr, off, ip);
	printf("%02x%02x%02x%02x\n", *s, *(s+1), *(s+2), *(s+3));
	while (m != mt) {
		i = 0;
		j = 256;
		t = outb;
		s = mt->b_rptr;
		sprintf((char *)t, j, "%d:", MTYPE(mt));
		t += strlen((char *)t);
		j -= strlen((char *)t);
		for (; (j > 0) && (i < 100) && (s < mt->b_wptr); i++) {
			sprintf((char *)t, j, "%02x%s", *s++,
				((i & 3) == 3) ? " " : "");
			t += ((i & 3) == 3) ? 3 : 2;
			j -= ((i & 3) == 3) ? 3 : 2;
		}
		*t++ = '\n';
		*t = '\0';
		printf("%s", outb);
		mt = mt->b_cont;
	}
	i = 0;
	j = 256;
	t = outb;
	s = m->b_rptr;
	sprintf((char *)t, j, "%d:", MTYPE(m));
	t += strlen((char *)t);
	j -= strlen((char *)t);
	for (; (j > 0) && (i < 100) && (s < m->b_wptr); i++) {
		sprintf((char *)t, j, "%02x%s", *s++,
			((i & 3) == 3) ? " " : "");
		t += ((i & 3) == 3) ? 3 : 2;
		j -= ((i & 3) == 3) ? 3 : 2;
	}
	*t++ = '\n';
	*t = '\0';
	printf("%s", outb);
}
#endif


/*
 * look for bad consistancies between the list of interfaces the filter knows
 * about and those which are currently configured.
 */
static int fr_qifsync(ip, hlen, il, out, qif, mp)
ip_t *ip;
int hlen;
void *il;
int out;
qif_t *qif;
mblk_t **mp;
{
	register struct frentry *f;
	register ipnat_t *np;

	frsync();
	/*
	 * Resync. any NAT `connections' using this interface and its IP #.
	 */
	fr_natsync(qif);
	fr_statesync(qif);
	return 0;
}


#ifdef	IPFDEBUG
void printiri(iri)
irinfo_t *iri;
{
	printf("iri: ll_hdr_mp %p rfq %p stq %p\n",
		iri->ir_ll_hdr_mp, iri->ir_rfq, iri->ir_stq);
	printf("iri: ll_hdr_length %d ir_ill %p\n",
		iri->ir_ll_hdr_length, iri->ir_ill);
}
#endif


int fr_fastroute(mb, mpp, fin, fdp)
mblk_t *mb, **mpp;
fr_info_t *fin;
frdest_t *fdp;
{
#ifdef	USE_INET6
	ip6_t *ip6 = (ip6_t *)fin->fin_ip;
#endif
	struct in_addr dst, src;
	ifinfo_t *ifp, *sifp;
	mblk_t *mp, **mps;
	size_t hlen = 0;
	qpktinfo_t *qpi;
	frentry_t *fr;
	irinfo_t ir;
	queue_t *q;
	u_char *s;
	ip_t *ip;
	int p, i;

	ip = fin->fin_ip;
	qpi = fin->fin_qpi;
	/*
	 * If this is a duplicate mblk then we want ip to point at that
	 * data, not the original, if and only if it is already pointing at
	 * the current mblk data.
	 */
	if (ip == (ip_t *)qpi->qpi_m->b_rptr && qpi->qpi_m != mb)
		ip = (ip_t *)mb->b_rptr;

	/*
	 * If there is another M_PROTO, we don't want it
	 */
	if (*mpp != mb) {
		mp = *mpp;
		(void) unlinkb(mp);
		mp = (*mpp)->b_cont;
		(*mpp)->b_cont = NULL;
		(*mpp)->b_prev = NULL;
		freemsg(*mpp);
		*mpp = mp;
	}

	ifp = (ifinfo_t *)fdp->fd_ifp;
	if (fdp && fdp->fd_ip.s_addr)
		dst = fdp->fd_ip;
	else
		dst.s_addr = fin->fin_fi.fi_daddr;

	src.s_addr = 0;
	bzero((char *)&ir, sizeof(ir));
	i = ir_lookup(&ir, &dst.s_addr, &src.s_addr,
		      ~(IR_ROUTE|IR_ROUTE_ASSOC|IR_ROUTE_REDIRECT), 4);
	mp = ir.ir_ll_hdr_mp;
	hlen = ir.ir_ll_hdr_length;
	if (!mp || !hlen || (i == 0))
		goto bad_fastroute;

	fr = fin->fin_fr;
	if ((ifp || (fr && (fr->fr_flags & FR_FASTROUTE)))) {
		if (ifp && (ir_to_ill(&ir) != ifp))
			goto bad_fastroute;
		/*
		 * In case we're here due to "to <if>" being used with
		 * "keep state", check that we're going in the correct
		 * direction.
		 */
		if ((fr != NULL) && (ifp != NULL) &&
		    (fin->fin_rev != 0) && (fdp == &fr->fr_tif))
			goto bad_fastroute;

		sifp = fin->fin_ifp;
		fin->fin_ifp = ir_to_ill(&ir);
		if (fin->fin_out == 0) {
			u_32_t pass;

			fin->fin_fr = ipacct[1][fr_active];
			if ((fin->fin_fr != NULL) &&
			    (fr_scanlist(fin, FR_NOMATCH) & FR_ACCOUNT)) {
				ATOMIC_INCL(frstats[1].fr_acct);
			}
			fin->fin_fr = NULL;
			if (!fr || !(fr->fr_flags & FR_RETMASK)) {

				(void) fr_checkstate(fin, &pass);
			}
			(void) fr_checknatout(fin, &pass);
		}
		fin->fin_ifp = sifp;

		s = mb->b_rptr;
		if ((hlen && (s - mb->b_datap->db_base) >= hlen)) {
			s -= hlen;
			mb->b_rptr = (u_char *)s;
			bcopy((char *)mp->b_rptr, (char *)s, hlen);
			freeb(mp);
			mp = NULL;
		} else {
			mblk_t	*mp2;

			linkb(mp, *mpp);
			*mpp = mp;
			mb = mp;
			mp = NULL;
		}

		q = NULL;
		if (ir.ir_stq)
			q = ir.ir_stq;
		else if (ir.ir_rfq)
			q = WR(ir.ir_rfq);
		if (q)
			q = q->q_next;
		if (q) {
			mb->b_prev = NULL;
			RWLOCK_EXIT(&ipf_global);
			putnext(q, mb);
			READ_ENTER(&ipf_global);
			fr_frouteok[0]++;
			return 0;
		}
	}
bad_fastroute:
	if (mp)
		freeb(mp);
	mb->b_prev = NULL;
	freemsg(*mpp);
	*mpp = NULL;
	fr_frouteok[1]++;
	return -1;
}


int fr_verifysrc(fin)
fr_info_t *fin;
{
	struct in_addr ips, ipa;
	irinfo_t ir, *dir, *gw;
	qif_t *qif;
	int i;

	ips.s_addr = 0;
	ipa = fin->fin_src;

	if (ir_lookup(&ir, (uint32_t *)&ipa, (uint32_t *)&ips, 0, 4) == 0)
		return 1;
	i = (ir_to_ill(&ir) == fin->fin_ifp);
	if (ir.ir_ll_hdr_mp)
		freeb(ir.ir_ll_hdr_mp);
	return i;
}


int ipfsync()
{
	char name[8];
	qif_t *qif;
	int i;

	for (i = 0; i < 10; i++) {
		sprintf(name, sizeof(name), "lan%d", i);
		spinlock(&pfil_rw);
		qif = qif_iflookup(name, 0);
		if (qif != NULL)
			fr_qifsync(NULL, 0, qif->qf_ill, -1, qif, NULL);
		spinunlock(&pfil_rw);
	}
	return 0;
}


static int fr_slowtimer()
{
	if (fr_running <= 0) {
		fr_timer_id = NULL;
		return;
	}

	READ_ENTER(&ipf_global);

	fr_fragexpire();
	fr_timeoutstate();
	fr_natexpire();
	fr_authexpire();
	fr_ticks++;
	fr_timer_id = NULL;
	if (fr_running <= 0) {
		RWLOCK_EXIT(&ipf_global);
		return;
	}
	fr_timer_id = mp_timeout(fr_slowtimer, NULL, hz/2);
	RWLOCK_EXIT(&ipf_global);
}

