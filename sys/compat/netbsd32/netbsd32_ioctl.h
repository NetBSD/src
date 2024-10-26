/*	$NetBSD: netbsd32_ioctl.h,v 1.79.4.1 2024/10/26 15:53:47 martin Exp $	*/

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

#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>

#include <net/zlib.h>

#include <dev/dkvar.h>
#include <dev/vndvar.h>

#include <dev/lockstat.h>
#include <dev/wscons/wsconsio.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/ip_mroute.h>
#include <net80211/ieee80211_ioctl.h>

#include <compat/netbsd32/netbsd32.h>

/* we define some handy macros here... */
#define IOCTL_STRUCT_CONV_TO(cmd, type)	\
		size = IOCPARM_LEN(cmd); \
		if (size > sizeof(stkbuf)) \
			data = memp = kmem_alloc(size, KM_SLEEP); \
		else \
			data = (void *)stkbuf; \
		__CONCAT(netbsd32_to_, type)((struct __CONCAT(netbsd32_, type) *) \
			data32, (struct type *)data, cmd); \
		error = (*fp->f_ops->fo_ioctl)(fp, cmd, data); \
		__CONCAT(netbsd32_from_, type)((struct type *)data, \
			(struct __CONCAT(netbsd32_, type) *)data32, cmd); \
		break

#define IOCTL_CONV_TO(cmd, type)	\
		size = IOCPARM_LEN(cmd); \
		if (size > sizeof(stkbuf)) \
			data = memp = kmem_alloc(size, KM_SLEEP); \
		else \
			data = (void *)stkbuf; \
		__CONCAT(netbsd32_to_, type)((__CONCAT(netbsd32_, type) *) \
			data32, (type *)data, cmd); \
		error = (*fp->f_ops->fo_ioctl)(fp, cmd, data); \
		__CONCAT(netbsd32_from_, type)((type *)data, \
			(__CONCAT(netbsd32_, type) *)data32, cmd); \
		break

/* from <sys/audioio.h> */
#define AUDIO_WSEEK32	_IOR('A', 25, netbsd32_u_long)

/* from <sys/dkio.h> */
typedef netbsd32_pointer_t netbsd32_disklabel_tp_t;
typedef netbsd32_pointer_t netbsd32_partition_tp_t;

#if 0	/* not implemented by anything */
struct netbsd32_format_op {
	netbsd32_charp df_buf;
	int	 df_count;		/* value-result */
	daddr_t	 df_startblk;
	int	 df_reg[8];		/* result */
};
#define DIOCRFORMAT32	_IOWR('d', 105, struct netbsd32_format_op)
#define DIOCWFORMAT32	_IOWR('d', 106, struct netbsd32_format_op)
#endif

/* from <sys/event.h> */

struct netbsd32_kfilter_mapping {
	netbsd32_charp	name;		/* name to lookup or return */
	netbsd32_size_t	len;		/* length of name */
	uint32_t	filter;		/* filter to lookup or return */
};

/* map filter to name (max size len) */
#define KFILTER_BYFILTER32	_IOWR('k', 0, struct netbsd32_kfilter_mapping)
/* map name to filter (len ignored) */
#define KFILTER_BYNAME32	_IOWR('k', 1, struct netbsd32_kfilter_mapping)

/* from <sys/ataio.h> */
struct netbsd32_atareq {
	netbsd32_u_long		flags;
	u_char			command;
	u_char			features;
	u_char			sec_count;
	u_char			sec_num;
	u_char			head;
	u_short			cylinder;
	netbsd32_voidp		databuf;
	netbsd32_u_long		datalen;
	int			timeout;
	u_char			retsts;
	u_char			error;
};
#define ATAIOCCOMMAND32		_IOWR('Q', 8, struct netbsd32_atareq)


/* from <net/bpf.h> */
struct netbsd32_bpf_program {
	u_int bf_len;
	netbsd32_pointer_t bf_insns;
};

struct netbsd32_bpf_dltlist {
	u_int bfl_len;
	netbsd32_pointer_t bfl_list;
};

#define	BIOCSETF32	_IOW('B',103, struct netbsd32_bpf_program)
#define BIOCSTCPF32	_IOW('B',114, struct netbsd32_bpf_program)
#define BIOCSUDPF32	_IOW('B',115, struct netbsd32_bpf_program)
#define BIOCGDLTLIST32	_IOWR('B',119, struct netbsd32_bpf_dltlist)
#define BIOCSRTIMEOUT32	_IOW('B',122, struct netbsd32_timeval)
#define BIOCSETWF32	_IOW('B',127, struct netbsd32_bpf_program)


struct netbsd32_wsdisplay_addscreendata {
	int idx; /* screen index */
	netbsd32_charp screentype;
	netbsd32_charp emul;
};
#define	WSDISPLAYIO_ADDSCREEN32	_IOW('W', 78, struct netbsd32_wsdisplay_addscreendata)

/* the first member must be matched with struct ifreq */
struct netbsd32_ieee80211req {
	char		i_name[IFNAMSIZ];	/* if_name, e.g. "wi0" */
	uint16_t	i_type;			/* req type */
	int16_t		i_val;			/* Index or simple value */
	uint16_t	i_len;			/* Index or simple value */
	netbsd32_voidp	i_data;			/* Extra data */
};
#define SIOCS8021132			_IOW('i', 244, struct netbsd32_ieee80211req)
#define SIOCG8021132			_IOWR('i', 245, struct netbsd32_ieee80211req)

/* the first member must be matched with struct ifreq */
struct netbsd32_ieee80211_nwkey {
	char		i_name[IFNAMSIZ];	/* if_name, e.g. "wi0" */
	int		i_wepon;		/* wep enabled flag */
	int		i_defkid;		/* default encrypt key id */
	struct {
		int		i_keylen;
		netbsd32_charp	i_keydat;
	}		i_key[IEEE80211_WEP_NKID];
};
#define	SIOCS80211NWKEY32		 _IOW('i', 232, struct netbsd32_ieee80211_nwkey)
#define	SIOCG80211NWKEY32		_IOWR('i', 233, struct netbsd32_ieee80211_nwkey)

/* for powerd */
#define POWER_EVENT_RECVDICT32	_IOWR('P', 1, struct netbsd32_plistref)

/* Colormap operations.  Not applicable to all display types. */
struct netbsd32_wsdisplay_cmap {
	u_int	index;				/* first element (0 origin) */
	u_int	count;				/* number of elements */
	netbsd32_charp red;			/* red color map elements */
	netbsd32_charp green;			/* green color map elements */
	netbsd32_charp blue;			/* blue color map elements */
};

#define	WSDISPLAYIO_GETCMAP32	_IOW('W', 66, struct netbsd32_wsdisplay_cmap)
#define	WSDISPLAYIO_PUTCMAP32	_IOW('W', 67, struct netbsd32_wsdisplay_cmap)

struct netbsd32_wsdisplay_cursor {
	u_int	which;				/* values to get/set */
	u_int	enable;				/* enable/disable */
	struct wsdisplay_curpos pos;		/* position */
	struct wsdisplay_curpos hot;		/* hot spot */
	struct netbsd32_wsdisplay_cmap cmap;	/* color map info */
	struct wsdisplay_curpos size;		/* bit map size */
	netbsd32_charp image;			/* image data */
	netbsd32_charp mask;			/* mask data */
};

/* Cursor control: get/set cursor attributes/shape */
#define	WSDISPLAYIO_GCURSOR32	_IOWR('W', 73, struct netbsd32_wsdisplay_cursor)
#define	WSDISPLAYIO_SCURSOR32	_IOW('W', 74, struct netbsd32_wsdisplay_cursor)

struct netbsd32_wsdisplay_font {
	netbsd32_charp name;
	int firstchar, numchars;
	int encoding;
	u_int fontwidth, fontheight, stride;
	int bitorder, byteorder;
	netbsd32_charp data;
};
#define	WSDISPLAYIO_LDFONT32	_IOW('W', 77, struct netbsd32_wsdisplay_font)

struct netbsd32_wsdisplay_usefontdata {
	netbsd32_charp name;
};
#define	WSDISPLAYIO_SFONT32	_IOW('W', 80, struct netbsd32_wsdisplay_usefontdata)

/* can wait! */
#if 0
dev/ccdvar.h:219:#define CCDIOCSET	_IOWR('F', 16, struct ccd_ioctl)   /* enable ccd */
dev/ccdvar.h:220:#define CCDIOCCLR	_IOW('F', 17, struct ccd_ioctl)    /* disable ccd */

dev/md.h:45:#define MD_GETCONF	_IOR('r', 0, struct md_conf)	/* get unit config */
dev/md.h:46:#define MD_SETCONF	_IOW('r', 1, struct md_conf)	/* set unit config */

dev/wscons/wsconsio.h:133:#define WSKBDIO_GETMAP		_IOWR('W', 13, struct wskbd_map_data)
dev/wscons/wsconsio.h:134:#define WSKBDIO_SETMAP		_IOW('W', 14, struct wskbd_map_data)

dev/wscons/wsconsio.h:188:#define WSDISPLAYIO_GETCMAP	_IOW('W', 66, struct wsdisplay_cmap)
dev/wscons/wsconsio.h:189:#define WSDISPLAYIO_PUTCMAP	_IOW('W', 67, struct wsdisplay_cmap)

dev/wscons/wsconsio.h:241:#define WSDISPLAYIO_SFONT	_IOW('W', 77, struct wsdisplay_font)

net/if_ppp.h:110:#define PPPIOCSPASS	_IOW('t', 71, struct bpf_program) /* set pass filter */
net/if_ppp.h:111:#define PPPIOCSACTIVE	_IOW('t', 70, struct bpf_program) /* set active filt */

net/if_ppp.h:105:#define PPPIOCSCOMPRESS	_IOW('t', 77, struct ppp_option_data)

sys/module.h?

sys/rnd.h:186:#define RNDGETPOOL      _IOR('R',  103, u_char *)  /* get whole pool */

sys/scanio.h:86:#define SCIOCGET	_IOR('S', 1, struct scan_io) /* retrieve parameters */
sys/scanio.h:87:#define SCIOCSET	_IOW('S', 2, struct scan_io) /* set parameters */

sys/scsiio.h:43:#define SCIOCCOMMAND	_IOWR('Q', 1, scsireq_t)
#endif

/* from <net/if.h> */

typedef netbsd32_pointer_t netbsd32_ifreq_tp_t;
/*
 * note that ifr_data is the only one that needs to be changed
 */
struct	netbsd32_oifreq {
	char	ifr_name[IFNAMSIZ];		/* if name, e.g. "en0" */
	union {
		struct	sockaddr ifru_addr;
		struct	sockaddr ifru_dstaddr;
		struct	sockaddr ifru_broadaddr;
		short	ifru_flags;
		int	ifru_metric;
		int	ifru_mtu;
		int	ifru_dlt;
		u_int	ifru_value;
		netbsd32_caddr_t ifru_data;
		struct {
			uint32_t	b_buflen;
			netbsd32_caddr_t b_buf;
		} ifru_b;
	} ifr_ifru;
#define	ifr_addr	ifr_ifru.ifru_addr	/* address */
#define	ifr_dstaddr	ifr_ifru.ifru_dstaddr	/* other end of p-to-p link */
#define	ifr_broadaddr	ifr_ifru.ifru_broadaddr	/* broadcast address */
#define	ifr_flags	ifr_ifru.ifru_flags	/* flags */
#define	ifr_metric	ifr_ifru.ifru_metric	/* metric */
#define	ifr_mtu		ifr_ifru.ifru_mtu	/* mtu */
#define	ifr_media	ifr_ifru.ifru_metric	/* media options (overload) */
#define	ifr_data	ifr_ifru.ifru_data	/* for use by interface */
};
struct	netbsd32_ifreq {
	char	ifr_name[IFNAMSIZ];		/* if name, e.g. "en0" */
	union {
		struct	sockaddr ifru_addr;
		struct	sockaddr ifru_dstaddr;
		struct	sockaddr ifru_broadaddr;
		struct	sockaddr_storage ifru_space;
		short	ifru_flags;
		int	ifru_metric;
		int	ifru_mtu;
		int	ifru_dlt;
		u_int	ifru_value;
		netbsd32_caddr_t ifru_data;
		struct {
			uint32_t	b_buflen;
			netbsd32_caddr_t b_buf;
		} ifru_b;
	} ifr_ifru;
};

struct netbsd32_if_addrprefreq {
	char			ifap_name[IFNAMSIZ];
	uint16_t		ifap_preference;
	struct {
		__uint8_t	ss_len;         /* address length */
		sa_family_t	ss_family;      /* address family */
		char		__ss_pad1[_SS_PAD1SIZE];
		__int32_t	__ss_align[2];
		char		__ss_pad2[_SS_PAD2SIZE];
	} ifap_addr;
};

struct netbsd32_if_clonereq {
	int	ifcr_total;
	int	ifcr_count;
	netbsd32_charp ifcr_buffer;
};

struct netbsd32_if_data {
	u_char	ifi_type;
	u_char	ifi_addrlen;
	u_char	ifi_hdrlen;
	u_char  __pack_dummy;
	int	ifi_link_state;
	uint64_t ifi_mtu;
	uint64_t ifi_metric;
	uint64_t ifi_baudrate;
	uint64_t ifi_ipackets;
	uint64_t ifi_ierrors;
	uint64_t ifi_opackets;
	uint64_t ifi_oerrors;
	uint64_t ifi_collisions;
	uint64_t ifi_ibytes;
	uint64_t ifi_obytes;
	uint64_t ifi_imcasts;
	uint64_t ifi_omcasts;
	uint64_t ifi_iqdrops;
	uint64_t ifi_noproto;
	struct	netbsd32_timespec ifi_lastchange;
} __packed;

struct netbsd32_ifdatareq {
	char	ifdr_name[IFNAMSIZ];		/* if name, e.g. "en0" */
	struct	netbsd32_if_data ifdr_data;
};


/* from <dev/pci/if_devar.h> */
#define	SIOCGADDRROM32		_IOW('i', 240, struct netbsd32_ifreq)	/* get 128 bytes of ROM */
#define	SIOCGCHIPID32		_IOWR('i', 241, struct netbsd32_ifreq)	/* get chipid */
/* from <sys/sockio.h> */
#define	SIOCSIFADDR32	 _IOW('i', 12, struct netbsd32_ifreq)	/* set ifnet address */
#define	OSIOCSIFADDR32	 _IOW('i', 12, struct netbsd32_oifreq)	/* set ifnet address */
#define	OOSIOCGIFADDR32	_IOWR('i', 13, struct netbsd32_oifreq)	/* get ifnet address */

#define	SIOCGIFADDR32	_IOWR('i', 33, struct netbsd32_ifreq)	/* get ifnet address */
#define	OSIOCGIFADDR32	_IOWR('i', 33, struct netbsd32_oifreq)	/* get ifnet address */

#define	SIOCSIFDSTADDR32	 _IOW('i', 14, struct netbsd32_ifreq)	/* set p-p address */
#define	OSIOCSIFDSTADDR32	 _IOW('i', 14, struct netbsd32_oifreq)	/* set p-p address */
#define	OOSIOCGIFDSTADDR32	_IOWR('i', 15, struct netbsd32_oifreq)	/* get p-p address */

#define	SIOCGIFDSTADDR32	_IOWR('i', 34, struct netbsd32_ifreq)	/* get p-p address */
#define	OSIOCGIFDSTADDR32	_IOWR('i', 34, struct netbsd32_oifreq)	/* get p-p address */

#define	SIOCSIFFLAGS32	 _IOW('i', 16, struct netbsd32_ifreq)	/* set ifnet flags */
#define	OSIOCSIFFLAGS32	 _IOW('i', 16, struct netbsd32_oifreq)	/* set ifnet flags */

#define	SIOCGIFFLAGS32	_IOWR('i', 17, struct netbsd32_ifreq)	/* get ifnet flags */
#define	OSIOCGIFFLAGS32	_IOWR('i', 17, struct netbsd32_oifreq)	/* get ifnet flags */


#define	SIOCSIFBRDADDR32	 _IOW('i', 19, struct netbsd32_ifreq)	/* set broadcast addr */
#define	OSIOCSIFBRDADDR32	 _IOW('i', 19, struct netbsd32_oifreq)	/* set broadcast addr */
#define	OOSIOCGIFBRDADDR32	_IOWR('i', 18, struct netbsd32_oifreq)	/* get broadcast addr */

#define	SIOCGIFBRDADDR32	_IOWR('i', 35, struct netbsd32_ifreq)	/* get broadcast addr */
#define	OSIOCGIFBRDADDR32	_IOWR('i', 35, struct netbsd32_oifreq)	/* get broadcast addr */

#define	OOSIOCGIFNETMASK32	_IOWR('i', 21, struct netbsd32_oifreq)	/* get net addr mask */

#define	SIOCGIFNETMASK32	_IOWR('i', 37, struct netbsd32_ifreq)	/* get net addr mask */
#define	OSIOCGIFNETMASK32	_IOWR('i', 37, struct netbsd32_oifreq)	/* get net addr mask */

#define	SIOCSIFNETMASK32	 _IOW('i', 22, struct netbsd32_ifreq)	/* set net addr mask */
#define	OSIOCSIFNETMASK32	 _IOW('i', 22, struct netbsd32_oifreq)	/* set net addr mask */

#define	SIOCGIFMETRIC32	_IOWR('i', 23, struct netbsd32_ifreq)	/* get IF metric */
#define	OSIOCGIFMETRIC32	_IOWR('i', 23, struct netbsd32_oifreq)	/* get IF metric */

#define	SIOCSIFMETRIC32	 _IOW('i', 24, struct netbsd32_ifreq)	/* set IF metric */
#define	OSIOCSIFMETRIC32	 _IOW('i', 24, struct netbsd32_oifreq)	/* set IF metric */

#define	SIOCDIFADDR32	 _IOW('i', 25, struct netbsd32_ifreq)	/* delete IF addr */
#define	OSIOCDIFADDR32	 _IOW('i', 25, struct netbsd32_oifreq)	/* delete IF addr */

#define SIOCSIFADDRPREF32	 _IOW('i', 31, struct netbsd32_if_addrprefreq)
#define SIOCGIFADDRPREF32	_IOWR('i', 32, struct netbsd32_if_addrprefreq)

#define	SIOCADDMULTI32	 _IOW('i', 49, struct netbsd32_ifreq)	/* add m'cast addr */
#define	OSIOCADDMULTI32	 _IOW('i', 49, struct netbsd32_oifreq)	/* add m'cast addr */

#define	SIOCDELMULTI32	 _IOW('i', 50, struct netbsd32_ifreq)	/* del m'cast addr */
#define	OSIOCDELMULTI32	 _IOW('i', 50, struct netbsd32_oifreq)	/* del m'cast addr */

#define	SIOCSIFMEDIA32_80	_IOWR('i', 53, struct netbsd32_ifreq)	/* set net media */
#define	SIOCSIFMEDIA32_43	_IOWR('i', 53, struct netbsd32_oifreq)	/* set net media */
#define	SIOCSIFMEDIA32		_IOWR('i', 55, struct netbsd32_ifreq)	/* set net media */

#define	SIOCSIFGENERIC32 _IOW('i', 57, struct netbsd32_ifreq)	/* generic IF set op */
#define	SIOCGIFGENERIC32 _IOWR('i', 58, struct netbsd32_ifreq)	/* generic IF get op */

#define	SIOCIFGCLONERS32 _IOWR('i', 120, struct netbsd32_if_clonereq) /* get cloners */

#define	SIOCSIFMTU32	 _IOW('i', 127, struct netbsd32_ifreq)	/* set ifnet mtu */
#define	OSIOCSIFMTU32	 _IOW('i', 127, struct netbsd32_oifreq)	/* set ifnet mtu */

#define	SIOCGIFMTU32	_IOWR('i', 126, struct netbsd32_ifreq)	/* get ifnet mtu */
#define	OSIOCGIFMTU32	_IOWR('i', 126, struct netbsd32_oifreq)	/* get ifnet mtu */

#define SIOCGIFDATA32	_IOWR('i', 133, struct netbsd32_ifdatareq)
#define SIOCZIFDATA32	_IOWR('i', 134, struct netbsd32_ifdatareq)

/* was 125 SIOCSIFASYNCMAP32 */
/* was 124 SIOCGIFASYNCMAP32 */
/* from <net/bpf.h> */
#define BIOCGETIF32	_IOR('B',107, struct netbsd32_ifreq)
#define BIOCSETIF32	_IOW('B',108, struct netbsd32_ifreq)
/* from <netatalk/phase2.h> */
#define SIOCPHASE1_32	_IOW('i', 100, struct netbsd32_ifreq)	/* AppleTalk phase 1 */
#define SIOCPHASE2_32	_IOW('i', 101, struct netbsd32_ifreq)	/* AppleTalk phase 2 */

/* from <net/if.h> */
struct	netbsd32_ifconf {
	int	ifc_len;		/* size of associated buffer */
	union {
		netbsd32_caddr_t ifcu_buf;
		netbsd32_ifreq_tp_t ifcu_req;
	} ifc_ifcu;
#define	ifc_buf	ifc_ifcu.ifcu_buf	/* buffer address */
#define	ifc_req	ifc_ifcu.ifcu_req	/* array of structures returned */
};
/* from <sys/sockio.h> */
#define	OOSIOCGIFCONF32	_IOWR('i', 20, struct netbsd32_ifconf)	/* get ifnet list */
#define	OSIOCGIFCONF32	_IOWR('i', 36, struct netbsd32_ifconf)	/* get ifnet list */
#define	SIOCGIFCONF32	_IOWR('i', 38, struct netbsd32_ifconf)	/* get ifnet list */

/* from <net/if.h> */
struct netbsd32_ifmediareq {
	char	ifm_name[IFNAMSIZ];		/* if name, e.g. "en0" */
	int	ifm_current;			/* current media options */
	int	ifm_mask;			/* don't care mask */
	int	ifm_status;			/* media status */
	int	ifm_active;			/* active options */
	int	ifm_count;			/* # entries in ifm_ulist
						   array */
	netbsd32_intp	ifm_ulist;		/* media words */
};
/* from <sys/sockio.h> */
#define	SIOCGIFMEDIA32_80 _IOWR('i', 54, struct netbsd32_ifmediareq) /* get net media */
#define	SIOCGIFMEDIA32	_IOWR('i', 56, struct netbsd32_ifmediareq) /* get net media */

/* from net/if_pppoe.h */
struct netbsd32_pppoediscparms {
	char	ifname[IFNAMSIZ];	/* pppoe interface name */
	char	eth_ifname[IFNAMSIZ];	/* external ethernet interface name */
	netbsd32_charp ac_name;		/* access concentrator name (or NULL) */
	netbsd32_size_t	ac_name_len;		/* on write: length of buffer for ac_name */
	netbsd32_charp service_name;	/* service name (or NULL) */
	netbsd32_size_t	service_name_len;	/* on write: length of buffer for service name */
};
#define	PPPOESETPARMS32	_IOW('i', 110, struct netbsd32_pppoediscparms)
#define	PPPOEGETPARMS32	_IOWR('i', 111, struct netbsd32_pppoediscparms)

/* from net/if_sppp.h */
struct netbsd32_spppauthcfg {
	char	ifname[IFNAMSIZ];	/* pppoe interface name */
	u_int	hisauth;		/* one of SPPP_AUTHPROTO_* above */
	u_int	myauth;			/* one of SPPP_AUTHPROTO_* above */
	u_int	myname_length;		/* includes terminating 0 */
	u_int	mysecret_length;	/* includes terminating 0 */
	u_int	hisname_length;		/* includes terminating 0 */
	u_int	hissecret_length;	/* includes terminating 0 */
	u_int	myauthflags;
	u_int	hisauthflags;
	netbsd32_charp	myname;
	netbsd32_charp	mysecret;
	netbsd32_charp	hisname;
	netbsd32_charp	hissecret;
};
#define SPPPGETAUTHCFG32 _IOWR('i', 120, struct netbsd32_spppauthcfg)
#define SPPPSETAUTHCFG32 _IOW('i', 121, struct netbsd32_spppauthcfg)

/* from <net/if.h> */
struct  netbsd32_ifdrv {
	char		ifd_name[IFNAMSIZ];	/* if name, e.g. "en0" */
	netbsd32_u_long	ifd_cmd;
	netbsd32_size_t	ifd_len;
	netbsd32_voidp	ifd_data;
};
/* from <sys/sockio.h> */
#define SIOCSDRVSPEC32	_IOW('i', 123, struct netbsd32_ifdrv)	/* set driver-specific */
#define SIOCGDRVSPEC32	_IOWR('i', 123, struct netbsd32_ifdrv)	/* get driver-specific */

/* from <netinet/ip_mroute.h> */
struct netbsd32_sioc_vif_req {
	vifi_t	vifi;			/* vif number */
	netbsd32_u_long	icount;		/* input packet count on vif */
	netbsd32_u_long	ocount;		/* output packet count on vif */
	netbsd32_u_long	ibytes;		/* input byte count on vif */
	netbsd32_u_long	obytes;		/* output byte count on vif */
};
/* from <sys/sockio.h> */
#define	SIOCGETVIFCNT32	_IOWR('u', 51, struct netbsd32_sioc_vif_req)/* vif pkt cnt */

/* from <netinet/if_arp.h> */
struct netbsd32_in_nbrinfo {
	char ifname[IFNAMSIZ];	/* if name, e.g. "en0" */
	struct in_addr	addr;	/* IPv4 address of the neighbor */
	netbsd32_long	asked;	/* number of queries already sent for this addr */
	int	state;		/* reachability state */
	int	expire;		/* lifetime for NDP state transition */
};
/* from <sys/sockio.h> */
#define	SIOCGNBRINFO32		_IOWR('i', 249, struct netbsd32_in_nbrinfo)

/* from <netinet6/nd6.h> */
struct netbsd32_in6_nbrinfo {
	char ifname[IFNAMSIZ];	/* if name, e.g. "en0" */
	struct in6_addr addr;	/* IPv6 address of the neighbor */
	netbsd32_long	asked;	/* number of queries already sent for this addr */
	int	isrouter;	/* if it acts as a router */
	int	state;		/* reachability state */
	int	expire;		/* lifetime for NDP state transition */
};
/* from <netinet6/in6_var.h> */
#define SIOCGNBRINFO_IN632	_IOWR('i', 78, struct netbsd32_in6_nbrinfo)

struct netbsd32_sioc_sg_req {
	struct	in_addr src;
	struct	in_addr grp;
	netbsd32_u_long	pktcnt;
	netbsd32_u_long	bytecnt;
	netbsd32_u_long	wrong_if;
};
/* from <sys/sockio.h> */
#define	SIOCGETSGCNT32	_IOWR('u', 52, struct netbsd32_sioc_sg_req) /* sg pkt cnt */

/* from <dev/ffsvar.h> */
struct netbsd32_fss_set {
	netbsd32_charp	fss_mount;	/* Mount point of file system */
	netbsd32_charp	fss_bstore;	/* Path of backing store */
	blksize_t	fss_csize;	/* Preferred cluster size */
	int		fss_flags;	/* Initial flags */
};

struct netbsd32_fss_get {
	char		fsg_mount[MNAMELEN]; /* Mount point of file system */
	struct netbsd32_timeval	fsg_time;	/* Time this snapshot was taken */
	blksize_t	fsg_csize;	/* Current cluster size */
	netbsd32_blkcnt_t	fsg_mount_size;	/* # clusters on file system */
	netbsd32_blkcnt_t	fsg_bs_size;	/* # clusters on backing store */
};

/* XXX: FSSIOCSET50 and FSSIOCGET50 are not (yet) handled */
#define FSSIOCSET32	_IOW('F', 5, struct netbsd32_fss_set)	/* Configure */
#define FSSIOCGET32	_IOR('F', 1, struct netbsd32_fss_get)	/* Status */

struct netbsd32_vnd_ioctl {
	netbsd32_charp	vnd_file;	/* pathname of file to mount */
	int		vnd_flags;	/* flags; see below */
	struct vndgeom	vnd_geom;	/* geometry to emulate */
	unsigned int	vnd_osize;	/* (returned) size of disk */
	netbsd32_uint64	vnd_size;	/* (returned) size of disk */
};

struct netbsd32_vnd_user {
	int		vnu_unit;	/* which vnd unit */
	netbsd32_dev_t	vnu_dev;	/* file is on this device... */
	netbsd32_ino_t	vnu_ino;	/* ...at this inode */
};

/* from <dev/vndvar.h> */
#define VNDIOCSET32	_IOWR('F', 0, struct netbsd32_vnd_ioctl)	/* enable disk */
#define VNDIOCCLR32	_IOW('F', 1, struct netbsd32_vnd_ioctl)	/* disable disk */
#define VNDIOCGET32	_IOWR('F', 3, struct netbsd32_vnd_user)	/* get list */

struct netbsd32_vnd_ioctl50 {
	netbsd32_charp	vnd_file;	/* pathname of file to mount */
	int		vnd_flags;	/* flags; see below */
	struct vndgeom	vnd_geom;	/* geometry to emulate */
	unsigned int	vnd_size;	/* (returned) size of disk */
} __packed;
/* from <dev/vnd.c> */
#define VNDIOCSET5032	_IOWR('F', 0, struct netbsd32_vnd_ioctl50)
#define VNDIOCCLR5032	_IOW('F', 1, struct netbsd32_vnd_ioctl50)

#define ENVSYS_GETDICTIONARY32	_IOWR('E', 0, struct netbsd32_plistref)
#define ENVSYS_SETDICTIONARY32	_IOWR('E', 1, struct netbsd32_plistref)
#define ENVSYS_REMOVEPROPS32	_IOWR('E', 2, struct netbsd32_plistref)

/* from <sys/wdog.h> */
struct netbsd32_wdog_conf {
	netbsd32_charp	wc_names;
	int		wc_count;
};
#define WDOGIOC_GWDOGS32	_IOWR('w', 5, struct netbsd32_wdog_conf)


struct netbsd32_clockctl_settimeofday {
	netbsd32_timevalp_t tv;
	netbsd32_voidp tzp;
};

#define CLOCKCTL_SETTIMEOFDAY32 _IOW('C', 0x5, \
    struct netbsd32_clockctl_settimeofday)

struct netbsd32_clockctl_adjtime {
	netbsd32_timevalp_t delta;
	netbsd32_timevalp_t olddelta;
};

#define CLOCKCTL_ADJTIME32 _IOWR('C', 0x6, struct netbsd32_clockctl_adjtime)

struct netbsd32_clockctl_clock_settime {
	netbsd32_clockid_t clock_id;
	netbsd32_timespecp_t tp;
};

#define CLOCKCTL_CLOCK_SETTIME32 _IOW('C', 0x7, \
    struct netbsd32_clockctl_clock_settime)

struct netbsd32_clockctl_ntp_adjtime {
	netbsd32_timexp_t tp;
	register32_t retval;
};

#define CLOCKCTL_NTP_ADJTIME32 _IOWR('C', 0x8, \
    struct netbsd32_clockctl_ntp_adjtime)

#ifdef KIOCGSYMBOL
struct netbsd32_ksyms_gsymbol {
	netbsd32_charp kg_name;
	union {
		Elf_Sym ku_sym;
	} _un;
};

struct netbsd32_ksyms_gvalue {
	netbsd32_charp kv_name;
	uint64_t kv_value;
};

#define	KIOCGVALUE32	_IOWR('l', 4, struct netbsd32_ksyms_gvalue)
#define	KIOCGSYMBOL32	_IOWR('l', 5, struct netbsd32_ksyms_gsymbol)
#endif /* KIOCGSYMBOL */

#include <net/npf/npf.h>

typedef struct netbsd32_npf_ioctl_buf {
	netbsd32_voidp		buf;
	netbsd32_size_t		len;
} netbsd32_npf_ioctl_buf_t;

typedef struct netbsd32_npf_ioctl_table {
        int			nct_cmd;  
        netbsd32_charp		nct_name;
        union {
		npf_ioctl_ent_t ent;
		netbsd32_npf_ioctl_buf_t buf;
        } nct_data;
} netbsd32_npf_ioctl_table_t;

#define IOC_NPF_LOAD32		_IOWR('N', 102, netbsd32_nvlist_ref_t)
#define IOC_NPF_TABLE32		_IOW('N', 103, struct netbsd32_npf_ioctl_table)
#define IOC_NPF_STATS32		_IOW('N', 104, netbsd32_voidp)
#define IOC_NPF_SAVE32		_IOR('N', 105, netbsd32_nvlist_ref_t)
#define IOC_NPF_RULE32		_IOWR('N', 107, netbsd32_nvlist_ref_t)
#define IOC_NPF_CONN_LOOKUP32	_IOWR('N', 108, netbsd32_nvlist_ref_t)

/* From sys/drvctlio.h */
struct netbsd32_devlistargs {
	char			l_devname[16];
	netbsd32_charpp		l_childname;
	netbsd32_size_t		l_children;
};

struct netbsd32_devrescanargs {
	char			busname[16];
	char			ifattr[16];
	unsigned int		numlocators;
	netbsd32_intp		locators;
};

#define	DRVRESCANBUS32		_IOW('D', 124, struct netbsd32_devrescanargs)
#define DRVCTLCOMMAND32		_IOWR('D', 125, struct netbsd32_plistref)
#define	DRVLISTDEV32		_IOWR('D', 127, struct netbsd32_devlistargs)
#define DRVGETEVENT32		_IOR('D', 128, struct netbsd32_plistref)

/* From sys/disk.h, sys/dkio.h */

struct netbsd32_dkwedge_list {
	netbsd32_voidp		dkwl_buf; /* storage for dkwedge_info array */
	netbsd32_size_t		dkwl_bufsize;	/* size	of that	buffer */
	u_int			dkwl_nwedges;	/* total number	of wedges */
	u_int			dkwl_ncopied;	/* number actually copied */
} __packed;

#define DIOCLWEDGES32		_IOWR('d', 124, struct netbsd32_dkwedge_list)

struct netbsd32_disk_strategy {
	char dks_name[DK_STRATEGYNAMELEN];	/* name of strategy */
	netbsd32_charp dks_param;		/* notyet; should be NULL */
	netbsd32_size_t dks_paramlen;		/* notyet; should be 0 */
} __packed;

#define DIOCGSTRATEGY32		_IOR('d', 125, struct netbsd32_disk_strategy)
#define DIOCSSTRATEGY32		_IOW('d', 126, struct netbsd32_disk_strategy)
#define DIOCGDISKINFO32		_IOR('d', 127, struct netbsd32_plistref)

/* from <dev/lockstat.h> */
struct netbsd32_lsenable {
	netbsd32_uintptr_t	le_csstart;	/* callsite start */
	netbsd32_uintptr_t	le_csend;	/* callsite end */
	netbsd32_uintptr_t	le_lockstart;	/* lock address start */
	netbsd32_uintptr_t	le_lockend;	/* lock address end */
	netbsd32_uintptr_t	le_nbufs;	/* buffers to allocate, 0 = default */
	u_int			le_flags;	/* request flags */
	u_int			le_mask;	/* event mask (LB_*) */
};

struct netbsd32_lsdisable {
	netbsd32_size_t		ld_size;	/* buffer space allocated */
	struct netbsd32_timespec ld_time;	/* time spent enabled */
	uint64_t		ld_freq[64];	/* counter HZ by CPU number */
};

#define	IOC_LOCKSTAT_ENABLE32	_IOW('L', 1, struct netbsd32_lsenable)
#define	IOC_LOCKSTAT_DISABLE32	_IOR('L', 2, struct netbsd32_lsdisable)

int	netbsd32_drm_ioctl(struct file *, unsigned long, void *, struct lwp *);
