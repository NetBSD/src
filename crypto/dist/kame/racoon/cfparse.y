/*	$KAME: cfparse.y,v 1.80 2000/12/22 03:12:55 sakane Exp $	*/

%{
#include <sys/types.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet6/ipsec.h>
#include <netkey/key_var.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#if !defined(HAVE_GETADDRINFO) || !defined(HAVE_GETNAMEINFO)
#include "addrinfo.h"
#endif

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "sockmisc.h"
#include "str2val.h"
#include "debug.h"

#include "cfparse.h"
#include "cftoken.h"
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
#include "ipsec_doi.h"
#include "strnames.h"
#ifdef GC
#include "gcmalloc.h"
#endif
#ifdef HAVE_GSSAPI
#include "gssapi.h"
#endif

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
};

static struct policyindex *cur_spidx;
static struct remoteconf *cur_rmconf;
static int tmpalgtype[MAXALGCLASS];
static struct sainfo *cur_sainfo;
static int cur_algclass;

static struct proposalspec *prhead;	/* the head is always current. */

static struct addrinfo *parse_addr __P((char *, char *, int));
#if 0
static struct policyindex * parse_spidx __P((caddr_t, int, int,
		caddr_t, int, int, int, int));
#endif
static struct proposalspec *newprspec __P((void));
static void cleanprhead __P((void));
static void insprspec __P((struct proposalspec *, struct proposalspec **));
static struct secprotospec *newspspec __P((void));
static void insspspec __P((struct secprotospec *, struct proposalspec **));

#if 0
static int set_ipsec_proposal __P((struct policyindex *, struct proposalspec *));
#endif
static int set_isakmp_proposal
	__P((struct remoteconf *, struct proposalspec *));
static void clean_tmpalgtype __P((void));
#if 0
static u_int32_t set_algtypes __P((struct secprotospec *, int));
static int expand_ipsecspec __P((int, int, int *, int, int,
	struct proposalspec *, struct secprotospec *, struct ipsecpolicy *));
#endif
static int expand_isakmpspec __P((int, int, int *,
	int, int, time_t, int, int, char *, struct remoteconf *));
%}

%union {
	unsigned long num;
	vchar_t *val;
	struct addrinfo *res;
	struct policyindex *spidx;
	struct remoteconf *rmconf;
	struct sockaddr *saddr;
	struct sainfoalg *alg;
}

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
%token LISTEN X_ISAKMP X_ADMIN STRICT_ADDRESS
	/* timer */
%token RETRY RETRY_COUNTER RETRY_INTERVAL RETRY_PERSEND
%token RETRY_PHASE1 RETRY_PHASE2
	/* algorithm */
%token ALGORITHM_LEVEL ALGORITHM_CLASS ALGORITHMTYPE STRENGTHTYPE
	/* policy */
%token POLICY DIRTYPE ACTION
%token PLADDRTYPE PROPOSAL WHICHSIDE
%token PROTOCOL SECLEVEL SECLEVELTYPE SECMODE SECMODETYPE
	/* sainfo */
%token SAINFO
	/* remote */
%token REMOTE ANONYMOUS
%token EXCHANGE_MODE EXCHANGETYPE DOI DOITYPE SITUATION SITUATIONTYPE
%token CERTIFICATE_TYPE CERTTYPE PEERS_CERTFILE VERIFY_CERT SEND_CERT SEND_CR
%token IDENTIFIERTYPE MY_IDENTIFIER PEERS_IDENTIFIER
%token CERT_X509
%token NONCE_SIZE DH_GROUP KEEPALIVE INITIAL_CONTACT
%token PROPOSAL_CHECK PROPOSAL_CHECK_LEVEL
%token GENERATE_POLICY SUPPORT_MIP6
%token POST_COMMAND
%token EXEC_PATH EXEC_COMMAND EXEC_SUCCESS EXEC_FAILURE
%token GSSAPI_ID

%token PREFIX PORT PORTANY UL_PROTO ANY
%token PFS_GROUP LIFETIME LIFETYPE UNITTYPE STRENGTH

	/* static sa */
%token STATICSA STATICSA_STATEMENT

%token NUMBER SWITCH BOOLEAN
%token HEXSTRING QUOTEDSTRING ADDRSTRING
%token EOS BOC EOC COMMA

%type <num> NUMBER BOOLEAN SWITCH keylength
%type <num> PATHTYPE IDENTIFIERTYPE LOGLEV 
%type <num> ALGORITHM_CLASS algorithm_types algorithm_type dh_group_num
%type <num> ALGORITHMTYPE STRENGTHTYPE
%type <num> PREFIX prefix PORT port ike_port DIRTYPE ACTION PLADDRTYPE WHICHSIDE
%type <num> ul_proto UL_PROTO secproto
%type <num> LIFETYPE UNITTYPE
%type <num> SECLEVELTYPE SECMODETYPE 
%type <num> EXCHANGETYPE DOITYPE SITUATIONTYPE
%type <num> CERTTYPE CERT_X509 PROPOSAL_CHECK_LEVEL
%type <val> QUOTEDSTRING HEXSTRING ADDRSTRING STATICSA_STATEMENT sainfo_id
%type <val> identifierstring
%type <res> ike_addrinfo_port
%type <spidx> policy_index
%type <saddr> remote_index
%type <alg> algorithm

%%

statements
	:	/* nothing */
	|	statements statement
	;
statement
	:	path_statement
	|	include_statement
	|	identifier_statement
	|	logging_statement
	|	padding_statement
	|	listen_statement
	|	timer_statement
	|	algorithm_statement
	|	policy_statement
	|	sainfo_statement
	|	remote_statement
	|	staticsa_statement
	;

	/* path */
path_statement
	:	PATH PATHTYPE QUOTEDSTRING EOS
		{
			if ($2 > LC_PATHTYPE_MAX) {
				yyerror("invalid path type %d", $2);
				return -1;
			}

			/* free old pathinfo */
			if (lcconf->pathinfo[$2])
				free(lcconf->pathinfo[$2]);

			/* set new pathinfo */
			lcconf->pathinfo[$2] = strdup($3->v);
			vfree($3);
		}
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

	/* self infomation */
identifier_statement
	:	IDENTIFIER identifier_stmt
	;
identifier_stmt
	:	VENDORID
		{
			/*XXX to be deleted */
		}
		QUOTEDSTRING EOS
	|	IDENTIFIERTYPE QUOTEDSTRING EOS
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
	;

	/* logging */
logging_statement
	:	LOGGING log_level EOS
	;
log_level
	:	HEXSTRING
		{
			yywarn("see racoon.conf(5), such a log specification will be obsoleted.");
			vfree($1);

			/* command line option has a priority than it. */
			if (!f_debugcmd)
				loglevel++;
		}
	|	LOGLEV
		{
			/* command line option has a priority than it. */
			if (!f_debugcmd)
				loglevel += $1;
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
	:	PAD_RANDOMIZE SWITCH EOS { lcconf->pad_random = $2; }
	|	PAD_RANDOMIZELEN SWITCH EOS { lcconf->pad_randomlen = $2; }
	|	PAD_MAXLEN NUMBER EOS { lcconf->pad_maxsize = $2; }
	|	PAD_STRICT SWITCH EOS { lcconf->pad_strict = $2; }
	|	PAD_EXCLTAIL SWITCH EOS { lcconf->pad_excltail = $2; }
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
	:	X_ISAKMP ike_addrinfo_port EOS
		{
			struct myaddrs *p;

			p = newmyaddr();
			if (p == NULL) {
				yyerror("failed to allocate myaddrs");
				return -1;
			}
			p->addr = dupsaddr($2->ai_addr);
			freeaddrinfo($2);
			if (p->addr == NULL) {
				yyerror("failed to copy sockaddr ");
				delmyaddr(p);
				return NULL;
			}

			insmyaddr(p, &lcconf->myaddrs);

			lcconf->autograbaddr = 0;
		}
	|	X_ADMIN PORT EOS
		{
			lcconf->port_admin = $2;
		}
	|	STRICT_ADDRESS EOS { lcconf->strict_address = TRUE; }
	;
ike_addrinfo_port
	:	ADDRSTRING ike_port
		{
			char portbuf[10];

			snprintf(portbuf, sizeof(portbuf), "%ld", $2);
			$$ = parse_addr($1->v, portbuf, AI_NUMERICHOST);
			vfree($1);
			if (!$$)
				return -1;
		}
	;
ike_port
	:	/* nothing */	{ $$ = PORT_ISAKMP; }
	|	PORT		{ $$ = $1; }
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
		} EOS
	|	RETRY_INTERVAL NUMBER UNITTYPE
		{
			lcconf->retry_interval = $2 * $3;
		} EOS
	|	RETRY_PERSEND NUMBER
		{
			lcconf->count_persend = $2;
		} EOS
	|	RETRY_PHASE1 NUMBER UNITTYPE
		{
			lcconf->retry_checkph1 = $2 * $3;
		} EOS
	|	RETRY_PHASE2 NUMBER UNITTYPE
		{
			lcconf->wait_ph2complete = $2 * $3;
		} EOS
	;

	/* algorithm */
algorithm_statement
	:	ALGORITHM_LEVEL
		{
			/*XXX to be deleted.XXX*/
		} BOC algorithm_stmts EOC
	;
algorithm_stmts
	:	/* nothing */
	|	algorithm_stmts algorithm_stmt
	;
algorithm_stmt
	:	algorithm_class BOC algorithm_strengthes EOC
	;
algorithm_class
	:	ALGORITHM_CLASS
	;
algorithm_strengthes
	:	/* nothing */
	|	algorithm_strengthes algorithm_strength
	;
algorithm_strength
	:	STRENGTHTYPE algorithm_types EOS
	;
algorithm_types
	:	algorithm_type 
	|	algorithm_type algorithm_types 
	;
algorithm_type
	:	ALGORITHMTYPE
	;

	/* policy */
policy_statement
	:	POLICY policy_index
		{
			/*XXX to be deleted*/
			cur_spidx = $2;
		}
		policy_specswrap
	;
policy_specswrap
	:	EOS
		{
			/*
			if (cur_spidx->action == IPSEC_POLICY_IPSEC) {
				yyerror("must define policy for IPsec");
				return -1;
			}
			*/
		}
	|	BOC
		{
			/*
			if (cur_spidx->action != IPSEC_POLICY_IPSEC) {
				yyerror("must not define policy for no IPsec");
				return -1;
			}

			cur_spidx->policy = newipsp();
			if (cur_spidx->policy == NULL) {
				yyerror("failed to allocate ipsec policy");
				return -1;
			}
			cur_spidx->policy->spidx = cur_spidx;
			*/
		}
		policy_specs EOC
		{
			/*
			if (set_ipsec_proposal(cur_spidx, prhead) != 0)
				return -1;
			*/

			/* DH group settting if PFS is required. */
			/*
			if (cur_spidx->policy->pfs_group != 0
			 && oakley_setdhgroup(cur_spidx->policy->pfs_group,
					&cur_spidx->policy->pfsgrp) < 0) {
				yyerror("failed to set DH value.\n");
				return -1;
			}

#if 0
			ipsecdoi_printsa(cur_spidx->policy->proposal);
#endif
			insspidx(cur_spidx);

			cleanprhead();
			*/
		}
	;
policy_index
	:	ADDRSTRING prefix port
		ADDRSTRING prefix port ul_proto DIRTYPE ACTION
		{
			/*
			$$ = parse_spidx($1->v, $2, $3, $4->v, $5, $6, $7, $8);
			$$->action = $9;
			vfree($1);
			vfree($4);
			*/
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
policy_specs
	:	/* nothing */
	|	policy_specs policy_spec
	;
policy_spec
	:	PFS_GROUP dh_group_num EOS
		{
			/*
			int doi;

			doi = algtype2doi(algclass_isakmp_dh, $2);
			if (doi == -1) {
				yyerror("must be DH group");
				return -1;
			}
			cur_spidx->policy->pfs_group = doi;
			*/
		}
	|	PROPOSAL
		{
			/*
			struct proposalspec *prspec;

			prspec = newprspec();
			if (prspec == NULL)
				return -1;
			prspec->lifetime = ipsecdoi_get_defaultlifetime();
			insprspec(prspec, &prhead);
			*/
		}
		BOC ipsecproposal_specs EOC
	;
ipsecproposal_specs
	:	/* nothing */
	|	ipsecproposal_specs ipsecproposal_spec
	;
ipsecproposal_spec
	:	LIFETIME LIFETYPE NUMBER UNITTYPE EOS
		{
			if ($2 == CF_LIFETYPE_TIME)
				prhead->lifetime = $3 * $4;
			else {
				prhead->lifebyte = $3 * $4;
				if (prhead->lifebyte < 1024) {
					yyerror("byte size should be more "
						"than 1024B.");
					return -1;
				}
				prhead->lifebyte /= 1024;
			}
		}
	|	PROTOCOL secproto
		{
			struct secprotospec *spspec;
	
			spspec = newspspec();
			if (spspec == NULL)
				return -1;
			insspspec(spspec, &prhead);

			prhead->spspec->proto_id = ipproto2doi($2);
		}
		BOC secproto_specs EOC
	;
secproto
	:	UL_PROTO {
			switch ($1) {
			case IPPROTO_ESP:
			case IPPROTO_AH:
			case IPPROTO_IPCOMP:
				break;
			default:
				yyerror("It's not security protocol");
				return -1;
			}
			$$ = $1;
		}
	;
secproto_specs
	:	/* nothing */
	|	secproto_specs secproto_spec
	;
secproto_spec
	:	SECLEVEL SECLEVELTYPE EOS { prhead->spspec->ipsec_level = $2; }
	|	SECMODE secmode EOS
	|	STRENGTH
		{
			yyerror("strength directive is obsoleted.");
		} STRENGTHTYPE EOS
	|	ALGORITHM_CLASS ALGORITHMTYPE keylength EOS
		{
			int doi;
			int defklen;

			doi = algtype2doi($1, $2);
			if (doi == -1) {
				yyerror("algorithm mismatched");
				return -1;
			}
			switch ($1) {
			case algclass_ipsec_enc:
				if (prhead->spspec->proto_id != IPSECDOI_PROTO_IPSEC_ESP) {
					yyerror("algorithm mismatched");
					return -1;
				}
				prhead->spspec->algclass[algclass_ipsec_enc] = doi;
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
					prhead->spspec->encklen = $3;
				else
					prhead->spspec->encklen = defklen;
				break;
			case algclass_ipsec_auth:
				if (prhead->spspec->proto_id == IPSECDOI_PROTO_IPCOMP) {
					yyerror("algorithm mismatched");
					return -1;
				}
				prhead->spspec->algclass[algclass_ipsec_auth] = doi;
				break;
			case algclass_ipsec_comp:
				if (prhead->spspec->proto_id != IPSECDOI_PROTO_IPCOMP) {
					yyerror("algorithm mismatched");
					return -1;
				}
				prhead->spspec->algclass[algclass_ipsec_comp] = doi;
				break;
			default:
				yyerror("algorithm mismatched");
				return -1;
			}
		}
	;
secmode
	:	SECMODETYPE {
			if ($1 == IPSECDOI_ATTR_ENC_MODE_TUNNEL) {
				yyerror("must specify peer's address");
				return -1;
			}
			prhead->spspec->encmode = $1;
			prhead->spspec->remote = NULL;
		}
	|	SECMODETYPE ADDRSTRING {
			struct addrinfo *res;

			if ($1 != IPSECDOI_ATTR_ENC_MODE_TUNNEL) {
				yyerror("should not specify peer's address");
				return -1;
			}
			prhead->spspec->encmode = $1;

			res = parse_addr($2->v, NULL, AI_NUMERICHOST);
			vfree($2);
			if (res == NULL)
				return -1;
			prhead->spspec->remote = dupsaddr(res->ai_addr);
			if (prhead->spspec->remote == NULL) {
				yyerror("failed to copy sockaddr ");
				return -1;
			}
			freeaddrinfo(res);
		}
	;
keylength
	:	/* nothing */ { $$ = 0; }
	|	NUMBER { $$ = $1; }
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
		sainfo_name BOC sainfo_specs
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
			check = getsainfo(cur_sainfo->idsrc, cur_sainfo->iddst);
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
			struct addrinfo *res;

			if (($5 == IPPROTO_ICMP || $5 == IPPROTO_ICMPV6)
			 && ($4 != IPSEC_PORT_ANY || $4 != IPSEC_PORT_ANY)) {
				yyerror("port number must be \"any\".");
				return -1;
			}

			snprintf(portbuf, sizeof(portbuf), "%lu", $4);
			res = parse_addr($2->v, portbuf, AI_NUMERICHOST);
			vfree($2);
			if (!res)
				return -1;

			switch (res->ai_family) {
			case AF_INET:
				if ($5 == IPPROTO_ICMPV6) {
					yyerror("upper layer protocol mismatched.\n");
					freeaddrinfo(res);
					return -1;
				}
				$$ = ipsecdoi_sockaddr2id(res->ai_addr,
					$3 == ~0 ? (sizeof(struct in_addr) << 3): $3,
					$5);
				break;
#ifdef INET6
			case AF_INET6:
				if ($5 == IPPROTO_ICMP) {
					yyerror("upper layer protocol mismatched.\n");
					freeaddrinfo(res);
					return -1;
				}
				$$ = ipsecdoi_sockaddr2id(res->ai_addr,
					$3 == ~0 ? (sizeof(struct in6_addr) << 3) : $3,
					$5);
				break;
#endif
			default:
				yyerror("invalid family: %d", res->ai_family);
				break;
			}
			freeaddrinfo(res);
			if ($$ == NULL)
				return -1;
		}
	|	IDENTIFIERTYPE QUOTEDSTRING
		{
			struct ipsecdoi_id_b *id_b;

			if ($1 == IDTYPE_ASN1DN) {
				yyerror("id type forbidden: %d", $1);
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
sainfo_specs
	:	/* nothing */
	|	sainfo_specs sainfo_spec
	;
sainfo_spec
	:	PFS_GROUP dh_group_num EOS
		{
			int doi;

			doi = algtype2doi(algclass_isakmp_dh, $2);
			if (doi == -1) {
				yyerror("must be DH group");
				return -1;
			}
			cur_sainfo->pfs_group = doi;
		}
	|	LIFETIME LIFETYPE NUMBER UNITTYPE EOS
		{
			if ($2 == CF_LIFETYPE_TIME)
				cur_sainfo->lifetime = $3 * $4;
			else {
				cur_sainfo->lifebyte = $3 * $4;
				if (cur_sainfo->lifebyte < 1024) {
					yyerror("byte size should be more "
						"than 1024B.");
					return -1;
				}
				cur_sainfo->lifebyte /= 1024;
			}
		}
	|	ALGORITHM_CLASS {
			cur_algclass = $1;
		}
		algorithms EOS
	|	IDENTIFIER IDENTIFIERTYPE
		{
			/*XXX to be deleted */
			if ($2 == IDTYPE_ASN1DN) {
				yyerror("id type forbidden: %d", $2);
				return -1;
			}
			cur_sainfo->idvtype = $2;
		}
		EOS
	|	MY_IDENTIFIER IDENTIFIERTYPE QUOTEDSTRING
		{
			if ($2 == IDTYPE_ASN1DN) {
				yyerror("id type forbidden: %d", $2);
				return -1;
			}
			if (set_identifier(&cur_sainfo->idv, $2, $3) != 0) {
				yyerror("failed to set identifer.\n");
				return -1;
			}
			cur_sainfo->idvtype = $2;
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
				yyerror("failed to get algorithm alocation");
				return -1;
			}

			$$->alg = algtype2doi(cur_algclass, $1);
			if ($$->alg == -1) {
				yyerror("algorithm mismatched");
				free($$);
				return -1;
			}

			defklen = default_keylen(cur_algclass, $1);
			if (defklen == 0) {
				if ($2) {
					yyerror("keylen not allowed");
					free($$);
					return -1;
				}
			} else {
				if ($2 && check_keylen(cur_algclass, $1, $2) < 0) {
					yyerror("invalid keylen %d", $2);
					free($$);
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
				yyerror("algorithm %s not supported",
					s_ipsecdoi_trns(a, b));
				free($$);
				return -1;
			}
		}
	;

	/* remote */
remote_statement
	:	REMOTE remote_index
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
			insprspec(prspec, &prhead);
		}
		BOC remote_specs EOC
		{
			if (cur_rmconf->idvtype == IDTYPE_ASN1DN
			 && cur_rmconf->mycertfile == NULL) {
				yyerror("id type mismatched due to "
					"no CERT defined.\n");
				return -1;
			}

			if (set_isakmp_proposal(cur_rmconf, prhead) != 0)
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
						"to each proposals's "
						"when aggressive mode.\n");
					return -1;
				}
				cur_rmconf->dh_group = b;

				if (cur_rmconf->dh_group == 0) {
					yyerror("DH group must be required.\n");
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

			cleanprhead();
		}
	;
remote_index
	:	ANONYMOUS ike_port
		{
			$$ = newsaddr(sizeof(struct sockaddr *));
			$$->sa_family = AF_UNSPEC;
			((struct sockaddr_in *)$$)->sin_port = htons($2);
		}
	|	ike_addrinfo_port
		{
			$$ = dupsaddr($1->ai_addr);
			freeaddrinfo($1);
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
	:	EXCHANGE_MODE exchange_types EOS
	|	DOI DOITYPE EOS { cur_rmconf->doitype = $2; }
	|	SITUATION SITUATIONTYPE EOS { cur_rmconf->sittype = $2; }
	|	CERTIFICATE_TYPE cert_spec
	|	PEERS_CERTFILE QUOTEDSTRING
		{
#ifdef HAVE_SIGNING_C
			cur_rmconf->peerscertfile = strdup($2->v);
			vfree($2);
#else
			yyerror("directive not supported");
			return -1;
#endif
		}
		EOS
	|	VERIFY_CERT SWITCH EOS { cur_rmconf->verify_cert = $2; }
	|	SEND_CERT SWITCH EOS { cur_rmconf->send_cert = $2; }
	|	SEND_CR SWITCH EOS { cur_rmconf->send_cr = $2; }
	|	IDENTIFIER IDENTIFIERTYPE EOS
		{
			/*XXX to be deleted */
			cur_rmconf->idvtype = $2;
		}
	|	MY_IDENTIFIER IDENTIFIERTYPE identifierstring EOS
		{
			if (set_identifier(&cur_rmconf->idv, $2, $3) != 0) {
				yyerror("failed to set identifer.\n");
				return -1;
			}
			cur_rmconf->idvtype = $2;
		}
	|	PEERS_IDENTIFIER IDENTIFIERTYPE identifierstring EOS
		{
			if (set_identifier(&cur_rmconf->idv_p, $2, $3) != 0) {
				yyerror("failed to set identifer.\n");
				return -1;
			}
			cur_rmconf->idvtype_p = $2;
		}
	|	NONCE_SIZE NUMBER EOS { cur_rmconf->nonce_size = $2; }
	|	DH_GROUP
		{
			yyerror("dh_group cannot be defined here.");
			return -1;
		}
		dh_group_num EOS
	|	KEEPALIVE EOS { cur_rmconf->keepalive = TRUE; }
	|	GENERATE_POLICY SWITCH EOS { cur_rmconf->gen_policy = $2; }
	|	SUPPORT_MIP6 SWITCH EOS { cur_rmconf->support_mip6 = $2; }
	|	INITIAL_CONTACT SWITCH EOS { cur_rmconf->ini_contact = $2; }
	|	PROPOSAL_CHECK PROPOSAL_CHECK_LEVEL EOS { cur_rmconf->pcheck_level = $2; }
	|	LIFETIME LIFETYPE NUMBER UNITTYPE EOS
		{
			if ($2 == CF_LIFETYPE_TIME)
				prhead->lifetime = $3 * $4;
			else {
				prhead->lifebyte = $3 * $4;
				/*
				 * check size.
				 * Must be more than 1024B because its unit
				 * is kilobytes.  That is defined RFC2407.
				 */
				if (prhead->lifebyte < 1024) {
					yyerror("byte size should be more "
						"than 1024B.");
					return -1;
				}
				prhead->lifebyte /= 1024;
			}
		}
	|	PROPOSAL
		{
			struct secprotospec *spspec;

			spspec = newspspec();
			if (spspec == NULL)
				return -1;
			insspspec(spspec, &prhead);
		}
		BOC isakmpproposal_specs EOC
	;
exchange_types
	:	/* nothing */
	|	exchange_types EXCHANGETYPE
		{
			struct etypes *new;
			new = malloc(sizeof(struct etypes));
			if (new == NULL) {
				yyerror("filed to allocate etypes");
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
#ifdef HAVE_SIGNING_C
			cur_rmconf->certtype = $1;
			cur_rmconf->mycertfile = strdup($2->v);
			vfree($2);
			cur_rmconf->myprivfile = strdup($3->v);
			vfree($3);
#else
			yyerror("directive not supported");
			return -1;
#endif
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
				return -1;
			}
		}
	;
identifierstring
	:	/* nothing */ { $$ = NULL; }
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
	|	LIFETIME LIFETYPE NUMBER UNITTYPE EOS
		{
			if ($2 == CF_LIFETYPE_TIME)
				prhead->spspec->lifetime = $3 * $4;
			else {
				prhead->spspec->lifebyte = $3 * $4;
				/*
				 * check size.
				 * Must be more than 1024B because its unit
				 * is kilobytes.  That is defined RFC2407.
				 */
				if (prhead->spspec->lifebyte < 1024) {
					yyerror("byte size should be "
						"more than 1024B.");
					return -1;
				}
				prhead->spspec->lifebyte /= 1024;
			}
		}
	|	DH_GROUP dh_group_num EOS
		{
			prhead->spspec->algclass[algclass_isakmp_dh] = $2;
		}
	|	GSSAPI_ID QUOTEDSTRING EOS
		{
			prhead->spspec->gssid = strdup($2->v);
		}
	|	ALGORITHM_CLASS ALGORITHMTYPE keylength EOS
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

				prhead->spspec->algclass[algclass_isakmp_enc] = doi;
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
					prhead->spspec->encklen = $3;
				else
					prhead->spspec->encklen = defklen;
				break;
			case algclass_isakmp_hash:
				prhead->spspec->algclass[algclass_isakmp_hash] = doi;
				break;
			case algclass_isakmp_ameth:
				prhead->spspec->algclass[algclass_isakmp_ameth] = doi;
				break;
			default:
				yyerror("algorithm mismatched 2");
				return -1;
			}
		}
	;

	/* static sa */
staticsa_statement
	:	STATICSA STATICSA_STATEMENT
		{
			/* execute static sa */
			/* like system("setkey $2->v"); */
			vfree($2);
		}
		EOS
	;

%%

static struct addrinfo *
parse_addr(host, port, flag)
	char *host;
	char *port;
	int flag;
{
	struct addrinfo hints, *res;
	int error;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = flag;
	error = getaddrinfo(host, port, &hints, &res);
	if (error != 0) {
		yyerror("getaddrinfo(%s%s%s): %s",
			host, port ? "," : "", port ? port : "",
			gai_strerror(error));
		return NULL;
	}
	if (res->ai_next != NULL) {
		yyerror("getaddrinfo(%s%s%s): "
			"resolved to multiple address, "
			"taking the first one",
			host, port ? "," : "", port ? port : "");
	}
	return res;
}

#if 0
static struct policyindex *
parse_spidx(src, prefs, ports, dst, prefd, portd, ul_proto, dir)
	caddr_t src, dst;
	int prefs, ports, prefd, portd, ul_proto, dir;
{
	struct policyindex *spidx;
	char portbuf[10];
	struct addrinfo *res;

	if ((ul_proto == IPPROTO_ICMP || ul_proto == IPPROTO_ICMPV6)
	 && (ports != IPSEC_PORT_ANY || portd != IPSEC_PORT_ANY)) {
		yyerror("port number must be \"any\".");
		return NULL;
	}

	spidx = newspidx();
	if (spidx == NULL) {
		yyerror("failed to allocate policy index");
		return NULL;
	}

	spidx->dir = dir;
	spidx->ul_proto = ul_proto;

	snprintf(portbuf, sizeof(portbuf), "%d", ports);
	res = parse_addr(src, portbuf, AI_NUMERICHOST);
	if (res == NULL) {
		delspidx(spidx);
		return NULL;
	}
	switch (res->ai_family) {
	case AF_INET:
		spidx->prefs = prefs == ~0
			? (sizeof(struct in_addr) << 3)
			: prefs;
		break;
#ifdef INET6
	case AF_INET6:
		spidx->prefs = prefs == ~0
			? (sizeof(struct in6_addr) << 3)
			: prefs;
		break;
#endif
	default:
		yyerror("invalid family: %d", res->ai_family);
		return NULL;
		break;
	}
	memcpy(&spidx->src, res->ai_addr, res->ai_addrlen);
	freeaddrinfo(res);

	snprintf(portbuf, sizeof(portbuf), "%d", portd);
	res = parse_addr(dst, portbuf, AI_NUMERICHOST);
	if (res == NULL) {
		delspidx(spidx);
		return NULL;
	}
	switch (res->ai_family) {
	case AF_INET:
		spidx->prefd = prefd == ~0
			? (sizeof(struct in_addr) << 3)
			: prefd;
		break;
#ifdef INET6
	case AF_INET6:
		spidx->prefd = prefd == ~0
			? (sizeof(struct in6_addr) << 3)
			: prefd;
		break;
#endif
	default:
		yyerror("invalid family: %d", res->ai_family);
		return NULL;
		break;
	}
	memcpy(&spidx->dst, res->ai_addr, res->ai_addrlen);
	freeaddrinfo(res);

	return spidx;
}
#endif

static struct proposalspec *
newprspec()
{
	struct proposalspec *new;

	new = CALLOC(sizeof(*new), struct proposalspec *);
	if (new == NULL)
		yyerror("failed to allocate proposal");

	return new;
}

static void
cleanprhead()
{
	struct proposalspec *p, *next;

	if (prhead == NULL)
		return;

	for (p = prhead; p != NULL; p = next) {
		next = p->next;
		free(p);
	}

	prhead = NULL;
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

	new = CALLOC(sizeof(*new), struct secprotospec *);
	if (new == NULL) {
		yyerror("failed to allocate spproto");
		return NULL;
	}

	new->encklen = 0;	/*XXX*/

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

#if 0
/* set final acceptable proposal */
static int
set_ipsec_proposal(spidx, prspec)
	struct policyindex *spidx;
	struct proposalspec *prspec;
{
	struct proposalspec *p;
	struct secprotospec *s;
	struct proposalspec *new;
	int prop_no; 
	int trns_no;
	u_int32_t types[MAXALGCLASS];

	if (spidx->policy == NULL)
		return -1;

	/*
	 * first, try to assign proposal/transform numbers to the table.
	 */
	for (p = prspec; p->next; p = p->next)
		;
	prop_no = 1;
	while (p) {
		for (s = p->spspec; s && s->next; s = s->next)
			;
		trns_no = 1;
		while (s) {
			s->prop_no = prop_no;
			s->trns_no = trns_no;
			trns_no++;
			s = s->prev;
		}

		prop_no++;
		p = p->prev;
	}

	/* split up proposals if necessary */
	for (p = prspec; p && p->next; p = p->next)
		;
	while (p) {
		int proto_id = 0;

		for (s = p->spspec; s && s->next; s = s->next)
			;
		if (s)
			proto_id = s->proto_id;
		new = NULL;
		while (s) {
			if (proto_id != s->proto_id) {
				if (!new)
					new = newprspec();
				if (!new)
					return -1;
				new->lifetime = p->lifetime;
				new->lifebyte = p->lifebyte;

				/* detach it from old list */
				if (s->prev)
					s->prev->next = s->next;
				else
					p->spspec = s->next;
				if (s->next)
					s->next->prev = s->prev;
				s->next = s->prev = NULL;

				/* insert to new list */
				insspspec(s, &new);
			}
			s = s->prev;
		}

		if (new) {
			new->prev = p->prev;
			if (p->prev)
				p->prev->next = new;
			new->next = p;
			p->prev = new;
			new = NULL;
		}

		p = p->prev;
	}

#if 0
	for (p = prspec; p; p = p->next) {
		fprintf(stderr, "prspec: %p next=%p prev=%p\n", p, p->next, p->prev);
		for (s = p->spspec; s; s = s->next) {
			fprintf(stderr, "    spspec: %p next=%p prev=%p prop:%d trns:%d\n", s, s->next, s->prev, s->prop_no, s->trns_no);
		}
	}
#endif

	for (p = prspec; p->next != NULL; p = p->next)
		;
	while (p != NULL) {
		for (s = p->spspec; s->next != NULL; s = s->next)
			;
		while (s != NULL) {
			plog(LLV_DEBUG2, LOCATION, NULL,
				"lifetime = %ld\n", (long)p->lifetime);
			plog(LLV_DEBUG2, LOCATION, NULL,
				"lifebyte = %d\n", p->lifebyte);
			plog(LLV_DEBUG2, LOCATION, NULL,
				"level=%s\n", s_ipsec_level(s->ipsec_level));
			plog(LLV_DEBUG2, LOCATION, NULL,
				"mode=%s\n", s_ipsecdoi_encmode(s->encmode));
			plog(LLV_DEBUG2, LOCATION, NULL,
				"remote=%s\n", saddrwop2str(s->remote));
			plog(LLV_DEBUG2, LOCATION, NULL,
				"proto=%s\n", s_ipsecdoi_proto(s->proto_id));
			plog(LLV_DEBUG2, LOCATION, NULL,
				"strength=%s\n", s_algstrength(s->strength));
			plog(LLV_DEBUG2, LOCATION, NULL,
				"encklen=%d\n", s->encklen);

			switch (s->proto_id) {
			case IPSECDOI_PROTO_IPSEC_ESP:
				types[algclass_ipsec_enc] =
					set_algtypes(s, algclass_ipsec_enc);
				types[algclass_ipsec_auth] =
					set_algtypes(s, algclass_ipsec_auth);
				types[algclass_ipsec_comp] = 0;

				/* expanding spspec */
				clean_tmpalgtype();
				trns_no = expand_ipsecspec(s->prop_no,
						s->trns_no, types,
						algclass_ipsec_enc,
						algclass_ipsec_auth + 1,
						p, s, spidx->policy);
				if (trns_no == -1) {
					plog(LLV_ERROR, LOCATION, NULL,
						"failed to expand "
						"ipsec proposal.\n");
					return -1;
				}
				break;
			case IPSECDOI_PROTO_IPSEC_AH:
				types[algclass_ipsec_enc] = 0;
				types[algclass_ipsec_auth] =
					set_algtypes(s, algclass_ipsec_auth);
				types[algclass_ipsec_comp] = 0;

				/* expanding spspec */
				clean_tmpalgtype();
				trns_no = expand_ipsecspec(s->prop_no,
						s->trns_no, types,
						algclass_ipsec_auth,
						algclass_ipsec_auth + 1,
						p, s, spidx->policy);
				if (trns_no == -1) {
					plog(LLV_ERROR, LOCATION, NULL,
						"failed to expand "
						"ipsec proposal.\n");
					return -1;
				}
				break;
			case IPSECDOI_PROTO_IPCOMP:
				types[algclass_ipsec_comp] =
					set_algtypes(s, algclass_ipsec_comp);
				types[algclass_ipsec_enc] = 0;
				types[algclass_ipsec_auth] = 0;

				/* expanding spspec */
				clean_tmpalgtype();
				trns_no = expand_ipsecspec(s->prop_no,
						s->trns_no, types,
						algclass_ipsec_comp,
						algclass_ipsec_comp + 1,
						p, s, spidx->policy);
				if (trns_no == -1) {
					plog(LLV_ERROR, LOCATION, NULL,
						"failed to expand "
						"ipsec proposal.\n");
					return -1;
				}
				break;
			default:
				yyerror("Invaled ipsec protocol %d\n",
					s->proto_id);
				return -1;
			}

			s = s->prev;
		}
		prop_no++; 
		trns_no = 1;	/* reset */
		p = p->prev;
	}

	return 0;
}
#endif

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
	u_int32_t types[MAXALGCLASS];

	p = prspec;
	if (p->next != 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"multiple proposal definition.\n");
		return -1;
	}

	/* mandatory check */
	if (p->spspec == NULL) {
		yyerror("no remote specification found: %s.\n",
			rm2str(rmconf));
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
			"strength=%s\n", s_algstrength(s->strength));
		plog(LLV_DEBUG2, LOCATION, NULL,
			"encklen=%d\n", s->encklen);

#if 0
		types[algclass_isakmp_enc] =
			set_algtypes(s, algclass_isakmp_enc);
		types[algclass_isakmp_hash] =
			set_algtypes(s, algclass_isakmp_hash);
		types[algclass_isakmp_dh] =
			set_algtypes(s, algclass_isakmp_dh);
		types[algclass_isakmp_ameth] =
			set_algtypes(s, algclass_isakmp_ameth);
#else
		memset(types, 0, ARRAYLEN(types));
		types[algclass_isakmp_enc] = s->algclass[algclass_isakmp_enc];
		types[algclass_isakmp_hash] = s->algclass[algclass_isakmp_hash];
		types[algclass_isakmp_dh] = s->algclass[algclass_isakmp_dh];
		types[algclass_isakmp_ameth] =
		    s->algclass[algclass_isakmp_ameth];
#endif

		/* expanding spspec */
		clean_tmpalgtype();
		trns_no = expand_isakmpspec(prop_no, trns_no, types,
				algclass_isakmp_enc, algclass_isakmp_ameth + 1,
				s->lifetime ? s->lifetime : p->lifetime,
				s->lifebyte ? s->lifebyte : p->lifebyte,
				s->encklen, s->gssid,
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

#if 0
static u_int32_t
set_algtypes(s, class)
	struct secprotospec *s;
	int class;
{
	u_int32_t algtype = 0;

	if (s->algclass[class])
		algtype = (1 << s->algclass[class]) >> 1;
	else
		algtype = lcconf->algstrength[class]->algtype[s->strength];

	plog(LLV_DEBUG2, LOCATION, NULL,
		"%s=\t%s\n", s_algclass(class), BIT2STR(algtype));

	return algtype;
}
#endif

static void
clean_tmpalgtype()
{
	int i;
	for (i = 0; i < MAXALGCLASS; i++)
		tmpalgtype[i] = 0;	/* means algorithm undefined. */
}

#if 0
static int
expand_ipsecspec(prop_no, trns_no, types,
		class, last, p, s, ipsp)
	int prop_no, trns_no;
	int *types, class, last;
	struct proposalspec *p;
	struct secprotospec *s;
	struct ipsecpolicy *ipsp;
{
	int b = types[class];
	int bl = sizeof(lcconf->algstrength[0]->algtype[0]) << 3;
	int i;

	if (class == last) {
		struct ipsecsa *new = NULL;

		{
			int j;
			char tb[4];
			plog(LLV_DEBUG2, LOCATION, NULL,
				"p:%d t:%d ", prop_no, trns_no);
			for (j = 0; j < MAXALGCLASS; j++) {
				snprintf(tb, sizeof(tb), "%d", tmpalgtype[j]);
				plog(LLV_DEBUG2, LOCATION, NULL,
					"%s%s%s%s%s",
					s_algtype(j, tmpalgtype[j]),
					tmpalgtype[j] ? "(" : "",
					tb[0] == '0' ? "" : tb,
					tmpalgtype[j] ? ")" : "",
					j == MAXALGCLASS ? "\n", " ");
			}
		}

		/* check mandatory values */
		if (ipsecdoi_checkalgtypes(s->proto_id, 
			tmpalgtype[algclass_ipsec_enc],
			tmpalgtype[algclass_ipsec_auth],
			tmpalgtype[algclass_ipsec_comp]) == -1) {
			return -1;
		}

		/* set new sa */
		new = newipsa();
		if (new == NULL) {
			yyerror("failed to allocate ipsec sa");
			return -1;
		}
		new->prop_no = prop_no;
		new->trns_no = trns_no;
		new->lifetime = p->lifetime;
		new->lifebyte = p->lifebyte;
		new->proto_id = s->proto_id;
		new->ipsec_level = s->ipsec_level;
		new->encmode = s->encmode;
		new->enctype = tmpalgtype[algclass_ipsec_enc];
		new->encklen = s->encklen;
		new->authtype = tmpalgtype[algclass_ipsec_auth];
		new->comptype = tmpalgtype[algclass_ipsec_comp];

#if 0
		insipsa(new, ipsp);
#endif

		return trns_no + 1;
	}

	if (b != 0) {
		for (i = 0; i < bl; i++) {
			if (b & 1) {
				tmpalgtype[class] = i + 1;
				trns_no = expand_ipsecspec(prop_no, trns_no,
				    types, class + 1, last, p, s, ipsp);
				if (trns_no == -1)
					return -1;
			}
			b >>= 1;
		}
	} else {
		tmpalgtype[class] = 0;
		trns_no = expand_ipsecspec(prop_no, trns_no, types, class + 1,
		    last, p, s, ipsp);
		if (trns_no == -1)
			return -1;
	}

	return trns_no;
}
#endif

static int
expand_isakmpspec(prop_no, trns_no, types,
		class, last, lifetime, lifebyte, encklen, gssid, rmconf)
	int prop_no, trns_no;
	int *types, class, last;
	time_t lifetime;
	int lifebyte;
	int encklen;
	char *gssid;
	struct remoteconf *rmconf;
{
	struct isakmpsa *new;

	/* debugging */
    {
	int j;
	char tb[10];
	plog(LLV_DEBUG2, LOCATION, NULL,
		"p:%d t:%d ", prop_no, trns_no);
	for (j = class; j < MAXALGCLASS; j++) {
		snprintf(tb, sizeof(tb), "%d", types[j]);
		plog(LLV_DEBUG2, LOCATION, NULL,
			"%s%s%s%s ",
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
#undef TMPALGTYPE2STR(n)

	/* set new sa */
	new = newisakmpsa();
	if (new == NULL) {
		yyerror("failed to allocate isakmp sa");
		return -1;
	}
	new->prop_no = prop_no;
	new->trns_no = trns_no;
	new->lifetime = lifetime;
	new->lifebyte = lifebyte;
	new->enctype = types[algclass_isakmp_enc];
	new->encklen = encklen;
	new->authmethod = types[algclass_isakmp_ameth];
	new->hashtype = types[algclass_isakmp_hash];
	new->dh_group = types[algclass_isakmp_dh];
#ifdef HAVE_GSSAPI
	if (gssid != NULL) {
		new->gssid = vmalloc(strlen(gssid) + 1);
		memcpy(new->gssid->v, gssid, new->gssid->l);
		free(gssid);
	} else
		new->gssid = NULL;
#endif
	insisakmpsa(new, rmconf);

	return trns_no;
}

int
cfparse()
{
	int error;

	yycf_init_buffer();

	if (yycf_set_buffer(lcconf->racoon_conf) != 0)
		return -1;

	prhead = NULL;

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
	cleanprhead();
	clean_tmpalgtype();
	yycf_init_buffer();

	if (yycf_set_buffer(lcconf->racoon_conf) != 0)
		return -1;

	return(cfparse());
}

