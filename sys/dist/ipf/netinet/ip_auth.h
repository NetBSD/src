/*	$NetBSD: ip_auth.h,v 1.6.32.1 2012/04/17 00:08:12 yamt Exp $	*/

/*
 * Copyright (C) 1997-2001 by Darren Reed & Guido Van Rooij.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: ip_auth.h,v 2.16.2.4 2008/03/16 06:58:36 darrenr Exp
 *
 */
#ifndef _NETINET_IP_AUTH_H_
#define _NETINET_IP_AUTH_H_

#define FR_NUMAUTH      32

typedef struct  frauth {
	int	fra_age;
	int	fra_len;
	int	fra_index;
	u_32_t	fra_pass;
	fr_info_t	fra_info;
	char	*fra_buf;
	u_32_t	fra_flx;
#ifdef	MENTAT
	queue_t	*fra_q;
	mb_t	*fra_m;
#endif
} frauth_t;

typedef	struct	frauthent  {
	struct	frentry	fae_fr;
	struct	frauthent	*fae_next;
	struct	frauthent	**fae_pnext;
	u_long	fae_age;
	int	fae_ref;
} frauthent_t;

typedef struct  fr_authstat {
	U_QUAD_T	fas_hits;
	U_QUAD_T	fas_miss;
	u_long		fas_nospace;
	u_long		fas_added;
	u_long		fas_sendfail;
	u_long		fas_sendok;
	u_long		fas_queok;
	u_long		fas_quefail;
	u_long		fas_expire;
	frauthent_t	*fas_faelist;
} fr_authstat_t;


extern	frentry_t	*ipauth;
extern	struct fr_authstat	fr_authstats;
extern	int	fr_defaultauthage;
extern	int	fr_authstart;
extern	int	fr_authend;
extern	int	fr_authsize;
extern	int	fr_authused;
extern	int	fr_auth_lock;
extern	frentry_t *fr_checkauth(fr_info_t *, u_32_t *);
extern	void	fr_authexpire(void);
extern	int	fr_authinit(void);
extern	void	fr_authunload(void);
extern	int	fr_authflush(void);
extern	mb_t	**fr_authpkts;
extern	int	fr_newauth(mb_t *, fr_info_t *);
extern	int	fr_preauthcmd(ioctlcmd_t, frentry_t *, frentry_t **);
extern	int	fr_auth_ioctl(void *, ioctlcmd_t, int, int, void *);
extern	int	fr_auth_waiting(void);

#endif	/* __IP_AUTH_H__ */
