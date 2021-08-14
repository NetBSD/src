/*	$NetBSD: back-asyncmeta.h,v 1.2 2021/08/14 16:14:59 christos Exp $	*/

/* back-asyncmeta.h - main header file for back-asyncmeta module */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2016-2021 The OpenLDAP Foundation.
 * Portions Copyright 2016 Symas Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

/* ACKNOWLEDGEMENTS:
 * This work was developed by Symas Corporation
 * based on back-meta module for inclusion in OpenLDAP Software.
 * This work was sponsored by Ericsson. */

#ifndef SLAPD_LDAP_H
#error "include servers/slapd/back-ldap/back-ldap.h before this file!"
#endif /* SLAPD_LDAP_H */

#ifndef SLAPD_ASYNCMETA_H
#define SLAPD_ASYNCMETA_H

#ifdef LDAP_DEVEL
#define SLAPD_META_CLIENT_PR 1
#endif /* LDAP_DEVEL */

#include "proto-asyncmeta.h"

#include "ldap_rq.h"

LDAP_BEGIN_DECL

/*
 * Set META_BACK_PRINT_CONNTREE larger than 0 to dump the connection tree (debug only)
 */
#ifndef META_BACK_PRINT_CONNTREE
#define META_BACK_PRINT_CONNTREE 0
#endif /* !META_BACK_PRINT_CONNTREE */

/*
 * A a_metasingleconn_t can be in the following, mutually exclusive states:
 *
 *	- none			(0x0U)
 *	- creating		META_BACK_FCONN_CREATING
 *	- initialized		META_BACK_FCONN_INITED
 *	- binding		LDAP_BACK_FCONN_BINDING
 *	- bound/anonymous	LDAP_BACK_FCONN_ISBOUND/LDAP_BACK_FCONN_ISANON
 *
 * possible modifiers are:
 *
 *	- privileged		LDAP_BACK_FCONN_ISPRIV
 *	- privileged, TLS	LDAP_BACK_FCONN_ISTLS
 *	- subjected to idassert	LDAP_BACK_FCONN_ISIDASR
 *	- tainted		LDAP_BACK_FCONN_TAINTED
 */

#define META_BACK_FCONN_INITED		(0x00100000U)
#define META_BACK_FCONN_CREATING	(0x00200000U)
#define META_BACK_FCONN_INVALID	        (0x00400000U)

#define	META_BACK_CONN_INITED(lc)		LDAP_BACK_CONN_ISSET((lc), META_BACK_FCONN_INITED)
#define	META_BACK_CONN_INITED_SET(lc)		LDAP_BACK_CONN_SET((lc), META_BACK_FCONN_INITED)
#define	META_BACK_CONN_INITED_CLEAR(lc)		LDAP_BACK_CONN_CLEAR((lc), META_BACK_FCONN_INITED)
#define	META_BACK_CONN_INITED_CPY(lc, mlc)	LDAP_BACK_CONN_CPY((lc), META_BACK_FCONN_INITED, (mlc))
#define	META_BACK_CONN_CREATING(lc)		LDAP_BACK_CONN_ISSET((lc), META_BACK_FCONN_CREATING)
#define	META_BACK_CONN_CREATING_SET(lc)		LDAP_BACK_CONN_SET((lc), META_BACK_FCONN_CREATING)
#define	META_BACK_CONN_CREATING_CLEAR(lc)	LDAP_BACK_CONN_CLEAR((lc), META_BACK_FCONN_CREATING)
#define	META_BACK_CONN_CREATING_CPY(lc, mlc)	LDAP_BACK_CONN_CPY((lc), META_BACK_FCONN_CREATING, (mlc))
#define	META_BACK_CONN_INVALID(lc)		LDAP_BACK_CONN_ISSET((lc), META_BACK_FCONN_INVALID)
#define	META_BACK_CONN_INVALID_SET(lc)		LDAP_BACK_CONN_SET((lc), META_BACK_FCONN_INVALID)
#define	META_BACK_CONN_INVALID_CLEAR(lc)	LDAP_BACK_CONN_CLEAR((lc), META_BACK_FCONN_INVALID)

struct a_metainfo_t;
struct a_metaconn_t;
struct a_metatarget_t;
#define	META_NOT_CANDIDATE		((ber_tag_t)0x0)
#define	META_CANDIDATE			((ber_tag_t)0x1)
#define	META_BINDING			((ber_tag_t)0x2)
#define	META_RETRYING			((ber_tag_t)0x4)

typedef struct bm_context_t {
	LDAP_STAILQ_ENTRY(bm_context_t) bc_next;
	struct a_metaconn_t *bc_mc;
	time_t			timeout;
	time_t                  stoptime;
	ldap_back_send_t	sendok;
	ldap_back_send_t	retrying;
	int candidate_match;
	volatile int bc_active;
	int searchtime;	/* stoptime is a search timelimit */
	int is_ok;
	int is_root;
	volatile sig_atomic_t bc_invalid;
	SlapReply		rs;
	Operation		*op;
	Operation               copy_op;
	LDAPControl	        **ctrls;
	int                     *msgids;
	int                     *nretries;  /* number of times to retry a failed send on an msc */
	struct berval	        c_peer_name; /* peer name of original op->o_conn*/
	SlapReply               *candidates;
} bm_context_t;

typedef struct a_metasingleconn_t {
#define META_CND_ISSET(rs,f)		( ( (rs)->sr_tag & (f) ) == (f) )
#define META_CND_SET(rs,f)		( (rs)->sr_tag |= (f) )
#define META_CND_CLEAR(rs,f)		( (rs)->sr_tag &= ~(f) )

#define META_CANDIDATE_RESET(rs)	( (rs)->sr_tag = 0 )
#define META_IS_CANDIDATE(rs)		META_CND_ISSET( (rs), META_CANDIDATE )
#define META_CANDIDATE_SET(rs)		META_CND_SET( (rs), META_CANDIDATE )
#define META_CANDIDATE_CLEAR(rs)	META_CND_CLEAR( (rs), META_CANDIDATE )
#define META_IS_BINDING(rs)		META_CND_ISSET( (rs), META_BINDING )
#define META_BINDING_SET(rs)		META_CND_SET( (rs), META_BINDING )
#define META_BINDING_CLEAR(rs)		META_CND_CLEAR( (rs), META_BINDING )
#define META_IS_RETRYING(rs)		META_CND_ISSET( (rs), META_RETRYING )
#define META_RETRYING_SET(rs)		META_CND_SET( (rs), META_RETRYING )
#define META_RETRYING_CLEAR(rs)		META_CND_CLEAR( (rs), META_RETRYING )

	LDAP            	*msc_ld;
	LDAP            	*msc_ldr;
	time_t			msc_time;
	time_t                  msc_binding_time;
	time_t                  msc_result_time;
	struct berval          	msc_bound_ndn;
	struct berval		msc_cred;
	unsigned		msc_mscflags;

	/* NOTE: lc_lcflags is redefined to msc_mscflags to reuse the macros
	 * defined for back-ldap */
#define	lc_lcflags		msc_mscflags
	volatile int msc_active;
		/* Connection for the select */
	Connection *conn;
} a_metasingleconn_t;

typedef struct a_metaconn_t {
	ldapconn_base_t		lc_base;
#define	mc_base			lc_base
//#define	mc_conn			mc_base.lcb_conn
//#define	mc_local_ndn		mc_base.lcb_local_ndn
//#define	mc_refcnt		mc_base.lcb_refcnt
//#define	mc_create_time		mc_base.lcb_create_time
//#define	mc_time			mc_base.lcb_time

	LDAP_TAILQ_ENTRY(a_metaconn_t)	mc_q;

	/* NOTE: msc_mscflags is used to recycle the #define
	 * in metasingleconn_t */
	unsigned		msc_mscflags;
	int	mc_active;

	/*
	 * means that the connection is bound;
	 * of course only one target actually is ...
	 */
	int             	mc_authz_target;
#define META_BOUND_NONE		(-1)
#define META_BOUND_ALL		(-2)

	struct a_metainfo_t	*mc_info;

	int pending_ops;
	ldap_pvt_thread_mutex_t	mc_om_mutex;
	/* queue for pending operations */
	LDAP_STAILQ_HEAD(BCList, bm_context_t) mc_om_list;
	/* supersedes the connection stuff */
	a_metasingleconn_t	*mc_conns;
} a_metaconn_t;

typedef enum meta_st_t {
#if 0 /* todo */
	META_ST_EXACT = LDAP_SCOPE_BASE,
#endif
	META_ST_SUBTREE = LDAP_SCOPE_SUBTREE,
	META_ST_SUBORDINATE = LDAP_SCOPE_SUBORDINATE,
	META_ST_REGEX /* last + 1 */
} meta_st_t;

typedef struct a_metasubtree_t {
	meta_st_t ms_type;
	union {
		struct berval msu_dn;
		struct {
			struct berval msr_regex_pattern;
			regex_t msr_regex;
		} msu_regex;
	} ms_un;
#define ms_dn ms_un.msu_dn
#define ms_regex ms_un.msu_regex.msr_regex
#define ms_regex_pattern ms_un.msu_regex.msr_regex_pattern

	struct a_metasubtree_t *ms_next;
} a_metasubtree_t;

typedef struct metafilter_t {
	struct metafilter_t *mf_next;
	struct berval mf_regex_pattern;
	regex_t mf_regex;
} metafilter_t;

typedef struct a_metacommon_t {
	int				mc_version;
	int				mc_nretries;
#define META_RETRY_UNDEFINED	(-2)
#define META_RETRY_FOREVER	(-1)
#define META_RETRY_NEVER	(0)
#define META_RETRY_DEFAULT	(2)

	unsigned		mc_flags;
#define	META_BACK_CMN_ISSET(mc,f)		( ( (mc)->mc_flags & (f) ) == (f) )
#define	META_BACK_CMN_QUARANTINE(mc)		META_BACK_CMN_ISSET( (mc), LDAP_BACK_F_QUARANTINE )
#define	META_BACK_CMN_CHASE_REFERRALS(mc)	META_BACK_CMN_ISSET( (mc), LDAP_BACK_F_CHASE_REFERRALS )
#define	META_BACK_CMN_NOREFS(mc)		META_BACK_CMN_ISSET( (mc), LDAP_BACK_F_NOREFS )
#define	META_BACK_CMN_NOUNDEFFILTER(mc)		META_BACK_CMN_ISSET( (mc), LDAP_BACK_F_NOUNDEFFILTER )
#define	META_BACK_CMN_SAVECRED(mc)		META_BACK_CMN_ISSET( (mc), LDAP_BACK_F_SAVECRED )
#define	META_BACK_CMN_ST_REQUEST(mc)		META_BACK_CMN_ISSET( (mc), LDAP_BACK_F_ST_REQUEST )

#ifdef SLAPD_META_CLIENT_PR
	/*
	 * client-side paged results:
	 * -1: accept unsolicited paged results responses
	 *  0: off
	 * >0: always request paged results with size == mt_ps
	 */
#define META_CLIENT_PR_DISABLE			(0)
#define META_CLIENT_PR_ACCEPT_UNSOLICITED	(-1)
	ber_int_t		mc_ps;
#endif /* SLAPD_META_CLIENT_PR */

	slap_retry_info_t	mc_quarantine;
	time_t			mc_network_timeout;
	struct timeval	mc_bind_timeout;
#define META_BIND_TIMEOUT	LDAP_BACK_RESULT_UTIMEOUT
	time_t			mc_timeout[ SLAP_OP_LAST ];
} a_metacommon_t;

typedef struct a_metatarget_t {
	char			*mt_uri;
	ldap_pvt_thread_mutex_t	mt_uri_mutex;

	/* TODO: we might want to enable different strategies
	 * for different targets */
	LDAP_REBIND_PROC	*mt_rebind_f;
	LDAP_URLLIST_PROC	*mt_urllist_f;
	void			*mt_urllist_p;

	metafilter_t	*mt_filter;
	a_metasubtree_t		*mt_subtree;
	/* F: subtree-include; T: subtree-exclude */
	int			mt_subtree_exclude;

	int			mt_scope;

	struct berval		mt_psuffix;		/* pretty suffix */
	struct berval		mt_nsuffix;		/* normalized suffix */

	struct berval		mt_lsuffixm;	/* local suffix for massage */
	struct berval		mt_rsuffixm;	/* remote suffix for massage */

	struct berval		mt_binddn;
	struct berval		mt_bindpw;

	/* we only care about the TLS options here */
	slap_bindconf		mt_tls;

	slap_idassert_t		mt_idassert;
#define	mt_idassert_mode	mt_idassert.si_mode
#define	mt_idassert_authcID	mt_idassert.si_bc.sb_authcId
#define	mt_idassert_authcDN	mt_idassert.si_bc.sb_binddn
#define	mt_idassert_passwd	mt_idassert.si_bc.sb_cred
#define	mt_idassert_authzID	mt_idassert.si_bc.sb_authzId
#define	mt_idassert_authmethod	mt_idassert.si_bc.sb_method
#define	mt_idassert_sasl_mech	mt_idassert.si_bc.sb_saslmech
#define	mt_idassert_sasl_realm	mt_idassert.si_bc.sb_realm
#define	mt_idassert_secprops	mt_idassert.si_bc.sb_secprops
#define	mt_idassert_tls		mt_idassert.si_bc.sb_tls
#define	mt_idassert_flags	mt_idassert.si_flags
#define	mt_idassert_authz	mt_idassert.si_authz

	sig_atomic_t		mt_isquarantined;
	ldap_pvt_thread_mutex_t	mt_quarantine_mutex;

	a_metacommon_t	mt_mc;
#define	mt_nretries	mt_mc.mc_nretries
#define	mt_flags	mt_mc.mc_flags
#define	mt_version	mt_mc.mc_version
#define	mt_ps		mt_mc.mc_ps
#define	mt_network_timeout	mt_mc.mc_network_timeout
#define	mt_bind_timeout	mt_mc.mc_bind_timeout
#define	mt_timeout	mt_mc.mc_timeout
#define	mt_quarantine	mt_mc.mc_quarantine

#define	META_BACK_TGT_ISSET(mt,f)		( ( (mt)->mt_flags & (f) ) == (f) )
#define	META_BACK_TGT_ISMASK(mt,m,f)		( ( (mt)->mt_flags & (m) ) == (f) )

#define META_BACK_TGT_SAVECRED(mt)		META_BACK_TGT_ISSET( (mt), LDAP_BACK_F_SAVECRED )

#define META_BACK_TGT_USE_TLS(mt)		META_BACK_TGT_ISSET( (mt), LDAP_BACK_F_USE_TLS )
#define META_BACK_TGT_PROPAGATE_TLS(mt)		META_BACK_TGT_ISSET( (mt), LDAP_BACK_F_PROPAGATE_TLS )
#define META_BACK_TGT_TLS_CRITICAL(mt)		META_BACK_TGT_ISSET( (mt), LDAP_BACK_F_TLS_CRITICAL )

#define META_BACK_TGT_CHASE_REFERRALS(mt)	META_BACK_TGT_ISSET( (mt), LDAP_BACK_F_CHASE_REFERRALS )

#define	META_BACK_TGT_T_F(mt)			META_BACK_TGT_ISMASK( (mt), LDAP_BACK_F_T_F_MASK, LDAP_BACK_F_T_F )
#define	META_BACK_TGT_T_F_DISCOVER(mt)		META_BACK_TGT_ISMASK( (mt), LDAP_BACK_F_T_F_MASK2, LDAP_BACK_F_T_F_DISCOVER )

#define	META_BACK_TGT_ABANDON(mt)		META_BACK_TGT_ISMASK( (mt), LDAP_BACK_F_CANCEL_MASK, LDAP_BACK_F_CANCEL_ABANDON )
#define	META_BACK_TGT_IGNORE(mt)		META_BACK_TGT_ISMASK( (mt), LDAP_BACK_F_CANCEL_MASK, LDAP_BACK_F_CANCEL_IGNORE )
#define	META_BACK_TGT_CANCEL(mt)		META_BACK_TGT_ISMASK( (mt), LDAP_BACK_F_CANCEL_MASK, LDAP_BACK_F_CANCEL_EXOP )
#define	META_BACK_TGT_CANCEL_DISCOVER(mt)	META_BACK_TGT_ISMASK( (mt), LDAP_BACK_F_CANCEL_MASK2, LDAP_BACK_F_CANCEL_EXOP_DISCOVER )
#define	META_BACK_TGT_QUARANTINE(mt)		META_BACK_TGT_ISSET( (mt), LDAP_BACK_F_QUARANTINE )

#ifdef SLAP_CONTROL_X_SESSION_TRACKING
#define	META_BACK_TGT_ST_REQUEST(mt)		META_BACK_TGT_ISSET( (mt), LDAP_BACK_F_ST_REQUEST )
#define	META_BACK_TGT_ST_RESPONSE(mt)		META_BACK_TGT_ISSET( (mt), LDAP_BACK_F_ST_RESPONSE )
#endif /* SLAP_CONTROL_X_SESSION_TRACKING */

#define	META_BACK_TGT_NOREFS(mt)		META_BACK_TGT_ISSET( (mt), LDAP_BACK_F_NOREFS )
#define	META_BACK_TGT_NOUNDEFFILTER(mt)		META_BACK_TGT_ISSET( (mt), LDAP_BACK_F_NOUNDEFFILTER )

#define META_BACK_CFG_MAX_PENDING_OPS		0x80
#define META_BACK_CFG_MAX_TARGET_CONNS		0xFF
#define META_BACK_CFG_DEFAULT_OPS_TIMEOUT	0x02

/* the interval of the timeout checking loop in microseconds
 * possibly make this configurable? */
#define META_BACK_CFG_MAX_TIMEOUT_LOOP		0x70000
	slap_mask_t		mt_rep_flags;
	int			mt_timeout_ops;
} a_metatarget_t;

typedef struct a_metadncache_t {
	ldap_pvt_thread_mutex_t mutex;
	Avlnode			*tree;

#define META_DNCACHE_DISABLED   (0)
#define META_DNCACHE_FOREVER    ((time_t)(-1))
	time_t			ttl;  /* seconds; 0: no cache, -1: no expiry */
} a_metadncache_t;

typedef struct a_metacandidates_t {
	int			mc_ntargets;
	SlapReply		*mc_candidates;
} a_metacandidates_t;

/*
 * Hook to allow mucking with a_metainfo_t/a_metatarget_t when quarantine is over
 */
typedef int (*asyncmeta_quarantine_f)( struct a_metainfo_t *, int target, void * );

struct meta_out_message_t;

typedef struct a_metainfo_t {
	int			mi_ntargets;
	int			mi_defaulttarget;
#define META_DEFAULT_TARGET_NONE	(-1)

#define	mi_nretries	mi_mc.mc_nretries
#define	mi_flags	mi_mc.mc_flags
#define	mi_version	mi_mc.mc_version
#define	mi_ps		mi_mc.mc_ps
#define	mi_network_timeout	mi_mc.mc_network_timeout
#define	mi_bind_timeout	mi_mc.mc_bind_timeout
#define	mi_timeout	mi_mc.mc_timeout
#define	mi_quarantine	mi_mc.mc_quarantine

	a_metatarget_t		**mi_targets;
	a_metacandidates_t	*mi_candidates;

	LDAP_REBIND_PROC	*mi_rebind_f;
	LDAP_URLLIST_PROC	*mi_urllist_f;

	a_metadncache_t		mi_cache;

	struct {
		int						mic_num;
		LDAP_TAILQ_HEAD(mc_conn_priv_q, a_metaconn_t)	mic_priv;
	}			mi_conn_priv[ LDAP_BACK_PCONN_LAST ];
	int			mi_conn_priv_max;

	/* NOTE: quarantine uses the connection mutex */
	asyncmeta_quarantine_f	mi_quarantine_f;
	void			*mi_quarantine_p;

#define	li_flags		mi_flags
/* uses flags as defined in <back-ldap/back-ldap.h> */
#define	META_BACK_F_ONERR_STOP		LDAP_BACK_F_ONERR_STOP
#define	META_BACK_F_ONERR_REPORT	(0x02000000U)
#define	META_BACK_F_ONERR_MASK		(META_BACK_F_ONERR_STOP|META_BACK_F_ONERR_REPORT)
#define	META_BACK_F_DEFER_ROOTDN_BIND	(0x04000000U)
#define	META_BACK_F_PROXYAUTHZ_ALWAYS	(0x08000000U)	/* users always proxyauthz */
#define	META_BACK_F_PROXYAUTHZ_ANON	(0x10000000U)	/* anonymous always proxyauthz */
#define	META_BACK_F_PROXYAUTHZ_NOANON	(0x20000000U)	/* anonymous remains anonymous */

#define	META_BACK_ONERR_STOP(mi)	LDAP_BACK_ISSET( (mi), META_BACK_F_ONERR_STOP )
#define	META_BACK_ONERR_REPORT(mi)	LDAP_BACK_ISSET( (mi), META_BACK_F_ONERR_REPORT )
#define	META_BACK_ONERR_CONTINUE(mi)	( !LDAP_BACK_ISSET( (mi), META_BACK_F_ONERR_MASK ) )

#define META_BACK_DEFER_ROOTDN_BIND(mi)	LDAP_BACK_ISSET( (mi), META_BACK_F_DEFER_ROOTDN_BIND )
#define META_BACK_PROXYAUTHZ_ALWAYS(mi)	LDAP_BACK_ISSET( (mi), META_BACK_F_PROXYAUTHZ_ALWAYS )
#define META_BACK_PROXYAUTHZ_ANON(mi)	LDAP_BACK_ISSET( (mi), META_BACK_F_PROXYAUTHZ_ANON )
#define META_BACK_PROXYAUTHZ_NOANON(mi)	LDAP_BACK_ISSET( (mi), META_BACK_F_PROXYAUTHZ_NOANON )

#define META_BACK_QUARANTINE(mi)	LDAP_BACK_ISSET( (mi), LDAP_BACK_F_QUARANTINE )

	time_t			mi_idle_timeout;
	struct re_s *mi_task;

	a_metacommon_t	mi_mc;
	ldap_extra_t	*mi_ldap_extra;

	int                    mi_max_timeout_ops;
	int                    mi_max_pending_ops;
	int                    mi_max_target_conns;
	/* mutex for access to the connection structures */
	ldap_pvt_thread_mutex_t	mi_mc_mutex;
	int                    mi_num_conns;
	int                    mi_next_conn;
	a_metaconn_t          *mi_conns;

	struct berval		mi_suffix;
} a_metainfo_t;

typedef enum meta_op_type {
	META_OP_ALLOW_MULTIPLE = 0,
	META_OP_REQUIRE_SINGLE,
	META_OP_REQUIRE_ALL
} meta_op_type;

/* Whatever context asyncmeta_dn_massage needs... */
typedef struct a_dncookie {
	Operation		*op;
	struct a_metatarget_t	*target;
	void	*memctx;
	int	to_from;
} a_dncookie;


#define MASSAGE_REQ	0
#define MASSAGE_REP	1

extern void
asyncmeta_dn_massage(a_dncookie *dc, struct berval *dn,
	struct berval *res);

extern void
asyncmeta_filter_map_rewrite(
	a_dncookie	*dc,
	Filter		*f,
	struct berval	*fstr );

extern void
asyncmeta_back_referral_result_rewrite(
	a_dncookie	*dc,
	BerVarray	a_vals );

extern a_metaconn_t *
asyncmeta_getconn(
	Operation		*op,
	SlapReply		*rs,
	SlapReply               *candidates,
	int			*candidate,
	ldap_back_send_t	sendok,
	int                     alloc_new);


extern int
asyncmeta_init_one_conn(
	Operation		*op,
	SlapReply		*rs,
	a_metaconn_t		*mc,
	int			candidate,
	int			ispriv,
	ldap_back_send_t	sendok,
	int			dolock );

extern void
asyncmeta_quarantine(
	Operation		*op,
	a_metainfo_t            *mi,
	SlapReply		*rs,
	int			candidate );

extern int
asyncmeta_proxy_authz_cred(
	a_metaconn_t		*mc,
	int			candidate,
	Operation		*op,
	SlapReply		*rs,
	ldap_back_send_t	sendok,
	struct berval		*binddn,
	struct berval		*bindcred,
	int			*method );

extern int
asyncmeta_controls_add(
	Operation	*op,
	SlapReply	*rs,
	a_metaconn_t	*mc,
	int		candidate,
	int             isroot,
	LDAPControl	***pctrls );

extern int
asyncmeta_LTX_init_module(
	int			argc,
	char			*argv[] );

/*
 * Candidate stuff
 */
extern int
asyncmeta_is_candidate(
	a_metatarget_t		*mt,
	struct berval		*ndn,
	int			scope );

extern int
asyncmeta_select_unique_candidate(
	a_metainfo_t		*mi,
	struct berval		*ndn );

extern int
asyncmeta_clear_unused_candidates(
	Operation		*op,
	int			candidate,
	a_metaconn_t            *mc,
	SlapReply	*candidates);

/*
 * Dn cache stuff (experimental)
 */
extern int
asyncmeta_dncache_cmp(
	const void		*c1,
	const void		*c2 );

extern int
asyncmeta_dncache_dup(
	void			*c1,
	void			*c2 );

#define META_TARGET_NONE	(-1)
#define META_TARGET_MULTIPLE	(-2)
extern int
asyncmeta_dncache_get_target(
	a_metadncache_t		*cache,
	struct berval		*ndn );

extern int
meta_dncache_update_entry(
	a_metadncache_t		*cache,
	struct berval		*ndn,
	int			target );

extern int
asyncmeta_dncache_delete_entry(
	a_metadncache_t		*cache,
	struct berval		*ndn );

extern void
asyncmeta_dncache_free( void *entry );

extern int
asyncmeta_subtree_destroy( a_metasubtree_t *ms );

extern void
asyncmeta_filter_destroy( metafilter_t *mf );

extern int
asyncmeta_target_finish( a_metainfo_t *mi, a_metatarget_t *mt,
	const char *log, char *msg, size_t msize
);


extern LDAP_REBIND_PROC		asyncmeta_back_default_rebind;
extern LDAP_URLLIST_PROC	asyncmeta_back_default_urllist;

/* IGNORE means that target does not (no longer) participate
 * in the search;
 * NOTREADY means the search on that target has not been initialized yet
 */
#define	META_MSGID_IGNORE	(-1)
#define	META_MSGID_NEED_BIND	(-2)
#define	META_MSGID_CONNECTING	(-3)
#define META_MSGID_UNDEFINED    (-4)
#define META_MSGID_GOT_BIND     (-5)

typedef enum meta_search_candidate_t {
	META_SEARCH_UNDEFINED = -2,
	META_SEARCH_ERR = -1,
	META_SEARCH_NOT_CANDIDATE,
	META_SEARCH_CANDIDATE,
	META_SEARCH_BINDING,
	META_SEARCH_NEED_BIND,
	META_SEARCH_CONNECTING
} meta_search_candidate_t;

Operation* asyncmeta_copy_op(Operation *op);
void asyncmeta_clear_bm_context(bm_context_t *bc);

int asyncmeta_add_message_queue(a_metaconn_t *mc, bm_context_t *bc);
void asyncmeta_drop_bc(a_metaconn_t *mc, bm_context_t *bc);
void asyncmeta_drop_bc_from_fconn(bm_context_t *bc);

bm_context_t *
asyncmeta_find_message(ber_int_t msgid, a_metaconn_t *mc, int candidate);

void* asyncmeta_op_handle_result(void *ctx, void *arg);
int asyncmeta_back_cleanup( Operation *op, SlapReply *rs, bm_context_t *bm );

int
asyncmeta_clear_one_msc(
	Operation	*op,
	a_metaconn_t	*msc,
	int		candidate,
	int             unbind,
	const char *          caller);

a_metaconn_t *
asyncmeta_get_next_mc( a_metainfo_t *mi );

void* asyncmeta_timeout_loop(void *ctx, void *arg);

int
asyncmeta_start_timeout_loop(a_metatarget_t *mt, a_metainfo_t *mi);

void asyncmeta_set_msc_time(a_metasingleconn_t *msc);

int asyncmeta_back_cancel(
	a_metaconn_t	*mc,
	Operation		*op,
	ber_int_t		msgid,
	int				candidate );

void
asyncmeta_send_result(bm_context_t* bc, int error, char *text);

int asyncmeta_new_bm_context(Operation *op,
			     SlapReply *rs,
			     bm_context_t **new_bc,
			     int ntargets,
			     a_metainfo_t       *mi);

int asyncmeta_start_listeners(a_metaconn_t *mc, SlapReply *candidates,  bm_context_t *bc);
int asyncmeta_start_one_listener(a_metaconn_t *mc, SlapReply *candidates,  bm_context_t *bc, int candidate);

meta_search_candidate_t
asyncmeta_back_search_start(
	Operation             *op,
	SlapReply             *rs,
	a_metaconn_t          *mc,
	bm_context_t          *bc,
	int                   candidate,
	struct berval	      *prcookie,
	ber_int_t	      prsize,
	int do_lock);

meta_search_candidate_t
asyncmeta_dobind_init(
	Operation            *op,
	SlapReply            *rs,
	bm_context_t         *bc,
	a_metaconn_t         *mc,
	int                  candidate);

meta_search_candidate_t
asyncmeta_dobind_init_with_retry(
	Operation            *op,
	SlapReply            *rs,
	bm_context_t         *bc,
	a_metaconn_t         *mc,
	int                  candidate);

meta_search_candidate_t
asyncmeta_back_add_start(Operation *op,
			 SlapReply *rs,
			 a_metaconn_t *mc,
			 bm_context_t *bc,
			 int candidate,
			 int do_lock);
meta_search_candidate_t
asyncmeta_back_modify_start(Operation *op,
			    SlapReply *rs,
			    a_metaconn_t *mc,
			    bm_context_t *bc,
			    int candidate,
			    int do_lock);

meta_search_candidate_t
asyncmeta_back_modrdn_start(Operation *op,
			    SlapReply *rs,
			    a_metaconn_t *mc,
			    bm_context_t *bc,
			    int candidate,
			    int do_lock);
meta_search_candidate_t
asyncmeta_back_delete_start(Operation *op,
			    SlapReply *rs,
			    a_metaconn_t *mc,
			    bm_context_t *bc,
			    int candidate,
			    int do_lock);

meta_search_candidate_t
asyncmeta_back_compare_start(Operation *op,
			     SlapReply *rs,
			     a_metaconn_t *mc,
			     bm_context_t *bc,
			     int candidate,
			     int do_lock);

bm_context_t *
asyncmeta_bc_in_queue(a_metaconn_t *mc,
		      bm_context_t *bc);

int
asyncmeta_error_cleanup(Operation *op,
			SlapReply *rs,
			bm_context_t *bc,
			a_metaconn_t *mc,
			int candidate);

int
asyncmeta_reset_msc(Operation	*op,
		    a_metaconn_t	*mc,
		    int		candidate,
		    int             unbind,
		    const char *caller);


void
asyncmeta_back_conn_free(
	            void 		*v_mc );

void asyncmeta_log_msc(a_metasingleconn_t *msc);
void asyncmeta_log_conns(a_metainfo_t *mi);

void asyncmeta_get_timestamp(char *buf);

int
asyncmeta_dncache_update_entry(a_metadncache_t	*cache,
			       struct berval	*ndn,
			       int 		target );

void
asyncmeta_dnattr_result_rewrite(a_dncookie		*dc,
				BerVarray		a_vals);

void
asyncmeta_referral_result_rewrite(a_dncookie		*dc,
				  BerVarray		a_vals);

meta_search_candidate_t
asyncmeta_send_all_pending_ops(a_metaconn_t *mc,
			       int          candidate,
			       void         *ctx,
			       int          dolock);
meta_search_candidate_t
asyncmeta_return_bind_errors(a_metaconn_t *mc,
			     int          candidate,
			     SlapReply    *bind_result,
			     void         *ctx,
			     int          dolock);

/* The the maximum time in seconds after a result has been received on a connection,
 * after which it can be reset if a sender error occurs. Should this be configurable? */
#define META_BACK_RESULT_INTERVAL (2)

extern int asyncmeta_debug;

LDAP_END_DECL

#endif /* SLAPD_ASYNCMETA_H */
