/*	$NetBSD: cfparse.y,v 1.49.14.1 2018/05/21 04:35:49 pgoyette Exp $	*/

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
#include "crypto_openssl.h"

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

static struct remoteconf *cur_rmconf = NULL;
static int tmpalgtype[MAXALGCLASS] = {0};
static struct sainfo *cur_sainfo = NULL;
static int cur_algclass = 0;
static int oldloglevel = LLV_BASE;

static struct secprotospec *newspspec __P((void));
static void insspspec __P((struct remoteconf *, struct secprotospec *));
void dupspspec_list __P((struct remoteconf *dst, struct remoteconf *src));
void flushspspec __P((struct remoteconf *));
static void adminsock_conf __P((vchar_t *, vchar_t *, vchar_t *, int));

static int set_isakmp_proposal __P((struct remoteconf *));
static void clean_tmpalgtype __P((void));
static int expand_isakmpspec __P((int, int, int *,
	int, int, time_t, int, int, int, char *, struct remoteconf *));

void freeetypes (struct etypes **etypes);

static int load_x509(const char *file, char **filenameptr,
		     vchar_t **certptr)
{
	char path[PATH_MAX];

	getpathname(path, sizeof(path), LC_PATHTYPE_CERT, file);
	*certptr = eay_get_x509cert(path);
	if (*certptr == NULL)
		return -1;

	*filenameptr = racoon_strdup(file);
	STRDUP_FATAL(*filenameptr);

	return 0;
}

static int process_rmconf(void)
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

	if (duprmconf_finish(cur_rmconf))
		return -1;

	if (set_isakmp_proposal(cur_rmconf) != 0)
		return -1;

	/* DH group settting if aggressive mode is there. */
	if (check_etypeok(cur_rmconf, (void*) ISAKMP_ETYPE_AGG)) {
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
	cur_rmconf = NULL; 

	return 0;
}

/* some frequently used warning texts */
static const char error_message_hybrid_config_not_configured[] = "racoon not configured with --enable-hybrid\n";
static const char error_message_ldap_config_not_configured[]   = "racoon not configured with --with-libldap\n";
static const char error_message_admin_port_not_compiled_in[] = "admin port support not compiled in\n";
static const char error_message_natt_not_compiled_in[] = "NAT-T support not compiled in\n";
static const char error_message_dpd_not_compiled_in[] = "DPD support not compiled in\n";

/* macros for aborting the parsing with freeing up allocated memory */
#define ABORT_CLEANUP {delrmconf(cur_rmconf); delsainfo(cur_sainfo); YYABORT;}
#define ABORT() ABORT_CLEANUP

#define ABORT_AND_VFREE(val0) {\
	vfree(val0); val0 = NULL;\
	ABORT_CLEANUP}
	
#define ABORT_AND_RACOON_FREE(val0) {\
	racoon_free(val0); val0 = NULL;\
	ABORT_CLEANUP}

#define ABORT_AND_VFREE2(val0, val1) {\
	vfree(val0); val0 = NULL;\
	vfree(val1); val1 = NULL;\
	ABORT_CLEANUP}

#define ABORT_AND_RACOON_FREE2(val0, val1) {\
	racoon_free(val0); val0 = NULL;\
	racoon_free(val1); val1 = NULL;\
	ABORT_CLEANUP}
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
	/* PFKEY_BUFFER */
%token PFKEY_BUFFER
	/* logging */
%token LOGGING LOGLEV
	/* padding */
%token PADDING PAD_RANDOMIZE PAD_RANDOMIZELEN PAD_MAXLEN PAD_STRICT PAD_EXCLTAIL
	/* listen */
%token LISTEN X_ISAKMP X_ISAKMP_NATT X_ADMIN STRICT_ADDRESS ADMINSOCK DISABLED
	/* ldap config */
%token LDAPCFG LDAP_HOST LDAP_PORT LDAP_TLS LDAP_PVER LDAP_BASE LDAP_BIND_DN LDAP_BIND_PW LDAP_SUBTREE
%token LDAP_ATTR_USER LDAP_ATTR_ADDR LDAP_ATTR_MASK LDAP_ATTR_GROUP LDAP_ATTR_MEMBER
	/* radius config */
%token RADCFG RAD_AUTH RAD_ACCT RAD_TIMEOUT RAD_RETRIES
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
%token REMOTE ANONYMOUS CLIENTADDR INHERIT REMOTE_ADDRESS
%token EXCHANGE_MODE EXCHANGETYPE DOI DOITYPE SITUATION SITUATIONTYPE
%token CERTIFICATE_TYPE CERTTYPE PEERS_CERTFILE CA_TYPE
%token VERIFY_CERT SEND_CERT SEND_CR MATCH_EMPTY_CR
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
%token REKEY

%token PREFIX PORT PORTANY UL_PROTO ANY IKE_FRAG ESP_FRAG MODE_CFG
%token PFS_GROUP LIFETIME LIFETYPE_TIME LIFETYPE_BYTE STRENGTH REMOTEID

%token SCRIPT PHASE1_UP PHASE1_DOWN PHASE1_DEAD

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
%type <saddr> ike_addrinfo_port_natt
%type <num> ike_port_natt

%%

statements
	:	/* nothing */
	|	statements statement
	;
statement
	:	privsep_statement
	|	path_statement
	|	include_statement
	|	pfkey_statement
	|	gssenc_statement
	|	logging_statement
	|	padding_statement
	|	listen_statement
	|	ldapcfg_statement
	|	radcfg_statement
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
			struct passwd *pw = getpwnam($2->v);
			vfree($2);

			if (pw == NULL) {
				yyerror("unknown user \"%s\"", $2->v);
				ABORT();
			}
			
			lcconf->uid = pw->pw_uid;
		} 
		EOS
	|	USER NUMBER { lcconf->uid = $2; } EOS
	|	GROUP QUOTEDSTRING
		{
			struct group *gr = getgrnam($2->v);
			vfree($2);

			if (gr == NULL) {
				yyerror("unknown group \"%s\"", $2->v);
				ABORT();
			}

			lcconf->gid = gr->gr_gid;
		}
		EOS
	|	GROUP NUMBER { lcconf->gid = $2; } EOS
	|	CHROOT QUOTEDSTRING 
		{ 
			lcconf_setchroot(racoon_strdup($2->v));
			vfree($2);					
		} EOS
	;

	/* path */
path_statement
	:	PATH PATHTYPE QUOTEDSTRING
		{
			char * path = racoon_strdup($3->v);

			if (path == NULL) {
				yyerror("copy string fatal error: %s", $3->v);
				ABORT_AND_VFREE($3);
			}
			
			if (lcconf_setpath(path, $2) < 0) {
				yyerror("invalid path type %d", $2);
				ABORT_AND_VFREE($3);
			}

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
				ABORT();
		}
	;

    /* pfkey_buffer */
pfkey_statement
    :   PFKEY_BUFFER NUMBER EOS
        {
			lcconf->pfkey_buffer_size = $2;
        }
    ;
	/* gss_id_enc */
gssenc_statement
	:	GSS_ID_ENC GSS_ID_ENCTYPE EOS
		{
			if ($2 >= LC_GSSENC_MAX) {
				yyerror("invalid GSS ID encoding %d", $2);
				ABORT();
			}

			lcconf->gss_id_enc = $2;
		}
	;

	/* logging */
logging_statement
	:	LOGGING log_level EOS
	;
log_level
	:	LOGLEV
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
			myaddr_listen($2, FALSE);
			racoon_free($2);
		}
		EOS
	|	X_ISAKMP_NATT ike_addrinfo_port_natt
		{
#ifdef ENABLE_NATT
			myaddr_listen($2, TRUE);
#else

			yywarn(error_message_natt_not_compiled_in);
#endif
			racoon_free($2);
		}
		EOS
	|	ADMINSOCK QUOTEDSTRING QUOTEDSTRING QUOTEDSTRING NUMBER 
		{
#ifdef ENABLE_ADMINPORT
			adminsock_conf($2, $3, $4, $5);
#else
			yywarn(error_message_admin_port_not_compiled_in);
#endif
			vfree($2);vfree($3);vfree($4);
		}
		EOS
	|	ADMINSOCK QUOTEDSTRING
		{
#ifdef ENABLE_ADMINPORT
			adminsock_conf($2, NULL, NULL, -1);
#else
			yywarn(error_message_admin_port_not_compiled_in);
#endif
			vfree($2);
		}
		EOS
	|	ADMINSOCK DISABLED
		{
#ifdef ENABLE_ADMINPORT
			adminsock_path = NULL;
#else
			yywarn(error_message_admin_port_not_compiled_in);
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
				ABORT();
		}
	;
ike_addrinfo_port_natt
	:	ADDRSTRING ike_port_natt
		{
			char portbuf[10];

			snprintf(portbuf, sizeof(portbuf), "%ld", $2);
			$$ = str2saddr($1->v, portbuf);
			
			vfree($1);
			if (!$$)
				ABORT();
		}
	;
ike_port
	:	/* nothing */	{	$$ = lcconf->port_isakmp; }
	|	PORT		{ $$ = $1; } 
	;
ike_port_natt
	:	/* nothing */ 
		{ 
			$$ = lcconf->port_isakmp_natt;  
		}
	|	PORT 
		{ 
			$$ = $1; 
#ifndef ENABLE_NATT
			yywarn(error_message_natt_not_compiled_in);
#endif			 
		}
	;
	/* radius configuration */
radcfg_statement
	:	RADCFG {
#ifndef ENABLE_HYBRID
			yyerror(error_message_hybrid_config_not_configured);
			ABORT();
#endif
#ifndef HAVE_LIBRADIUS
			yyerror("racoon not configured with --with-libradius");
			ABORT();
#endif
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBRADIUS
			xauth_rad_config.timeout = 3;
			xauth_rad_config.retries = 3;
#endif
#endif
		} BOC radcfg_stmts EOC
	;
radcfg_stmts
	:	/* nothing */
	|	radcfg_stmts radcfg_stmt
	;
radcfg_stmt
	:	RAD_AUTH QUOTEDSTRING QUOTEDSTRING
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBRADIUS
			int i = xauth_rad_config.auth_server_count;
			if (i == RADIUS_MAX_SERVERS) {
				yyerror("maximum radius auth servers exceeded");
				ABORT_AND_VFREE2($2, $3);
			}

			xauth_rad_config.auth_server_list[i].host = vdup($2);
			xauth_rad_config.auth_server_list[i].secret = vdup($3);
			xauth_rad_config.auth_server_list[i].port = 0; /* default port */
			xauth_rad_config.auth_server_count++;
#endif
#endif
			vfree($2); vfree($3);
		}
		EOS
	|	RAD_AUTH QUOTEDSTRING NUMBER QUOTEDSTRING
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBRADIUS
			int i = xauth_rad_config.auth_server_count;
			if (i == RADIUS_MAX_SERVERS) {
				yyerror("maximum radius auth servers exceeded");
				ABORT_AND_VFREE2($2, $4);
			}

			xauth_rad_config.auth_server_list[i].host = vdup($2);
			xauth_rad_config.auth_server_list[i].secret = vdup($4);
			xauth_rad_config.auth_server_list[i].port = $3;
			xauth_rad_config.auth_server_count++;
#endif
#endif
			vfree($2); vfree($4);
		}
		EOS
	|	RAD_ACCT QUOTEDSTRING QUOTEDSTRING
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBRADIUS
			int i = xauth_rad_config.acct_server_count;
			if (i == RADIUS_MAX_SERVERS) {
				yyerror("maximum radius account servers exceeded");
				ABORT_AND_VFREE2($2, $3);
			}

			xauth_rad_config.acct_server_list[i].host = vdup($2);
			xauth_rad_config.acct_server_list[i].secret = vdup($3);
			xauth_rad_config.acct_server_list[i].port = 0; /* default port */
			xauth_rad_config.acct_server_count++;
#endif
#endif
			vfree($2); vfree($3);
		}
		EOS
	|	RAD_ACCT QUOTEDSTRING NUMBER QUOTEDSTRING
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBRADIUS
			int i = xauth_rad_config.acct_server_count;
			if (i == RADIUS_MAX_SERVERS) {
				yyerror("maximum radius account servers exceeded");
				ABORT_AND_VFREE2($2, $4);
			}

			xauth_rad_config.acct_server_list[i].host = vdup($2);
			xauth_rad_config.acct_server_list[i].secret = vdup($4);
			xauth_rad_config.acct_server_list[i].port = $3;
			xauth_rad_config.acct_server_count++;
#endif
#endif
			vfree($2); vfree($4);
		}
		EOS
	|	RAD_TIMEOUT NUMBER
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBRADIUS
			xauth_rad_config.timeout = $2;
#endif
#endif
		}
		EOS
	|	RAD_RETRIES NUMBER
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBRADIUS
			xauth_rad_config.retries = $2;
#endif
#endif
		}
		EOS
	;

	/* ldap configuration */
ldapcfg_statement
	:	LDAPCFG {
#ifndef ENABLE_HYBRID
			yyerror(error_message_hybrid_config_not_configured);
			ABORT();
#endif
#ifndef HAVE_LIBLDAP
			yyerror(error_message_ldap_config_not_configured);
			ABORT();
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
				yywarn("invalid ldap protocol version (2|3)");

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
			vfree($2);
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
	|	LDAP_TLS SWITCH
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBLDAP
			xauth_ldap_config.tls = $2;
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
			vfree($2);
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
			vfree($2);
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
			vfree($2);
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
			vfree($2);
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
			vfree($2);
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
			vfree($2);
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
			vfree($2);
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
			vfree($2);
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
			yywarn(error_message_hybrid_config_not_configured);
#endif
			vfree($2);
		}
		EOS
	|	CFG_MASK4 ADDRSTRING
		{
#ifdef ENABLE_HYBRID
			if (inet_pton(AF_INET, $2->v,
			    &isakmp_cfg_config.netmask4) != 1)
				yyerror("bad IPv4 netmask address.");
#else
			yywarn(error_message_hybrid_config_not_configured);
#endif
			vfree($2);
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
			yywarn(error_message_hybrid_config_not_configured);
#endif
		}
		EOS
	|	CFG_SPLIT_NETWORK CFG_SPLIT_INCLUDE splitnetlist
		{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.splitnet_type = UNITY_SPLIT_INCLUDE;
#else
			yywarn(error_message_hybrid_config_not_configured);
#endif
		}
		EOS
	|	CFG_SPLIT_DNS splitdnslist
		{
#ifndef ENABLE_HYBRID
			yywarn(error_message_hybrid_config_not_configured);
#endif
		}
		EOS
	|	CFG_DEFAULT_DOMAIN QUOTEDSTRING
		{
#ifdef ENABLE_HYBRID
			strncpy(&isakmp_cfg_config.default_domain[0], 
			    $2->v, MAXPATHLEN);
			isakmp_cfg_config.default_domain[MAXPATHLEN] = '\0';
#else
			yyerror(error_message_hybrid_config_not_configured);
#endif
			vfree($2);
		}
		EOS
	|	CFG_AUTH_SOURCE CFG_SYSTEM
		{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.authsource = ISAKMP_CFG_AUTH_SYSTEM;
#else
			yywarn(error_message_hybrid_config_not_configured);
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
			yywarn(error_message_hybrid_config_not_configured);
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
			yywarn(error_message_hybrid_config_not_configured);
#endif /* ENABLE_HYBRID */
		}
		EOS
	|	CFG_AUTH_SOURCE CFG_LDAP
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBLDAP
			isakmp_cfg_config.authsource = ISAKMP_CFG_AUTH_LDAP;
#else /* HAVE_LIBLDAP */
			yywarn(error_message_ldap_config_not_configured);
#endif /* HAVE_LIBLDAP */
#else /* ENABLE_HYBRID */
			yywarn(error_message_hybrid_config_not_configured);
#endif /* ENABLE_HYBRID */
		}
		EOS
	|	CFG_AUTH_GROUPS authgrouplist
		{
#ifndef ENABLE_HYBRID
			yywarn(error_message_hybrid_config_not_configured);
#endif
		}
		EOS
	|	CFG_GROUP_SOURCE CFG_SYSTEM
		{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.groupsource = ISAKMP_CFG_GROUP_SYSTEM;
#else
			yywarn(error_message_hybrid_config_not_configured);
#endif
		}
		EOS
	|	CFG_GROUP_SOURCE CFG_LDAP
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBLDAP
			isakmp_cfg_config.groupsource = ISAKMP_CFG_GROUP_LDAP;
#else /* HAVE_LIBLDAP */
			yywarn(error_message_ldap_config_not_configured);
#endif /* HAVE_LIBLDAP */
#else /* ENABLE_HYBRID */
			yywarn(error_message_hybrid_config_not_configured);
#endif /* ENABLE_HYBRID */
		}
		EOS
	|	CFG_ACCOUNTING CFG_NONE
		{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.accounting = ISAKMP_CFG_ACCT_NONE;
#else
			yywarn(error_message_hybrid_config_not_configured);
#endif
		}
		EOS
	|	CFG_ACCOUNTING CFG_SYSTEM
		{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.accounting = ISAKMP_CFG_ACCT_SYSTEM;
#else
			yywarn(error_message_hybrid_config_not_configured);
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
			yywarn(error_message_hybrid_config_not_configured);
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
			yywarn(error_message_hybrid_config_not_configured);
#endif /* ENABLE_HYBRID */
		}
		EOS
	|	CFG_POOL_SIZE NUMBER
		{
#ifdef ENABLE_HYBRID
			if (isakmp_cfg_resize_pool($2) != 0)
				yyerror("cannot allocate memory for pool");
#else /* ENABLE_HYBRID */
			yywarn(error_message_hybrid_config_not_configured);
#endif /* ENABLE_HYBRID */
		}
		EOS
	|	CFG_PFS_GROUP NUMBER
		{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.pfs_group = $2;
#else /* ENABLE_HYBRID */
			yywarn(error_message_hybrid_config_not_configured);
#endif /* ENABLE_HYBRID */
		}
		EOS
	|	CFG_SAVE_PASSWD SWITCH
		{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.save_passwd = $2;
#else /* ENABLE_HYBRID */
			yywarn(error_message_hybrid_config_not_configured);
#endif /* ENABLE_HYBRID */
		}
		EOS
	|	CFG_AUTH_THROTTLE NUMBER
		{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.auth_throttle = $2;
#else /* ENABLE_HYBRID */
			yywarn(error_message_hybrid_config_not_configured);
#endif /* ENABLE_HYBRID */
		}
		EOS
	|	CFG_CONF_SOURCE CFG_LOCAL
		{
#ifdef ENABLE_HYBRID
			isakmp_cfg_config.confsource = ISAKMP_CFG_CONF_LOCAL;
#else /* ENABLE_HYBRID */
			yywarn(error_message_hybrid_config_not_configured);
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
			yywarn(error_message_hybrid_config_not_configured);
#endif /* ENABLE_HYBRID */
		}
		EOS
	|	CFG_CONF_SOURCE CFG_LDAP
		{
#ifdef ENABLE_HYBRID
#ifdef HAVE_LIBLDAP
			isakmp_cfg_config.confsource = ISAKMP_CFG_CONF_LDAP;
#else /* HAVE_LIBLDAP */
			yywarn(error_message_ldap_config_not_configured);
#endif /* HAVE_LIBLDAP */
#else /* ENABLE_HYBRID */
			yywarn(error_message_hybrid_config_not_configured);
#endif /* ENABLE_HYBRID */
		}
		EOS
	|	CFG_MOTD QUOTEDSTRING
		{
#ifdef ENABLE_HYBRID
			strncpy(&isakmp_cfg_config.motd[0], $2->v, MAXPATHLEN);
			isakmp_cfg_config.motd[MAXPATHLEN] = '\0';
#else
			yywarn(error_message_hybrid_config_not_configured);
#endif
			vfree($2);
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
			yywarn(error_message_hybrid_config_not_configured);
#endif
			vfree($1);
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
			yywarn(error_message_hybrid_config_not_configured);
#endif
			vfree($1);
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
			memset(&network,0,sizeof(network));

			if (inet_pton(AF_INET, $1->v, &network.addr4) != 1)
				yyerror("bad IPv4 SPLIT address.");

			/* Turn $2 (the prefix) into a subnet mask */
			network.mask4.s_addr = ($2) ? htonl(~((1 << (32 - $2)) - 1)) : 0;

			/* add the network to our list */ 
			if (splitnet_list_add(&icc->splitnet_list, &network,&icc->splitnet_count))
				yyerror("Unable to allocate split network");
#else
			yywarn(error_message_hybrid_config_not_configured);
#endif
			vfree($1);
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
			if (grouplist == NULL) {
				yyerror("unable to allocate auth group list");
				ABORT_AND_VFREE($1);
			}


			groupname = racoon_malloc($1->l+1);
			if (groupname == NULL) {
				yyerror("unable to allocate auth group name");
				ABORT_AND_VFREE($1);
			}

			memcpy(groupname,$1->v,$1->l);
			groupname[$1->l]=0;
			grouplist[icc->groupcount]=groupname;
			icc->grouplist = grouplist;
			icc->groupcount++;

#else
			yywarn(error_message_hybrid_config_not_configured);
#endif
			vfree($1);
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
				if (icc->splitdns_list == NULL) {
					yyerror("error allocating splitdns list buffer");
					ABORT_AND_VFREE($1);
				}

				memcpy(icc->splitdns_list,$1->v,$1->l);
				icc->splitdns_len = $1->l;
			}
			else
			{
				int len = icc->splitdns_len + $1->l + 1;
				icc->splitdns_list = racoon_realloc(icc->splitdns_list,len);
				if (icc->splitdns_list == NULL) {
					yyerror("error allocating splitdns list buffer");
					ABORT_AND_VFREE($1);
				}

				icc->splitdns_list[icc->splitdns_len] = ',';
				memcpy(icc->splitdns_list + icc->splitdns_len + 1, $1->v, $1->l);
				icc->splitdns_len = len;
			}
#else
			yywarn(error_message_hybrid_config_not_configured);
#endif
			vfree($1);
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
			yyerror(error_message_natt_not_compiled_in);
#endif
		}
		EOS
	;

	/* sainfo */
sainfo_statement
	:	SAINFO
		{
			delsainfo(cur_sainfo);
			cur_sainfo = newsainfo();
			if (cur_sainfo == NULL) {
				yyerror("failed to allocate sainfo");
				ABORT();
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
					  NULL,
					  cur_sainfo->remoteid);

			if (check && ((check->idsrc != SAINFO_ANONYMOUS) &&
				      (cur_sainfo->idsrc != SAINFO_ANONYMOUS))) {
				yyerror("duplicated sainfo: %s",
					sainfo2str(cur_sainfo));
				return -1;
			}

			inssainfo(cur_sainfo);
			cur_sainfo = NULL;
		}
		EOC
	;
sainfo_name
	:	ANONYMOUS
		{
			cur_sainfo->idsrc = SAINFO_ANONYMOUS;
			cur_sainfo->iddst = SAINFO_ANONYMOUS;
		}
	|	ANONYMOUS CLIENTADDR
		{
			cur_sainfo->idsrc = SAINFO_ANONYMOUS;
			cur_sainfo->iddst = SAINFO_CLIENTADDR;
		}
	|	ANONYMOUS sainfo_id
		{
			cur_sainfo->idsrc = SAINFO_ANONYMOUS;
			cur_sainfo->iddst = $2;
		}
	|	sainfo_id ANONYMOUS
		{
			cur_sainfo->idsrc = $1;
			cur_sainfo->iddst = SAINFO_ANONYMOUS;
		}
	|	sainfo_id CLIENTADDR
		{
			cur_sainfo->idsrc = $1;
			cur_sainfo->iddst = SAINFO_CLIENTADDR;
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

			switch ($5) {
			case IPPROTO_ICMP:
			case IPPROTO_ICMPV6:
				if ($4 == IPSEC_PORT_ANY)
					break;
				yyerror("port must be \"any\" for icmp{,6}.");
				return -1;
			default:
				break;
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
			yywarn(error_message_hybrid_config_not_configured);
			ABORT_AND_VFREE($2);
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
			ABORT();
#else
			cur_sainfo->lifebyte = fix_lifebyte($3 * $4);
			if (cur_sainfo->lifebyte == 0)
				ABORT();
#endif
		}
		EOS
	|	ALGORITHM_CLASS {
			cur_algclass = $1;
		}
		algorithms EOS
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
			int encklen_tmp;

			$$ = newsainfoalg();
			if ($$ == NULL) {
				yyerror("failed to get algorithm allocation");
				ABORT();
			}

			$$->alg = algtype2doi(cur_algclass, $1);
			if ($$->alg == -1) {
				yyerror("algorithm mismatched");
				ABORT_AND_RACOON_FREE($$);
			}

			defklen = default_keylen(cur_algclass, $1);
			if (defklen == 0) {
				if ($2) {
					yyerror("keylen not allowed");
					ABORT_AND_RACOON_FREE($$);
				}

			} else {
				if ($2 && check_keylen(cur_algclass, $1, $2) < 0) {
					yyerror("invalid keylen %d", $2);
					ABORT_AND_RACOON_FREE($$);
				}
			}

			if ($2)
				$$->encklen = $2;
			else
				$$->encklen = defklen;

			/* Check keymat size instead of "human" key size
			 * because kernel store keymat size instead of "human key size".
			 * For example, the keymat size of aes_gcm_16 128 is 160 bits
			 * (128 bits + 4 bytes) instead of 128 bits.
			 *
			 * Currently, it is only useful for aes_gcm_16 (ipsec_enc).
			 */
			if (cur_algclass == algclass_ipsec_enc)
			{
				encklen_tmp = alg_ipsec_encdef_keylen($$->alg, $$->encklen);
				if (encklen_tmp < 0)
				{
					yyerror("Failed to convert keylen %d to keymat len for alg %d",
						$$->encklen, $$->alg);
					racoon_free($$);
					$$ = NULL;
					return -1;
				}
			}
			else
			{
				/* XXX Convert key size to keymat size for other algorithm ?
				 */
				encklen_tmp = $$->encklen;
			}

			/* check if it's supported algorithm by kernel */
			if (!(cur_algclass == algclass_ipsec_auth && $1 == algtype_non_auth)
			 && pk_checkalg(cur_algclass, $1, encklen_tmp)) {
				int a = algclass2doi(cur_algclass);
				int b = algtype2doi(cur_algclass, $1);
				if (a == IPSECDOI_ATTR_AUTH)
					a = IPSECDOI_PROTO_IPSEC_AH;
				yyerror("algorithm %s not supported by the kernel (missing module?)",
					s_ipsecdoi_trns(a, b));
				ABORT_AND_RACOON_FREE($$);
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
	: REMOTE QUOTEDSTRING INHERIT QUOTEDSTRING
		{
			struct remoteconf *from, *new;

			if (getrmconf_by_name($2->v) != NULL) {
				yyerror("named remoteconf \"%s\" already exists.",
					$2->v);
				ABORT_AND_VFREE2($2, $4);
			}

			from = getrmconf_by_name($4->v);
			if (from == NULL) {
				yyerror("named parent remoteconf \"%s\" does not exist.",
					$4->v);
				ABORT_AND_VFREE2($2, $4);
			}

			new = duprmconf_shallow(from);
			if (new == NULL) {
				yyerror("failed to duplicate remoteconf from \"%s\".",
					$4->v);
				ABORT_AND_VFREE2($2, $4);
			}

			new->name = racoon_strdup($2->v);
			
			delrmconf(cur_rmconf);
			cur_rmconf = new;

			vfree($2);
			vfree($4);
		}
		remote_specs_inherit_block
	| REMOTE QUOTEDSTRING
		{
			struct remoteconf *new;

			if (getrmconf_by_name($2->v) != NULL) {
				yyerror("Named remoteconf \"%s\" already exists.",
					$2->v);
				ABORT_AND_VFREE($2);
			}

			new = newrmconf();
			if (new == NULL) {
				yyerror("failed to get new remoteconf.");
				ABORT_AND_VFREE($2);
			}

			new->name = racoon_strdup($2->v);

			delrmconf(cur_rmconf);
			cur_rmconf = new;

			vfree($2);
		}
		remote_specs_block
	| REMOTE remote_index INHERIT remote_index
		{
			struct remoteconf *from, *new;

			from = getrmconf($4, GETRMCONF_F_NO_ANONYMOUS);
			if (from == NULL) {
				yyerror("failed to get remoteconf for %s.",
					saddr2str($4));
				ABORT_AND_RACOON_FREE2($2, $4);
			}

			new = duprmconf_shallow(from);
			if (new == NULL) {
				yyerror("failed to duplicate remoteconf from %s.",
					saddr2str($4));
				ABORT_AND_RACOON_FREE2($2, $4);
			}

			racoon_free($4);
			new->remote = $2;
			delrmconf(cur_rmconf);
			cur_rmconf = new;
		}
		remote_specs_inherit_block
	|	REMOTE remote_index
		{
			struct remoteconf *new;

			new = newrmconf();
			if (new == NULL) {
				yyerror("failed to get new remoteconf.");
				ABORT_AND_RACOON_FREE($2);
			}

			new->remote = $2;
			delrmconf(cur_rmconf);
			cur_rmconf = new;
		}
		remote_specs_block
	;

remote_specs_inherit_block
	:	remote_specs_block
	|	EOS /* inheritance without overriding any settings */
		{
			if (process_rmconf() != 0)
				ABORT();
		}
	;

remote_specs_block
	:	BOC remote_specs EOC
		{
			if (process_rmconf() != 0)
				ABORT();
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
				yyerror("failed to allocate sockaddr\n");
				ABORT();
			}
		}
	;
remote_specs
	:	/* nothing */
	|	remote_specs remote_spec
	;
remote_spec
	:	REMOTE_ADDRESS ike_addrinfo_port
		{
			if (cur_rmconf->remote != NULL) {
				yyerror("remote_address already specified\n");
				ABORT_AND_RACOON_FREE($2);
			}

			cur_rmconf->remote = $2;
		}
		EOS
	|	EXCHANGE_MODE
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

			if (cur_rmconf->peerscert != NULL) {
				yyerror("peers_certfile already defined\n");
				ABORT_AND_VFREE($2);
			}

			if (load_x509($2->v, &cur_rmconf->peerscertfile,
					&cur_rmconf->peerscert)) {
				yyerror("failed to load certificate \"%s\"\n",
					$2->v);
				ABORT_AND_VFREE($2);
			}

			vfree($2);
		}
		EOS
	|	PEERS_CERTFILE CERT_X509 QUOTEDSTRING
		{
			if (cur_rmconf->peerscert != NULL) {
				yyerror("peers_certfile already defined\n");
				ABORT_AND_VFREE($3);
			}

			if (load_x509($3->v, &cur_rmconf->peerscertfile,
					&cur_rmconf->peerscert)) {
				yyerror("failed to load certificate \"%s\"\n",
					$3->v);
				ABORT_AND_VFREE($3);
			}
			vfree($3);
		}
		EOS
	|	PEERS_CERTFILE CERT_PLAINRSA QUOTEDSTRING
		{
			char path[MAXPATHLEN];

			if (cur_rmconf->peerscert != NULL) {
				yyerror("peers_certfile already defined\n");
				ABORT_AND_VFREE($3);
			}

			cur_rmconf->peerscert = vmalloc(1);
			if (cur_rmconf->peerscert == NULL) {
				yyerror("failed to allocate peerscert\n");
				ABORT_AND_VFREE($3);
			}

			cur_rmconf->peerscert->v[0] = ISAKMP_CERT_PLAINRSA;

			getpathname(path, sizeof(path),
				    LC_PATHTYPE_CERT, $3->v);
			if (rsa_parse_file(cur_rmconf->rsa_public, path,
					RSA_TYPE_PUBLIC)) {
				yyerror("Couldn't parse keyfile.\n", path);
				ABORT_AND_VFREE2(cur_rmconf->peerscert, $3);
			}

			plog(LLV_DEBUG, LOCATION, NULL,
			     "Public PlainRSA keyfile parsed: %s\n", path);

			vfree($3);
		}
		EOS
	|	PEERS_CERTFILE DNSSEC
		{
			if (cur_rmconf->peerscert != NULL) {
				yyerror("peers_certfile already defined\n");
				ABORT();
			}

			cur_rmconf->peerscert = vmalloc(1);
			if (cur_rmconf->peerscert == NULL) {
				yyerror("failed to allocate peerscert\n");
				ABORT();
			}

			cur_rmconf->peerscert->v[0] = ISAKMP_CERT_DNS;
		}
		EOS
	|	CA_TYPE CERT_X509 QUOTEDSTRING
		{
			if (cur_rmconf->cacert != NULL) {
				yyerror("ca_type already defined\n");
				ABORT_AND_VFREE($3);
			}

			if (load_x509($3->v, &cur_rmconf->cacertfile,
					&cur_rmconf->cacert)) {
				yyerror("failed to load certificate \"%s\"\n",
					$3->v);
				ABORT_AND_VFREE($3);
			}

			vfree($3);
		}
		EOS
	|	VERIFY_CERT SWITCH { cur_rmconf->verify_cert = $2; } EOS
	|	SEND_CERT SWITCH { cur_rmconf->send_cert = $2; } EOS
	|	SEND_CR SWITCH { cur_rmconf->send_cr = $2; } EOS
	|	MATCH_EMPTY_CR SWITCH { cur_rmconf->match_empty_cr = $2; } EOS
	|	MY_IDENTIFIER IDENTIFIERTYPE identifierstring
		{
			if (set_identifier(&cur_rmconf->idv, $2, $3) != 0) {
				yyerror("failed to set identifer.\n");
				ABORT_AND_VFREE($3);
			}

			cur_rmconf->idvtype = $2;
			vfree($3);
		}
		EOS
	|	MY_IDENTIFIER IDENTIFIERTYPE IDENTIFIERQUAL identifierstring
		{
			if (set_identifier_qual(&cur_rmconf->idv, $2, $4, $3) != 0) {
				yyerror("failed to set identifer.\n");
				ABORT_AND_VFREE($4);
			}

			cur_rmconf->idvtype = $2;
			vfree($4);
		}
		EOS
	|	XAUTH_LOGIN identifierstring
		{
#ifdef ENABLE_HYBRID
			/* formerly identifier type login */
			if (xauth_rmconf_used(&cur_rmconf->xauth) == -1) {
				yyerror("failed to allocate xauth state\n");
				ABORT_AND_VFREE($2);
			}

			if ((cur_rmconf->xauth->login = vdup($2)) == NULL) {
				yyerror("failed to set identifer\n");
				ABORT_AND_VFREE($2);
			}

#else
			yywarn(error_message_hybrid_config_not_configured);
#endif
			vfree($2);
		}
		EOS
	|	PEERS_IDENTIFIER IDENTIFIERTYPE identifierstring
		{
			struct idspec  *idspec = NULL;
			vchar_t* id = NULL; 
			if (set_identifier(&id, $2, $3) != 0) {
				yyerror("failed to set identifer\n");
				ABORT_AND_VFREE2(id, $3);
			}
			
			if ((idspec = newidspec()) == NULL) {
				yyerror("failed to allocate idspec\n");
				ABORT_AND_VFREE2(id, $3);
			}

			idspec->id = id; /* hand over id to idspec. */
			idspec->idtype = $2;
			genlist_append (cur_rmconf->idvl_p, idspec);
			vfree($3);
		}
		EOS
	|	PEERS_IDENTIFIER IDENTIFIERTYPE IDENTIFIERQUAL identifierstring
		{
			struct idspec *idspec = NULL;
			{
				vchar_t* id = NULL;
				if (set_identifier_qual(&id, $2, $4, $3) != 0) {
					yyerror("failed to set identifer\n");
					ABORT_AND_VFREE2(id, $4);		
				}
				
				if ((idspec = newidspec()) == NULL) {
					yyerror("failed to allocate idspec\n");
					ABORT_AND_VFREE2(id, $4);
				}
	
				idspec->id = id; /* hand over id to idspec. */
			}
			idspec->idtype = $2;
			genlist_append (cur_rmconf->idvl_p, idspec);

			vfree($4);
		}
		EOS
	|	VERIFY_IDENTIFIER SWITCH { cur_rmconf->verify_identifier = $2; } EOS
	|	NONCE_SIZE NUMBER { cur_rmconf->nonce_size = $2; } EOS
	|	DH_GROUP
		{
			yyerror("dh_group cannot be defined here\n");
			ABORT();
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
            	yywarn("libipsec lacks IKE frag support\n");
#else
			yywarn("Your kernel does not support esp_frag\n");
#endif
		} EOS
	|	SCRIPT QUOTEDSTRING PHASE1_UP { 
			if (cur_rmconf->script[SCRIPT_PHASE1_UP] != NULL)
				vfree(cur_rmconf->script[SCRIPT_PHASE1_UP]);

			cur_rmconf->script[SCRIPT_PHASE1_UP] = 
			    script_path_add(vdup($2));

			vfree($2);
		} EOS
	|	SCRIPT QUOTEDSTRING PHASE1_DOWN { 
			if (cur_rmconf->script[SCRIPT_PHASE1_DOWN] != NULL)
				vfree(cur_rmconf->script[SCRIPT_PHASE1_DOWN]);

			cur_rmconf->script[SCRIPT_PHASE1_DOWN] = 
			    script_path_add(vdup($2));

			vfree($2);
		} EOS
	|	SCRIPT QUOTEDSTRING PHASE1_DEAD { 
			if (cur_rmconf->script[SCRIPT_PHASE1_DEAD] != NULL)
				vfree(cur_rmconf->script[SCRIPT_PHASE1_DEAD]);

			cur_rmconf->script[SCRIPT_PHASE1_DEAD] = 
			    script_path_add(vdup($2));

			vfree($2);
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
				yywarn("libipsec lacks NAT-T support\n");
#else
			yywarn(error_message_natt_not_compiled_in);
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
			yywarn(error_message_natt_not_compiled_in);
#endif
		} EOS
	|	DPD SWITCH
		{
#ifdef ENABLE_DPD
			cur_rmconf->dpd = $2;
#else
			yywarn(error_message_dpd_not_compiled_in);
#endif
		} EOS
	|	DPD_DELAY NUMBER
		{
#ifdef ENABLE_DPD
			cur_rmconf->dpd_interval = $2;
#else
			yywarn(error_message_dpd_not_compiled_in);
#endif
		}
		EOS
	|	DPD_RETRY NUMBER
		{
#ifdef ENABLE_DPD
			cur_rmconf->dpd_retry = $2;
#else
			yywarn(error_message_dpd_not_compiled_in);
#endif
		}
		EOS
	|	DPD_MAXFAIL NUMBER
		{
#ifdef ENABLE_DPD
			cur_rmconf->dpd_maxfails = $2;
#else
			yywarn(error_message_dpd_not_compiled_in);
#endif
		}
		EOS
	|	REKEY SWITCH { cur_rmconf->rekey = $2; } EOS
	|	REKEY REMOTE_FORCE_LEVEL { cur_rmconf->rekey = REKEY_FORCE; } EOS
	|	PH1ID NUMBER
		{
			cur_rmconf->ph1id = $2;
		}
		EOS
	|	LIFETIME LIFETYPE_TIME NUMBER unittype_time
		{
			cur_rmconf->lifetime = $3 * $4;
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
			cur_rmconf->lifebyte = fix_lifebyte($3 * $4);
			if (cur_rmconf->lifebyte == 0)
				ABORT();
#endif
		}
		EOS
	|	PROPOSAL
		{
			struct secprotospec *spspec = newspspec();
			if (spspec == NULL)
				ABORT();

			insspspec(cur_rmconf, spspec);
		}
		BOC isakmpproposal_specs EOC
	;
exchange_types
	:	/* nothing */
	|	exchange_types EXCHANGETYPE
		{
			struct etypes *new = racoon_malloc(sizeof(struct etypes));
			if (new == NULL) {
				yyerror("failed to allocate etypes");
				ABORT();
			}

			new->next = NULL; new->type = $2;
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
			if (cur_rmconf->mycert != NULL) {
				yyerror("certificate_type already defined\n");
				ABORT_AND_VFREE2($2, $3);
			}

			if (load_x509($2->v, &cur_rmconf->mycertfile,
					&cur_rmconf->mycert)) {
				yyerror("failed to load certificate \"%s\"\n",
					$2->v);
				ABORT_AND_VFREE2($2, $3);
			}

			cur_rmconf->myprivfile = racoon_strdup($3->v);
			if (!cur_rmconf->myprivfile) {
				yyerror("failed to allocate myprivfile\n");
				ABORT_AND_VFREE2($2, $3);
			}

			vfree($2);
			vfree($3);
		}
		EOS
	|	CERT_PLAINRSA QUOTEDSTRING
		{
			char path[MAXPATHLEN];

			if (cur_rmconf->mycert != NULL) {
				yyerror("certificate_type already defined\n");
				ABORT_AND_VFREE($2);
			}

			cur_rmconf->mycert = vmalloc(1);
			if (cur_rmconf->mycert == NULL) {
				yyerror("failed to allocate mycert\n");
				ABORT_AND_VFREE($2);
			}

			cur_rmconf->mycert->v[0] = ISAKMP_CERT_PLAINRSA;

			getpathname(path, sizeof(path),
				    LC_PATHTYPE_CERT, $2->v);
			cur_rmconf->send_cr = FALSE;
			cur_rmconf->send_cert = FALSE;
			cur_rmconf->verify_cert = FALSE;
			if (rsa_parse_file(cur_rmconf->rsa_private, path,
					RSA_TYPE_PRIVATE)) {
				yyerror("Couldn't parse keyfile %s\n", path);
				ABORT_AND_VFREE($2);
			}

			plog(LLV_DEBUG, LOCATION, NULL,
			     "Private PlainRSA keyfile parsed: %s\n", path);
			vfree($2);
		}
		EOS
	;
dh_group_num
	:	ALGORITHMTYPE
		{
			$$ = algtype2doi(algclass_isakmp_dh, $1);
			if ($$ == -1) {
				yyerror("must be DH group\n");
				ABORT();
			}
		}
	|	NUMBER
		{
			if (ARRAYLEN(num2dhgroup) > $1 && num2dhgroup[$1] != 0) {
				$$ = num2dhgroup[$1];
			} else {
				$$ = 0;
				yyerror("must be DH group\n");
				ABORT();
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
	:	LIFETIME LIFETYPE_TIME NUMBER unittype_time
		{
			cur_rmconf->spspec->lifetime = $3 * $4;
		}
		EOS
	|	LIFETIME LIFETYPE_BYTE NUMBER unittype_byte
		{
#if 1
			yyerror("byte lifetime support is deprecated\n");
			ABORT();
#else
			cur_rmconf->spspec->lifebyte = fix_lifebyte($3 * $4);
			if (cur_rmconf->spspec->lifebyte == 0)
				ABORT();
#endif
		}
		EOS
	|	DH_GROUP dh_group_num
		{
			cur_rmconf->spspec->algclass[algclass_isakmp_dh] = $2;
		}
		EOS
	|	GSS_ID QUOTEDSTRING
		{
			if (cur_rmconf->spspec->vendorid != VENDORID_GSSAPI) {
				yyerror("wrong Vendor ID for gssapi_id\n");
				ABORT_AND_VFREE($2);
			}

			if (cur_rmconf->spspec->gssid != NULL)
				racoon_free(cur_rmconf->spspec->gssid);
			cur_rmconf->spspec->gssid =
			    racoon_strdup($2->v);
			if (!cur_rmconf->spspec->gssid) {
				yyerror("failed to allocate gssid\n");
				ABORT_AND_VFREE($2);
			}
		}
		EOS
	|	ALGORITHM_CLASS ALGORITHMTYPE keylength
		{
			int doi;
			int defklen;

			doi = algtype2doi($1, $2);
			if (doi == -1) {
				yyerror("algorithm mismatched 1\n");
				ABORT();
			}

			switch ($1) {
			case algclass_isakmp_enc:
			/* reject suppressed algorithms */
#ifndef HAVE_OPENSSL_RC5_H
				if ($2 == algtype_rc5) {
					yyerror("algorithm %s not supported\n",
						s_attr_isakmp_enc(doi));
					ABORT();
				}
#endif
#ifndef HAVE_OPENSSL_IDEA_H
				if ($2 == algtype_idea) {
					yyerror("algorithm %s not supported\n",
						s_attr_isakmp_enc(doi));
					ABORT();
				}
#endif

				cur_rmconf->spspec->algclass[algclass_isakmp_enc] = doi;
				defklen = default_keylen($1, $2);
				if (defklen == 0) {
					if ($3) {
						yyerror("keylen not allowed\n");
						ABORT();
					}
				} else {
					if ($3 && check_keylen($1, $2, $3) < 0) {
						yyerror("invalid keylen %d\n", $3);
						ABORT();
					}
				}
				if ($3)
					cur_rmconf->spspec->encklen = $3;
				else
					cur_rmconf->spspec->encklen = defklen;
				break;
			case algclass_isakmp_hash:
				cur_rmconf->spspec->algclass[algclass_isakmp_hash] = doi;
				break;
			case algclass_isakmp_ameth:
				cur_rmconf->spspec->algclass[algclass_isakmp_ameth] = doi;
				/*
				 * We may have to set the Vendor ID for the
				 * authentication method we're using.
				 */
				switch ($2) {
				case algtype_gssapikrb:
					if (cur_rmconf->spspec->vendorid !=
						VENDORID_UNKNOWN) {
						yyerror("Vendor ID mismatch for auth method\n");
						ABORT();
					}
					/*
					 * For interoperability with Win2k,
					 * we set the Vendor ID to "GSSAPI".
					 */
					cur_rmconf->spspec->vendorid =
					    VENDORID_GSSAPI;
					break;
				case algtype_rsasig:
					if (oakley_get_certtype(cur_rmconf->peerscert) == ISAKMP_CERT_PLAINRSA) {
						if (rsa_list_count(cur_rmconf->rsa_private) == 0) {
							yyerror ("Private PlainRSA key not set."
								"Use directive 'certificate_type plainrsa ...'\n");
							ABORT();
						}

						if (rsa_list_count(cur_rmconf->rsa_public) == 0) {
							yyerror ("Public PlainRSA keys not set."
								"Use directive 'peers_certfile plainrsa ...'\n");
							ABORT();
						}
					}
					break;
				default:
					break;
				}
				break;
			default:
				yyerror("algorithm mismatched 2\n");
				ABORT();
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
insspspec(rmconf, spspec)
	struct remoteconf *rmconf;
	struct secprotospec *spspec;
{
	if (rmconf->spspec != NULL)
		rmconf->spspec->prev = spspec;
	spspec->next = rmconf->spspec;
	rmconf->spspec = spspec;
}

static struct secprotospec *
dupspspec(struct secprotospec *spspec)
{
	struct secprotospec *new;

	new = newspspec();
	if (new == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "dupspspec: malloc failed\n");
		return NULL;
	}
	memcpy(new, spspec, sizeof(*new));

	if (spspec->gssid) {
		new->gssid = racoon_strdup(spspec->gssid);
		STRDUP_FATAL(new->gssid);
	}
	if (spspec->remote) {
		new->remote = racoon_malloc(sizeof(*new->remote));
		if (new->remote == NULL) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "dupspspec: malloc failed (remote)\n");
			return NULL;
		}
		memcpy(new->remote, spspec->remote, sizeof(*new->remote));
	}

	return new;
}

/*
 * copy the whole list
 */
void
dupspspec_list(dst, src)
	struct remoteconf *dst, *src;
{
	struct secprotospec *p, *new, *last;

	for(p = src->spspec, last = NULL; p; p = p->next, last = new) {
		new = dupspspec(p);
		if (new == NULL)
			exit(1);

		new->prev = last;
		new->next = NULL; /* not necessary but clean */

		if (last)
			last->next = new;
		else /* first element */
			dst->spspec = new;

	}
}

/*
 * delete the whole list
 */
void
flushspspec(rmconf)
	struct remoteconf *rmconf;
{
	struct secprotospec *p;

	while(rmconf->spspec != NULL) {
		p = rmconf->spspec;
		rmconf->spspec = p->next;
		if (p->next != NULL)
			p->next->prev = NULL; /* not necessary but clean */

		if (p->gssid)
			racoon_free(p->gssid);
		if (p->remote)
			racoon_free(p->remote);
		racoon_free(p);
	}
	rmconf->spspec = NULL;
}

/* set final acceptable proposal */
static int
set_isakmp_proposal(rmconf)
	struct remoteconf *rmconf;
{
	struct secprotospec *s;
	int prop_no = 1; 
	int trns_no = 1;
	int32_t types[MAXALGCLASS];

	/* mandatory check */
	if (rmconf->spspec == NULL) {
		yyerror("no remote specification found: %s.\n",
			saddr2str(rmconf->remote));
		return -1;
	}
	for (s = rmconf->spspec; s != NULL; s = s->next) {
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
	for (s = rmconf->spspec; s->next != NULL; s = s->next)
		;

	while (s != NULL) {
		plog(LLV_DEBUG2, LOCATION, NULL,
			"lifetime = %ld\n", (long)
			(s->lifetime ? s->lifetime : rmconf->lifetime));
		plog(LLV_DEBUG2, LOCATION, NULL,
			"lifebyte = %d\n",
			s->lifebyte ? s->lifebyte : rmconf->lifebyte);
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
				s->lifetime ? s->lifetime : rmconf->lifetime,
				s->lifebyte ? s->lifebyte : rmconf->lifebyte,
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

	yyerrorcount = 0;
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
		yywarn("User \"%s\" does not exist\n", owner->v);

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
		yywarn("Group \"%s\" does not exist\n", group->v);

	if (gr)
		adminsock_group = gr->gr_gid;
	else
		adminsock_group = gid;

	if (mode_dec == -1)
		return;

	if (mode_dec > 777)
		yywarn("Mode 0%03o is invalid\n", mode_dec);

	if (mode_dec >= 400) { mode += 0400; mode_dec -= 400; }
	if (mode_dec >= 200) { mode += 0200; mode_dec -= 200; }
	if (mode_dec >= 100) { mode += 0200; mode_dec -= 100; }

	if (mode_dec > 77)
		yywarn("Mode 0%03o is invalid\n", mode_dec);

	if (mode_dec >= 40) { mode += 040; mode_dec -= 40; }
	if (mode_dec >= 20) { mode += 020; mode_dec -= 20; }
	if (mode_dec >= 10) { mode += 020; mode_dec -= 10; }

	if (mode_dec > 7)
		yywarn("Mode 0%03o is invalid\n", mode_dec);

	if (mode_dec >= 4) { mode += 04; mode_dec -= 4; }
	if (mode_dec >= 2) { mode += 02; mode_dec -= 2; }
	if (mode_dec >= 1) { mode += 02; mode_dec -= 1; }
	
	adminsock_mode = mode;

	return;
}
#endif
