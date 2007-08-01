/*	$NetBSD: cfparse.y,v 1.18.4.3 2007/08/01 11:52:19 vanhu Exp $	*/

/* Id: cfparse.y,v 1.66 2006/08/22 18:17:17 manubsd Exp */

%{
/*
 * Copyright (C) 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002 and 2003 WIDE Project.
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

#include "config.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include PATH_IPSEC_H

#ifdef ENABLE_HYBRID
#include <arpa/inet.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <pwd.h>
#include <grp.h>

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "sockmisc.h"
#include "str2val.h"
#include "genlist.h"
#include "debug.h"

#include "admin.h"
#include "privsep.h"
#include "cfparse_proto.h"
#include "cftoken_proto.h"
#include "algorithm.h"
#include "localconf.h"
#include "policy.h"
#include "sainfo.h"
#include "oakley.h"
#include "pfkey.h"
#include "remoteconf.h"
#include "grabmyaddr.h"
#include "isakmp_var.h"
#include "handler.h"
#include "isakmp.h"
#include "nattraversal.h"
#include "isakmp_frag.h"
#ifdef ENABLE_HYBRID
#include "resolv.h"
#include "isakmp_unity.h"
#include "isakmp_xauth.h"
#include "isakmp_cfg.h"
#endif
#include "ipsec_doi.h"
#include "strnames.h"
#include "gcmalloc.h"
#ifdef HAVE_GSSAPI
#include "gssapi.h"
#endif
#include "vendorid.h"
#include "rsalist.h"

struct proposalspec {
	time_t lifetime;		/* for isakmp/ipsec */
	int lifebyte;			/* for isakmp/ipsec */
	struct secprotospec *spspec;	/* the head is always current spec. */
	struct proposalspec *next;	/* the tail is the most prefered. */
	struct proposalspec *prev;
};

struct secprotospec {
	int prop_no;
	int trns_no;
	int strength;		/* for isakmp/ipsec */
	int encklen;		/* for isakmp/ipsec */
	time_t lifetime;	/* for isakmp */
	int lifebyte;		/* for isakmp */
	int proto_id;		/* for ipsec (isakmp?) */
	int ipsec_level;	/* for ipsec */
	int encmode;		/* for ipsec */
	int vendorid;		/* for isakmp */
	char *gssid;
	struct sockaddr *remote;
	int algclass[MAXALGCLASS];

	struct secprotospec *next;	/* the tail is the most prefiered. */
	struct secprotospec *prev;
	struct proposalspec *back;
};

static int num2dhgroup[] = {
	0,
	OAKLEY_ATTR_GRP_DESC_MODP768,
	OAKLEY_ATTR_GRP_DESC_MODP1024,
	OAKLEY_ATTR_GRP_DESC_EC2N155,
	OAKLEY_ATTR_GRP_DESC_EC2N185,
	OAKLEY_ATTR_GRP_DESC_MODP1536,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	OAKLEY_ATTR_GRP_DESC_MODP2048,
	OAKLEY_ATTR_GRP_DESC_MODP3072,
	OAKLEY_ATTR_GRP_DESC_MODP4096,
	OAKLEY_ATTR_GRP_DESC_MODP6144,
	OAKLEY_ATTR_GRP_DESC_MODP8192
};

static struct remoteconf *cur_rmconf;
static int tmpalgtype[MAXALGCLASS];
static struct sainfo *cur_sainfo;
static int cur_algclass;
static int oldloglevel = LLV_BASE;

static struct proposalspec *newprspec __P((void));
static void insprspec __P((struct proposalspec *, struct proposalspec **));
static struct secprotospec *newspspec __P((void));
static void insspspec __P((struct secprotospec *, struct proposalspec **));
static void adminsock_conf __P((vchar_t *, vchar_t *, vchar_t *, int));

static int set_isakmp_proposal
	__P((struct remoteconf *, struct proposalspec *));
static void clean_tmpalgtype __P((void));
static int expand_isakmpspec __P((int, int, int *,
	int, int, time_t, int, int, int, char *, struct remoteconf *));
static int listen_addr __P((struct sockaddr *addr, int udp_encap));

void freeetypes (struct etypes **etypes);

#if 0
static int fix_lifebyte __P((u_long));
#endif
%}

%union {
	unsigned long num;
	vchar_t *val;
	struct remoteconf *rmconf;
	struct sockaddr *saddr;
	struct sainfoalg *alg;
}

	/* privsep */
%token PRIVSEP USER GROUP CHROOT
	/* path */
%token PATH PATHTYPE
	/* include */
%token INCLUDE
	/* self information */
%token IDENTIFIER VENDORID
	/* logging */
%token LOGGING LOGLEV
	/* padding */
%token PADDING PAD_RANDOMIZE PAD_RANDOMIZELEN PAD_MAXLEN PAD_STRICT PAD_EXCLTAIL
	/* listen */
%token LISTEN X_ISAKMP X_ISAKMP_NATT X_ADMIN STRICT_ADDRESS ADMINSOCK DISABLED
	/* ldap config */
%token LDAPCFG LDAP_HOST LDAP_PORT LDAP_PVER LDAP_BASE LDAP_BIND_DN LDAP_BIND_PW LDAP_SUBTREE
%token LDAP_ATTR_USER LDAP_ATTR_ADDR LDAP_ATTR_MASK LDAP_ATTR_GROUP LDAP_ATTR_MEMBER
	/* modecfg */
%token MODECFG CFG_NET4 CFG_MASK4 CFG_DNS4 CFG_NBNS4 CFG_DEFAULT_DOMAIN
%token CFG_AUTH_SOURCE CFG_AUTH_GROUPS CFG_SYSTEM CFG_RADIUS CFG_PAM CFG_LDAP CFG_LOCAL CFG_NONE
%token CFG_GROUP_SOURCE CFG_ACCOUNTING CFG_CONF_SOURCE CFG_MOTD CFG_POOL_SIZE CFG_AUTH_THROTTLE
%token CFG_SPLIT_NETWORK CFG_SPLIT_LOCAL CFG_SPLIT_INCLUDE CFG_SPLIT_DNS
%token CFG_PFS_GROUP CFG_SAVE_PASSWD

	/* timer */
%token RETRY RETRY_COUNTER RETRY_INTERVAL RETRY_PERSEND
%token RETRY_PHASE1 RETRY_PHASE2 NATT_KA
	/* algorithm */
%token ALGORITHM_CLASS ALGORITHMTYPE STRENGTHTYPE
	/* sainfo */
%token SAINFO FROM
	/* remote */
%token REMOTE ANONYMOUS INHERIT
%token EXCHANGE_MODE EXCHANGETYPE DOI DOITYPE SITUATION SITUATIONTYPE
%token CERTIFICATE_TYPE CERTTYPE PEERS_CERTFILE CA_TYPE
%token VERIFY_CERT SEND_CERT SEND_CR
%token IDENTIFIERTYPE IDENTIFIERQUAL MY_IDENTIFIER 
%token PEERS_IDENTIFIER VERIFY_IDENTIFIER
%token DNSSEC CERT_X509 CERT_PLAINRSA
%token NONCE_SIZE DH_GROUP KEEPALIVE PASSIVE INITIAL_CONTACT
%token NAT_TRAVERSAL REMOTE_FORCE_LEVEL
%token PROPOSAL_CHECK PROPOSAL_CHECK_LEVEL
%token GENERATE_POLICY GENERATE_LEVEL SUPPORT_PROXY
%token PROPOSAL
%token EXEC_PATH EXEC_COMMAND EXEC_SUCCESS EXEC_FAILURE
%token GSS_ID GSS_ID_ENC GSS_ID_ENCTYPE
%token COMPLEX_BUNDLE
%token DPD DPD_DELAY DPD_RETRY DPD_MAXFAIL
%token PH1ID
%token XAUTH_LOGIN WEAK_PHASE1_CHECK

%token PREFIX PORT PORTANY UL_PROTO ANY IKE_FRAG ESP_FRAG MODE_CFG
%token PFS_GROUP LIFETIME LIFETYPE_TIME LIFETYPE_BYTE STRENGTH REMOTEID

%token SCRIPT PHASE1_UP PHASE1_DOWN

%token NUMBER SWITCH BOOLEAN
%token HEXSTRING QUOTEDSTRING ADDRSTRING ADDRRANGE
%token UNITTYPE_BYTE UNITTYPE_KBYTES UNITTYPE_MBYTES UNITTYPE_TBYTES
%token UNITTYPE_SEC UNITTYPE_MIN UNITTYPE_HOUR
%token EOS BOC EOC COMMA

%type <num> NUMBER BOOLEAN SWITCH keylength
%type <num> PATHTYPE IDENTIFIERTYPE IDENTIFIERQUAL LOGLEV GSS_ID_ENCTYPE 
%type <num> ALGORITHM_CLASS dh_group_num
%type <num> ALGORITHMTYPE STRENGTHTYPE
%type <num> PREFIX prefix PORT port ike_port
%type <num> ul_proto UL_PROTO
%type <num> EXCHANGETYPE DOITYPE SITUATIONTYPE
%type <num> CERTTYPE CERT_X509 CERT_PLAINRSA PROPOSAL_CHECK_LEVEL REMOTE_FORCE_LEVEL GENERATE_LEVEL
%type <num> unittype_time unittype_byte
%type <val> QUOTEDSTRING HEXSTRING ADDRSTRING ADDRRANGE sainfo_id
%type <val> identifierstring
%type <saddr> remote_index ike_addrinfo_port
%type <alg> algorithm

%%

statements
	:	/* nothing */
	|	statements statement
	;
statement
	:	privsep_statement
	|	path_statement
	|	include_statement
	|	gssenc_statement
	|	identifier_statement
	|	logging_statement
	|	padding_statement
	|	listen_statement
	|	ldapcfg_statement
	|	modecfg_statement
	|	timer_statement
	|	sainfo_statement
	|	remote_statement
	|	special_statement
	;

	/* privsep */
privsep_statement
	:	PRIVSEP BOC privsep_stmts EOC
	;
privsep_stmts
	:	/* nothing */
	|	privsep_stmts privsep_stmt
	;
privsep_stmt
	:	USER QUOTEDSTRING
		{
			struct passwd *pw;

			if ((pw = getpwnam($2->v)) == NULL) {
				yyerror("unknown user \"%s\"", $2->v);
				return -1;
			}
			lcconf->uid = pw->pw_uid;
		} 
		EOS
	|	USER NUMBER { lcconf->uid = $2; } EOS
	|	GROUP QUOTEDSTRING
		{
			struct group *gr;

			if ((gr = getgrnam($2->v)) == NULL) {
				yyerror("unknown group \"%s\"", $2->v);
				return -1;
			}
			lcconf->gid = gr->gr_gid;
		}
		EOS
	|	GROUP NUMBER { lcconf->gid = $2; } EOS
	|	CHROOT QUOTEDSTRING { lcconf->chroot = $2->v; } EOS
	;

	/* path */
path_statement
	:	PATH PATHTYPE QUOTEDSTRING
		{
			if ($2 >= LC_PATHTYPE_MAX) {
				yyerror("invalid path type %d", $2);
				return -1;
			}

			/* free old pathinfo */
			if (lcconf->pathinfo[$2])
				racoon_free(lcconf->pathinfo[$2]);

			/* set new pathinfo */
			lcconf->pathinfo[$2] = racoon_strdup($3->v);
			STRDUP_FATAL(lcconf->pathinfo[$2]);
			vfree($3);
		}
		EOS
	;

	/* special */
special_statement
	:	COMPLEX_BUNDLE SWITCH { lcconf->complex_bundle = $2; } EOS
	;

	/* include */
include_statement
	:	INCLUDE QUOTEDSTRING EOS
		{
			char path[MAXPATHLEN];

			getpathname(path, sizeof(path),
				LC_PATHTYPE_INCLUDE, $2->v);
			vfree($2);
			if (yycf_switch_buffer(path) != 0)
				return -1;
		}
	;

	/* gss_id_enc */
gssenc_statement
	:	GSS_ID_ENC GSS_ID_ENCTYPE EOS
		{
			if ($2 >= LC_GSSENC_MAX) {
				yyerror("invalid GSS ID encoding %d", $2);
				return -1;
			}
			lcconf->gss_id_enc = $2;
		}
	;

	/* self information */
identifier_statement
	:	IDENTIFIER identifier_stmt
	;
identifier_stmt
	:	VENDORID
		{
			/*XXX to be deleted */
		}
		QUOTEDSTRING EOS
	|	IDENTIFIERTYPE QUOTEDSTRING
		{
			/*XXX to be deleted */
			$2->l--;	/* nuke '\0' */
			lcconf->ident[$1] = $2;
			if (lcconf->ident[$1] == NULL) {
				yyerror("failed to set my ident: %s",
					strerror(errno));
				return -1;
			}
		}
		EOS
	;

	/* logging */
logging_statement
	:	LOGGING log_level EOS
	;
log_level
	:	HEXSTRING
		{
			/*
			 * XXX ignore it because this specification
			 * will be obsoleted.
			 */
			yywarn("see racoon.conf(5), such a log specification will be obsoleted.");
			vfree($1);
		}
	|	LOGLEV
		{
			/*
			 * set the loglevel to the value specified
			 * in the configuration file plus the number
			 * of -d options specified on the command line
			 */
			loglevel += $1 - oldloglevel;
			oldloglevel = $1;
		}
	;

	/* padding */
padding_statement
	:	PADDING BOC padding_stmts EOC
	;
padding_stmts
	:	/* nothing */
	|	padding_stmts padding_stmt
	;
padding_stmt
	:	PAD_RANDOMIZE SWITCH { lcconf->pad_random = $2; } EOS
	|	PAD_RANDOMIZELEN SWITCH { lcconf->pad_randomlen = $2; } EOS
	|	PAD_MAXLEN NUMBER { lcconf->pad_maxsize = $2; } EOS
	|	PAD_STRICT SWITCH { lcconf->pad_strict = $2; } EOS
	|	PAD_EXCLTAIL SWITCH { lcconf->pad_excltail = $2; } EOS
	;

	/* listen */
listen_statement
	:	LISTEN BOC listen_stmts EOC
	;
listen_stmts
	:	/* nothing */
	|	listen_stmts listen_stmt
	;
listen_stmt
	:	X_ISAKMP ike_addrinfo_port
		{
			listen_addr ($2, 0);
		}
		EOS
	|	X_ISAKMP_NATT ike_addrinfo_port
		{
#ifdef ENABLE_NATT
			listen_addr ($2, 1);
#else
			yyerror("NAT-T support not compiled in.");
#endif
		}
		EOS
	|	X_ADMIN
		{
			yyerror("admin directive is obsoleted.");
		}
		PORT EOS
	|	ADMINSOCK QUOTEDSTRING QUOTEDSTRING QUOTEDSTRING NUMBER 
		{
#ifdef ENABLE_ADMINPORT
			adminsock_conf($2, $3, $4, $5);
#else
			yywarn("admin port support not compiled in");
#endif
		}
		EOS
	|	ADMINSOCK QUOTEDSTRING
		{
#ifdef ENABLE_ADMINPORT
			adminsock_conf($2, NULL, NULL, -1);
#else
			yywarn("admin port support not compiled in");
#endif
		}
		EOS
	|	ADMINSOCK DISABLED
		{
#ifdef ENABLE_ADMINPORT
			adminsock_path = NULL;
#else
			yywarn("admin port support not compiled in");
#endif
		}
		EOS
	|	STRICT_ADDRESS { lcconf->strict_address = TRUE; } EOS
	;
ike_addrinfo_port
	:	ADDRSTRING ike_port
		{
			char portbuf[10];

			snprintf(portbuf, sizeof(portbuf), "%ld", $2);
			$$ = str2saddr($1->v, portbuf);
			vfree($1);
			if (!$$)
				return -1;
		}
	;
ike_port
	:	/* nothing */	{ $$ = PORT_ISAKMP; }
	|	PORT		{ $$ = $1; }
	;

	/* ldap configuration */
ldapcfg_statement
	:	LDAPCFG {
#ifndef ENABLE_HYBRID
			yyerror("racoon not configured with --enable-hybrid");
			return -1;
#endif
#ifndef HAVE_LIBLDAP
			yyerror("racoon not configured with --with-libldap");
			return -1;
#endif
		} BOC ldapcfg_stmts EOC
	;
ldapcfg_stmts
	:	/* nothing */
	|	ldapcfg_stmts ldapcfg_stmt
	;
ldapcfg_stmt
	:	LDAP_PVER NUMBER
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBLDAP
			if (($2<2)||($2>3))
				yyerror("invalid ldap protocol version (2|3)");
			xauth_ldap_config.pver = $2;
#endif
#endif
		}
		EOS
	|	LDAP_HOST QUOTEDSTRING
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBLDAP
			if (xauth_ldap_config.host != NULL)
				vfree(xauth_ldap_config.host);
			xauth_ldap_config.host = vdup($2);
#endif
#endif
		}
		EOS
	|	LDAP_PORT NUMBER
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBLDAP
			xauth_ldap_config.port = $2;
#endif
#endif
		}
		EOS
	|	LDAP_BASE QUOTEDSTRING
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBLDAP
			if (xauth_ldap_config.base != NULL)
				vfree(xauth_ldap_config.base);
			xauth_ldap_config.base = vdup($2);
#endif
#endif
		}
		EOS
	|	LDAP_SUBTREE SWITCH
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBLDAP
			xauth_ldap_config.subtree = $2;
#endif
#endif
		}
		EOS
	|	LDAP_BIND_DN QUOTEDSTRING
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBLDAP
			if (xauth_ldap_config.bind_dn != NULL)
				vfree(xauth_ldap_config.bind_dn);
			xauth_ldap_config.bind_dn = vdup($2);
#endif
#endif
		}
		EOS
	|	LDAP_BIND_PW QUOTEDSTRING
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBLDAP
			if (xauth_ldap_config.bind_pw != NULL)
				vfree(xauth_ldap_config.bind_pw);
			xauth_ldap_config.bind_pw = vdup($2);
#endif
#endif
		}
		EOS
	|	LDAP_ATTR_USER QUOTEDSTRING
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBLDAP
			if (xauth_ldap_config.attr_user != NULL)
				vfree(xauth_ldap_config.attr_user);
			xauth_ldap_config.attr_user = vdup($2);
#endif
#endif
		}
		EOS
	|	LDAP_ATTR_ADDR QUOTEDSTRING
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBLDAP
			if (xauth_ldap_config.attr_addr != NULL)
				vfree(xauth_ldap_config.attr_addr);
			xauth_ldap_config.attr_addr = vdup($2);
#endif
#endif
		}
		EOS
	|	LDAP_ATTR_MASK QUOTEDSTRING
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBLDAP
			if (xauth_ldap_config.attr_mask != NULL)
				vfree(xauth_ldap_config.attr_mask);
			xauth_ldap_config.attr_mask = vdup($2);
#endif
#endif
		}
		EOS
	|	LDAP_ATTR_GROUP QUOTEDSTRING
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBLDAP
			if (xauth_ldap_config.attr_group != NULL)
				vfree(xauth_ldap_config.attr_group);
			xauth_ldap_config.attr_group = vdup($2);
#endif
#endif
		}
		EOS
	|	LDAP_ATTR_MEMBER QUOTEDSTRING
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBLDAP
			if (xauth_ldap_config.attr_member != NULL)
				vfree(xauth_ldap_config.attr_member);
			xauth_ldap_config.attr_member = vdup($2);
#endif
#endif
		}
		EOS
	;

	/* modecfg */
modecfg_statement
	:	MODECFG BOC modecfg_stmts EOC
	;
modecfg_stmts
	:	/* nothing */
	|	modecfg_stmts modecfg_stmt
	;
modecfg_stmt
	:	CFG_NET4 ADDRSTRING
		{
#ifdef ENABLE_HYBRID
			if (inet_pton(AF_INET, $2->v,
			     &isakmp_cfg_config.network4) != 1)
				yyerror("bad IPv4 network address.");
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		}
		EOS
	|	CFG_MASK4 ADDRSTRING
		{
#ifdef ENABLE_HYBRID
			if (inet_pton(AF_INET, $2->v,
			    &isakmp_cfg_config.netmask4) != 1)
				yyerror("bad IPv4 netmask address.");
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		}
		EOS
	|	CFG_DNS4 addrdnslist
		EOS
	|	CFG_NBNS4 addrwinslist
		EOS
	|	CFG_SPLIT_NETWORK CFG_SPLIT_LOCAL splitnetlist
		{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.splitnet_type = UNITY_LOCAL_LAN;
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		}
		EOS
	|	CFG_SPLIT_NETWORK CFG_SPLIT_INCLUDE splitnetlist
		{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.splitnet_type = UNITY_SPLIT_INCLUDE;
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		}
		EOS
	|	CFG_SPLIT_DNS splitdnslist
		{
#ifndef ENABLE_HYBRID
			yyerror("racoon not configured with --enable-hybrid");
#endif
		}
		EOS
	|	CFG_DEFAULT_DOMAIN QUOTEDSTRING
		{
#ifdef ENABLE_HYBRID
			strncpy(&isakmp_cfg_config.default_domain[0], 
			    $2->v, MAXPATHLEN);
			isakmp_cfg_config.default_domain[MAXPATHLEN] = '\0';
			vfree($2);
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		}
		EOS
	|	CFG_AUTH_SOURCE CFG_SYSTEM
		{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.authsource = ISAKMP_CFG_AUTH_SYSTEM;
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		}
		EOS
	|	CFG_AUTH_SOURCE CFG_RADIUS
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBRADIUS
			isakmp_cfg_config.authsource = ISAKMP_CFG_AUTH_RADIUS;
#else /* HAVE_LIBRADIUS */
			yyerror("racoon not configured with --with-libradius");
#endif /* HAVE_LIBRADIUS */
#else /* ENABLE_HYBRID */
			yyerror("racoon not configured with --enable-hybrid");
#endif /* ENABLE_HYBRID */
		}
		EOS
	|	CFG_AUTH_SOURCE CFG_PAM
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBPAM
			isakmp_cfg_config.authsource = ISAKMP_CFG_AUTH_PAM;
#else /* HAVE_LIBPAM */
			yyerror("racoon not configured with --with-libpam");
#endif /* HAVE_LIBPAM */
#else /* ENABLE_HYBRID */
			yyerror("racoon not configured with --enable-hybrid");
#endif /* ENABLE_HYBRID */
		}
		EOS
	|	CFG_AUTH_SOURCE CFG_LDAP
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBLDAP
			isakmp_cfg_config.authsource = ISAKMP_CFG_AUTH_LDAP;
#else /* HAVE_LIBLDAP */
			yyerror("racoon not configured with --with-libldap");
#endif /* HAVE_LIBLDAP */
#else /* ENABLE_HYBRID */
			yyerror("racoon not configured with --enable-hybrid");
#endif /* ENABLE_HYBRID */
		}
		EOS
	|	CFG_AUTH_GROUPS authgrouplist
		{
#ifndef ENABLE_HYBRID
			yyerror("racoon not configured with --enable-hybrid");
#endif
		}
		EOS
	|	CFG_GROUP_SOURCE CFG_SYSTEM
		{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.groupsource = ISAKMP_CFG_GROUP_SYSTEM;
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		}
		EOS
	|	CFG_GROUP_SOURCE CFG_LDAP
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBLDAP
			isakmp_cfg_config.groupsource = ISAKMP_CFG_GROUP_LDAP;
#else /* HAVE_LIBLDAP */
			yyerror("racoon not configured with --with-libldap");
#endif /* HAVE_LIBLDAP */
#else /* ENABLE_HYBRID */
			yyerror("racoon not configured with --enable-hybrid");
#endif /* ENABLE_HYBRID */
		}
		EOS
	|	CFG_ACCOUNTING CFG_NONE
		{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.accounting = ISAKMP_CFG_ACCT_NONE;
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		}
		EOS
	|	CFG_ACCOUNTING CFG_SYSTEM
		{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.accounting = ISAKMP_CFG_ACCT_SYSTEM;
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		}
		EOS
	|	CFG_ACCOUNTING CFG_RADIUS
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBRADIUS
			isakmp_cfg_config.accounting = ISAKMP_CFG_ACCT_RADIUS;
#else /* HAVE_LIBRADIUS */
			yyerror("racoon not configured with --with-libradius");
#endif /* HAVE_LIBRADIUS */
#else /* ENABLE_HYBRID */
			yyerror("racoon not configured with --enable-hybrid");
#endif /* ENABLE_HYBRID */
		}
		EOS
	|	CFG_ACCOUNTING CFG_PAM
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBPAM
			isakmp_cfg_config.accounting = ISAKMP_CFG_ACCT_PAM;
#else /* HAVE_LIBPAM */
			yyerror("racoon not configured with --with-libpam");
#endif /* HAVE_LIBPAM */
#else /* ENABLE_HYBRID */
			yyerror("racoon not configured with --enable-hybrid");
#endif /* ENABLE_HYBRID */
		}
		EOS
	|	CFG_POOL_SIZE NUMBER
		{
#ifdef ENABLE_HYBRID
			if (isakmp_cfg_resize_pool($2) != 0)
				yyerror("cannot allocate memory for pool");
#else /* ENABLE_HYBRID */
			yyerror("racoon not configured with --enable-hybrid");
#endif /* ENABLE_HYBRID */
		}
		EOS
	|	CFG_PFS_GROUP NUMBER
		{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.pfs_group = $2;
#else /* ENABLE_HYBRID */
			yyerror("racoon not configured with --enable-hybrid");
#endif /* ENABLE_HYBRID */
		}
		EOS
	|	CFG_SAVE_PASSWD SWITCH
		{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.save_passwd = $2;
#else /* ENABLE_HYBRID */
			yyerror("racoon not configured with --enable-hybrid");
#endif /* ENABLE_HYBRID */
		}
		EOS
	|	CFG_AUTH_THROTTLE NUMBER
		{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.auth_throttle = $2;
#else /* ENABLE_HYBRID */
			yyerror("racoon not configured with --enable-hybrid");
#endif /* ENABLE_HYBRID */
		}
		EOS
	|	CFG_CONF_SOURCE CFG_LOCAL
		{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.confsource = ISAKMP_CFG_CONF_LOCAL;
#else /* ENABLE_HYBRID */
			yyerror("racoon not configured with --enable-hybrid");
#endif /* ENABLE_HYBRID */
		}
		EOS
	|	CFG_CONF_SOURCE CFG_RADIUS
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBRADIUS
			isakmp_cfg_config.confsource = ISAKMP_CFG_CONF_RADIUS;
#else /* HAVE_LIBRADIUS */
			yyerror("racoon not configured with --with-libradius");
#endif /* HAVE_LIBRADIUS */
#else /* ENABLE_HYBRID */
			yyerror("racoon not configured with --enable-hybrid");
#endif /* ENABLE_HYBRID */
		}
		EOS
	|	CFG_CONF_SOURCE CFG_LDAP
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBLDAP
			isakmp_cfg_config.confsource = ISAKMP_CFG_CONF_LDAP;
#else /* HAVE_LIBLDAP */
			yyerror("racoon not configured with --with-libldap");
#endif /* HAVE_LIBLDAP */
#else /* ENABLE_HYBRID */
			yyerror("racoon not configured with --enable-hybrid");
#endif /* ENABLE_HYBRID */
		}
		EOS
	|	CFG_MOTD QUOTEDSTRING
		{
#ifdef ENABLE_HYBRID
			strncpy(&isakmp_cfg_config.motd[0], $2->v, MAXPATHLEN);
			isakmp_cfg_config.motd[MAXPATHLEN] = '\0';
			vfree($2);
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		}
		EOS
	;

addrdnslist
	:	addrdns
	|	addrdns COMMA addrdnslist
	;
addrdns
	:	ADDRSTRING
		{
#ifdef ENABLE_HYBRID
			struct isakmp_cfg_config *icc = &isakmp_cfg_config;

			if (icc->dns4_index > MAXNS)
				yyerror("No more than %d DNS", MAXNS);
			if (inet_pton(AF_INET, $1->v,
			    &icc->dns4[icc->dns4_index++]) != 1)
				yyerror("bad IPv4 DNS address.");
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		}
	;

addrwinslist
	:	addrwins
	|	addrwins COMMA addrwinslist
	;
addrwins
	:	ADDRSTRING
		{
#ifdef ENABLE_HYBRID
			struct isakmp_cfg_config *icc = &isakmp_cfg_config;

			if (icc->nbns4_index > MAXWINS)
				yyerror("No more than %d WINS", MAXWINS);
			if (inet_pton(AF_INET, $1->v,
			    &icc->nbns4[icc->nbns4_index++]) != 1)
				yyerror("bad IPv4 WINS address.");
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		}
	;

splitnetlist
	:	splitnet
	|	splitnetlist COMMA splitnet
	;
splitnet
	:	ADDRSTRING PREFIX
		{
#ifdef ENABLE_HYBRID
			struct isakmp_cfg_config *icc = &isakmp_cfg_config;
			struct unity_network network;

			if (inet_pton(AF_INET, $1->v, &network.addr4) != 1)
				yyerror("bad IPv4 SPLIT address.");

			/* Turn $2 (the prefix) into a subnet mask */
			network.mask4.s_addr = ($2) ? htonl(~((1 << (32 - $2)) - 1)) : 0;

			/* add the network to our list */ 
			if (splitnet_list_add(&icc->splitnet_list, &network,&icc->splitnet_count))
				yyerror("Unable to allocate split network");
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		}
	;

authgrouplist
	:	authgroup
	|	authgroup COMMA authgrouplist
	;
authgroup
	:	QUOTEDSTRING
		{
#ifdef ENABLE_HYBRID
			char * groupname = NULL;
			char ** grouplist = NULL;
			struct isakmp_cfg_config *icc = &isakmp_cfg_config;

			grouplist = racoon_realloc(icc->grouplist,
					sizeof(char**)*(icc->groupcount+1));
			if (grouplist == NULL)
				yyerror("unable to allocate auth group list");

			groupname = racoon_malloc($1->l+1);
			if (groupname == NULL)
				yyerror("unable to allocate auth group name");

			memcpy(groupname,$1->v,$1->l);
			groupname[$1->l]=0;
			grouplist[icc->groupcount]=groupname;
			icc->grouplist = grouplist;
			icc->groupcount++;

			vfree($1);
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		}
	;

splitdnslist
	:	splitdns
	|	splitdns COMMA splitdnslist
	;
splitdns
	:	QUOTEDSTRING
		{
#ifdef ENABLE_HYBRID
			struct isakmp_cfg_config *icc = &isakmp_cfg_config;

			if (!icc->splitdns_len)
			{
				icc->splitdns_list = racoon_malloc($1->l);
				if(icc->splitdns_list == NULL)
					yyerror("error allocating splitdns list buffer");
				memcpy(icc->splitdns_list,$1->v,$1->l);
				icc->splitdns_len = $1->l;
			}
			else
			{
				int len = icc->splitdns_len + $1->l + 1;
				icc->splitdns_list = racoon_realloc(icc->splitdns_list,len);
				if(icc->splitdns_list == NULL)
					yyerror("error allocating splitdns list buffer");
				icc->splitdns_list[icc->splitdns_len] = ',';
				memcpy(icc->splitdns_list + icc->splitdns_len + 1, $1->v, $1->l);
				icc->splitdns_len = len;
			}
			vfree($1);
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		}
	;


	/* timer */
timer_statement
	:	RETRY BOC timer_stmts EOC
	;
timer_stmts
	:	/* nothing */
	|	timer_stmts timer_stmt
	;
timer_stmt
	:	RETRY_COUNTER NUMBER
		{
			lcconf->retry_counter = $2;
		}
		EOS
	|	RETRY_INTERVAL NUMBER unittype_time
		{
			lcconf->retry_interval = $2 * $3;
		}
		EOS
	|	RETRY_PERSEND NUMBER
		{
			lcconf->count_persend = $2;
		}
		EOS
	|	RETRY_PHASE1 NUMBER unittype_time
		{
			lcconf->retry_checkph1 = $2 * $3;
		}
		EOS
	|	RETRY_PHASE2 NUMBER unittype_time
		{
			lcconf->wait_ph2complete = $2 * $3;
		}
		EOS
	|	NATT_KA NUMBER unittype_time
		{
#ifdef ENABLE_NATT
        		if (libipsec_opt & LIBIPSEC_OPT_NATT)
				lcconf->natt_ka_interval = $2 * $3;
			else
                		yyerror("libipsec lacks NAT-T support");
#else
			yyerror("NAT-T support not compiled in.");
#endif
		}
		EOS
	;

	/* sainfo */
sainfo_statement
	:	SAINFO
		{
			cur_sainfo = newsainfo();
			if (cur_sainfo == NULL) {
				yyerror("failed to allocate sainfo");
				return -1;
			}
		}
		sainfo_name sainfo_param BOC sainfo_specs
		{
			struct sainfo *check;

			/* default */
			if (cur_sainfo->algs[algclass_ipsec_enc] == 0) {
				yyerror("no encryption algorithm at %s",
					sainfo2str(cur_sainfo));
				return -1;
			}
			if (cur_sainfo->algs[algclass_ipsec_auth] == 0) {
				yyerror("no authentication algorithm at %s",
					sainfo2str(cur_sainfo));
				return -1;
			}
			if (cur_sainfo->algs[algclass_ipsec_comp] == 0) {
				yyerror("no compression algorithm at %s",
					sainfo2str(cur_sainfo));
				return -1;
			}

			/* duplicate check */
			check = getsainfo(cur_sainfo->idsrc,
					  cur_sainfo->iddst,
					  cur_sainfo->id_i,
					  cur_sainfo->remoteid);
			if (check && (!check->idsrc && !cur_sainfo->idsrc)) {
				yyerror("duplicated sainfo: %s",
					sainfo2str(cur_sainfo));
				return -1;
			}
			inssainfo(cur_sainfo);
		}
		EOC
	;
sainfo_name
	:	ANONYMOUS
		{
			cur_sainfo->idsrc = NULL;
			cur_sainfo->iddst = NULL;
		}
	|	ANONYMOUS sainfo_id
		{
			cur_sainfo->idsrc = NULL;
			cur_sainfo->iddst = $2;
		}
	|	sainfo_id ANONYMOUS
		{
			cur_sainfo->idsrc = $1;
			cur_sainfo->iddst = NULL;
		}
	|	sainfo_id sainfo_id
		{
			cur_sainfo->idsrc = $1;
			cur_sainfo->iddst = $2;
		}
	;
sainfo_id
	:	IDENTIFIERTYPE ADDRSTRING prefix port ul_proto
		{
			char portbuf[10];
			struct sockaddr *saddr;

			if (($5 == IPPROTO_ICMP || $5 == IPPROTO_ICMPV6)
			 && ($4 != IPSEC_PORT_ANY || $4 != IPSEC_PORT_ANY)) {
				yyerror("port number must be \"any\".");
				return -1;
			}

			snprintf(portbuf, sizeof(portbuf), "%lu", $4);
			saddr = str2saddr($2->v, portbuf);
			vfree($2);
			if (saddr == NULL)
				return -1;

			switch (saddr->sa_family) {
			case AF_INET:
				if ($5 == IPPROTO_ICMPV6) {
					yyerror("upper layer protocol mismatched.\n");
					racoon_free(saddr);
					return -1;
				}
				$$ = ipsecdoi_sockaddr2id(saddr,
										  $3 == ~0 ? (sizeof(struct in_addr) << 3): $3,
										  $5);
				break;
#ifdef INET6
			case AF_INET6:
				if ($5 == IPPROTO_ICMP) {
					yyerror("upper layer protocol mismatched.\n");
					racoon_free(saddr);
					return -1;
				}
				$$ = ipsecdoi_sockaddr2id(saddr, 
										  $3 == ~0 ? (sizeof(struct in6_addr) << 3): $3,
										  $5);
				break;
#endif
			default:
				yyerror("invalid family: %d", saddr->sa_family);
				$$ = NULL;
				break;
			}
			racoon_free(saddr);
			if ($$ == NULL)
				return -1;
		}
	|	IDENTIFIERTYPE ADDRSTRING ADDRRANGE prefix port ul_proto
		{
			char portbuf[10];
			struct sockaddr *laddr = NULL, *haddr = NULL;
			char *cur = NULL;

			if (($6 == IPPROTO_ICMP || $6 == IPPROTO_ICMPV6)
			 && ($5 != IPSEC_PORT_ANY || $5 != IPSEC_PORT_ANY)) {
				yyerror("port number must be \"any\".");
				return -1;
			}

			snprintf(portbuf, sizeof(portbuf), "%lu", $5);
			
			laddr = str2saddr($2->v, portbuf);
			if (laddr == NULL) {
			    return -1;
			}
			vfree($2);
			haddr = str2saddr($3->v, portbuf);
			if (haddr == NULL) {
			    racoon_free(laddr);
			    return -1;
			}
			vfree($3);

			switch (laddr->sa_family) {
			case AF_INET:
				if ($6 == IPPROTO_ICMPV6) {
				    yyerror("upper layer protocol mismatched.\n");
				    if (laddr)
					racoon_free(laddr);
				    if (haddr)
					racoon_free(haddr);
				    return -1;
				}
                                $$ = ipsecdoi_sockrange2id(laddr, haddr, 
							   $6);
				break;
#ifdef INET6
			case AF_INET6:
				if ($6 == IPPROTO_ICMP) {
					yyerror("upper layer protocol mismatched.\n");
					if (laddr)
					    racoon_free(laddr);
					if (haddr)
					    racoon_free(haddr);
					return -1;
				}
				$$ = ipsecdoi_sockrange2id(laddr, haddr, 
							       $6);
				break;
#endif
			default:
				yyerror("invalid family: %d", laddr->sa_family);
				$$ = NULL;
				break;
			}
			if (laddr)
			    racoon_free(laddr);
			if (haddr)
			    racoon_free(haddr);
			if ($$ == NULL)
				return -1;
		}
	|	IDENTIFIERTYPE QUOTEDSTRING
		{
			struct ipsecdoi_id_b *id_b;

			if ($1 == IDTYPE_ASN1DN) {
				yyerror("id type forbidden: %d", $1);
				$$ = NULL;
				return -1;
			}

			$2->l--;

			$$ = vmalloc(sizeof(*id_b) + $2->l);
			if ($$ == NULL) {
				yyerror("failed to allocate identifier");
				return -1;
			}

			id_b = (struct ipsecdoi_id_b *)$$->v;
			id_b->type = idtype2doi($1);

			id_b->proto_id = 0;
			id_b->port = 0;

			memcpy($$->v + sizeof(*id_b), $2->v, $2->l);
		}
	;
sainfo_param
	:	/* nothing */
		{
			cur_sainfo->id_i = NULL;
		}
	|	FROM IDENTIFIERTYPE identifierstring
		{
			struct ipsecdoi_id_b *id_b;
			vchar_t *idv;

			if (set_identifier(&idv, $2, $3) != 0) {
				yyerror("failed to set identifer.\n");
				return -1;
			}
			cur_sainfo->id_i = vmalloc(sizeof(*id_b) + idv->l);
			if (cur_sainfo->id_i == NULL) {
				yyerror("failed to allocate identifier");
				return -1;
			}

			id_b = (struct ipsecdoi_id_b *)cur_sainfo->id_i->v;
			id_b->type = idtype2doi($2);

			id_b->proto_id = 0;
			id_b->port = 0;

			memcpy(cur_sainfo->id_i->v + sizeof(*id_b),
			       idv->v, idv->l);
			vfree(idv);
		}
	|	GROUP QUOTEDSTRING
		{
#ifdef ENABLE_HYBRID
			if ((cur_sainfo->group = vdup($2)) == NULL) {
				yyerror("failed to set sainfo xauth group.\n");
				return -1;
			}
#else
			yyerror("racoon not configured with --enable-hybrid");
			return -1;
#endif
 		}
	;
sainfo_specs
	:	/* nothing */
	|	sainfo_specs sainfo_spec
	;
sainfo_spec
	:	PFS_GROUP dh_group_num
		{
			cur_sainfo->pfs_group = $2;
		}
		EOS
	|	REMOTEID NUMBER
		{
			cur_sainfo->remoteid = $2;
		}
		EOS
	|	LIFETIME LIFETYPE_TIME NUMBER unittype_time
		{
			cur_sainfo->lifetime = $3 * $4;
		}
		EOS
	|	LIFETIME LIFETYPE_BYTE NUMBER unittype_byte
		{
#if 1
			yyerror("byte lifetime support is deprecated");
			return -1;
#else
			cur_sainfo->lifebyte = fix_lifebyte($3 * $4);
			if (cur_sainfo->lifebyte == 0)
				return -1;
#endif
		}
		EOS
	|	ALGORITHM_CLASS {
			cur_algclass = $1;
		}
		algorithms EOS
	|	IDENTIFIER IDENTIFIERTYPE
		{
			yyerror("it's deprecated to specify a identifier in phase 2");
		}
		EOS
	|	MY_IDENTIFIER IDENTIFIERTYPE QUOTEDSTRING
		{
			yyerror("it's deprecated to specify a identifier in phase 2");
		}
		EOS
	;

algorithms
	:	algorithm
		{
			inssainfoalg(&cur_sainfo->algs[cur_algclass], $1);
		}
	|	algorithm
		{
			inssainfoalg(&cur_sainfo->algs[cur_algclass], $1);
		}
		COMMA algorithms
	;
algorithm
	:	ALGORITHMTYPE keylength
		{
			int defklen;

			$$ = newsainfoalg();
			if ($$ == NULL) {
				yyerror("failed to get algorithm allocation");
				return -1;
			}

			$$->alg = algtype2doi(cur_algclass, $1);
			if ($$->alg == -1) {
				yyerror("algorithm mismatched");
				racoon_free($$);
				$$ = NULL;
				return -1;
			}

			defklen = default_keylen(cur_algclass, $1);
			if (defklen == 0) {
				if ($2) {
					yyerror("keylen not allowed");
					racoon_free($$);
					$$ = NULL;
					return -1;
				}
			} else {
				if ($2 && check_keylen(cur_algclass, $1, $2) < 0) {
					yyerror("invalid keylen %d", $2);
					racoon_free($$);
					$$ = NULL;
					return -1;
				}
			}

			if ($2)
				$$->encklen = $2;
			else
				$$->encklen = defklen;

			/* check if it's supported algorithm by kernel */
			if (!(cur_algclass == algclass_ipsec_auth && $1 == algtype_non_auth)
			 && pk_checkalg(cur_algclass, $1, $$->encklen)) {
				int a = algclass2doi(cur_algclass);
				int b = algtype2doi(cur_algclass, $1);
				if (a == IPSECDOI_ATTR_AUTH)
					a = IPSECDOI_PROTO_IPSEC_AH;
				yyerror("algorithm %s not supported by the kernel (missing module?)",
					s_ipsecdoi_trns(a, b));
				racoon_free($$);
				$$ = NULL;
				return -1;
			}
		}
	;
prefix
	:	/* nothing */ { $$ = ~0; }
	|	PREFIX { $$ = $1; }
	;
port
	:	/* nothing */ { $$ = IPSEC_PORT_ANY; }
	|	PORT { $$ = $1; }
	|	PORTANY { $$ = IPSEC_PORT_ANY; }
	;
ul_proto
	:	NUMBER { $$ = $1; }
	|	UL_PROTO { $$ = $1; }
	|	ANY { $$ = IPSEC_ULPROTO_ANY; }
	;
keylength
	:	/* nothing */ { $$ = 0; }
	|	NUMBER { $$ = $1; }
	;

	/* remote */
remote_statement
	:	REMOTE remote_index INHERIT remote_index
		{
			struct remoteconf *new;
			struct proposalspec *prspec;

			new = copyrmconf($4);
			if (new == NULL) {
				yyerror("failed to get remoteconf for %s.", saddr2str ($4));
				return -1;
			}

			new->remote = $2;
			new->inherited_from = getrmconf_strict($4, 1);
			new->proposal = NULL;
			new->prhead = NULL;
			cur_rmconf = new;

			prspec = newprspec();
			if (prspec == NULL || !cur_rmconf->inherited_from 
				|| !cur_rmconf->inherited_from->proposal)
				return -1;
			prspec->lifetime = cur_rmconf->inherited_from->proposal->lifetime;
			prspec->lifebyte = cur_rmconf->inherited_from->proposal->lifebyte;
			insprspec(prspec, &cur_rmconf->prhead);
		}
		remote_specs_block
	|	REMOTE remote_index
		{
			struct remoteconf *new;
			struct proposalspec *prspec;

			new = newrmconf();
			if (new == NULL) {
				yyerror("failed to get new remoteconf.");
				return -1;
			}

			new->remote = $2;
			cur_rmconf = new;

			prspec = newprspec();
			if (prspec == NULL)
				return -1;
			prspec->lifetime = oakley_get_defaultlifetime();
			insprspec(prspec, &cur_rmconf->prhead);
		}
		remote_specs_block
	;

remote_specs_block
	:	BOC remote_specs EOC
		{
			/* check a exchange mode */
			if (cur_rmconf->etypes == NULL) {
				yyerror("no exchange mode specified.\n");
				return -1;
			}

			if (cur_rmconf->idvtype == IDTYPE_UNDEFINED)
				cur_rmconf->idvtype = IDTYPE_ADDRESS;


			if (cur_rmconf->idvtype == IDTYPE_ASN1DN) {
				if (cur_rmconf->mycertfile) {
					if (cur_rmconf->idv)
						yywarn("Both CERT and ASN1 ID "
						       "are set. Hope this is OK.\n");
					/* TODO: Preparse the DN here */
				} else if (cur_rmconf->idv) {
					/* OK, using asn1dn without X.509. */
				} else {
					yyerror("ASN1 ID not specified "
						"and no CERT defined!\n");
					return -1;
				}
			}
			
			if (cur_rmconf->prhead->spspec == NULL
				&& cur_rmconf->inherited_from
				&& cur_rmconf->inherited_from->prhead) {
				cur_rmconf->prhead->spspec = cur_rmconf->inherited_from->prhead->spspec;
			}
			if (set_isakmp_proposal(cur_rmconf, cur_rmconf->prhead) != 0)
				return -1;

			/* DH group settting if aggressive mode is there. */
			if (check_etypeok(cur_rmconf, ISAKMP_ETYPE_AGG) != NULL) {
				struct isakmpsa *p;
				int b = 0;

				/* DH group */
				for (p = cur_rmconf->proposal; p; p = p->next) {
					if (b == 0 || (b && b == p->dh_group)) {
						b = p->dh_group;
						continue;
					}
					yyerror("DH group must be equal "
						"in all proposals "
						"when aggressive mode is "
						"used.\n");
					return -1;
				}
				cur_rmconf->dh_group = b;

				if (cur_rmconf->dh_group == 0) {
					yyerror("DH group must be set in the proposal.\n");
					return -1;
				}

				/* DH group settting if PFS is required. */
				if (oakley_setdhgroup(cur_rmconf->dh_group,
						&cur_rmconf->dhgrp) < 0) {
					yyerror("failed to set DH value.\n");
					return -1;
				}
			}

			insrmconf(cur_rmconf);
		}
	;
remote_index
	:	ANONYMOUS ike_port
		{
			$$ = newsaddr(sizeof(struct sockaddr));
			$$->sa_family = AF_UNSPEC;
			((struct sockaddr_in *)$$)->sin_port = htons($2);
		}
	|	ike_addrinfo_port
		{
			$$ = $1;
			if ($$ == NULL) {
				yyerror("failed to allocate sockaddr");
				return -1;
			}
		}
	;
remote_specs
	:	/* nothing */
	|	remote_specs remote_spec
	;
remote_spec
	:	EXCHANGE_MODE
		{
			cur_rmconf->etypes = NULL;
		}
		exchange_types EOS
	|	DOI DOITYPE { cur_rmconf->doitype = $2; } EOS
	|	SITUATION SITUATIONTYPE { cur_rmconf->sittype = $2; } EOS
	|	CERTIFICATE_TYPE cert_spec
	|	PEERS_CERTFILE QUOTEDSTRING
		{
			yywarn("This directive without certtype will be removed!\n");
			yywarn("Please use 'peers_certfile x509 \"%s\";' instead\n", $2->v);
			cur_rmconf->getcert_method = ISAKMP_GETCERT_LOCALFILE;

			if (cur_rmconf->peerscertfile != NULL)
				racoon_free(cur_rmconf->peerscertfile);
			cur_rmconf->peerscertfile = racoon_strdup($2->v);
			STRDUP_FATAL(cur_rmconf->peerscertfile);
			vfree($2);
		}
		EOS
	|	CA_TYPE CERT_X509 QUOTEDSTRING
		{
			cur_rmconf->cacerttype = $2;
			cur_rmconf->getcacert_method = ISAKMP_GETCERT_LOCALFILE;
			if (cur_rmconf->cacertfile != NULL)
				racoon_free(cur_rmconf->cacertfile);
			cur_rmconf->cacertfile = racoon_strdup($3->v);
			STRDUP_FATAL(cur_rmconf->cacertfile);
			vfree($3);
		}
		EOS
	|	PEERS_CERTFILE CERT_X509 QUOTEDSTRING
		{
			cur_rmconf->getcert_method = ISAKMP_GETCERT_LOCALFILE;
			if (cur_rmconf->peerscertfile != NULL)
				racoon_free(cur_rmconf->peerscertfile);
			cur_rmconf->peerscertfile = racoon_strdup($3->v);
			STRDUP_FATAL(cur_rmconf->peerscertfile);
			vfree($3);
		}
		EOS
	|	PEERS_CERTFILE CERT_PLAINRSA QUOTEDSTRING
		{
			char path[MAXPATHLEN];
			int ret = 0;

			getpathname(path, sizeof(path),
				LC_PATHTYPE_CERT, $3->v);
			vfree($3);

			if (cur_rmconf->getcert_method == ISAKMP_GETCERT_DNS) {
				yyerror("Different peers_certfile method "
					"already defined: %d!\n",
					cur_rmconf->getcert_method);
				return -1;
			}
			cur_rmconf->getcert_method = ISAKMP_GETCERT_LOCALFILE;
			if (rsa_parse_file(cur_rmconf->rsa_public, path, RSA_TYPE_PUBLIC)) {
				yyerror("Couldn't parse keyfile.\n", path);
				return -1;
			}
			plog(LLV_DEBUG, LOCATION, NULL, "Public PlainRSA keyfile parsed: %s\n", path);
		}
		EOS
	|	PEERS_CERTFILE DNSSEC
		{
			if (cur_rmconf->getcert_method) {
				yyerror("Different peers_certfile method already defined!\n");
				return -1;
			}
			cur_rmconf->getcert_method = ISAKMP_GETCERT_DNS;
			cur_rmconf->peerscertfile = NULL;
		}
		EOS
	|	VERIFY_CERT SWITCH { cur_rmconf->verify_cert = $2; } EOS
	|	SEND_CERT SWITCH { cur_rmconf->send_cert = $2; } EOS
	|	SEND_CR SWITCH { cur_rmconf->send_cr = $2; } EOS
	|	MY_IDENTIFIER IDENTIFIERTYPE identifierstring
		{
			if (set_identifier(&cur_rmconf->idv, $2, $3) != 0) {
				yyerror("failed to set identifer.\n");
				return -1;
			}
			cur_rmconf->idvtype = $2;
		}
		EOS
	|	MY_IDENTIFIER IDENTIFIERTYPE IDENTIFIERQUAL identifierstring
		{
			if (set_identifier_qual(&cur_rmconf->idv, $2, $4, $3) != 0) {
				yyerror("failed to set identifer.\n");
				return -1;
			}
			cur_rmconf->idvtype = $2;
		}
		EOS
	|	XAUTH_LOGIN identifierstring
		{
#ifdef ENABLE_HYBRID
			/* formerly identifier type login */
			if (xauth_rmconf_used(&cur_rmconf->xauth) == -1) {
				yyerror("failed to allocate xauth state\n");
				return -1;
			}
			if ((cur_rmconf->xauth->login = vdup($2)) == NULL) {
				yyerror("failed to set identifer.\n");
				return -1;
			}
#else
			yyerror("racoon not configured with --enable-hybrid");
#endif
		}
		EOS
	|	PEERS_IDENTIFIER IDENTIFIERTYPE identifierstring
		{
			struct idspec  *id;
			id = newidspec();
			if (id == NULL) {
				yyerror("failed to allocate idspec");
				return -1;
			}
			if (set_identifier(&id->id, $2, $3) != 0) {
				yyerror("failed to set identifer.\n");
				racoon_free(id);
				return -1;
			}
			id->idtype = $2;
			genlist_append (cur_rmconf->idvl_p, id);
		}
		EOS
	|	PEERS_IDENTIFIER IDENTIFIERTYPE IDENTIFIERQUAL identifierstring
		{
			struct idspec  *id;
			id = newidspec();
			if (id == NULL) {
				yyerror("failed to allocate idspec");
				return -1;
			}
			if (set_identifier_qual(&id->id, $2, $4, $3) != 0) {
				yyerror("failed to set identifer.\n");
				racoon_free(id);
				return -1;
			}
			id->idtype = $2;
			genlist_append (cur_rmconf->idvl_p, id);
		}
		EOS
	|	VERIFY_IDENTIFIER SWITCH { cur_rmconf->verify_identifier = $2; } EOS
	|	NONCE_SIZE NUMBER { cur_rmconf->nonce_size = $2; } EOS
	|	DH_GROUP
		{
			yyerror("dh_group cannot be defined here.");
			return -1;
		}
		dh_group_num EOS
	|	PASSIVE SWITCH { cur_rmconf->passive = $2; } EOS
	|	IKE_FRAG SWITCH { cur_rmconf->ike_frag = $2; } EOS
	|	IKE_FRAG REMOTE_FORCE_LEVEL { cur_rmconf->ike_frag = ISAKMP_FRAG_FORCE; } EOS
	|	ESP_FRAG NUMBER { 
#ifdef SADB_X_EXT_NAT_T_FRAG
        		if (libipsec_opt & LIBIPSEC_OPT_FRAG)
				cur_rmconf->esp_frag = $2; 
			else
                		yywarn("libipsec lacks IKE frag support");
#else
			yywarn("Your kernel does not support esp_frag");
#endif
		} EOS
	|	SCRIPT QUOTEDSTRING PHASE1_UP { 
			if (cur_rmconf->script[SCRIPT_PHASE1_UP] != NULL)
				vfree(cur_rmconf->script[SCRIPT_PHASE1_UP]);

			cur_rmconf->script[SCRIPT_PHASE1_UP] = 
			    script_path_add(vdup($2));
		} EOS
	|	SCRIPT QUOTEDSTRING PHASE1_DOWN { 
			if (cur_rmconf->script[SCRIPT_PHASE1_DOWN] != NULL)
				vfree(cur_rmconf->script[SCRIPT_PHASE1_DOWN]);

			cur_rmconf->script[SCRIPT_PHASE1_DOWN] = 
			    script_path_add(vdup($2));
		} EOS
	|	MODE_CFG SWITCH { cur_rmconf->mode_cfg = $2; } EOS
	|	WEAK_PHASE1_CHECK SWITCH {
			cur_rmconf->weak_phase1_check = $2;
		} EOS
	|	GENERATE_POLICY SWITCH { cur_rmconf->gen_policy = $2; } EOS
	|	GENERATE_POLICY GENERATE_LEVEL { cur_rmconf->gen_policy = $2; } EOS
	|	SUPPORT_PROXY SWITCH { cur_rmconf->support_proxy = $2; } EOS
	|	INITIAL_CONTACT SWITCH { cur_rmconf->ini_contact = $2; } EOS
	|	NAT_TRAVERSAL SWITCH
		{
#ifdef ENABLE_NATT
        		if (libipsec_opt & LIBIPSEC_OPT_NATT)
				cur_rmconf->nat_traversal = $2;
			else
                		yyerror("libipsec lacks NAT-T support");
#else
			yyerror("NAT-T support not compiled in.");
#endif
		} EOS
	|	NAT_TRAVERSAL REMOTE_FORCE_LEVEL
		{
#ifdef ENABLE_NATT
			if (libipsec_opt & LIBIPSEC_OPT_NATT)
				cur_rmconf->nat_traversal = NATT_FORCE;
			else
                		yyerror("libipsec lacks NAT-T support");
#else
			yyerror("NAT-T support not compiled in.");
#endif
		} EOS
	|	DPD SWITCH
		{
#ifdef ENABLE_DPD
			cur_rmconf->dpd = $2;
#else
			yyerror("DPD support not compiled in.");
#endif
		} EOS
	|	DPD_DELAY NUMBER
		{
#ifdef ENABLE_DPD
			cur_rmconf->dpd_interval = $2;
#else
			yyerror("DPD support not compiled in.");
#endif
		}
		EOS
	|	DPD_RETRY NUMBER
		{
#ifdef ENABLE_DPD
			cur_rmconf->dpd_retry = $2;
#else
			yyerror("DPD support not compiled in.");
#endif
		}
		EOS
	|	DPD_MAXFAIL NUMBER
		{
#ifdef ENABLE_DPD
			cur_rmconf->dpd_maxfails = $2;
#else
			yyerror("DPD support not compiled in.");
#endif
		}
		EOS
	|	PH1ID NUMBER
		{
			cur_rmconf->ph1id = $2;
		}
		EOS
	|	LIFETIME LIFETYPE_TIME NUMBER unittype_time
		{
			cur_rmconf->prhead->lifetime = $3 * $4;
		}
		EOS
	|	PROPOSAL_CHECK PROPOSAL_CHECK_LEVEL { cur_rmconf->pcheck_level = $2; } EOS
	|	LIFETIME LIFETYPE_BYTE NUMBER unittype_byte
		{
#if 1
			yyerror("byte lifetime support is deprecated in Phase1");
			return -1;
#else
			yywarn("the lifetime of bytes in phase 1 "
				"will be ignored at the moment.");
			cur_rmconf->prhead->lifebyte = fix_lifebyte($3 * $4);
			if (cur_rmconf->prhead->lifebyte == 0)
				return -1;
#endif
		}
		EOS
	|	PROPOSAL
		{
			struct secprotospec *spspec;

			spspec = newspspec();
			if (spspec == NULL)
				return -1;
			insspspec(spspec, &cur_rmconf->prhead);
		}
		BOC isakmpproposal_specs EOC
	;
exchange_types
	:	/* nothing */
	|	exchange_types EXCHANGETYPE
		{
			struct etypes *new;
			new = racoon_malloc(sizeof(struct etypes));
			if (new == NULL) {
				yyerror("failed to allocate etypes");
				return -1;
			}
			new->type = $2;
			new->next = NULL;
			if (cur_rmconf->etypes == NULL)
				cur_rmconf->etypes = new;
			else {
				struct etypes *p;
				for (p = cur_rmconf->etypes;
				     p->next != NULL;
				     p = p->next)
					;
				p->next = new;
			}
		}
	;
cert_spec
	:	CERT_X509 QUOTEDSTRING QUOTEDSTRING
		{
			cur_rmconf->certtype = $1;
			if (cur_rmconf->mycertfile != NULL)
				racoon_free(cur_rmconf->mycertfile);
			cur_rmconf->mycertfile = racoon_strdup($2->v);
			STRDUP_FATAL(cur_rmconf->mycertfile);
			vfree($2);
			if (cur_rmconf->myprivfile != NULL)
				racoon_free(cur_rmconf->myprivfile);
			cur_rmconf->myprivfile = racoon_strdup($3->v);
			STRDUP_FATAL(cur_rmconf->myprivfile);
			vfree($3);
		}
		EOS
	|	CERT_PLAINRSA QUOTEDSTRING
		{
			char path[MAXPATHLEN];
			int ret = 0;

			getpathname(path, sizeof(path),
				LC_PATHTYPE_CERT, $2->v);
			vfree($2);

			cur_rmconf->certtype = $1;
			cur_rmconf->send_cr = FALSE;
			cur_rmconf->send_cert = FALSE;
			cur_rmconf->verify_cert = FALSE;
			if (rsa_parse_file(cur_rmconf->rsa_private, path, RSA_TYPE_PRIVATE)) {
				yyerror("Couldn't parse keyfile.\n", path);
				return -1;
			}
			plog(LLV_DEBUG, LOCATION, NULL, "Private PlainRSA keyfile parsed: %s\n", path);
		}
		EOS
	;
dh_group_num
	:	ALGORITHMTYPE
		{
			$$ = algtype2doi(algclass_isakmp_dh, $1);
			if ($$ == -1) {
				yyerror("must be DH group");
				return -1;
			}
		}
	|	NUMBER
		{
			if (ARRAYLEN(num2dhgroup) > $1 && num2dhgroup[$1] != 0) {
				$$ = num2dhgroup[$1];
			} else {
				yyerror("must be DH group");
				$$ = 0;
				return -1;
			}
		}
	;
identifierstring
	:	/* nothing */ { $$ = NULL; }
	|	ADDRSTRING { $$ = $1; }
	|	QUOTEDSTRING { $$ = $1; }
	;
isakmpproposal_specs
	:	/* nothing */
	|	isakmpproposal_specs isakmpproposal_spec
	;
isakmpproposal_spec
	:	STRENGTH
		{
			yyerror("strength directive is obsoleted.");
		} STRENGTHTYPE EOS
	|	LIFETIME LIFETYPE_TIME NUMBER unittype_time
		{
			cur_rmconf->prhead->spspec->lifetime = $3 * $4;
		}
		EOS
	|	LIFETIME LIFETYPE_BYTE NUMBER unittype_byte
		{
#if 1
			yyerror("byte lifetime support is deprecated");
			return -1;
#else
			cur_rmconf->prhead->spspec->lifebyte = fix_lifebyte($3 * $4);
			if (cur_rmconf->prhead->spspec->lifebyte == 0)
				return -1;
#endif
		}
		EOS
	|	DH_GROUP dh_group_num
		{
			cur_rmconf->prhead->spspec->algclass[algclass_isakmp_dh] = $2;
		}
		EOS
	|	GSS_ID QUOTEDSTRING
		{
			if (cur_rmconf->prhead->spspec->vendorid != VENDORID_GSSAPI) {
				yyerror("wrong Vendor ID for gssapi_id");
				return -1;
			}
			if (cur_rmconf->prhead->spspec->gssid != NULL)
				racoon_free(cur_rmconf->prhead->spspec->gssid);
			cur_rmconf->prhead->spspec->gssid = 
			    racoon_strdup($2->v);
			STRDUP_FATAL(cur_rmconf->prhead->spspec->gssid);
		}
		EOS
	|	ALGORITHM_CLASS ALGORITHMTYPE keylength
		{
			int doi;
			int defklen;

			doi = algtype2doi($1, $2);
			if (doi == -1) {
				yyerror("algorithm mismatched 1");
				return -1;
			}

			switch ($1) {
			case algclass_isakmp_enc:
			/* reject suppressed algorithms */
#ifndef HAVE_OPENSSL_RC5_H
				if ($2 == algtype_rc5) {
					yyerror("algorithm %s not supported",
					    s_attr_isakmp_enc(doi));
					return -1;
				}
#endif
#ifndef HAVE_OPENSSL_IDEA_H
				if ($2 == algtype_idea) {
					yyerror("algorithm %s not supported",
					    s_attr_isakmp_enc(doi));
					return -1;
				}
#endif

				cur_rmconf->prhead->spspec->algclass[algclass_isakmp_enc] = doi;
				defklen = default_keylen($1, $2);
				if (defklen == 0) {
					if ($3) {
						yyerror("keylen not allowed");
						return -1;
					}
				} else {
					if ($3 && check_keylen($1, $2, $3) < 0) {
						yyerror("invalid keylen %d", $3);
						return -1;
					}
				}
				if ($3)
					cur_rmconf->prhead->spspec->encklen = $3;
				else
					cur_rmconf->prhead->spspec->encklen = defklen;
				break;
			case algclass_isakmp_hash:
				cur_rmconf->prhead->spspec->algclass[algclass_isakmp_hash] = doi;
				break;
			case algclass_isakmp_ameth:
				cur_rmconf->prhead->spspec->algclass[algclass_isakmp_ameth] = doi;
				/*
				 * We may have to set the Vendor ID for the
				 * authentication method we're using.
				 */
				switch ($2) {
				case algtype_gssapikrb:
					if (cur_rmconf->prhead->spspec->vendorid !=
					    VENDORID_UNKNOWN) {
						yyerror("Vendor ID mismatch "
						    "for auth method");
						return -1;
					}
					/*
					 * For interoperability with Win2k,
					 * we set the Vendor ID to "GSSAPI".
					 */
					cur_rmconf->prhead->spspec->vendorid =
					    VENDORID_GSSAPI;
					break;
				case algtype_rsasig:
					if (cur_rmconf->certtype == ISAKMP_CERT_PLAINRSA) {
						if (rsa_list_count(cur_rmconf->rsa_private) == 0) {
							yyerror ("Private PlainRSA key not set. "
								"Use directive 'certificate_type plainrsa ...'\n");
							return -1;
						}
						if (rsa_list_count(cur_rmconf->rsa_public) == 0) {
							yyerror ("Public PlainRSA keys not set. "
								"Use directive 'peers_certfile plainrsa ...'\n");
							return -1;
						}
					}
					break;
				default:
					break;
				}
				break;
			default:
				yyerror("algorithm mismatched 2");
				return -1;
			}
		}
		EOS
	;

unittype_time
	:	UNITTYPE_SEC	{ $$ = 1; }
	|	UNITTYPE_MIN	{ $$ = 60; }
	|	UNITTYPE_HOUR	{ $$ = (60 * 60); }
	;
unittype_byte
	:	UNITTYPE_BYTE	{ $$ = 1; }
	|	UNITTYPE_KBYTES	{ $$ = 1024; }
	|	UNITTYPE_MBYTES	{ $$ = (1024 * 1024); }
	|	UNITTYPE_TBYTES	{ $$ = (1024 * 1024 * 1024); }
	;
%%

static struct proposalspec *
newprspec()
{
	struct proposalspec *new;

	new = racoon_calloc(1, sizeof(*new));
	if (new == NULL)
		yyerror("failed to allocate proposal");

	return new;
}

/*
 * insert into head of list.
 */
static void
insprspec(prspec, head)
	struct proposalspec *prspec;
	struct proposalspec **head;
{
	if (*head != NULL)
		(*head)->prev = prspec;
	prspec->next = *head;
	*head = prspec;
}

static struct secprotospec *
newspspec()
{
	struct secprotospec *new;

	new = racoon_calloc(1, sizeof(*new));
	if (new == NULL) {
		yyerror("failed to allocate spproto");
		return NULL;
	}

	new->encklen = 0;	/*XXX*/

	/*
	 * Default to "uknown" vendor -- we will override this
	 * as necessary.  When we send a Vendor ID payload, an
	 * "unknown" will be translated to a KAME/racoon ID.
	 */
	new->vendorid = VENDORID_UNKNOWN;

	return new;
}

/*
 * insert into head of list.
 */
static void
insspspec(spspec, head)
	struct secprotospec *spspec;
	struct proposalspec **head;
{
	spspec->back = *head;

	if ((*head)->spspec != NULL)
		(*head)->spspec->prev = spspec;
	spspec->next = (*head)->spspec;
	(*head)->spspec = spspec;
}

/* set final acceptable proposal */
static int
set_isakmp_proposal(rmconf, prspec)
	struct remoteconf *rmconf;
	struct proposalspec *prspec;
{
	struct proposalspec *p;
	struct secprotospec *s;
	int prop_no = 1; 
	int trns_no = 1;
	int32_t types[MAXALGCLASS];

	p = prspec;
	if (p->next != 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"multiple proposal definition.\n");
		return -1;
	}

	/* mandatory check */
	if (p->spspec == NULL) {
		yyerror("no remote specification found: %s.\n",
			saddr2str(rmconf->remote));
		return -1;
	}
	for (s = p->spspec; s != NULL; s = s->next) {
		/* XXX need more to check */
		if (s->algclass[algclass_isakmp_enc] == 0) {
			yyerror("encryption algorithm required.");
			return -1;
		}
		if (s->algclass[algclass_isakmp_hash] == 0) {
			yyerror("hash algorithm required.");
			return -1;
		}
		if (s->algclass[algclass_isakmp_dh] == 0) {
			yyerror("DH group required.");
			return -1;
		}
		if (s->algclass[algclass_isakmp_ameth] == 0) {
			yyerror("authentication method required.");
			return -1;
		}
	}

	/* skip to last part */
	for (s = p->spspec; s->next != NULL; s = s->next)
		;

	while (s != NULL) {
		plog(LLV_DEBUG2, LOCATION, NULL,
			"lifetime = %ld\n", (long)
			(s->lifetime ? s->lifetime : p->lifetime));
		plog(LLV_DEBUG2, LOCATION, NULL,
			"lifebyte = %d\n",
			s->lifebyte ? s->lifebyte : p->lifebyte);
		plog(LLV_DEBUG2, LOCATION, NULL,
			"encklen=%d\n", s->encklen);

		memset(types, 0, ARRAYLEN(types));
		types[algclass_isakmp_enc] = s->algclass[algclass_isakmp_enc];
		types[algclass_isakmp_hash] = s->algclass[algclass_isakmp_hash];
		types[algclass_isakmp_dh] = s->algclass[algclass_isakmp_dh];
		types[algclass_isakmp_ameth] =
		    s->algclass[algclass_isakmp_ameth];

		/* expanding spspec */
		clean_tmpalgtype();
		trns_no = expand_isakmpspec(prop_no, trns_no, types,
				algclass_isakmp_enc, algclass_isakmp_ameth + 1,
				s->lifetime ? s->lifetime : p->lifetime,
				s->lifebyte ? s->lifebyte : p->lifebyte,
				s->encklen, s->vendorid, s->gssid,
				rmconf);
		if (trns_no == -1) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to expand isakmp proposal.\n");
			return -1;
		}

		s = s->prev;
	}

	if (rmconf->proposal == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"no proposal found.\n");
		return -1;
	}

	return 0;
}

static void
clean_tmpalgtype()
{
	int i;
	for (i = 0; i < MAXALGCLASS; i++)
		tmpalgtype[i] = 0;	/* means algorithm undefined. */
}

static int
expand_isakmpspec(prop_no, trns_no, types,
		class, last, lifetime, lifebyte, encklen, vendorid, gssid,
		rmconf)
	int prop_no, trns_no;
	int *types, class, last;
	time_t lifetime;
	int lifebyte;
	int encklen;
	int vendorid;
	char *gssid;
	struct remoteconf *rmconf;
{
	struct isakmpsa *new;

	/* debugging */
    {
	int j;
	char tb[10];
	plog(LLV_DEBUG2, LOCATION, NULL,
		"p:%d t:%d\n", prop_no, trns_no);
	for (j = class; j < MAXALGCLASS; j++) {
		snprintf(tb, sizeof(tb), "%d", types[j]);
		plog(LLV_DEBUG2, LOCATION, NULL,
			"%s%s%s%s\n",
			s_algtype(j, types[j]),
			types[j] ? "(" : "",
			tb[0] == '0' ? "" : tb,
			types[j] ? ")" : "");
	}
	plog(LLV_DEBUG2, LOCATION, NULL, "\n");
    }

#define TMPALGTYPE2STR(n) \
	s_algtype(algclass_isakmp_##n, types[algclass_isakmp_##n])
		/* check mandatory values */
		if (types[algclass_isakmp_enc] == 0
		 || types[algclass_isakmp_ameth] == 0
		 || types[algclass_isakmp_hash] == 0
		 || types[algclass_isakmp_dh] == 0) {
			yyerror("few definition of algorithm "
				"enc=%s ameth=%s hash=%s dhgroup=%s.\n",
				TMPALGTYPE2STR(enc),
				TMPALGTYPE2STR(ameth),
				TMPALGTYPE2STR(hash),
				TMPALGTYPE2STR(dh));
			return -1;
		}
#undef TMPALGTYPE2STR

	/* set new sa */
	new = newisakmpsa();
	if (new == NULL) {
		yyerror("failed to allocate isakmp sa");
		return -1;
	}
	new->prop_no = prop_no;
	new->trns_no = trns_no++;
	new->lifetime = lifetime;
	new->lifebyte = lifebyte;
	new->enctype = types[algclass_isakmp_enc];
	new->encklen = encklen;
	new->authmethod = types[algclass_isakmp_ameth];
	new->hashtype = types[algclass_isakmp_hash];
	new->dh_group = types[algclass_isakmp_dh];
	new->vendorid = vendorid;
#ifdef HAVE_GSSAPI
	if (new->authmethod == OAKLEY_ATTR_AUTH_METHOD_GSSAPI_KRB) {
		if (gssid != NULL) {
			if ((new->gssid = vmalloc(strlen(gssid))) == NULL) {
				racoon_free(new);
				yyerror("failed to allocate gssid");
				return -1;
			}
			memcpy(new->gssid->v, gssid, new->gssid->l);
			racoon_free(gssid);
		} else {
			/*
			 * Allocate the default ID so that it gets put
			 * into a GSS ID attribute during the Phase 1
			 * exchange.
			 */
			new->gssid = gssapi_get_default_gss_id();
		}
	}
#endif
	insisakmpsa(new, rmconf);

	return trns_no;
}

static int
listen_addr (struct sockaddr *addr, int udp_encap)
{
	struct myaddrs *p;

	p = newmyaddr();
	if (p == NULL) {
		yyerror("failed to allocate myaddrs");
		return -1;
	}
	p->addr = addr;
	if (p->addr == NULL) {
		yyerror("failed to copy sockaddr ");
		delmyaddr(p);
		return -1;
	}
	p->udp_encap = udp_encap;

	insmyaddr(p, &lcconf->myaddrs);

	lcconf->autograbaddr = 0;
	return 0;
}

#if 0
/*
 * fix lifebyte.
 * Must be more than 1024B because its unit is kilobytes.
 * That is defined RFC2407.
 */
static int
fix_lifebyte(t)
	unsigned long t;
{
	if (t < 1024) {
		yyerror("byte size should be more than 1024B.");
		return 0;
	}

	return(t / 1024);
}
#endif

int
cfparse()
{
	int error;

	yycf_init_buffer();

	if (yycf_switch_buffer(lcconf->racoon_conf) != 0) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "could not read configuration file \"%s\"\n", 
		    lcconf->racoon_conf);
		return -1;
	}

	error = yyparse();
	if (error != 0) {
		if (yyerrorcount) {
			plog(LLV_ERROR, LOCATION, NULL,
				"fatal parse failure (%d errors)\n",
				yyerrorcount);
		} else {
			plog(LLV_ERROR, LOCATION, NULL,
				"fatal parse failure.\n");
		}
		return -1;
	}

	if (error == 0 && yyerrorcount) {
		plog(LLV_ERROR, LOCATION, NULL,
			"parse error is nothing, but yyerrorcount is %d.\n",
				yyerrorcount);
		exit(1);
	}

	yycf_clean_buffer();

	plog(LLV_DEBUG2, LOCATION, NULL, "parse successed.\n");

	return 0;
}

int
cfreparse()
{
	flushph2();
	flushph1();
	flushrmconf();
	flushsainfo();
	clean_tmpalgtype();
	yycf_init_buffer();

	if (yycf_switch_buffer(lcconf->racoon_conf) != 0)
		return -1;

	return(cfparse());
}

#ifdef ENABLE_ADMINPORT
static void
adminsock_conf(path, owner, group, mode_dec)
	vchar_t *path;
	vchar_t *owner;
	vchar_t *group;
	int mode_dec;
{
	struct passwd *pw = NULL;
	struct group *gr = NULL;
	mode_t mode = 0;
	uid_t uid;
	gid_t gid;
	int isnum;

	adminsock_path = path->v;

	if (owner == NULL)
		return;

	errno = 0;
	uid = atoi(owner->v);
	isnum = !errno;
	if (((pw = getpwnam(owner->v)) == NULL) && !isnum)
		yyerror("User \"%s\" does not exist", owner->v);

	if (pw)
		adminsock_owner = pw->pw_uid;
	else
		adminsock_owner = uid;

	if (group == NULL)
		return;

	errno = 0;
	gid = atoi(group->v);
	isnum = !errno;
	if (((gr = getgrnam(group->v)) == NULL) && !isnum)
		yyerror("Group \"%s\" does not exist", group->v);

	if (gr)
		adminsock_group = gr->gr_gid;
	else
		adminsock_group = gid;

	if (mode_dec == -1)
		return;

	if (mode_dec > 777)
		yyerror("Mode 0%03o is invalid", mode_dec);
	if (mode_dec >= 400) { mode += 0400; mode_dec -= 400; }
	if (mode_dec >= 200) { mode += 0200; mode_dec -= 200; }
	if (mode_dec >= 100) { mode += 0200; mode_dec -= 100; }

	if (mode_dec > 77)
		yyerror("Mode 0%03o is invalid", mode_dec);
	if (mode_dec >= 40) { mode += 040; mode_dec -= 40; }
	if (mode_dec >= 20) { mode += 020; mode_dec -= 20; }
	if (mode_dec >= 10) { mode += 020; mode_dec -= 10; }

	if (mode_dec > 7)
		yyerror("Mode 0%03o is invalid", mode_dec);
	if (mode_dec >= 4) { mode += 04; mode_dec -= 4; }
	if (mode_dec >= 2) { mode += 02; mode_dec -= 2; }
	if (mode_dec >= 1) { mode += 02; mode_dec -= 1; }
	
	adminsock_mode = mode;

	return;
}
#endif
