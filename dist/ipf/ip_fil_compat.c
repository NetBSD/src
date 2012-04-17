/*	$NetBSD$	*/

/*
 * Copyright (C) 2010 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 */
#if defined(KERNEL) || defined(_KERNEL)
# undef KERNEL
# undef _KERNEL
# define        KERNEL	1
# define        _KERNEL	1
#endif
#if defined(__osf__)
# define _PROTO_NET_H_
#endif
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#if __FreeBSD_version >= 220000 && defined(_KERNEL)
# include <sys/fcntl.h>
# include <sys/filio.h>
#else
# include <sys/ioctl.h>
#endif
#if !defined(_KERNEL)
# include <string.h>
# define _KERNEL
# ifdef __OpenBSD__
struct file;
# endif
# include <sys/uio.h>
# undef _KERNEL
#endif
#include <sys/socket.h>
#if (defined(__osf__) || defined(AIX) || defined(__hpux) || defined(__sgi)) && defined(_KERNEL)
# include "radix_ipf_local.h"
# define _RADIX_H_
#endif
#include <net/if.h>
#if defined(__FreeBSD__)
#  include <sys/cdefs.h>
#  include <sys/proc.h>
#endif
#if defined(_KERNEL)
# include <sys/systm.h>
# if !defined(__SVR4) && !defined(__svr4__)
#  include <sys/mbuf.h>
# endif
#endif
#include <netinet/in.h>

#include "netinet/ip_compat.h"
#include "netinet/ip_fil.h"
#include "netinet/ip_pool.h"
#include "netinet/ip_htable.h"
#include "netinet/ip_lookup.h"
#include "netinet/ip_nat.h"
#include "netinet/ip_state.h"
#include "netinet/ip_proxy.h"
#include "netinet/ip_auth.h"
/* END OF INCLUDES */

/*
 * NetBSD has moved to 64bit time_t for all architectures.
 * For some, such as sparc64, there is no change because long is already
 * 64bit, but for others (i386), there is...
 */
#ifdef IPFILTER_COMPAT

# ifdef __NetBSD__
typedef struct timeval_l {
	long	tv_sec;
	long	tv_usec;
} timeval_l_t;
# endif

/* ------------------------------------------------------------------------ */

/*
 * 4.1.34 changed the size of the time structure used for pps (current)
 * 4.1.16 moved the location of fr_flineno
 * 4.1.0 base version
 */
typedef	struct	frentry_4_1_16 {
	ipfmutex_t	fr_lock;
	struct	frentry	*fr_next;
	struct	frentry	**fr_grp;
	struct	ipscan	*fr_isc;
	void	*fr_ifas[4];
	void	*fr_ptr;
	char	*fr_comment;
	int	fr_ref;
	int	fr_statecnt;
	int	fr_flineno;
	U_QUAD_T	fr_hits;
	U_QUAD_T	fr_bytes;
	union {
#ifdef __NetBSD__
		timeval_l_t	frp_lastpkt;
#else
		struct timeval	frp_lastpkt;
#endif
	} fr_lpu;
	int		fr_curpps;
	union	{
		void		*fru_data;
		caddr_t		fru_caddr;
		fripf_t		*fru_ipf;
		frentfunc_t	fru_func;
	} fr_dun;
	ipfunc_t fr_func;
	int	fr_dsize;
	int	fr_pps;
	int	fr_statemax;
	u_32_t	fr_type;
	u_32_t	fr_flags;
	u_32_t	fr_logtag;
	u_32_t	fr_collect;
	u_int	fr_arg;
	u_int	fr_loglevel;
	u_int	fr_age[2];
	u_char	fr_v;
	u_char	fr_icode;
	char	fr_group[FR_GROUPLEN];
	char	fr_grhead[FR_GROUPLEN];
	ipftag_t fr_nattag;
	char	fr_ifnames[4][LIFNAMSIZ];
	char	fr_isctag[16];
	frdest_t fr_tifs[2];
	frdest_t fr_dif;
	u_int	fr_cksum;
} frentry_4_1_16_t;

typedef	struct	frentry_4_1_0 {
	ipfmutex_t	fr_lock;
	struct	frentry	*fr_next;
	struct	frentry	**fr_grp;
	struct	ipscan	*fr_isc;
	void	*fr_ifas[4];
	void	*fr_ptr;
	char	*fr_comment;
	int	fr_ref;
	int	fr_statecnt;
	U_QUAD_T	fr_hits;
	U_QUAD_T	fr_bytes;
	union {
#ifdef __NetBSD__
		timeval_l_t	frp_lastpkt;
#else
		struct timeval	frp_lastpkt;
#endif
	} fr_lpu;
	int		fr_curpps;

	union	{
		void		*fru_data;
		caddr_t		fru_caddr;
		fripf_t		*fru_ipf;
		frentfunc_t	fru_func;
	} fr_dun;
	/*
	 * Fields after this may not change whilst in the kernel.
	 */
	ipfunc_t fr_func;
	int	fr_dsize;
	int	fr_pps;
	int	fr_statemax;
	int	fr_flineno;
	u_32_t	fr_type;
	u_32_t	fr_flags;
	u_32_t	fr_logtag;
	u_32_t	fr_collect;
	u_int	fr_arg;
	u_int	fr_loglevel;
	u_int	fr_age[2];
	u_char	fr_v;
	u_char	fr_icode;
	char	fr_group[FR_GROUPLEN];
	char	fr_grhead[FR_GROUPLEN];
	ipftag_t fr_nattag;
	char	fr_ifnames[4][LIFNAMSIZ];
	char	fr_isctag[16];
	frdest_t fr_tifs[2];
	frdest_t fr_dif;
	u_int	fr_cksum;
} frentry_4_1_0_t;

/* ------------------------------------------------------------------------ */

/*
 * 4.1.32 removed both fin_state and fin_nat, added fin_pktnum (current)
 * 4.1.24 added fin_cksum
 * 4.1.23 added fin_exthdr
 * 4.1.11 added fin_ifname
 * 4.1.4  added fin_hbuf
 */
typedef struct  fr_info_4_1_24 {
	void    *fin_ifp;
	fr_ip_t fin_fi;
	union   {
	        u_short fid_16[2];
	        u_32_t  fid_32;
	} fin_dat;
	int     fin_out;
	int     fin_rev;
	u_short fin_hlen;
	u_char  fin_tcpf;
	u_char  fin_icode;
	u_32_t  fin_rule;
	char    fin_group[FR_GROUPLEN];
	struct  frentry *fin_fr;
	void    *fin_dp;
	int     fin_dlen;
	int     fin_plen;
	int     fin_ipoff;
	u_short fin_id;
	u_short fin_off;
	int     fin_depth;
	int     fin_error;
	int     fin_cksum;
	void	*fin_state;
	void	*fin_nat;
	void    *fin_nattag;
	void    *fin_exthdr;
	ip_t    *fin_ip;
	mb_t    **fin_mp;
	mb_t    *fin_m;
#ifdef  MENTAT
	mb_t    *fin_qfm;
	void    *fin_qpi;
	char    fin_ifname[LIFNAMSIZ];
#endif
#ifdef  __sgi
	void    *fin_hbuf;
#endif
} fr_info_4_1_24_t;

typedef struct  fr_info_4_1_23 {
	void    *fin_ifp;
	fr_ip_t fin_fi;
	union   {
	        u_short fid_16[2];
	        u_32_t  fid_32;
	} fin_dat;
	int     fin_out;
	int     fin_rev;
	u_short fin_hlen;
	u_char  fin_tcpf;
	u_char  fin_icode;
	u_32_t  fin_rule;
	char    fin_group[FR_GROUPLEN];
	struct  frentry *fin_fr;
	void    *fin_dp;
	int     fin_dlen;
	int     fin_plen;
	int     fin_ipoff;
	u_short fin_id;
	u_short fin_off;
	int     fin_depth;
	int     fin_error;
	void	*fin_state;
	void	*fin_nat;
	void    *fin_nattag;
	void    *fin_exthdr;
	ip_t    *fin_ip;
	mb_t    **fin_mp;
	mb_t    *fin_m;
#ifdef  MENTAT
	mb_t    *fin_qfm;
	void    *fin_qpi;
	char    fin_ifname[LIFNAMSIZ];
#endif
#ifdef  __sgi
	void    *fin_hbuf;
#endif
} fr_info_4_1_23_t;

typedef struct  fr_info_4_1_11 {
	void    *fin_ifp;
	fr_ip_t fin_fi;
	union   {
	        u_short fid_16[2];
	        u_32_t  fid_32;
	} fin_dat;
	int     fin_out;
	int     fin_rev;
	u_short fin_hlen;
	u_char  fin_tcpf;
	u_char  fin_icode;
	u_32_t  fin_rule;
	char    fin_group[FR_GROUPLEN];
	struct  frentry *fin_fr;
	void    *fin_dp;
	int     fin_dlen;
	int     fin_plen;
	int     fin_ipoff;
	u_short fin_id;
	u_short fin_off;
	int     fin_depth;
	int     fin_error;
	void	*fin_state;
	void	*fin_nat;
	void    *fin_nattag;
	ip_t    *fin_ip;
	mb_t    **fin_mp;
	mb_t    *fin_m;
#ifdef  MENTAT
	mb_t    *fin_qfm;
	void    *fin_qpi;
	char    fin_ifname[LIFNAMSIZ];
#endif
#ifdef  __sgi
	void    *fin_hbuf;
#endif
} fr_info_4_1_11_t;

/* ------------------------------------------------------------------------ */

/*
 * 4.1.33 changed the size of f_locks from IPL_LOGMAX to IPL_LOGSIZE (current)
 */

typedef struct  friostat_4_1_0       {
	struct  filterstats     f_st[2];
	struct  frentry         *f_ipf[2][2];
	struct  frentry         *f_acct[2][2];
	struct  frentry         *f_ipf6[2][2];
	struct  frentry         *f_acct6[2][2];
	struct  frentry         *f_auth;
	struct  frgroup         *f_groups[IPL_LOGSIZE][2];
	u_long  f_froute[2];
	u_long  f_ticks;
	int     f_locks[IPL_LOGMAX];
	size_t  f_kmutex_sz;
	size_t  f_krwlock_sz;
	int     f_defpass;
	int     f_active;
	int     f_running;
	int     f_logging;
	int     f_features;
	char    f_version[32];
} friostat_4_1_0_t;

/*
 * 4.1.14 added in_lock (current)
 */
typedef	struct	ipnat_4_1_0	{
	struct	ipnat	*in_next;
	struct	ipnat	*in_rnext;
	struct	ipnat	**in_prnext;
	struct	ipnat	*in_mnext;
	struct	ipnat	**in_pmnext;
	struct	ipftq	*in_tqehead[2];
	void		*in_ifps[2];
	void		*in_apr;
	char		*in_comment;
	i6addr_t	in_next6;
	u_long		in_space;
	u_long		in_hits;
	u_int		in_use;
	u_int		in_hv;
	int		in_flineno;
	u_short		in_pnext;
	u_char		in_v;
	u_char		in_xxx;
	u_32_t		in_flags;
	u_32_t		in_mssclamp;
	u_int		in_age[2];
	int		in_redir;
	int		in_p;
	i6addr_t	in_in[2];
	i6addr_t	in_out[2];
	i6addr_t	in_src[2];
	frtuc_t		in_tuc;
	u_short		in_port[2];
	u_short		in_ppip;
	u_short		in_ippip;
	char		in_ifnames[2][LIFNAMSIZ];
	char		in_plabel[APR_LABELLEN];
	ipftag_t	in_tag;
} ipnat_4_1_0_t;

/* ------------------------------------------------------------------------ */

/*
 * 4.1.25 added nat_seqnext (current)
 * 4.1.14 added nat_redir
 * 4.1.3  moved nat_rev
 * 4.1.2  added nat_rev
 */
typedef	struct	nat_4_1_14	{
	ipfmutex_t	nat_lock;
	struct	nat	*nat_next;
	struct	nat	**nat_pnext;
	struct	nat	*nat_hnext[2];
	struct	nat	**nat_phnext[2];
	struct	hostmap	*nat_hm;
	void		*nat_data;
	struct	nat	**nat_me;
	struct	ipstate	*nat_state;
	struct	ap_session	*nat_aps;
	frentry_t	*nat_fr;
	struct	ipnat	*nat_ptr;
	void		*nat_ifps[2];
	void		*nat_sync;
	ipftqent_t	nat_tqe;
	u_32_t		nat_flags;
	u_32_t		nat_sumd[2];
	u_32_t		nat_ipsumd;
	u_32_t		nat_mssclamp;
	i6addr_t	nat_inip6;
	i6addr_t	nat_outip6;
	i6addr_t	nat_oip6;
	U_QUAD_T	nat_pkts[2];
	U_QUAD_T	nat_bytes[2];
	union	{
		udpinfo_t	nat_unu;
		tcpinfo_t	nat_unt;
		icmpinfo_t	nat_uni;
		greinfo_t	nat_ugre;
	} nat_un;
	u_short		nat_oport;
	u_short		nat_use;
	u_char		nat_p;
	int		nat_dir;
	int		nat_ref;
	int		nat_hv[2];
	char		nat_ifnames[2][LIFNAMSIZ];
	int		nat_rev;
	int		nat_redir;
} nat_4_1_14_t;

typedef	struct	nat_4_1_3	{
	ipfmutex_t	nat_lock;
	struct	nat	*nat_next;
	struct	nat	**nat_pnext;
	struct	nat	*nat_hnext[2];
	struct	nat	**nat_phnext[2];
	struct	hostmap	*nat_hm;
	void		*nat_data;
	struct	nat	**nat_me;
	struct	ipstate	*nat_state;
	struct	ap_session	*nat_aps;
	frentry_t	*nat_fr;
	struct	ipnat	*nat_ptr;
	void		*nat_ifps[2];
	void		*nat_sync;
	ipftqent_t	nat_tqe;
	u_32_t		nat_flags;
	u_32_t		nat_sumd[2];
	u_32_t		nat_ipsumd;
	u_32_t		nat_mssclamp;
	i6addr_t	nat_inip6;
	i6addr_t	nat_outip6;
	i6addr_t	nat_oip6;
	U_QUAD_T	nat_pkts[2];
	U_QUAD_T	nat_bytes[2];
	union	{
		udpinfo_t	nat_unu;
		tcpinfo_t	nat_unt;
		icmpinfo_t	nat_uni;
		greinfo_t	nat_ugre;
	} nat_un;
	u_short		nat_oport;
	u_short		nat_use;
	u_char		nat_p;
	int		nat_dir;
	int		nat_ref;
	int		nat_hv[2];
	char		nat_ifnames[2][LIFNAMSIZ];
	int		nat_rev;
} nat_4_1_3_t;


typedef	struct	nat_save_4_1_16	{
	void		*ipn_next;
	nat_4_1_14_t	ipn_nat;
	ipnat_t		ipn_ipnat;
	frentry_4_1_16_t	ipn_fr;
	int		ipn_dsize;
	char		ipn_data[4];
} nat_save_4_1_16_t;

typedef	struct	nat_save_4_1_14	{
	void		*ipn_next;
	nat_4_1_14_t	ipn_nat;
	ipnat_t		ipn_ipnat;
	frentry_4_1_0_t	ipn_fr;
	int		ipn_dsize;
	char		ipn_data[4];
} nat_save_4_1_14_t;

typedef	struct	nat_save_4_1_3	{
	void		*ipn_next;
	nat_4_1_3_t	ipn_nat;
	ipnat_4_1_0_t	ipn_ipnat;
	frentry_4_1_0_t	ipn_fr;
	int		ipn_dsize;
	char		ipn_data[4];
} nat_save_4_1_3_t;

/* ------------------------------------------------------------------------ */

/*
 * 4.1.32 added ns_uncreate (current)
 * 4.1.27 added ns_orphans
 * 4.1.16 added ns_ticks
 */
typedef struct  natstat_4_1_27 {
	u_long	ns_mapped[2];
	u_long	ns_rules;
	u_long	ns_added;
	u_long	ns_expire;
	u_long	ns_inuse;
	u_long	ns_logged;
	u_long	ns_logfail;
	u_long	ns_memfail;
	u_long	ns_badnat;
	u_long	ns_addtrpnt;
	nat_t	**ns_table[2];
	hostmap_t **ns_maptable;
	ipnat_t *ns_list;
	void    *ns_apslist;
	u_int   ns_wilds;
	u_int   ns_nattab_sz;
	u_int   ns_nattab_max;
	u_int   ns_rultab_sz;
	u_int   ns_rdrtab_sz;
	u_int   ns_trpntab_sz;
	u_int   ns_hostmap_sz;
	nat_t   *ns_instances;
	hostmap_t *ns_maplist;
	u_long  *ns_bucketlen[2];
	u_long  ns_ticks;
	u_int   ns_orphans;
} natstat_4_1_27_t;

typedef struct  natstat_4_1_16 {
	u_long	ns_mapped[2];
	u_long	ns_rules;
	u_long	ns_added;
	u_long	ns_expire;
	u_long	ns_inuse;
	u_long	ns_logged;
	u_long	ns_logfail;
	u_long	ns_memfail;
	u_long	ns_badnat;
	u_long	ns_addtrpnt;
	nat_t	**ns_table[2];
	hostmap_t **ns_maptable;
	ipnat_t *ns_list;
	void    *ns_apslist;
	u_int   ns_wilds;
	u_int   ns_nattab_sz;
	u_int   ns_nattab_max;
	u_int   ns_rultab_sz;
	u_int   ns_rdrtab_sz;
	u_int   ns_trpntab_sz;
	u_int   ns_hostmap_sz;
	nat_t   *ns_instances;
	hostmap_t *ns_maplist;
	u_long  *ns_bucketlen[2];
	u_long  ns_ticks;
} natstat_4_1_16_t;

typedef struct  natstat_4_1_0 {
	u_long	ns_mapped[2];
	u_long	ns_rules;
	u_long	ns_added;
	u_long	ns_expire;
	u_long	ns_inuse;
	u_long	ns_logged;
	u_long	ns_logfail;
	u_long	ns_memfail;
	u_long	ns_badnat;
	u_long	ns_addtrpnt;
	nat_t	**ns_table[2];
	hostmap_t **ns_maptable;
	ipnat_t *ns_list;
	void    *ns_apslist;
	u_int   ns_wilds;
	u_int   ns_nattab_sz;
	u_int   ns_nattab_max;
	u_int   ns_rultab_sz;
	u_int   ns_rdrtab_sz;
	u_int   ns_trpntab_sz;
	u_int   ns_hostmap_sz;
	nat_t   *ns_instances;
	hostmap_t *ns_maplist;
	u_long  *ns_bucketlen[2];
} natstat_4_1_0_t;

/* ------------------------------------------------------------------------ */

/*
 * 4.1.32 fra_info:removed both fin_state & fin_nat, added fin_pktnum (current)
 * 4.1.29 added fra_flx
 * 4.1.24 fra_info:added fin_cksum
 * 4.1.23 fra_info:added fin_exthdr
 * 4.1.11 fra_info:added fin_ifname
 * 4.1.4  fra_info:added fin_hbuf
 */
typedef struct  frauth_4_1_29 {
	int	fra_age;
	int	fra_len;
	int	fra_index;
	u_32_t	fra_pass;
	fr_info_4_1_24_t	fra_info;
	char	*fra_buf;
	u_32_t	fra_flx;
#ifdef	MENTAT
	queue_t	*fra_q;
	mb_t	*fra_m;
#endif
} frauth_4_1_29_t;

typedef struct  frauth_4_1_24 {
	int	fra_age;
	int	fra_len;
	int	fra_index;
	u_32_t	fra_pass;
	fr_info_4_1_24_t	fra_info;
	char	*fra_buf;
#ifdef	MENTAT
	queue_t	*fra_q;
	mb_t	*fra_m;
#endif
} frauth_4_1_24_t;

typedef struct  frauth_4_1_23 {
	int	fra_age;
	int	fra_len;
	int	fra_index;
	u_32_t	fra_pass;
	fr_info_4_1_23_t	fra_info;
	char	*fra_buf;
#ifdef	MENTAT
	queue_t	*fra_q;
	mb_t	*fra_m;
#endif
} frauth_4_1_23_t;

typedef struct  frauth_4_1_11 {
	int	fra_age;
	int	fra_len;
	int	fra_index;
	u_32_t	fra_pass;
	fr_info_4_1_11_t	fra_info;
	char	*fra_buf;
#ifdef	MENTAT
	queue_t	*fra_q;
	mb_t	*fra_m;
#endif
} frauth_4_1_11_t;

/* ------------------------------------------------------------------------ */

/*
 * 4.1.16 removed is_nat (current)
 */
typedef struct ipstate_4_1_0 {
	ipfmutex_t	is_lock;
	struct	ipstate	*is_next;
	struct	ipstate	**is_pnext;
	struct	ipstate	*is_hnext;
	struct	ipstate	**is_phnext;
	struct	ipstate	**is_me;
	void		*is_ifp[4];
	void		*is_sync;
	void		*is_nat[2];
	frentry_t	*is_rule;
	struct	ipftq	*is_tqehead[2];
	struct	ipscan	*is_isc;
	U_QUAD_T	is_pkts[4];
	U_QUAD_T	is_bytes[4];
	U_QUAD_T	is_icmppkts[4];
	struct	ipftqent is_sti;
	u_int	is_frage[2];
	int	is_ref;
	int	is_isninc[2];
	u_short	is_sumd[2];
	i6addr_t	is_src;
	i6addr_t	is_dst;
	u_int	is_pass;
	u_char	is_p;
	u_char	is_v;
	u_32_t	is_hv;
	u_32_t	is_tag;
	u_32_t	is_opt[2];
	u_32_t	is_optmsk[2];
	u_short	is_sec;
	u_short	is_secmsk;
	u_short	is_auth;
	u_short	is_authmsk;
	union {
		icmpinfo_t	is_ics;
		tcpinfo_t	is_ts;
		udpinfo_t	is_us;
		greinfo_t	is_ug;
	} is_ps;
	u_32_t	is_flags;
	int	is_flx[2][2];
	u_32_t	is_rulen;
	u_32_t	is_s0[2];
	u_short	is_smsk[2];
	char	is_group[FR_GROUPLEN];
	char	is_sbuf[2][16];
	char	is_ifname[4][LIFNAMSIZ];
} ipstate_4_1_0_t;

typedef	struct	ipstate_save_4_1_16	{
	void		*ips_next;
	ipstate_4_1_0_t	ips_is;
	frentry_4_1_16_t	ips_fr;
} ipstate_save_4_1_16_t;

typedef	struct	ipstate_save_4_1_0	{
	void		*ips_next;
	ipstate_4_1_0_t	ips_is;
	frentry_4_1_0_t	ips_fr;
} ipstate_save_4_1_0_t;

/* ------------------------------------------------------------------------ */

/*
 * 4.1.21 added iss_tcptab (current)
 */
typedef	struct	ips_stat_4_1_0 {
	u_long	iss_hits;
	u_long	iss_miss;
	u_long	iss_max;
	u_long	iss_maxref;
	u_long	iss_tcp;
	u_long	iss_udp;
	u_long	iss_icmp;
	u_long	iss_nomem;
	u_long	iss_expire;
	u_long	iss_fin;
	u_long	iss_active;
	u_long	iss_logged;
	u_long	iss_logfail;
	u_long	iss_inuse;
	u_long	iss_wild;
	u_long	iss_killed;
	u_long	iss_ticks;
	u_long	iss_bucketfull;
	int	iss_statesize;
	int	iss_statemax;
	ipstate_t **iss_table;
	ipstate_t *iss_list;
	u_long	*iss_bucketlen;
} ips_stat_4_1_0_t;

/* ------------------------------------------------------------------------ */

static void
friostat_current_to_0(void *, friostat_4_1_0_t *, int);
static void
ipstate_current_to_0(void *, ipstate_4_1_0_t *);
static void
ipnat_current_to_0(void *, ipnat_4_1_0_t *);
static void
frauth_current_to_11(void *, frauth_4_1_11_t *);
static void
frauth_current_to_23(void *, frauth_4_1_23_t *);
static void
frauth_current_to_24(void *, frauth_4_1_24_t *);
static void
frauth_current_to_29(void *, frauth_4_1_29_t *);
static void
frentry_current_to_0(void *, frentry_4_1_0_t *);
static void
frentry_current_to_16(void *, frentry_4_1_16_t *);
static void
fr_info_current_to_11(void *, fr_info_4_1_11_t *);
static void
fr_info_current_to_23(void *, fr_info_4_1_23_t *);
static void
fr_info_current_to_24(void *, fr_info_4_1_24_t *);
static void
nat_save_current_to_3(void *, nat_save_4_1_3_t *);
static void
nat_save_current_to_14(void *, nat_save_4_1_14_t *);
static void
nat_save_current_to_16(void *, nat_save_4_1_16_t *);

static void
ipstate_save_current_to_0(void *, ipstate_save_4_1_0_t *);
static void
ipstate_save_current_to_16(void *, ipstate_save_4_1_16_t *);

static void
friostat_0_to_current(friostat_4_1_0_t *, void *);
static void
ipnat_0_to_current(ipnat_4_1_0_t *, void *);
static void
frauth_11_to_current(frauth_4_1_11_t *, void *);
static void
frauth_23_to_current(frauth_4_1_23_t *, void *);
static void
frauth_24_to_current(frauth_4_1_24_t *, void *);
static void
frauth_29_to_current(frauth_4_1_29_t *, void *);
static void
frentry_0_to_current(frentry_4_1_0_t *, void *);
static void
frentry_16_to_current(frentry_4_1_16_t *, void *);
static void
fr_info_11_to_current(fr_info_4_1_11_t *, void *);
static void
fr_info_23_to_current(fr_info_4_1_23_t *, void *);
static void
fr_info_24_to_current(fr_info_4_1_24_t *, void *);
static void
nat_save_3_to_current(nat_save_4_1_3_t *, void *);
static void
nat_save_14_to_current(nat_save_4_1_14_t *, void *);
static void
nat_save_16_to_current(nat_save_4_1_16_t *, void *);


/* ------------------------------------------------------------------------ */

int
fr_in_compat(ipfobj_t *obj, void *ptr)
{
	int error;
	int sz;

	error = EINVAL;

	switch (obj->ipfo_type)
	{
	default :
		break;

	case IPFOBJ_FRENTRY :
		if (obj->ipfo_rev >= 4011600) {
			frentry_4_1_16_t *old;

			KMALLOC(old, frentry_4_1_16_t *);
			if (old == NULL) {
				error = ENOMEM;
				break;
			}
			error = COPYIN(obj->ipfo_ptr, old, sizeof(*old));
			if (error == 0)
				frentry_16_to_current(old, ptr);
		} else {
			frentry_4_1_0_t *old;

			KMALLOC(old, frentry_4_1_0_t *);
			if (old == NULL) {
				error = ENOMEM;
				break;
			}
			error = COPYIN(obj->ipfo_ptr, old, sizeof(*old));
			if (error == 0)
				frentry_0_to_current(old, ptr);
			KFREE(old);
		}
		break;

	case IPFOBJ_IPFSTAT :
	    {
		friostat_4_1_0_t *old;

		KMALLOC(old, friostat_4_1_0_t *);
		if (old == NULL) {
			error = ENOMEM;
			break;
		}
		error = COPYIN(obj->ipfo_ptr, old, sizeof(*old));
		if (error == 0)
			friostat_0_to_current(old, ptr);
		break;
	    }

	case IPFOBJ_IPFINFO :	/* unused */
		break;

	case IPFOBJ_IPNAT :
	    {
		ipnat_4_1_0_t *old;

		KMALLOC(old, ipnat_4_1_0_t *);
		if (old == NULL) {
			error = ENOMEM;
			break;
		}
		error = COPYIN(obj->ipfo_ptr, old, sizeof(*old));
		if (error == 0)
			ipnat_0_to_current(old, ptr);
		KFREE(old);
		break;
	    }

	case IPFOBJ_NATSTAT :
		/*
		 * Statistics are not copied in.
		 */
		break;

	case IPFOBJ_NATSAVE :
		if (obj->ipfo_rev >= 4011600) {
			nat_save_4_1_16_t *old16;

			KMALLOC(old16, nat_save_4_1_16_t *);
			if (old16 == NULL) {
				error = ENOMEM;
				break;
			}
			error = COPYIN(obj->ipfo_ptr, old16, sizeof(*old16));
			if (error == 0)
				nat_save_16_to_current(old16, ptr);
			KFREE(old16);
		} else if (obj->ipfo_rev >= 4011400) {
			nat_save_4_1_14_t *old14;

			KMALLOC(old14, nat_save_4_1_14_t *);
			if (old14 == NULL) {
				error = ENOMEM;
				break;
			}
			error = COPYIN(obj->ipfo_ptr, old14, sizeof(*old14));
			if (error == 0)
				nat_save_14_to_current(old14, ptr);
			KFREE(old14);
		} else if (obj->ipfo_rev >= 4010300) {
			nat_save_4_1_3_t *old3;

			KMALLOC(old3, nat_save_4_1_3_t *);
			if (old3 == NULL) {
				error = ENOMEM;
				break;
			}
			error = COPYIN(obj->ipfo_ptr, old3, sizeof(*old3));
			if (error == 0)
				nat_save_3_to_current(old3, ptr);
			KFREE(old3);
		}
		break;

	case IPFOBJ_STATESAVE :
		if (obj->ipfo_rev >= 4011600) {
			ipstate_save_4_1_16_t *old;

			KMALLOC(old, ipstate_save_4_1_16_t *);
			if (old == NULL) {
				error = ENOMEM;
				break;
			}
			error = COPYIN(obj->ipfo_ptr, old, sizeof(*old));
			if (error == 0)
				/* ipstate_save_16_to_current(&old, ptr); */
				;
			KFREE(old);
		} else {
			ipstate_save_4_1_0_t *old;

			KMALLOC(old, ipstate_save_4_1_0_t *);
			if (old == NULL) {
				error = ENOMEM;
				break;
			}
			error = COPYIN(obj->ipfo_ptr, old, sizeof(*old));
			if (error == 0)
				/* ipstate_save_0_to_current(&old, ptr); */
				;
			KFREE(old);
		}
		break;

	case IPFOBJ_IPSTATE :
		/*
		 * This structure is not copied in by itself.
		 */
		break;

	case IPFOBJ_STATESTAT :
		/*
		 * Statistics are not copied in.
		 */
		break;

	case IPFOBJ_FRAUTH :
		if (obj->ipfo_rev >= 4012900) {
			frauth_4_1_29_t *old29;

			KMALLOC(old29, frauth_4_1_29_t *);
			if (old29 == NULL) {
				error = ENOMEM;
				break;
			}
			error = COPYIN(obj->ipfo_ptr, old29, sizeof(*old29));
			if (error == 0)
				frauth_29_to_current(old29, ptr);
			KFREE(old29);
		} else if (obj->ipfo_rev >= 4012400) {
			frauth_4_1_24_t *old24;

			KMALLOC(old24, frauth_4_1_24_t *);
			if (old24 == NULL) {
				error = ENOMEM;
				break;
			}
			error = COPYIN(obj->ipfo_ptr, old24, sizeof(*old24));
			if (error == 0)
				frauth_24_to_current(old24, ptr);
			KFREE(old24);
		} else if (obj->ipfo_rev >= 4012300) {
			frauth_4_1_23_t *old23;

			KMALLOC(old23, frauth_4_1_23_t *);
			if (old23 == NULL) {
				error = ENOMEM;
				break;
			}
			error = COPYIN(obj->ipfo_ptr, old23, sizeof(*old23));
			if (error == 0)
				frauth_23_to_current(old23, ptr);
			KFREE(old23);
		} else if (obj->ipfo_rev >= 4011100) {
			frauth_4_1_11_t *old11;

			KMALLOC(old11, frauth_4_1_11_t *);
			if (old11 == NULL) {
				error = ENOMEM;
				break;
			}
			error = COPYIN(obj->ipfo_ptr, old11, sizeof(*old11));
			if (error == 0)
				frauth_11_to_current(old11, ptr);
			KFREE(old11);
		}
		break;

	case IPFOBJ_NAT :
		if (obj->ipfo_rev >= 4011400) {
			sz = sizeof(nat_4_1_14_t);
		} else if (obj->ipfo_rev >= 4010300) {
			sz = sizeof(nat_4_1_3_t);
		} else {
			break;
		}
		bzero(ptr, sizeof(nat_t));
		error = COPYIN(obj->ipfo_ptr, ptr, sz);
		break;
	}

	return error;
}


static void
frentry_16_to_current(frentry_4_1_16_t *old, void *current)
{
	frentry_t *fr = (frentry_t *)current;

	fr->fr_lock = old->fr_lock;
	fr->fr_next = old->fr_next;
	fr->fr_grp = old->fr_grp;
	fr->fr_isc = old->fr_isc;
	fr->fr_ifas[0] = old->fr_ifas[0];
	fr->fr_ifas[1] = old->fr_ifas[1];
	fr->fr_ifas[2] = old->fr_ifas[2];
	fr->fr_ifas[3] = old->fr_ifas[3];
	fr->fr_ptr = old->fr_ptr;
	fr->fr_comment = old->fr_comment;
	fr->fr_ref = old->fr_ref;
	fr->fr_statecnt = old->fr_statecnt;
	fr->fr_hits = old->fr_hits;
	fr->fr_bytes = old->fr_bytes;
	fr->fr_lastpkt.tv_sec = old->fr_lastpkt.tv_sec;
	fr->fr_lastpkt.tv_usec = old->fr_lastpkt.tv_usec;
	fr->fr_curpps = old->fr_curpps;
	bcopy(&old->fr_dun, &fr->fr_dun, sizeof(old->fr_dun));
	fr->fr_func = old->fr_func;
	fr->fr_dsize = old->fr_dsize;
	fr->fr_pps = old->fr_pps;
	fr->fr_statemax = old->fr_statemax;
	fr->fr_flineno = old->fr_flineno;
	fr->fr_type = old->fr_type;
	fr->fr_flags = old->fr_flags;
	fr->fr_logtag = old->fr_logtag;
	fr->fr_collect = old->fr_collect;
	fr->fr_arg = old->fr_arg;
	fr->fr_loglevel = old->fr_loglevel;
	fr->fr_age[0] = old->fr_age[0];
	fr->fr_age[1] = old->fr_age[1];
	fr->fr_v = old->fr_v;
	fr->fr_icode = old->fr_icode;
	bcopy(&old->fr_group, &fr->fr_group, sizeof(old->fr_group));
	bcopy(&old->fr_grhead, &fr->fr_grhead, sizeof(old->fr_grhead));
	bcopy(&old->fr_nattag, &fr->fr_nattag, sizeof(old->fr_nattag));
	bcopy(&old->fr_ifnames, &fr->fr_ifnames, sizeof(old->fr_ifnames));
	bcopy(&old->fr_isctag, &fr->fr_isctag, sizeof(old->fr_isctag));
	bcopy(&old->fr_tifs, &fr->fr_tifs, sizeof(old->fr_tifs));
	bcopy(&old->fr_dif, &fr->fr_dif, sizeof(old->fr_dif));
	fr->fr_cksum = old->fr_cksum;
}


static void
frentry_0_to_current(frentry_4_1_0_t *old, void *current)
{
	frentry_t *fr = (frentry_t *)current;

	fr->fr_lock = old->fr_lock;
	fr->fr_next = old->fr_next;
	fr->fr_grp = old->fr_grp;
	fr->fr_isc = old->fr_isc;
	fr->fr_ifas[0] = old->fr_ifas[0];
	fr->fr_ifas[1] = old->fr_ifas[1];
	fr->fr_ifas[2] = old->fr_ifas[2];
	fr->fr_ifas[3] = old->fr_ifas[3];
	fr->fr_ptr = old->fr_ptr;
	fr->fr_comment = old->fr_comment;
	fr->fr_ref = old->fr_ref;
	fr->fr_statecnt = old->fr_statecnt;
	fr->fr_hits = old->fr_hits;
	fr->fr_bytes = old->fr_bytes;
	fr->fr_lastpkt.tv_sec = old->fr_lastpkt.tv_sec;
	fr->fr_lastpkt.tv_usec = old->fr_lastpkt.tv_usec;
	fr->fr_curpps = old->fr_curpps;
	bcopy(&old->fr_dun, &fr->fr_dun, sizeof(old->fr_dun));
	fr->fr_func = old->fr_func;
	fr->fr_dsize = old->fr_dsize;
	fr->fr_pps = old->fr_pps;
	fr->fr_statemax = old->fr_statemax;
	fr->fr_flineno = old->fr_flineno;
	fr->fr_type = old->fr_type;
	fr->fr_flags = old->fr_flags;
	fr->fr_logtag = old->fr_logtag;
	fr->fr_collect = old->fr_collect;
	fr->fr_arg = old->fr_arg;
	fr->fr_loglevel = old->fr_loglevel;
	fr->fr_age[0] = old->fr_age[0];
	fr->fr_age[1] = old->fr_age[1];
	fr->fr_v = old->fr_v;
	fr->fr_icode = old->fr_icode;
	bcopy(&old->fr_group, &fr->fr_group, sizeof(old->fr_group));
	bcopy(&old->fr_grhead, &fr->fr_grhead, sizeof(old->fr_grhead));
	bcopy(&old->fr_nattag, &fr->fr_nattag, sizeof(old->fr_nattag));
	bcopy(&old->fr_ifnames, &fr->fr_ifnames, sizeof(old->fr_ifnames));
	bcopy(&old->fr_isctag, &fr->fr_isctag, sizeof(old->fr_isctag));
	bcopy(&old->fr_tifs, &fr->fr_tifs, sizeof(old->fr_tifs));
	bcopy(&old->fr_dif, &fr->fr_dif, sizeof(old->fr_dif));
	fr->fr_cksum = old->fr_cksum;
}


static void
friostat_0_to_current(friostat_4_1_0_t *old, void *current)
{
	friostat_t *fiop = (friostat_t *)current;

	bcopy(&old->f_st, &fiop->f_st, sizeof(old->f_st));
	fiop->f_ipf[0][0] = old->f_ipf[0][0];
	fiop->f_ipf[0][1] = old->f_ipf[0][1];
	fiop->f_ipf[1][0] = old->f_ipf[1][0];
	fiop->f_ipf[1][1] = old->f_ipf[1][1];
	fiop->f_acct[0][0] = old->f_acct[0][0];
	fiop->f_acct[0][1] = old->f_acct[0][1];
	fiop->f_acct[1][0] = old->f_acct[1][0];
	fiop->f_acct[1][1] = old->f_acct[1][1];
	fiop->f_ipf6[0][0] = old->f_ipf6[0][0];
	fiop->f_ipf6[0][1] = old->f_ipf6[0][1];
	fiop->f_ipf6[1][0] = old->f_ipf6[1][0];
	fiop->f_ipf6[1][1] = old->f_ipf6[1][1];
	fiop->f_acct6[0][0] = old->f_acct6[0][0];
	fiop->f_acct6[0][1] = old->f_acct6[0][1];
	fiop->f_acct6[1][0] = old->f_acct6[1][0];
	fiop->f_acct6[1][1] = old->f_acct6[1][1];
	fiop->f_auth = fiop->f_auth;
	bcopy(&old->f_groups, &fiop->f_groups, sizeof(old->f_groups));
	bcopy(&old->f_froute, &fiop->f_froute, sizeof(old->f_froute));
	fiop->f_ticks = old->f_ticks;
	bcopy(&old->f_locks, &fiop->f_locks, sizeof(old->f_locks));
	fiop->f_kmutex_sz = old->f_kmutex_sz;
	fiop->f_krwlock_sz = old->f_krwlock_sz;
	fiop->f_defpass = old->f_defpass;
	fiop->f_active = old->f_active;
	fiop->f_running = old->f_running;
	fiop->f_logging = old->f_logging;
	fiop->f_features = old->f_features;
	bcopy(old->f_version, fiop->f_version, sizeof(old->f_version));
}


static void
ipnat_0_to_current(ipnat_4_1_0_t *old, void *current)
{
	ipnat_t *np = (ipnat_t *)current;

	np->in_next = old->in_next;
	np->in_rnext = old->in_rnext;
	np->in_prnext = old->in_prnext;
	np->in_mnext = old->in_mnext;
	np->in_pmnext = old->in_pmnext;
	np->in_tqehead[0] = old->in_tqehead[0];
	np->in_tqehead[1] = old->in_tqehead[1];
	np->in_ifps[0] = old->in_ifps[0];
	np->in_ifps[1] = old->in_ifps[1];
	np->in_apr = old->in_apr;
	np->in_comment = old->in_comment;
	np->in_next6 = old->in_next6;
	np->in_space = old->in_space;
	np->in_hits = old->in_hits;
	np->in_use = old->in_use;
	np->in_hv = old->in_hv;
	np->in_flineno = old->in_flineno;
	np->in_pnext = old->in_pnext;
	np->in_v = old->in_v;
	np->in_xxx = old->in_xxx;
	np->in_flags = old->in_flags;
	np->in_mssclamp = old->in_mssclamp;
	bcopy(&old->in_age, &np->in_age, sizeof(np->in_age));
	np->in_redir = old->in_redir;
	np->in_p = old->in_p;
	bcopy(&old->in_in, &np->in_in, sizeof(np->in_in));
	bcopy(&old->in_out, &np->in_out, sizeof(np->in_out));
	bcopy(&old->in_src, &np->in_src, sizeof(np->in_src));
	bcopy(&old->in_tuc, &np->in_tuc, sizeof(np->in_tuc));
	np->in_port[0] = old->in_port[0];
	np->in_port[1] = old->in_port[1];
	np->in_ppip = old->in_ppip;
	np->in_ippip = old->in_ippip;
	bcopy(&old->in_ifnames, &np->in_ifnames, sizeof(np->in_ifnames));
	bcopy(&old->in_plabel, &np->in_plabel, sizeof(np->in_plabel));
	bcopy(&old->in_tag, &np->in_tag, sizeof(np->in_tag));
}


static void
frauth_29_to_current(frauth_4_1_29_t *old, void *current)
{
	frauth_t *fra = (frauth_t *)current;

	fra->fra_age = old->fra_age;
	fra->fra_len = old->fra_len;
	fra->fra_index = old->fra_index;
	fra->fra_pass = old->fra_pass;
	fr_info_24_to_current(&old->fra_info, &fra->fra_info);
	fra->fra_buf = old->fra_buf;
	fra->fra_flx = old->fra_flx;
#ifdef	MENTAT
	fra->fra_q = old->fra_q;
	fra->fra_m = old->fra_m;
#endif
}


static void
frauth_24_to_current(frauth_4_1_24_t *old, void *current)
{
	frauth_t *fra = (frauth_t *)current;

	fra->fra_age = old->fra_age;
	fra->fra_len = old->fra_len;
	fra->fra_index = old->fra_index;
	fra->fra_pass = old->fra_pass;
	fr_info_24_to_current(&old->fra_info, &fra->fra_info);
	fra->fra_buf = old->fra_buf;
#ifdef	MENTAT
	fra->fra_q = old->fra_q;
	fra->fra_m = old->fra_m;
#endif
}


static void
frauth_23_to_current(frauth_4_1_23_t *old, void *current)
{
	frauth_t *fra = (frauth_t *)current;

	fra->fra_age = old->fra_age;
	fra->fra_len = old->fra_len;
	fra->fra_index = old->fra_index;
	fra->fra_pass = old->fra_pass;
	fr_info_23_to_current(&old->fra_info, &fra->fra_info);
	fra->fra_buf = old->fra_buf;
#ifdef	MENTAT
	fra->fra_q = old->fra_q;
	fra->fra_m = old->fra_m;
#endif
}


static void
frauth_11_to_current(frauth_4_1_11_t *old, void *current)
{
	frauth_t *fra = (frauth_t *)current;

	fra->fra_age = old->fra_age;
	fra->fra_len = old->fra_len;
	fra->fra_index = old->fra_index;
	fra->fra_pass = old->fra_pass;
	fr_info_11_to_current(&old->fra_info, &fra->fra_info);
	fra->fra_buf = old->fra_buf;
#ifdef	MENTAT
	fra->fra_q = old->fra_q;
	fra->fra_m = old->fra_m;
#endif
}


static void
fr_info_24_to_current(fr_info_4_1_24_t *old, void *current)
{
	fr_info_t *fin = (fr_info_t *)current;

	fin->fin_ifp = old->fin_ifp;
	fin->fin_fi = old->fin_fi;
	bcopy(&old->fin_dat, &fin->fin_dat, sizeof(old->fin_dat));
	fin->fin_out = old->fin_out;
	fin->fin_rev = old->fin_rev;
	fin->fin_hlen = old->fin_hlen;
	fin->fin_tcpf = old->fin_tcpf;
	fin->fin_icode = old->fin_icode;
	fin->fin_rule = old->fin_rule;
	bcopy(old->fin_group, fin->fin_group, sizeof(old->fin_group));
	fin->fin_fr = old->fin_fr;
	fin->fin_dp = old->fin_dp;
	fin->fin_dlen = old->fin_dlen;
	fin->fin_plen = old->fin_plen;
	fin->fin_ipoff = old->fin_ipoff;
	fin->fin_id = old->fin_id;
	fin->fin_off = old->fin_off;
	fin->fin_depth = old->fin_depth;
	fin->fin_error = old->fin_error;
	fin->fin_cksum = old->fin_cksum;
	fin->fin_nattag = old->fin_nattag;
	fin->fin_exthdr = old->fin_exthdr;
	fin->fin_ip = old->fin_ip;
	fin->fin_mp = old->fin_mp;
	fin->fin_m = old->fin_m;
#ifdef  MENTAT
	fin->fin_qfm = old->fin_qfm;
	fin->fin_qpi = old->fin_qpi;
	fin->fin_ifname = old_ifname;
#endif
#ifdef  __sgi
	fin->fin_hbuf = old->fin_hbuf;
#endif
}


static void
fr_info_23_to_current(fr_info_4_1_23_t *old, void *current)
{
	fr_info_t *fin = (fr_info_t *)current;

	fin->fin_ifp = old->fin_ifp;
	fin->fin_fi = old->fin_fi;
	bcopy(&old->fin_dat, &fin->fin_dat, sizeof(old->fin_dat));
	fin->fin_out = old->fin_out;
	fin->fin_rev = old->fin_rev;
	fin->fin_hlen = old->fin_hlen;
	fin->fin_tcpf = old->fin_tcpf;
	fin->fin_icode = old->fin_icode;
	fin->fin_rule = old->fin_rule;
	bcopy(old->fin_group, fin->fin_group, sizeof(old->fin_group));
	fin->fin_fr = old->fin_fr;
	fin->fin_dp = old->fin_dp;
	fin->fin_dlen = old->fin_dlen;
	fin->fin_plen = old->fin_plen;
	fin->fin_ipoff = old->fin_ipoff;
	fin->fin_id = old->fin_id;
	fin->fin_off = old->fin_off;
	fin->fin_depth = old->fin_depth;
	fin->fin_error = old->fin_error;
	fin->fin_nattag = old->fin_nattag;
	fin->fin_exthdr = old->fin_exthdr;
	fin->fin_ip = old->fin_ip;
	fin->fin_mp = old->fin_mp;
	fin->fin_m = old->fin_m;
#ifdef  MENTAT
	fin->fin_qfm = old->fin_qfm;
	fin->fin_qpi = old->fin_qpi;
	fin->fin_ifname = old_ifname;
#endif
#ifdef  __sgi
	fin->fin_hbuf = fin->fin_hbuf;
#endif
}


static void
fr_info_11_to_current(fr_info_4_1_11_t *old, void *current)
{
	fr_info_t *fin = (fr_info_t *)current;

	fin->fin_ifp = old->fin_ifp;
	fin->fin_fi = old->fin_fi;
	bcopy(&old->fin_dat, &fin->fin_dat, sizeof(old->fin_dat));
	fin->fin_out = old->fin_out;
	fin->fin_rev = old->fin_rev;
	fin->fin_hlen = old->fin_hlen;
	fin->fin_tcpf = old->fin_tcpf;
	fin->fin_icode = old->fin_icode;
	fin->fin_rule = old->fin_rule;
	bcopy(old->fin_group, fin->fin_group, sizeof(old->fin_group));
	fin->fin_fr = old->fin_fr;
	fin->fin_dp = old->fin_dp;
	fin->fin_dlen = old->fin_dlen;
	fin->fin_plen = old->fin_plen;
	fin->fin_ipoff = old->fin_ipoff;
	fin->fin_id = old->fin_id;
	fin->fin_off = old->fin_off;
	fin->fin_depth = old->fin_depth;
	fin->fin_error = old->fin_error;
	fin->fin_nattag = old->fin_nattag;
	fin->fin_ip = old->fin_ip;
	fin->fin_mp = old->fin_mp;
	fin->fin_m = old->fin_m;
#ifdef  MENTAT
	fin->fin_qfm = old->fin_qfm;
	fin->fin_qpi = old->fin_qpi;
	fin->fin_ifname = old_ifname;
#endif
#ifdef  __sgi
	fin->fin_hbuf = fin->fin_hbuf;
#endif
}


static void
nat_3_to_current(nat_4_1_3_t *old, nat_t *current)
{
	bzero((void *)current, sizeof(*current));
	bcopy((void *)old, (void *)current, sizeof(*old));
}


static void
nat_14_to_current(nat_4_1_14_t *old, nat_t *current)
{
	bzero((void *)current, sizeof(*current));
	bcopy((void *)old, (void *)current, sizeof(*old));
}


static void
nat_save_16_to_current(nat_save_4_1_16_t *old, void *current)
{
	nat_save_t *nats = (nat_save_t *)current;

	nats->ipn_next = old->ipn_next;
	nat_14_to_current(&old->ipn_nat, &nats->ipn_nat);
	bcopy(&old->ipn_ipnat, &nats->ipn_ipnat, sizeof(old->ipn_ipnat));
	bcopy(&old->ipn_fr, &nats->ipn_fr, sizeof(old->ipn_fr));
	nats->ipn_dsize = old->ipn_dsize;
	bcopy(old->ipn_data, nats->ipn_data, sizeof(nats->ipn_data));
}


static void
nat_save_14_to_current(nat_save_4_1_14_t *old, void *current)
{
	nat_save_t *nats = (nat_save_t *)current;

	nats->ipn_next = old->ipn_next;
	nat_14_to_current(&old->ipn_nat, &nats->ipn_nat);
	bcopy(&old->ipn_ipnat, &nats->ipn_ipnat, sizeof(old->ipn_ipnat));
	frentry_0_to_current(&old->ipn_fr, &nats->ipn_fr);
	nats->ipn_dsize = old->ipn_dsize;
	bcopy(old->ipn_data, nats->ipn_data, sizeof(nats->ipn_data));
}


static void
nat_save_3_to_current(nat_save_4_1_3_t *old, void *current)
{
	nat_save_t *nats = (nat_save_t *)current;

	nats->ipn_next = old->ipn_next;
	nat_3_to_current(&old->ipn_nat, &nats->ipn_nat);
	ipnat_0_to_current(&old->ipn_ipnat, &nats->ipn_ipnat);
	frentry_0_to_current(&old->ipn_fr, &nats->ipn_fr);
	nats->ipn_dsize = old->ipn_dsize;
	bcopy(old->ipn_data, nats->ipn_data, sizeof(nats->ipn_data));
}


static void
ipstate_save_current_to_16(void *current, ipstate_save_4_1_16_t *old)
{
	ipstate_save_t *ips = (ipstate_save_t *)current;

	old->ips_next = ips->ips_next;
	ipstate_current_to_0(&ips->ips_is, &old->ips_is);
	frentry_current_to_16(&ips->ips_fr, &old->ips_fr);
}


static void
ipstate_save_current_to_0(void *current, ipstate_save_4_1_0_t *old)
{
	ipstate_save_t *ips = (ipstate_save_t *)current;

	old->ips_next = ips->ips_next;
	ipstate_current_to_0(&ips->ips_is, &old->ips_is);
	frentry_current_to_0(&ips->ips_fr, &old->ips_fr);
}


int
fr_out_compat(ipfobj_t *obj, void *ptr)
{
	int error;
	int sz;

	error = EINVAL;

	switch (obj->ipfo_type)
	{
	default :
		break;

	case IPFOBJ_FRENTRY :
		if (obj->ipfo_rev >= 4011600) {
			frentry_4_1_16_t *old;

			KMALLOC(old, frentry_4_1_16_t *);
			if (old == NULL) {
				error = ENOMEM;
				break;
			}
			frentry_current_to_16(ptr, old);
			error = COPYOUT(old, obj->ipfo_ptr, sizeof(*old));
			KFREE(old);
			obj->ipfo_size = sizeof(*old);
		} else {
			frentry_4_1_0_t *old;

			KMALLOC(old, frentry_4_1_0_t *);
			if (old == NULL) {
				error = ENOMEM;
				break;
			}
			frentry_current_to_0(ptr, old);
			error = COPYOUT(old, obj->ipfo_ptr, sizeof(*old));
			KFREE(old);
			obj->ipfo_size = sizeof(*old);
		}
		break;

	case IPFOBJ_IPFSTAT :
	    {
		friostat_4_1_0_t *old;

		KMALLOC(old, friostat_4_1_0_t *);
		if (old == NULL) {
			error = ENOMEM;
			break;
		}
		friostat_current_to_0(ptr, old, obj->ipfo_rev);
		error = COPYOUT(old, obj->ipfo_ptr, sizeof(*old));
		KFREE(old);
		break;
	    }

	case IPFOBJ_IPFINFO :	/* unused */
		break;

	case IPFOBJ_IPNAT :
	    {
		ipnat_4_1_0_t *old;

		KMALLOC(old, ipnat_4_1_0_t *);
		if (old == NULL) {
			error = ENOMEM;
			break;
		}
		ipnat_current_to_0(ptr, old);
		error = COPYOUT(old, obj->ipfo_ptr, sizeof(*old));
		KFREE(old);
		break;
	    }

	case IPFOBJ_NATSTAT :
		if (obj->ipfo_rev >= 4012700)
			sz = sizeof(natstat_4_1_27_t);
		else if (obj->ipfo_rev >= 4011600)
			sz = sizeof(natstat_4_1_16_t);
		else
			sz = sizeof(natstat_4_1_0_t);
		error = COPYOUT(ptr, obj->ipfo_ptr, sz);
		break;

	case IPFOBJ_STATESAVE :
		if (obj->ipfo_rev >= 4011600) {
			ipstate_save_4_1_16_t *old;

			KMALLOC(old, ipstate_save_4_1_16_t *);
			if (old == NULL) {
				error = ENOMEM;
				break;
			}
			ipstate_save_current_to_16(ptr, old);
			error = COPYOUT(old, obj->ipfo_ptr, sizeof(*old));
			KFREE(old);
		} else {
			ipstate_save_4_1_0_t *old;

			KMALLOC(old, ipstate_save_4_1_0_t *);
			if (old == NULL) {
				error = ENOMEM;
				break;
			}
			ipstate_save_current_to_0(ptr, old);
			error = COPYOUT(old, obj->ipfo_ptr, sizeof(*old));
			KFREE(old);
		}
		break;

	case IPFOBJ_NATSAVE :
		if (obj->ipfo_rev >= 4011600) {
			nat_save_4_1_16_t *old16;

			KMALLOC(old16, nat_save_4_1_16_t *);
			if (old16 == NULL) {
				error = ENOMEM;
				break;
			}
			nat_save_current_to_16(ptr, old16);
			error = COPYOUT(&old16, obj->ipfo_ptr, sizeof(*old16));
			KFREE(old16);
		} else if (obj->ipfo_rev >= 4011400) {
			nat_save_4_1_14_t *old14;

			KMALLOC(old14, nat_save_4_1_14_t *);
			if (old14 == NULL) {
				error = ENOMEM;
				break;
			}
			nat_save_current_to_14(ptr, old14);
			error = COPYOUT(&old14, obj->ipfo_ptr, sizeof(*old14));
			KFREE(old14);
		} else if (obj->ipfo_rev >= 4010300) {
			nat_save_4_1_3_t *old3;

			KMALLOC(old3, nat_save_4_1_3_t *);
			if (old3 == NULL) {
				error = ENOMEM;
				break;
			}
			nat_save_current_to_3(ptr, old3);
			error = COPYOUT(&old3, obj->ipfo_ptr, sizeof(*old3));
			KFREE(old3);
		}
		break;

	case IPFOBJ_IPSTATE :
	    {
		ipstate_4_1_0_t *old;

		KMALLOC(old, ipstate_4_1_0_t *);
		if (old == NULL) {
			error = ENOMEM;
			break;
		}
		ipstate_current_to_0(ptr, old);
		error = COPYOUT(old, obj->ipfo_ptr, sizeof(*old));
		KFREE(old);
		break;
	    }

	case IPFOBJ_STATESTAT :
		error = COPYOUT(ptr, obj->ipfo_ptr, sizeof(ips_stat_4_1_0_t));
		break;

	case IPFOBJ_FRAUTH :
		if (obj->ipfo_rev >= 4012900) {
			frauth_4_1_29_t *old29;

			KMALLOC(old29, frauth_4_1_29_t *);
			if (old29 == NULL) {
				error = ENOMEM;
				break;
			}
			frauth_current_to_29(ptr, old29);
			error = COPYOUT(old29, obj->ipfo_ptr, sizeof(*old29));
			KFREE(old29);
		} else if (obj->ipfo_rev >= 4012400) {
			frauth_4_1_24_t *old24;

			KMALLOC(old24, frauth_4_1_24_t *);
			if (old24 == NULL) {
				error = ENOMEM;
				break;
			}
			frauth_current_to_24(ptr, old24);
			error = COPYOUT(old24, obj->ipfo_ptr, sizeof(*old24));
			KFREE(old24);
		} else if (obj->ipfo_rev >= 4012300) {
			frauth_4_1_23_t *old23;

			KMALLOC(old23, frauth_4_1_23_t *);
			if (old23 == NULL) {
				error = ENOMEM;
				break;
			}
			frauth_current_to_23(ptr, old23);
			error = COPYOUT(old23, obj->ipfo_ptr, sizeof(*old23));
			KFREE(old23);
		} else if (obj->ipfo_rev >= 4011100) {
			frauth_4_1_11_t *old11;

			KMALLOC(old11, frauth_4_1_11_t *);
			if (old11 == NULL) {
				error = ENOMEM;
				break;
			}
			frauth_current_to_11(ptr, old11);
			error = COPYOUT(old11, obj->ipfo_ptr, sizeof(*old11));
			KFREE(old11);
		}
		break;

	case IPFOBJ_NAT :
		if (obj->ipfo_rev >= 4011400) {
			sz = sizeof(nat_4_1_14_t);
		} else if (obj->ipfo_rev >= 4010300) {
			sz = sizeof(nat_4_1_3_t);
		} else {
			break;
		}
		error = COPYOUT(ptr, obj->ipfo_ptr, sz);
		break;
	}
	return error;
}


static void
friostat_current_to_0(void *current, friostat_4_1_0_t *old, int rev)
{
	friostat_t *fiop = (friostat_t *)current;

	bcopy(&fiop->f_st, &old->f_st, sizeof(fiop->f_st));
	old->f_ipf[0][0] = fiop->f_ipf[0][0];
	old->f_ipf[0][1] = fiop->f_ipf[0][1];
	old->f_ipf[1][0] = fiop->f_ipf[1][0];
	old->f_ipf[1][1] = fiop->f_ipf[1][1];
	old->f_acct[0][0] = fiop->f_acct[0][0];
	old->f_acct[0][1] = fiop->f_acct[0][1];
	old->f_acct[1][0] = fiop->f_acct[1][0];
	old->f_acct[1][1] = fiop->f_acct[1][1];
	old->f_ipf6[0][0] = fiop->f_ipf6[0][0];
	old->f_ipf6[0][1] = fiop->f_ipf6[0][1];
	old->f_ipf6[1][0] = fiop->f_ipf6[1][0];
	old->f_ipf6[1][1] = fiop->f_ipf6[1][1];
	old->f_acct6[0][0] = fiop->f_acct6[0][0];
	old->f_acct6[0][1] = fiop->f_acct6[0][1];
	old->f_acct6[1][0] = fiop->f_acct6[1][0];
	old->f_acct6[1][1] = fiop->f_acct6[1][1];
	old->f_auth = fiop->f_auth;
	bcopy(&fiop->f_groups, &old->f_groups, sizeof(old->f_groups));
	bcopy(&fiop->f_froute, &old->f_froute, sizeof(old->f_froute));
	old->f_ticks = fiop->f_ticks;
	bcopy(&fiop->f_locks, &old->f_locks, sizeof(old->f_locks));
	old->f_kmutex_sz = fiop->f_kmutex_sz;
	old->f_krwlock_sz = fiop->f_krwlock_sz;
	old->f_defpass = fiop->f_defpass;
	old->f_active = fiop->f_active;
	old->f_running = fiop->f_running;
	old->f_logging = fiop->f_logging;
	old->f_features = fiop->f_features;
	sprintf(old->f_version, "IP Filter: v%d.%d.%d",
		(rev / 1000000) % 100,
		(rev / 10000) % 100,
		(rev / 100) % 100);
}


static void
frentry_current_to_16(void *current, frentry_4_1_16_t *old)
{
	frentry_t *fr = (frentry_t *)current;

	old->fr_lock = fr->fr_lock;
	old->fr_next = fr->fr_next;
	old->fr_grp = fr->fr_grp;
	old->fr_isc = fr->fr_isc;
	old->fr_ifas[0] = fr->fr_ifas[0];
	old->fr_ifas[1] = fr->fr_ifas[1];
	old->fr_ifas[2] = fr->fr_ifas[2];
	old->fr_ifas[3] = fr->fr_ifas[3];
	old->fr_ptr = fr->fr_ptr;
	old->fr_comment = fr->fr_comment;
	old->fr_ref = fr->fr_ref;
	old->fr_statecnt = fr->fr_statecnt;
	old->fr_hits = fr->fr_hits;
	old->fr_bytes = fr->fr_bytes;
	old->fr_lastpkt.tv_sec = fr->fr_lastpkt.tv_sec;
	old->fr_lastpkt.tv_usec = fr->fr_lastpkt.tv_usec;
	old->fr_curpps = fr->fr_curpps;
	bcopy(&fr->fr_dun, &old->fr_dun, sizeof(fr->fr_dun));
	old->fr_func = fr->fr_func;
	old->fr_dsize = fr->fr_dsize;
	old->fr_pps = fr->fr_pps;
	old->fr_statemax = fr->fr_statemax;
	old->fr_flineno = fr->fr_flineno;
	old->fr_type = fr->fr_type;
	old->fr_flags = fr->fr_flags;
	old->fr_logtag = fr->fr_logtag;
	old->fr_collect = fr->fr_collect;
	old->fr_arg = fr->fr_arg;
	old->fr_loglevel = fr->fr_loglevel;
	old->fr_age[0] = fr->fr_age[0];
	old->fr_age[1] = fr->fr_age[1];
	old->fr_v = fr->fr_v;
	old->fr_icode = fr->fr_icode;
	bcopy(&fr->fr_group, &old->fr_group, sizeof(fr->fr_group));
	bcopy(&fr->fr_grhead, &old->fr_grhead, sizeof(fr->fr_grhead));
	bcopy(&fr->fr_nattag, &old->fr_nattag, sizeof(fr->fr_nattag));
	bcopy(&fr->fr_ifnames, &old->fr_ifnames, sizeof(fr->fr_ifnames));
	bcopy(&fr->fr_isctag, &old->fr_isctag, sizeof(fr->fr_isctag));
	bcopy(&fr->fr_tifs, &old->fr_tifs, sizeof(fr->fr_tifs));
	bcopy(&fr->fr_dif, &old->fr_dif, sizeof(fr->fr_dif));
	old->fr_cksum = fr->fr_cksum;
}


static void
frentry_current_to_0(void *current, frentry_4_1_0_t *old)
{
	frentry_t *fr = (frentry_t *)current;

	old->fr_lock = fr->fr_lock;
	old->fr_next = fr->fr_next;
	old->fr_grp = fr->fr_grp;
	old->fr_isc = fr->fr_isc;
	old->fr_ifas[0] = fr->fr_ifas[0];
	old->fr_ifas[1] = fr->fr_ifas[1];
	old->fr_ifas[2] = fr->fr_ifas[2];
	old->fr_ifas[3] = fr->fr_ifas[3];
	old->fr_ptr = fr->fr_ptr;
	old->fr_comment = fr->fr_comment;
	old->fr_ref = fr->fr_ref;
	old->fr_statecnt = fr->fr_statecnt;
	old->fr_hits = fr->fr_hits;
	old->fr_bytes = fr->fr_bytes;
	old->fr_lastpkt.tv_sec = fr->fr_lastpkt.tv_sec;
	old->fr_lastpkt.tv_usec = fr->fr_lastpkt.tv_usec;
	old->fr_curpps = fr->fr_curpps;
	bcopy(&fr->fr_dun, &old->fr_dun, sizeof(fr->fr_dun));
	old->fr_func = fr->fr_func;
	old->fr_dsize = fr->fr_dsize;
	old->fr_pps = fr->fr_pps;
	old->fr_statemax = fr->fr_statemax;
	old->fr_flineno = fr->fr_flineno;
	old->fr_type = fr->fr_type;
	old->fr_flags = fr->fr_flags;
	old->fr_logtag = fr->fr_logtag;
	old->fr_collect = fr->fr_collect;
	old->fr_arg = fr->fr_arg;
	old->fr_loglevel = fr->fr_loglevel;
	old->fr_age[0] = fr->fr_age[0];
	old->fr_age[1] = fr->fr_age[1];
	old->fr_v = fr->fr_v;
	old->fr_icode = fr->fr_icode;
	bcopy(&fr->fr_group, &old->fr_group, sizeof(fr->fr_group));
	bcopy(&fr->fr_grhead, &old->fr_grhead, sizeof(fr->fr_grhead));
	bcopy(&fr->fr_nattag, &old->fr_nattag, sizeof(fr->fr_nattag));
	bcopy(&fr->fr_ifnames, &old->fr_ifnames, sizeof(fr->fr_ifnames));
	bcopy(&fr->fr_isctag, &old->fr_isctag, sizeof(fr->fr_isctag));
	bcopy(&fr->fr_tifs, &old->fr_tifs, sizeof(fr->fr_tifs));
	bcopy(&fr->fr_dif, &old->fr_dif, sizeof(fr->fr_dif));
	old->fr_cksum = fr->fr_cksum;
}


static void
fr_info_current_to_24(void *current, fr_info_4_1_24_t *old)
{
	fr_info_t *fin = (fr_info_t *)current;

	old->fin_ifp = fin->fin_ifp;
	old->fin_fi = fin->fin_fi;
	bcopy(&fin->fin_dat, &old->fin_dat, sizeof(fin->fin_dat));
	old->fin_out = fin->fin_out;
	old->fin_rev = fin->fin_rev;
	old->fin_hlen = fin->fin_hlen;
	old->fin_tcpf = fin->fin_tcpf;
	old->fin_icode = fin->fin_icode;
	old->fin_rule = fin->fin_rule;
	bcopy(fin->fin_group, old->fin_group, sizeof(fin->fin_group));
	old->fin_fr = fin->fin_fr;
	old->fin_dp = fin->fin_dp;
	old->fin_dlen = fin->fin_dlen;
	old->fin_plen = fin->fin_plen;
	old->fin_ipoff = fin->fin_ipoff;
	old->fin_id = fin->fin_id;
	old->fin_off = fin->fin_off;
	old->fin_depth = fin->fin_depth;
	old->fin_error = fin->fin_error;
	old->fin_cksum = fin->fin_cksum;
	old->fin_state = NULL;
	old->fin_nat = NULL;
	old->fin_nattag = fin->fin_nattag;
	old->fin_exthdr = fin->fin_exthdr;
	old->fin_ip = fin->fin_ip;
	old->fin_mp = fin->fin_mp;
	old->fin_m = fin->fin_m;
#ifdef  MENTAT
	old->fin_qfm = fin->fin_qfm;
	old->fin_qpi = fin->fin_qpi;
	old->fin_ifname = fin_ifname;
#endif
#ifdef  __sgi
	old->fin_hbuf = fin->fin_hbuf;
#endif
}


static void
fr_info_current_to_23(void *current, fr_info_4_1_23_t *old)
{
	fr_info_t *fin = (fr_info_t *)current;

	old->fin_ifp = fin->fin_ifp;
	old->fin_fi = fin->fin_fi;
	bcopy(&fin->fin_dat, &old->fin_dat, sizeof(fin->fin_dat));
	old->fin_out = fin->fin_out;
	old->fin_rev = fin->fin_rev;
	old->fin_hlen = fin->fin_hlen;
	old->fin_tcpf = fin->fin_tcpf;
	old->fin_icode = fin->fin_icode;
	old->fin_rule = fin->fin_rule;
	bcopy(fin->fin_group, old->fin_group, sizeof(fin->fin_group));
	old->fin_fr = fin->fin_fr;
	old->fin_dp = fin->fin_dp;
	old->fin_dlen = fin->fin_dlen;
	old->fin_plen = fin->fin_plen;
	old->fin_ipoff = fin->fin_ipoff;
	old->fin_id = fin->fin_id;
	old->fin_off = fin->fin_off;
	old->fin_depth = fin->fin_depth;
	old->fin_error = fin->fin_error;
	old->fin_state = NULL;
	old->fin_nat = NULL;
	old->fin_nattag = fin->fin_nattag;
	old->fin_exthdr = fin->fin_exthdr;
	old->fin_ip = fin->fin_ip;
	old->fin_mp = fin->fin_mp;
	old->fin_m = fin->fin_m;
#ifdef  MENTAT
	old->fin_qfm = fin->fin_qfm;
	old->fin_qpi = fin->fin_qpi;
	old->fin_ifname = fin_ifname;
#endif
#ifdef  __sgi
	old->fin_hbuf = fin->fin_hbuf;
#endif
}


static void
fr_info_current_to_11(void *current, fr_info_4_1_11_t *old)
{
	fr_info_t *fin = (fr_info_t *)current;

	old->fin_ifp = fin->fin_ifp;
	old->fin_fi = fin->fin_fi;
	bcopy(&fin->fin_dat, &old->fin_dat, sizeof(fin->fin_dat));
	old->fin_out = fin->fin_out;
	old->fin_rev = fin->fin_rev;
	old->fin_hlen = fin->fin_hlen;
	old->fin_tcpf = fin->fin_tcpf;
	old->fin_icode = fin->fin_icode;
	old->fin_rule = fin->fin_rule;
	bcopy(fin->fin_group, old->fin_group, sizeof(fin->fin_group));
	old->fin_fr = fin->fin_fr;
	old->fin_dp = fin->fin_dp;
	old->fin_dlen = fin->fin_dlen;
	old->fin_plen = fin->fin_plen;
	old->fin_ipoff = fin->fin_ipoff;
	old->fin_id = fin->fin_id;
	old->fin_off = fin->fin_off;
	old->fin_depth = fin->fin_depth;
	old->fin_error = fin->fin_error;
	old->fin_state = NULL;
	old->fin_nat = NULL;
	old->fin_nattag = fin->fin_nattag;
	old->fin_ip = fin->fin_ip;
	old->fin_mp = fin->fin_mp;
	old->fin_m = fin->fin_m;
#ifdef  MENTAT
	old->fin_qfm = fin->fin_qfm;
	old->fin_qpi = fin->fin_qpi;
	old->fin_ifname = fin_ifname;
#endif
#ifdef  __sgi
	old->fin_hbuf = fin->fin_hbuf;
#endif
}


static void
frauth_current_to_29(void *current, frauth_4_1_29_t *old)
{
	frauth_t *fra = (frauth_t *)current;

	old->fra_age = fra->fra_age;
	old->fra_len = fra->fra_len;
	old->fra_index = fra->fra_index;
	old->fra_pass = fra->fra_pass;
	fr_info_current_to_24(&fra->fra_info, &old->fra_info);
	old->fra_buf = fra->fra_buf;
	old->fra_flx = fra->fra_flx;
#ifdef	MENTAT
	old->fra_q = fra->fra_q;
	old->fra_m = fra->fra_m;
#endif
}


static void
frauth_current_to_24(void *current, frauth_4_1_24_t *old)
{
	frauth_t *fra = (frauth_t *)current;

	old->fra_age = fra->fra_age;
	old->fra_len = fra->fra_len;
	old->fra_index = fra->fra_index;
	old->fra_pass = fra->fra_pass;
	fr_info_current_to_24(&fra->fra_info, &old->fra_info);
	old->fra_buf = fra->fra_buf;
#ifdef	MENTAT
	old->fra_q = fra->fra_q;
	old->fra_m = fra->fra_m;
#endif
}


static void
frauth_current_to_23(void *current, frauth_4_1_23_t *old)
{
	frauth_t *fra = (frauth_t *)current;

	old->fra_age = fra->fra_age;
	old->fra_len = fra->fra_len;
	old->fra_index = fra->fra_index;
	old->fra_pass = fra->fra_pass;
	fr_info_current_to_23(&fra->fra_info, &old->fra_info);
	old->fra_buf = fra->fra_buf;
#ifdef	MENTAT
	old->fra_q = fra->fra_q;
	old->fra_m = fra->fra_m;
#endif
}


static void
frauth_current_to_11(void *current, frauth_4_1_11_t *old)
{
	frauth_t *fra = (frauth_t *)current;

	old->fra_age = fra->fra_age;
	old->fra_len = fra->fra_len;
	old->fra_index = fra->fra_index;
	old->fra_pass = fra->fra_pass;
	fr_info_current_to_11(&fra->fra_info, &old->fra_info);
	old->fra_buf = fra->fra_buf;
#ifdef	MENTAT
	old->fra_q = fra->fra_q;
	old->fra_m = fra->fra_m;
#endif
}


static void
ipnat_current_to_0(void *current, ipnat_4_1_0_t *old)
{
	ipnat_t *np = (ipnat_t *)current;

	old->in_next = np->in_next;
	old->in_rnext = np->in_rnext;
	old->in_prnext = np->in_prnext;
	old->in_mnext = np->in_mnext;
	old->in_pmnext = np->in_pmnext;
	old->in_tqehead[0] = np->in_tqehead[0];
	old->in_tqehead[1] = np->in_tqehead[1];
	old->in_ifps[0] = np->in_ifps[0];
	old->in_ifps[1] = np->in_ifps[1];
	old->in_apr = np->in_apr;
	old->in_comment = np->in_comment;
	old->in_next6 = np->in_next6;
	old->in_space = np->in_space;
	old->in_hits = np->in_hits;
	old->in_use = np->in_use;
	old->in_hv = np->in_hv;
	old->in_flineno = np->in_flineno;
	old->in_pnext = np->in_pnext;
	old->in_v = np->in_v;
	old->in_xxx = np->in_xxx;
	old->in_flags = np->in_flags;
	old->in_mssclamp = np->in_mssclamp;
	bcopy(&np->in_age, &old->in_age, sizeof(np->in_age));
	old->in_redir = np->in_redir;
	old->in_p = np->in_p;
	bcopy(&np->in_in, &old->in_in, sizeof(np->in_in));
	bcopy(&np->in_out, &old->in_out, sizeof(np->in_out));
	bcopy(&np->in_src, &old->in_src, sizeof(np->in_src));
	bcopy(&np->in_tuc, &old->in_tuc, sizeof(np->in_tuc));
	old->in_port[0] = np->in_port[0];
	old->in_port[1] = np->in_port[1];
	old->in_ppip = np->in_ppip;
	old->in_ippip = np->in_ippip;
	bcopy(&np->in_ifnames, &old->in_ifnames, sizeof(np->in_ifnames));
	bcopy(&np->in_plabel, &old->in_plabel, sizeof(np->in_plabel));
	bcopy(&np->in_tag, &old->in_tag, sizeof(np->in_tag));
}


static void
ipstate_current_to_0(void *current, ipstate_4_1_0_t *old)
{
	ipstate_t *is = (ipstate_t *)current;

	old->is_lock = is->is_lock;
	old->is_next = is->is_next;
	old->is_pnext = is->is_pnext;
	old->is_hnext = is->is_hnext;
	old->is_phnext = is->is_phnext;
	old->is_me = is->is_me;
	old->is_ifp[0] = is->is_ifp[0];
	old->is_ifp[1] = is->is_ifp[1];
	old->is_sync = is->is_sync;
	bzero(&old->is_nat, sizeof(old->is_nat));
	old->is_rule = is->is_rule;
	old->is_tqehead[0] = is->is_tqehead[0];
	old->is_tqehead[1] = is->is_tqehead[1];
	old->is_isc = is->is_isc;
	old->is_pkts[0] = is->is_pkts[0];
	old->is_pkts[1] = is->is_pkts[1];
	old->is_pkts[2] = is->is_pkts[2];
	old->is_pkts[3] = is->is_pkts[3];
	old->is_bytes[0] = is->is_bytes[0];
	old->is_bytes[1] = is->is_bytes[1];
	old->is_bytes[2] = is->is_bytes[2];
	old->is_bytes[3] = is->is_bytes[3];
	old->is_icmppkts[0] = is->is_icmppkts[0];
	old->is_icmppkts[1] = is->is_icmppkts[1];
	old->is_icmppkts[2] = is->is_icmppkts[2];
	old->is_icmppkts[3] = is->is_icmppkts[3];
	old->is_sti = is->is_sti;
	old->is_frage[0] = is->is_frage[0];
	old->is_frage[1] = is->is_frage[1];
	old->is_ref = is->is_ref;
	old->is_isninc[0] = is->is_isninc[0];
	old->is_isninc[1] = is->is_isninc[1];
	old->is_sumd[0] = is->is_sumd[0];
	old->is_sumd[1] = is->is_sumd[1];
	old->is_src = is->is_src;
	old->is_dst = is->is_dst;
	old->is_pass = is->is_pass;
	old->is_p = is->is_p;
	old->is_v = is->is_v;
	old->is_hv = is->is_hv;
	old->is_tag = is->is_tag;
	old->is_opt[0] = is->is_opt[0];
	old->is_opt[1] = is->is_opt[1];
	old->is_optmsk[0] = is->is_optmsk[0];
	old->is_optmsk[1] = is->is_optmsk[1];
	old->is_sec = is->is_sec;
	old->is_secmsk = is->is_secmsk;
	old->is_auth = is->is_auth;
	old->is_authmsk = is->is_authmsk;
	bcopy(&is->is_ps, &old->is_ps, sizeof(is->is_ps));
	old->is_flags = is->is_flags;
	old->is_flx[0][0] = is->is_flx[0][0];
	old->is_flx[0][1] = is->is_flx[0][1];
	old->is_flx[1][0] = is->is_flx[1][0];
	old->is_flx[1][1] = is->is_flx[1][1];
	old->is_rulen = is->is_rulen;
	old->is_s0[0] = is->is_s0[0];
	old->is_s0[1] = is->is_s0[1];
	old->is_smsk[0] = is->is_smsk[0];
	old->is_smsk[1] = is->is_smsk[1];
	bcopy(is->is_group, old->is_group, sizeof(is->is_group));
	bcopy(is->is_sbuf, old->is_sbuf, sizeof(is->is_sbuf));
	bcopy(is->is_ifname, old->is_ifname, sizeof(is->is_ifname));
}


static void
nat_save_current_to_16(void *current, nat_save_4_1_16_t *old)
{
	nat_save_t *nats = (nat_save_t *)current;

	old->ipn_next = nats->ipn_next;
	bcopy(&nats->ipn_nat, &old->ipn_nat, sizeof(old->ipn_nat));
	bcopy(&nats->ipn_ipnat, &old->ipn_ipnat, sizeof(old->ipn_ipnat));
	frentry_current_to_16(&nats->ipn_fr, &old->ipn_fr);
	old->ipn_dsize = nats->ipn_dsize;
	bcopy(nats->ipn_data, old->ipn_data, sizeof(nats->ipn_data));
}


static void
nat_save_current_to_14(void *current, nat_save_4_1_14_t *old)
{
	nat_save_t *nats = (nat_save_t *)current;

	old->ipn_next = nats->ipn_next;
	bcopy(&nats->ipn_nat, &old->ipn_nat, sizeof(old->ipn_nat));
	bcopy(&nats->ipn_ipnat, &old->ipn_ipnat, sizeof(old->ipn_ipnat));
	frentry_current_to_0(&nats->ipn_fr, &old->ipn_fr);
	old->ipn_dsize = nats->ipn_dsize;
	bcopy(nats->ipn_data, old->ipn_data, sizeof(nats->ipn_data));
}


static void
nat_save_current_to_3(void *current, nat_save_4_1_3_t *old)
{
	nat_save_t *nats = (nat_save_t *)current;

	old->ipn_next = nats->ipn_next;
	bcopy(&nats->ipn_nat, &old->ipn_nat, sizeof(old->ipn_nat));
	bcopy(&nats->ipn_ipnat, &old->ipn_ipnat, sizeof(old->ipn_ipnat));
	frentry_current_to_0(&nats->ipn_fr, &old->ipn_fr);
	old->ipn_dsize = nats->ipn_dsize;
	bcopy(nats->ipn_data, old->ipn_data, sizeof(nats->ipn_data));
}

#endif /* IPFILTER_COMPAT */
