/*	$NetBSD: netbsd32_ioctl.c,v 1.69.6.4 2016/10/05 20:55:39 skrll Exp $	*/

/*
 * Copyright (c) 1998, 2001 Matthew R. Green
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * handle ioctl conversions from netbsd32 -> 64-bit kernel
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_ioctl.c,v 1.69.6.4 2016/10/05 20:55:39 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/socketvar.h>
#include <sys/audioio.h>
#include <sys/disklabel.h>
#include <sys/dkio.h>
#include <sys/ataio.h>
#include <sys/sockio.h>
#include <sys/socket.h>
#include <sys/ttycom.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/ktrace.h>
#include <sys/kmem.h>
#include <sys/envsys.h>
#include <sys/wdog.h>
#include <sys/clockctl.h>
#include <sys/exec_elf.h>
#include <sys/ksyms.h>
#include <sys/drvctlio.h>

#ifdef __sparc__
#include <dev/sun/fbio.h>
#include <machine/openpromio.h>
#endif

#include <net/if.h>
#include <net/route.h>

#include <net/if_pppoe.h>
#include <net/if_sppp.h>

#include <net/npf/npf.h>

#include <net/bpf.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/igmp.h>
#include <netinet/igmp_var.h>
#include <netinet/ip_mroute.h>

#include <compat/sys/sockio.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_ioctl.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <dev/vndvar.h>

/* convert to/from different structures */

#if 0
static inline void
netbsd32_to_format_op(struct netbsd32_format_op *s32p, struct format_op *p, u_long cmd)
{

	p->df_buf = (char *)NETBSD32PTR64(s32p->df_buf);
	p->df_count = s32p->df_count;
	p->df_startblk = s32p->df_startblk;
	memcpy(p->df_reg, s32p->df_reg, sizeof(s32p->df_reg));
}
#endif

static inline void
netbsd32_to_ifreq(struct netbsd32_ifreq *s32p, struct ifreq *p, u_long cmd)
{

	memcpy(p, s32p, sizeof *s32p);
	/*
	 * XXX
	 * struct ifreq says the same, but sometimes the ifr_data
	 * union member needs to be converted to 64 bits... this
	 * is very driver specific and so we ignore it for now..
	 */
	if (cmd == SIOCGIFDATA || cmd == SIOCZIFDATA)
		p->ifr_data = (void *)NETBSD32PTR64(s32p->ifr_data);
}

static inline void
netbsd32_to_oifreq(struct netbsd32_oifreq *s32p, struct oifreq *p, u_long cmd)
{

	memcpy(p, s32p, sizeof *s32p);
	/*
	 * XXX
	 * struct ifreq says the same, but sometimes the ifr_data
	 * union member needs to be converted to 64 bits... this
	 * is very driver specific and so we ignore it for now..
	 */
	if (cmd == SIOCGIFDATA || cmd == SIOCZIFDATA)
		p->ifr_data = (void *)NETBSD32PTR64(s32p->ifr_data);
}

static inline void
netbsd32_to_if_addrprefreq(const struct netbsd32_if_addrprefreq *ifap32,
	struct if_addrprefreq *ifap, u_long cmd)
{
	strlcpy(ifap->ifap_name, ifap32->ifap_name, sizeof(ifap->ifap_name));
	ifap->ifap_preference = ifap32->ifap_preference;
	memcpy(&ifap->ifap_addr, &ifap32->ifap_addr,
	    min(ifap32->ifap_addr.ss_len, _SS_MAXSIZE));
}

static inline void
netbsd32_to_ifconf(struct netbsd32_ifconf *s32p, struct ifconf *p, u_long cmd)
{

	p->ifc_len = s32p->ifc_len;
	/* ifc_buf & ifc_req are the same size so this works */
	p->ifc_buf = (void *)NETBSD32PTR64(s32p->ifc_buf);
}

static inline void
netbsd32_to_ifmediareq(struct netbsd32_ifmediareq *s32p, struct ifmediareq *p, u_long cmd)
{

	memcpy(p, s32p, sizeof *s32p);
	p->ifm_ulist = (int *)NETBSD32PTR64(s32p->ifm_ulist);
}

static inline void
netbsd32_to_pppoediscparms(struct netbsd32_pppoediscparms *s32p,
    struct pppoediscparms *p, u_long cmd)
{

	memcpy(p->ifname, s32p->ifname, sizeof p->ifname);
	memcpy(p->eth_ifname, s32p->eth_ifname, sizeof p->eth_ifname);
	p->ac_name = (char *)NETBSD32PTR64(s32p->ac_name);
	p->ac_name_len = s32p->ac_name_len;
	p->service_name = (char *)NETBSD32PTR64(s32p->service_name);
	p->service_name_len = s32p->service_name_len;
}

static inline void
netbsd32_to_spppauthcfg(struct netbsd32_spppauthcfg *s32p,
    struct spppauthcfg *p, u_long cmd)
{

	memcpy(p->ifname, s32p->ifname, sizeof p->ifname);
	p->hisauth = s32p->hisauth;
	p->myauth = s32p->myauth;
	p->myname_length = s32p->myname_length;
	p->mysecret_length = s32p->mysecret_length;
	p->hisname_length = s32p->hisname_length;
	p->hissecret_length = s32p->hissecret_length;
	p->myauthflags = s32p->myauthflags;
	p->hisauthflags = s32p->hisauthflags;
	p->myname = (char *)NETBSD32PTR64(s32p->myname);
	p->mysecret = (char *)NETBSD32PTR64(s32p->mysecret);
	p->hisname = (char *)NETBSD32PTR64(s32p->hisname);
	p->hissecret = (char *)NETBSD32PTR64(s32p->hissecret);
}

static inline void
netbsd32_to_ifdrv(struct netbsd32_ifdrv *s32p, struct ifdrv *p, u_long cmd)
{

	memcpy(p->ifd_name, s32p->ifd_name, sizeof p->ifd_name);
	p->ifd_cmd = s32p->ifd_cmd;
	p->ifd_len = s32p->ifd_len;
	p->ifd_data = (void *)NETBSD32PTR64(s32p->ifd_data);
}

static inline void
netbsd32_to_sioc_vif_req(struct netbsd32_sioc_vif_req *s32p, struct sioc_vif_req *p, u_long cmd)
{

	p->vifi = s32p->vifi;
	p->icount = (u_long)s32p->icount;
	p->ocount = (u_long)s32p->ocount;
	p->ibytes = (u_long)s32p->ibytes;
	p->obytes = (u_long)s32p->obytes;
}

static inline void
netbsd32_to_sioc_sg_req(struct netbsd32_sioc_sg_req *s32p, struct sioc_sg_req *p, u_long cmd)
{

	p->src = s32p->src;
	p->grp = s32p->grp;
	p->pktcnt = (u_long)s32p->pktcnt;
	p->bytecnt = (u_long)s32p->bytecnt;
	p->wrong_if = (u_long)s32p->wrong_if;
}

static inline void
netbsd32_to_atareq(struct netbsd32_atareq *s32p, struct atareq *p, u_long cmd)
{
	p->flags = (u_long)s32p->flags;
	p->command = s32p->command;
	p->features = s32p->features;
	p->sec_count = s32p->sec_count;
	p->sec_num = s32p->sec_num;
	p->head = s32p->head;
	p->cylinder = s32p->cylinder;
	p->databuf =  (char *)NETBSD32PTR64(s32p->databuf);
	p->datalen = (u_long)s32p->datalen;
	p->timeout = s32p->timeout;
	p->retsts = s32p->retsts;
	p->error = s32p->error;
}

static inline void
netbsd32_to_vnd_ioctl(struct netbsd32_vnd_ioctl *s32p, struct vnd_ioctl *p, u_long cmd)
{

	p->vnd_file = (char *)NETBSD32PTR64(s32p->vnd_file);
	p->vnd_flags = s32p->vnd_flags;
	p->vnd_geom = s32p->vnd_geom;
	p->vnd_osize = s32p->vnd_osize;
	p->vnd_size = s32p->vnd_size;
}

static inline void
netbsd32_to_vnd_user(struct netbsd32_vnd_user *s32p, struct vnd_user *p, u_long cmd)
{

	p->vnu_unit = s32p->vnu_unit;
	p->vnu_dev = s32p->vnu_dev;
	p->vnu_ino = s32p->vnu_ino;
}

static inline void
netbsd32_to_vnd_ioctl50(struct netbsd32_vnd_ioctl50 *s32p, struct vnd_ioctl50 *p, u_long cmd)
{

	p->vnd_file = (char *)NETBSD32PTR64(s32p->vnd_file);
	p->vnd_flags = s32p->vnd_flags;
	p->vnd_geom = s32p->vnd_geom;
	p->vnd_size = s32p->vnd_size;
}

static inline void
netbsd32_to_plistref(struct netbsd32_plistref *s32p, struct plistref *p, u_long cmd)
{

	p->pref_plist = NETBSD32PTR64(s32p->pref_plist);
	p->pref_len = s32p->pref_len;
}

static inline void
netbsd32_to_u_long(netbsd32_u_long *s32p, u_long *p, u_long cmd)
{

	*p = (u_long)*s32p;
}

static inline void
netbsd32_to_voidp(netbsd32_voidp *s32p, voidp *p, u_long cmd)
{

	*p = (void *)NETBSD32PTR64(*s32p);
}

static inline void
netbsd32_to_wdog_conf(struct netbsd32_wdog_conf *s32p, struct wdog_conf *p, u_long cmd)
{

	p->wc_names = (char *)NETBSD32PTR64(s32p->wc_names);
	p->wc_count = s32p->wc_count;
}

static inline void
netbsd32_to_bpf_program(struct netbsd32_bpf_program *s32p, struct bpf_program *p, u_long cmd)
{

	p->bf_insns = (void *)NETBSD32PTR64(s32p->bf_insns);
	p->bf_len = s32p->bf_len;
}

static inline void
netbsd32_to_bpf_dltlist(struct netbsd32_bpf_dltlist *s32p, struct bpf_dltlist *p, u_long cmd)
{

	p->bfl_list = (void *)NETBSD32PTR64(s32p->bfl_list);
	p->bfl_len = s32p->bfl_len;
}

/* wsdisplay stuff */
static inline void
netbsd32_to_wsdisplay_addscreendata(struct netbsd32_wsdisplay_addscreendata *asd32,
					       struct wsdisplay_addscreendata *asd,
					       u_long cmd)
{
	asd->screentype = (char *)NETBSD32PTR64(asd32->screentype);
	asd->emul = (char *)NETBSD32PTR64(asd32->emul);
	asd->idx = asd32->idx;
}

static inline void
netbsd32_to_ieee80211req(struct netbsd32_ieee80211req *ireq32,
			 struct ieee80211req *ireq, u_long cmd)
{
	strncpy(ireq->i_name, ireq32->i_name, IFNAMSIZ);
	ireq->i_type = ireq32->i_type;
	ireq->i_val = ireq32->i_val;
	ireq->i_len = ireq32->i_len;
	ireq->i_data = NETBSD32PTR64(ireq32->i_data);
}

static inline void
netbsd32_to_ieee80211_nwkey(struct netbsd32_ieee80211_nwkey *nwk32,
					       struct ieee80211_nwkey *nwk,
					       u_long cmd)
{
	int i;

	strncpy(nwk->i_name, nwk32->i_name, IFNAMSIZ);
	nwk->i_wepon = nwk32->i_wepon;
	nwk->i_defkid = nwk32->i_defkid;
	for (i = 0; i < IEEE80211_WEP_NKID; i++) {
		nwk->i_key[i].i_keylen = nwk32->i_key[i].i_keylen;
		nwk->i_key[i].i_keydat =
		    NETBSD32PTR64(nwk32->i_key[i].i_keydat);
	}
}

static inline void
netbsd32_to_wsdisplay_cursor(struct netbsd32_wsdisplay_cursor *c32,
					       struct wsdisplay_cursor *c,
					       u_long cmd)
{
	c->which = c32->which;
	c->enable = c32->enable;
	c->pos.x = c32->pos.x;
	c->pos.y = c32->pos.y;
	c->hot.x = c32->hot.x;
	c->hot.y = c32->hot.y;
	c->size.x = c32->size.x;
	c->size.y = c32->size.y;
	c->cmap.index = c32->cmap.index;
	c->cmap.count = c32->cmap.count;
	c->cmap.red = NETBSD32PTR64(c32->cmap.red);
	c->cmap.green = NETBSD32PTR64(c32->cmap.green);
	c->cmap.blue = NETBSD32PTR64(c32->cmap.blue);
	c->image = NETBSD32PTR64(c32->image);
	c->mask = NETBSD32PTR64(c32->mask);
}

static inline void
netbsd32_to_wsdisplay_cmap(struct netbsd32_wsdisplay_cmap *c32,
					       struct wsdisplay_cmap *c,
					       u_long cmd)
{
	c->index = c32->index;
	c->count = c32->count;
	c->red   = NETBSD32PTR64(c32->red);
	c->green = NETBSD32PTR64(c32->green);
	c->blue  = NETBSD32PTR64(c32->blue);
}

static inline void
netbsd32_to_clockctl_settimeofday(
    const struct netbsd32_clockctl_settimeofday *s32p,
    struct clockctl_settimeofday *p,
    u_long cmd)
{

	p->tv = NETBSD32PTR64(s32p->tv);
	p->tzp = NETBSD32PTR64(s32p->tzp);
}

static inline void
netbsd32_to_clockctl_adjtime(
    const struct netbsd32_clockctl_adjtime *s32p,
    struct clockctl_adjtime *p,
    u_long cmd)
{

	p->delta = NETBSD32PTR64(s32p->delta);
	p->olddelta = NETBSD32PTR64(s32p->olddelta);
}

static inline void
netbsd32_to_clockctl_clock_settime(
    const struct netbsd32_clockctl_clock_settime *s32p,
    struct clockctl_clock_settime *p,
    u_long cmd)
{

	p->clock_id = s32p->clock_id;
	p->tp = NETBSD32PTR64(s32p->tp);
}

static inline void
netbsd32_to_clockctl_ntp_adjtime(
    const struct netbsd32_clockctl_ntp_adjtime *s32p,
    struct clockctl_ntp_adjtime *p,
    u_long cmd)
{

	p->tp = NETBSD32PTR64(s32p->tp);
	p->retval = s32p->retval;
}

static inline void
netbsd32_to_ksyms_gsymbol(
    const struct netbsd32_ksyms_gsymbol *s32p,
    struct ksyms_gsymbol *p,
    u_long cmd)
{

	p->kg_name = NETBSD32PTR64(s32p->kg_name);
}

static inline void
netbsd32_to_ksyms_gvalue(
    const struct netbsd32_ksyms_gvalue *s32p,
    struct ksyms_gvalue *p,
    u_long cmd)
{

	p->kv_name = NETBSD32PTR64(s32p->kv_name);
}

static inline void
netbsd32_to_npf_ioctl_table(
    const struct netbsd32_npf_ioctl_table *s32p,
    struct npf_ioctl_table *p,
    u_long cmd)
{

	p->nct_cmd = s32p->nct_cmd;
	p->nct_name = NETBSD32PTR64(s32p->nct_name);
	switch (s32p->nct_cmd) {
	case NPF_CMD_TABLE_LOOKUP:
	case NPF_CMD_TABLE_ADD:
	case NPF_CMD_TABLE_REMOVE:
		p->nct_data.ent.alen = s32p->nct_data.ent.alen;
		p->nct_data.ent.addr = s32p->nct_data.ent.addr;
		p->nct_data.ent.mask = s32p->nct_data.ent.mask;
		break;
	case NPF_CMD_TABLE_LIST:
		p->nct_data.buf.buf = NETBSD32PTR64(s32p->nct_data.buf.buf);
		p->nct_data.buf.len = s32p->nct_data.buf.len;
		break;
	}
}

static inline void
netbsd32_to_devlistargs(
    const struct netbsd32_devlistargs *s32p,
    struct devlistargs *p,
    u_long cmd)
{
	memcpy(p->l_devname, s32p->l_devname, sizeof(p->l_devname));
	p->l_children = s32p->l_children;
	p->l_childname = NETBSD32PTR64(s32p->l_childname);
}

static inline void
netbsd32_to_devrescanargs(
    const struct netbsd32_devrescanargs *s32p,
    struct devrescanargs *p,
    u_long cmd)
{
	memcpy(p->busname, s32p->busname, sizeof(p->busname));
	memcpy(p->ifattr, s32p->ifattr, sizeof(p->ifattr));
	p->numlocators = s32p->numlocators;
	p->locators = NETBSD32PTR64(s32p->locators);
}

/*
 * handle ioctl conversions from 64-bit kernel -> netbsd32
 */

#if 0
static inline void
netbsd32_from_format_op(struct format_op *p, struct netbsd32_format_op *s32p, u_long cmd)
{

/* filled in */
#if 0
	s32p->df_buf = (netbsd32_charp)p->df_buf;
#endif
	s32p->df_count = p->df_count;
	s32p->df_startblk = p->df_startblk;
	memcpy(s32p->df_reg, p->df_reg, sizeof(p->df_reg));
}
#endif

static inline void
netbsd32_from_ifreq(struct ifreq *p, struct netbsd32_ifreq *s32p, u_long cmd)
{

	/*
	 * XXX
	 * struct ifreq says the same, but sometimes the ifr_data
	 * union member needs to be converted to 64 bits... this
	 * is very driver specific and so we ignore it for now..
	 */
	memcpy(s32p, p, sizeof *s32p);
	if (cmd == SIOCGIFDATA || cmd == SIOCZIFDATA)
		NETBSD32PTR32(s32p->ifr_data, p->ifr_data);
}

static inline void
netbsd32_from_oifreq(struct oifreq *p, struct netbsd32_oifreq *s32p, u_long cmd)
{

	/*
	 * XXX
	 * struct ifreq says the same, but sometimes the ifr_data
	 * union member needs to be converted to 64 bits... this
	 * is very driver specific and so we ignore it for now..
	 */
	memcpy(s32p, p, sizeof *s32p);
	if (cmd == SIOCGIFDATA || cmd == SIOCZIFDATA)
		NETBSD32PTR32(s32p->ifr_data, p->ifr_data);
}

static inline void
netbsd32_from_if_addrprefreq(const struct if_addrprefreq *ifap,
	struct netbsd32_if_addrprefreq *ifap32, u_long cmd)
{
	strlcpy(ifap32->ifap_name, ifap->ifap_name, sizeof(ifap32->ifap_name));
	ifap32->ifap_preference = ifap->ifap_preference;
	memcpy(&ifap32->ifap_addr, &ifap->ifap_addr,
	    min(ifap->ifap_addr.ss_len, _SS_MAXSIZE));
}

static inline void
netbsd32_from_ifconf(struct ifconf *p, struct netbsd32_ifconf *s32p, u_long cmd)
{

	s32p->ifc_len = p->ifc_len;
	/* ifc_buf & ifc_req are the same size so this works */
	NETBSD32PTR32(s32p->ifc_buf, p->ifc_buf);
}

static inline void
netbsd32_from_ifmediareq(struct ifmediareq *p, struct netbsd32_ifmediareq *s32p, u_long cmd)
{

	memcpy(s32p, p, sizeof *p);
/* filled in? */
#if 0
	s32p->ifm_ulist = (netbsd32_intp_t)p->ifm_ulist;
#endif
}

static inline void
netbsd32_from_pppoediscparms(struct pppoediscparms *p,
    struct netbsd32_pppoediscparms *s32p, u_long cmd)
{

	memcpy(s32p->ifname, p->ifname, sizeof s32p->ifname);
	memcpy(s32p->eth_ifname, p->eth_ifname, sizeof s32p->eth_ifname);
	NETBSD32PTR32(s32p->ac_name, p->ac_name);
	s32p->ac_name_len = p->ac_name_len;
	NETBSD32PTR32(s32p->service_name, p->service_name);
	s32p->service_name_len = p->service_name_len;
}

static inline void
netbsd32_from_spppauthcfg(struct spppauthcfg *p,
    struct netbsd32_spppauthcfg *s32p, u_long cmd)
{

	memcpy(s32p->ifname, p->ifname, sizeof s32p->ifname);
	s32p->hisauth = p->hisauth;
	s32p->myauth = p->myauth;
	s32p->myname_length = p->myname_length;
	s32p->mysecret_length = p->mysecret_length;
	s32p->hisname_length = p->hisname_length;
	s32p->hissecret_length = p->hissecret_length;
	s32p->myauthflags = p->myauthflags;
	s32p->hisauthflags = p->hisauthflags;
	NETBSD32PTR32(s32p->myname, p->myname);
	NETBSD32PTR32(s32p->mysecret, p->mysecret);
	NETBSD32PTR32(s32p->hisname, p->hisname);
	NETBSD32PTR32(s32p->hissecret, p->hissecret);
}

static inline void
netbsd32_from_ifdrv(struct ifdrv *p, struct netbsd32_ifdrv *s32p, u_long cmd)
{

	memcpy(s32p->ifd_name, p->ifd_name, sizeof s32p->ifd_name);
	s32p->ifd_cmd = p->ifd_cmd;
	s32p->ifd_len = p->ifd_len;
	NETBSD32PTR32(s32p->ifd_data, p->ifd_data);
}

static inline void
netbsd32_from_sioc_vif_req(struct sioc_vif_req *p, struct netbsd32_sioc_vif_req *s32p, u_long cmd)
{

	s32p->vifi = p->vifi;
	s32p->icount = (netbsd32_u_long)p->icount;
	s32p->ocount = (netbsd32_u_long)p->ocount;
	s32p->ibytes = (netbsd32_u_long)p->ibytes;
	s32p->obytes = (netbsd32_u_long)p->obytes;
}

static inline void
netbsd32_from_sioc_sg_req(struct sioc_sg_req *p, struct netbsd32_sioc_sg_req *s32p, u_long cmd)
{

	s32p->src = p->src;
	s32p->grp = p->grp;
	s32p->pktcnt = (netbsd32_u_long)p->pktcnt;
	s32p->bytecnt = (netbsd32_u_long)p->bytecnt;
	s32p->wrong_if = (netbsd32_u_long)p->wrong_if;
}

static inline void
netbsd32_from_atareq(struct atareq *p, struct netbsd32_atareq *s32p, u_long cmd)
{
	s32p->flags = (netbsd32_u_long)p->flags;
	s32p->command = p->command;
	s32p->features = p->features;
	s32p->sec_count = p->sec_count;
	s32p->sec_num = p->sec_num;
	s32p->head = p->head;
	s32p->cylinder = p->cylinder;
	NETBSD32PTR32(s32p->databuf, p->databuf);
	s32p->datalen = (netbsd32_u_long)p->datalen;
	s32p->timeout = p->timeout;
	s32p->retsts = p->retsts;
	s32p->error = p->error;
}

static inline void
netbsd32_from_vnd_ioctl(struct vnd_ioctl *p, struct netbsd32_vnd_ioctl *s32p, u_long cmd)
{

	s32p->vnd_flags = p->vnd_flags;
	s32p->vnd_geom = p->vnd_geom;
	s32p->vnd_osize = p->vnd_osize;
	s32p->vnd_size = p->vnd_size;
}

static inline void
netbsd32_from_vnd_user(struct vnd_user *p, struct netbsd32_vnd_user *s32p, u_long cmd)
{

	s32p->vnu_unit = p->vnu_unit;
	s32p->vnu_dev = p->vnu_dev;
	s32p->vnu_ino = p->vnu_ino;
}

static inline void
netbsd32_from_vnd_ioctl50(struct vnd_ioctl50 *p, struct netbsd32_vnd_ioctl50 *s32p, u_long cmd)
{

	s32p->vnd_flags = p->vnd_flags;
	s32p->vnd_geom = p->vnd_geom;
	s32p->vnd_size = p->vnd_size;
}

static inline void
netbsd32_from_plistref(struct plistref *p, struct netbsd32_plistref *s32p, u_long cmd)
{

	NETBSD32PTR32(s32p->pref_plist, p->pref_plist);
	s32p->pref_len = p->pref_len;
}

static inline void
netbsd32_from_wdog_conf(struct wdog_conf *p, struct netbsd32_wdog_conf *s32p, u_long cmd)
{

	NETBSD32PTR32(s32p->wc_names, p->wc_names);
	s32p->wc_count = p->wc_count;
}

/* wsdisplay stuff */
static inline void
netbsd32_from_wsdisplay_addscreendata(struct wsdisplay_addscreendata *asd,
					struct netbsd32_wsdisplay_addscreendata *asd32,
					u_long cmd)
{
	NETBSD32PTR32(asd32->screentype, asd->screentype);
	NETBSD32PTR32(asd32->emul, asd->emul);
	asd32->idx = asd->idx;
}

static inline void
netbsd32_from_wsdisplay_cursor(struct wsdisplay_cursor *c,
					       struct netbsd32_wsdisplay_cursor *c32,
					       u_long cmd)
{
	c32->which = c->which;
	c32->enable = c->enable;
	c32->pos.x = c->pos.x;
	c32->pos.y = c->pos.y;
	c32->hot.x = c->hot.x;
	c32->hot.y = c->hot.y;
	c32->size.x = c->size.x;
	c32->size.y = c->size.y;
	c32->cmap.index = c->cmap.index;
	c32->cmap.count = c->cmap.count;
	NETBSD32PTR32(c32->cmap.red, c->cmap.red);
	NETBSD32PTR32(c32->cmap.green, c->cmap.green);
	NETBSD32PTR32(c32->cmap.blue, c->cmap.blue);
	NETBSD32PTR32(c32->image, c->image);
	NETBSD32PTR32(c32->mask, c->mask);
}

static inline void
netbsd32_from_wsdisplay_cmap(struct wsdisplay_cmap *c,
					   struct netbsd32_wsdisplay_cmap *c32,
					   u_long cmd)
{
	c32->index = c->index;
	c32->count = c->count;
	NETBSD32PTR32(c32->red, c->red);
	NETBSD32PTR32(c32->green, c->green);
	NETBSD32PTR32(c32->blue, c->blue);
}

static inline void
netbsd32_from_ieee80211req(struct ieee80211req *ireq,
			   struct netbsd32_ieee80211req *ireq32, u_long cmd)
{
	strncpy(ireq32->i_name, ireq->i_name, IFNAMSIZ);
	ireq32->i_type = ireq->i_type;
	ireq32->i_val = ireq->i_val;
	ireq32->i_len = ireq->i_len;
	NETBSD32PTR32(ireq32->i_data, ireq->i_data);
}

static inline void
netbsd32_from_ieee80211_nwkey(struct ieee80211_nwkey *nwk,
				struct netbsd32_ieee80211_nwkey *nwk32,
				u_long cmd)
{
	int i;

	strncpy(nwk32->i_name, nwk->i_name, IFNAMSIZ);
	nwk32->i_wepon = nwk->i_wepon;
	nwk32->i_defkid = nwk->i_defkid;
	for (i = 0; i < IEEE80211_WEP_NKID; i++) {
		nwk32->i_key[i].i_keylen = nwk->i_key[i].i_keylen;
		NETBSD32PTR32(nwk32->i_key[i].i_keydat,
				nwk->i_key[i].i_keydat);
	}
}

static inline void
netbsd32_from_bpf_program(struct bpf_program *p, struct netbsd32_bpf_program *s32p, u_long cmd)
{

	NETBSD32PTR32(s32p->bf_insns, p->bf_insns);
	s32p->bf_len = p->bf_len;
}

static inline void
netbsd32_from_bpf_dltlist(struct bpf_dltlist *p, struct netbsd32_bpf_dltlist *s32p, u_long cmd)
{

	NETBSD32PTR32(s32p->bfl_list, p->bfl_list);
	s32p->bfl_len = p->bfl_len;
}

static inline void
netbsd32_from_u_long(u_long *p, netbsd32_u_long *s32p, u_long cmd)
{

	*s32p = (netbsd32_u_long)*p;
}

static inline void
netbsd32_from_voidp(voidp *p, netbsd32_voidp *s32p, u_long cmd)
{

	NETBSD32PTR32(*s32p, *p);
}


static inline void
netbsd32_from_clockctl_settimeofday(
    const struct clockctl_settimeofday *p,
    struct netbsd32_clockctl_settimeofday *s32p,
    u_long cmd)
{

	NETBSD32PTR32(s32p->tv, p->tv);
	NETBSD32PTR32(s32p->tzp, p->tzp);
}

static inline void
netbsd32_from_clockctl_adjtime(
    const struct clockctl_adjtime *p,
    struct netbsd32_clockctl_adjtime *s32p,
    u_long cmd)
{

	NETBSD32PTR32(s32p->delta, p->delta);
	NETBSD32PTR32(s32p->olddelta, p->olddelta);
}

static inline void
netbsd32_from_clockctl_clock_settime(
    const struct clockctl_clock_settime *p,
    struct netbsd32_clockctl_clock_settime *s32p,
    u_long cmd)
{

	s32p->clock_id = p->clock_id;
	NETBSD32PTR32(s32p->tp, p->tp);
}

static inline void
netbsd32_from_clockctl_ntp_adjtime(
    const struct clockctl_ntp_adjtime *p,
    struct netbsd32_clockctl_ntp_adjtime *s32p,
    u_long cmd)
{

	NETBSD32PTR32(s32p->tp, p->tp);
	s32p->retval = p->retval;
}

static inline void
netbsd32_from_ksyms_gsymbol(
    const struct ksyms_gsymbol *p,
    struct netbsd32_ksyms_gsymbol *s32p,
    u_long cmd)
{

	NETBSD32PTR32(s32p->kg_name, p->kg_name);
	s32p->kg_sym = p->kg_sym;
}

static inline void
netbsd32_from_ksyms_gvalue(
    const struct ksyms_gvalue *p,
    struct netbsd32_ksyms_gvalue *s32p,
    u_long cmd)
{

	NETBSD32PTR32(s32p->kv_name, p->kv_name);
	s32p->kv_value = p->kv_value;
}

static inline void
netbsd32_from_npf_ioctl_table(
    const struct npf_ioctl_table *p,
    struct netbsd32_npf_ioctl_table *s32p,
    u_long cmd)
{

	s32p->nct_cmd = p->nct_cmd;
	NETBSD32PTR32(s32p->nct_name, p->nct_name);
	switch (p->nct_cmd) {
	case NPF_CMD_TABLE_LOOKUP:
	case NPF_CMD_TABLE_ADD:
	case NPF_CMD_TABLE_REMOVE:
		s32p->nct_data.ent.alen = p->nct_data.ent.alen;
		s32p->nct_data.ent.addr = p->nct_data.ent.addr;
		s32p->nct_data.ent.mask = p->nct_data.ent.mask;
		break;
	case NPF_CMD_TABLE_LIST:
		NETBSD32PTR32(s32p->nct_data.buf.buf, p->nct_data.buf.buf);
		s32p->nct_data.buf.len = p->nct_data.buf.len;
		break;
	}
}

static inline void
netbsd32_from_devlistargs(
    const struct devlistargs *p,
    struct netbsd32_devlistargs *s32p,
    u_long cmd)
{
	memcpy(s32p->l_devname, p->l_devname, sizeof(s32p->l_devname));
	s32p->l_children = p->l_children;
	NETBSD32PTR32(s32p->l_childname, p->l_childname);
}

static inline void
netbsd32_from_devrescanargs(
    const struct devrescanargs *p,
    struct netbsd32_devrescanargs *s32p,
    u_long cmd)
{
	memcpy(s32p->busname, p->busname, sizeof(s32p->busname));
	memcpy(s32p->ifattr, p->ifattr, sizeof(s32p->ifattr));
	s32p->numlocators = p->numlocators;
	NETBSD32PTR32(s32p->locators, p->locators);
}

/*
 * main ioctl syscall.
 *
 * ok, here we are in the biggy.  we have to do fix ups depending
 * on the ioctl command before and afterwards.
 */
int
netbsd32_ioctl(struct lwp *l, const struct netbsd32_ioctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_u_long) com;
		syscallarg(netbsd32_voidp) data;
	} */
	struct proc *p = l->l_proc;
	struct file *fp;
	struct filedesc *fdp;
	u_long com;
	int error = 0;
	size_t size;
	size_t alloc_size32, size32;
	void *data, *memp = NULL;
	void *data32, *memp32 = NULL;
	unsigned int fd;
	fdfile_t *ff;
	int tmp;
#define STK_PARAMS	128
	uint64_t stkbuf[STK_PARAMS/sizeof(uint64_t)];
	uint64_t stkbuf32[STK_PARAMS/sizeof(uint64_t)];

	/*
	 * we need to translate some commands (_IOW) before calling sys_ioctl,
	 * some after (_IOR), and some both (_IOWR).
	 */
#if 0
	{
		const char * const dirs[8] = {
		    "NONE!", "VOID", "OUT", "VOID|OUT!", "IN", "VOID|IN!",
		    "INOUT", "VOID|IN|OUT!"
		};

		printf("netbsd32_ioctl(%d, %x, %x): "
		    "%s group %c base %d len %d\n",
		    SCARG(uap, fd), SCARG(uap, com), SCARG(uap, data).i32,
		    dirs[((SCARG(uap, com) & IOC_DIRMASK)>>29)],
		    IOCGROUP(SCARG(uap, com)), IOCBASECMD(SCARG(uap, com)),
		    IOCPARM_LEN(SCARG(uap, com)));
	}
#endif

	memp = NULL;
	memp32 = NULL;
	alloc_size32 = 0;
	size32 = 0;
	size = 0;

	fdp = p->p_fd;
	fd = SCARG(uap, fd);
	if ((fp = fd_getfile(fd)) == NULL)
		return (EBADF);
	if ((fp->f_flag & (FREAD | FWRITE)) == 0) {
		error = EBADF;
		goto out;
	}

	ff = fdp->fd_dt->dt_ff[SCARG(uap, fd)];
	switch (com = SCARG(uap, com)) {
	case FIOCLEX:
		ff->ff_exclose = true;
		fdp->fd_exclose = true;
		goto out;

	case FIONCLEX:
		ff->ff_exclose = false;
		goto out;
	}

	/*
	 * Interpret high order word to find amount of data to be
	 * copied to/from the user's address space.
	 */
	size32 = IOCPARM_LEN(com);
	alloc_size32 = size32;

	/*
	 * The disklabel is now padded to a multiple of 8 bytes however the old
	 * disklabel on 32bit platforms wasn't.  This leaves a difference in
	 * size of 4 bytes between the two but are otherwise identical.
	 * To deal with this, we allocate enough space for the new disklabel
	 * but only copyin/out the smaller amount.
	 */
	if (IOCGROUP(com) == 'd') {
		u_long ncom = com ^ (DIOCGDINFO ^ DIOCGDINFO32);
		switch (ncom) {
		case DIOCGDINFO:
		case DIOCWDINFO:
		case DIOCSDINFO:
		case DIOCGDEFLABEL:
			com = ncom;
			if (IOCPARM_LEN(DIOCGDINFO32) < IOCPARM_LEN(DIOCGDINFO))
				alloc_size32 = IOCPARM_LEN(DIOCGDINFO);
			break;
		}
	}
	if (alloc_size32 > IOCPARM_MAX) {
		error = ENOTTY;
		goto out;
	}
	if (alloc_size32 > sizeof(stkbuf)) {
		memp32 = kmem_alloc(alloc_size32, KM_SLEEP);
		data32 = memp32;
	} else
		data32 = (void *)stkbuf32;
	if ((com >> IOCPARM_SHIFT) == 0)  {
		/* UNIX-style ioctl. */
		data32 = SCARG_P32(uap, data);
	} else {
		if (com&IOC_IN) {
			if (size32) {
				error = copyin(SCARG_P32(uap, data), data32,
				    size32);
				if (error) {
					goto out;
				}
				/*
				 * The data between size and alloc_size has
				 * not been overwritten.  It shouldn't matter
				 * but let's clear that anyway.
				 */
				if (__predict_false(size32 < alloc_size32)) {
					memset((char *)data32+size32, 0,
					    alloc_size32 - size32);
				}
				ktrgenio(fd, UIO_WRITE, SCARG_P32(uap, data),
				    size32, 0);
			} else
				*(void **)data32 = SCARG_P32(uap, data);
		} else if ((com&IOC_OUT) && size32) {
			/*
			 * Zero the buffer so the user always
			 * gets back something deterministic.
			 */
			memset(data32, 0, alloc_size32);
		} else if (com&IOC_VOID) {
			*(void **)data32 = SCARG_P32(uap, data);
		}
	}

	/*
	 * convert various structures, pointers, and other objects that
	 * change size from 32 bit -> 64 bit, for all ioctl commands.
	 */
	switch (SCARG(uap, com)) {
	case FIONBIO:
		mutex_enter(&fp->f_lock);
		if ((tmp = *(int *)data32) != 0)
			fp->f_flag |= FNONBLOCK;
		else
			fp->f_flag &= ~FNONBLOCK;
		mutex_exit(&fp->f_lock);
		error = (*fp->f_ops->fo_ioctl)(fp, FIONBIO, (void *)&tmp);
		break;

	case FIOASYNC:
		mutex_enter(&fp->f_lock);
		if ((tmp = *(int *)data32) != 0)
			fp->f_flag |= FASYNC;
		else
			fp->f_flag &= ~FASYNC;
		mutex_exit(&fp->f_lock);
		error = (*fp->f_ops->fo_ioctl)(fp, FIOASYNC, (void *)&tmp);
		break;

	case AUDIO_WSEEK32:
		IOCTL_CONV_TO(AUDIO_WSEEK, u_long);

#if 0	/* not implemented by anything */
	case DIOCRFORMAT32:
		IOCTL_STRUCT_CONV_TO(DIOCRFORMAT, format_op);
	case DIOCWFORMAT32:
		IOCTL_STRUCT_CONV_TO(DIOCWFORMAT, format_op);
#endif

	case ATAIOCCOMMAND32:
		IOCTL_STRUCT_CONV_TO(ATAIOCCOMMAND, atareq);

	case SIOCIFGCLONERS32:
		{
			struct netbsd32_if_clonereq *req =
			    (struct netbsd32_if_clonereq *)data32;
			char *buf = NETBSD32PTR64(req->ifcr_buffer);

			error = if_clone_list(req->ifcr_count,
			    buf, &req->ifcr_total);
			break;
		}

/*
 * only a few ifreq syscalls need conversion and those are
 * all driver specific... XXX
 */
#if 0
	case SIOCGADDRROM3232:
		IOCTL_STRUCT_CONV_TO(SIOCGADDRROM32, ifreq);
	case SIOCGCHIPID32:
		IOCTL_STRUCT_CONV_TO(SIOCGCHIPID, ifreq);
	case SIOCSIFADDR32:
		IOCTL_STRUCT_CONV_TO(SIOCSIFADDR, ifreq);
	case OSIOCGIFADDR32:
		IOCTL_STRUCT_CONV_TO(OSIOCGIFADDR, ifreq);
	case SIOCGIFADDR32:
		IOCTL_STRUCT_CONV_TO(SIOCGIFADDR, ifreq);
	case SIOCSIFDSTADDR32:
		IOCTL_STRUCT_CONV_TO(SIOCSIFDSTADDR, ifreq);
	case OSIOCGIFDSTADDR32:
		IOCTL_STRUCT_CONV_TO(OSIOCGIFDSTADDR, ifreq);
	case SIOCGIFDSTADDR32:
		IOCTL_STRUCT_CONV_TO(SIOCGIFDSTADDR, ifreq);
	case OSIOCGIFBRDADDR32:
		IOCTL_STRUCT_CONV_TO(OSIOCGIFBRDADDR, ifreq);
	case SIOCGIFBRDADDR32:
		IOCTL_STRUCT_CONV_TO(SIOCGIFBRDADDR, ifreq);
	case SIOCSIFBRDADDR32:
		IOCTL_STRUCT_CONV_TO(SIOCSIFBRDADDR, ifreq);
	case OSIOCGIFNETMASK32:
		IOCTL_STRUCT_CONV_TO(OSIOCGIFNETMASK, ifreq);
	case SIOCGIFNETMASK32:
		IOCTL_STRUCT_CONV_TO(SIOCGIFNETMASK, ifreq);
	case SIOCSIFNETMASK32:
		IOCTL_STRUCT_CONV_TO(SIOCSIFNETMASK, ifreq);
	case SIOCGIFMETRIC32:
		IOCTL_STRUCT_CONV_TO(SIOCGIFMETRIC, ifreq);
	case SIOCSIFMETRIC32:
		IOCTL_STRUCT_CONV_TO(SIOCSIFMETRIC, ifreq);
	case SIOCDIFADDR32:
		IOCTL_STRUCT_CONV_TO(SIOCDIFADDR, ifreq);
	case SIOCADDMULTI32:
		IOCTL_STRUCT_CONV_TO(SIOCADDMULTI, ifreq);
	case SIOCDELMULTI32:
		IOCTL_STRUCT_CONV_TO(SIOCDELMULTI, ifreq);
	case SIOCSIFMEDIA32:
		IOCTL_STRUCT_CONV_TO(SIOCSIFMEDIA, ifreq);
	case SIOCSIFMTU32:
		IOCTL_STRUCT_CONV_TO(SIOCSIFMTU, ifreq);
	case SIOCGIFMTU32:
		IOCTL_STRUCT_CONV_TO(SIOCGIFMTU, ifreq);
	case BIOCGETIF32:
		IOCTL_STRUCT_CONV_TO(BIOCGETIF, ifreq);
	case BIOCSETIF32:
		IOCTL_STRUCT_CONV_TO(BIOCSETIF, ifreq);
	case SIOCPHASE132:
		IOCTL_STRUCT_CONV_TO(SIOCPHASE1, ifreq);
	case SIOCPHASE232:
		IOCTL_STRUCT_CONV_TO(SIOCPHASE2, ifreq);
#endif

	case OOSIOCGIFCONF32:
		IOCTL_STRUCT_CONV_TO(OOSIOCGIFCONF, ifconf);
	case OSIOCGIFCONF32:
		IOCTL_STRUCT_CONV_TO(OSIOCGIFCONF, ifconf);
	case SIOCGIFCONF32:
		IOCTL_STRUCT_CONV_TO(SIOCGIFCONF, ifconf);

	case SIOCGIFFLAGS32:
		IOCTL_STRUCT_CONV_TO(SIOCGIFFLAGS, ifreq);
	case SIOCSIFFLAGS32:
		IOCTL_STRUCT_CONV_TO(SIOCSIFFLAGS, ifreq);

	case SIOCGIFADDRPREF32:
		IOCTL_STRUCT_CONV_TO(SIOCGIFADDRPREF, if_addrprefreq);
	case SIOCSIFADDRPREF32:
		IOCTL_STRUCT_CONV_TO(SIOCSIFADDRPREF, if_addrprefreq);


	case OSIOCGIFFLAGS32:
		IOCTL_STRUCT_CONV_TO(OSIOCGIFFLAGS, oifreq);
	case OSIOCSIFFLAGS32:
		IOCTL_STRUCT_CONV_TO(OSIOCSIFFLAGS, oifreq);

	case SIOCGIFMEDIA32:
		IOCTL_STRUCT_CONV_TO(SIOCGIFMEDIA, ifmediareq);

	case PPPOESETPARMS32:
		IOCTL_STRUCT_CONV_TO(PPPOESETPARMS, pppoediscparms);
	case PPPOEGETPARMS32:
		IOCTL_STRUCT_CONV_TO(PPPOEGETPARMS, pppoediscparms);
	case SPPPGETAUTHCFG32:
		IOCTL_STRUCT_CONV_TO(SPPPGETAUTHCFG, spppauthcfg);
	case SPPPSETAUTHCFG32:
		IOCTL_STRUCT_CONV_TO(SPPPSETAUTHCFG, spppauthcfg);

	case SIOCSDRVSPEC32:
		IOCTL_STRUCT_CONV_TO(SIOCSDRVSPEC, ifdrv);
	case SIOCGDRVSPEC32:
		IOCTL_STRUCT_CONV_TO(SIOCGDRVSPEC, ifdrv);

	case SIOCGETVIFCNT32:
		IOCTL_STRUCT_CONV_TO(SIOCGETVIFCNT, sioc_vif_req);

	case SIOCGETSGCNT32:
		IOCTL_STRUCT_CONV_TO(SIOCGETSGCNT, sioc_sg_req);

	case VNDIOCSET32:
		IOCTL_STRUCT_CONV_TO(VNDIOCSET, vnd_ioctl);

	case VNDIOCCLR32:
		IOCTL_STRUCT_CONV_TO(VNDIOCCLR, vnd_ioctl);

	case VNDIOCGET32:
		IOCTL_STRUCT_CONV_TO(VNDIOCGET, vnd_user);

	case VNDIOCSET5032:
		IOCTL_STRUCT_CONV_TO(VNDIOCSET50, vnd_ioctl50);

	case VNDIOCCLR5032:
		IOCTL_STRUCT_CONV_TO(VNDIOCCLR50, vnd_ioctl50);

	case ENVSYS_GETDICTIONARY32:
		IOCTL_STRUCT_CONV_TO(ENVSYS_GETDICTIONARY, plistref);
	case ENVSYS_SETDICTIONARY32:
		IOCTL_STRUCT_CONV_TO(ENVSYS_SETDICTIONARY, plistref);
	case ENVSYS_REMOVEPROPS32:
		IOCTL_STRUCT_CONV_TO(ENVSYS_REMOVEPROPS, plistref);

	case WDOGIOC_GWDOGS32:
		IOCTL_STRUCT_CONV_TO(WDOGIOC_GWDOGS, wdog_conf);

	case BIOCSETF32:
		IOCTL_STRUCT_CONV_TO(BIOCSETF, bpf_program);
	case BIOCSTCPF32:
		IOCTL_STRUCT_CONV_TO(BIOCSTCPF, bpf_program);
	case BIOCSUDPF32:
		IOCTL_STRUCT_CONV_TO(BIOCSUDPF, bpf_program);
	case BIOCGDLTLIST32:
		IOCTL_STRUCT_CONV_TO(BIOCGDLTLIST, bpf_dltlist);

	case WSDISPLAYIO_ADDSCREEN32:
		IOCTL_STRUCT_CONV_TO(WSDISPLAYIO_ADDSCREEN, wsdisplay_addscreendata);

	case WSDISPLAYIO_GCURSOR32:
		IOCTL_STRUCT_CONV_TO(WSDISPLAYIO_GCURSOR, wsdisplay_cursor);
	case WSDISPLAYIO_SCURSOR32:
		IOCTL_STRUCT_CONV_TO(WSDISPLAYIO_SCURSOR, wsdisplay_cursor);

	case WSDISPLAYIO_GETCMAP32:
		IOCTL_STRUCT_CONV_TO(WSDISPLAYIO_GETCMAP, wsdisplay_cmap);
	case WSDISPLAYIO_PUTCMAP32:
		IOCTL_STRUCT_CONV_TO(WSDISPLAYIO_PUTCMAP, wsdisplay_cmap);

	case SIOCS8021132:
		IOCTL_STRUCT_CONV_TO(SIOCS80211, ieee80211req);
	case SIOCG8021132:
		IOCTL_STRUCT_CONV_TO(SIOCG80211, ieee80211req);
	case SIOCS80211NWKEY32:
		IOCTL_STRUCT_CONV_TO(SIOCS80211NWKEY, ieee80211_nwkey);
	case SIOCG80211NWKEY32:
		IOCTL_STRUCT_CONV_TO(SIOCG80211NWKEY, ieee80211_nwkey);

	case POWER_EVENT_RECVDICT32:
		IOCTL_STRUCT_CONV_TO(POWER_EVENT_RECVDICT, plistref);

	case CLOCKCTL_SETTIMEOFDAY32:
		IOCTL_STRUCT_CONV_TO(CLOCKCTL_SETTIMEOFDAY,
		    clockctl_settimeofday);
	case CLOCKCTL_ADJTIME32:
		IOCTL_STRUCT_CONV_TO(CLOCKCTL_ADJTIME, clockctl_adjtime);
	case CLOCKCTL_CLOCK_SETTIME32:
		IOCTL_STRUCT_CONV_TO(CLOCKCTL_CLOCK_SETTIME,
		    clockctl_clock_settime);
	case CLOCKCTL_NTP_ADJTIME32:
		IOCTL_STRUCT_CONV_TO(CLOCKCTL_NTP_ADJTIME,
		    clockctl_ntp_adjtime);

	case KIOCGSYMBOL32:
		IOCTL_STRUCT_CONV_TO(KIOCGSYMBOL, ksyms_gsymbol);
	case KIOCGVALUE32:
		IOCTL_STRUCT_CONV_TO(KIOCGVALUE, ksyms_gvalue);

	case IOC_NPF_LOAD32:
		IOCTL_STRUCT_CONV_TO(IOC_NPF_LOAD, plistref);
	case IOC_NPF_TABLE32:
		IOCTL_STRUCT_CONV_TO(IOC_NPF_TABLE, npf_ioctl_table);
	case IOC_NPF_STATS32:
		IOCTL_CONV_TO(IOC_NPF_STATS, voidp);
	case IOC_NPF_SAVE32:
		IOCTL_STRUCT_CONV_TO(IOC_NPF_SAVE, plistref);
	case IOC_NPF_RULE32:
		IOCTL_STRUCT_CONV_TO(IOC_NPF_RULE, plistref);

	case DRVRESCANBUS32:
		IOCTL_STRUCT_CONV_TO(DRVRESCANBUS, devrescanargs);
	case DRVLISTDEV32:
		IOCTL_STRUCT_CONV_TO(DRVLISTDEV, devlistargs);
	case DRVCTLCOMMAND32:
		IOCTL_STRUCT_CONV_TO(DRVCTLCOMMAND, plistref);
	case DRVGETEVENT32:
		IOCTL_STRUCT_CONV_TO(DRVGETEVENT, plistref);

	default:
#ifdef NETBSD32_MD_IOCTL
		error = netbsd32_md_ioctl(fp, com, data32, l);
#else
		error = (*fp->f_ops->fo_ioctl)(fp, com, data32);
#endif
		break;
	}

	if (error == EPASSTHROUGH)
		error = ENOTTY;

	/*
	 * Copy any data to user, size was
	 * already set and checked above.
	 */
	if (error == 0 && (com&IOC_OUT) && size32) {
		error = copyout(data32, SCARG_P32(uap, data), size32);
		ktrgenio(fd, UIO_READ, SCARG_P32(uap, data),
		    size32, error);
	}

 out:
	/* If we allocated data, free it here. */
	if (memp32)
		kmem_free(memp32, alloc_size32);
	if (memp)
		kmem_free(memp, size);
	fd_putfile(fd);
	return (error);
}
