/*	$NetBSD: ldapauth.h,v 1.3 2015/04/03 23:58:19 christos Exp $	*/
/* $Id: ldapauth.h,v 1.3 2015/04/03 23:58:19 christos Exp $ 
 */

/*
 *
 * Copyright (c) 2005, Eric AUGE <eau@phear.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 * Neither the name of the phear.org nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, 
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, 
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

#ifndef LDAPAUTH_H
#define LDAPAUTH_H

#define LDAP_DEPRECATED 1

#include <string.h>
#include <time.h>
#include <ldap.h>
#include <lber.h>

/* tokens in use for config */
#define _DEFAULT_LPK_TOKEN "UseLPK"
#define _DEFAULT_SRV_TOKEN "LpkServers"
#define _DEFAULT_USR_TOKEN "LpkUserDN"
#define _DEFAULT_GRP_TOKEN "LpkGroupDN"
#define _DEFAULT_BDN_TOKEN "LpkBindDN"
#define _DEFAULT_BPW_TOKEN "LpkBindPw"
#define _DEFAULT_MYG_TOKEN "LpkServerGroup"
#define _DEFAULT_FIL_TOKEN "LpkFilter"
#define _DEFAULT_TLS_TOKEN "LpkForceTLS"
#define _DEFAULT_BTI_TOKEN "LpkBindTimelimit"
#define _DEFAULT_STI_TOKEN "LpkSearchTimelimit"
#define _DEFAULT_LDP_TOKEN "LpkLdapConf"

#define _DEFAULT_PUB_TOKEN "LpkPubKeyAttr"

/* default options */
#define _DEFAULT_LPK_ON 0
#define _DEFAULT_LPK_SERVERS NULL
#define _DEFAULT_LPK_UDN NULL
#define _DEFAULT_LPK_GDN NULL
#define _DEFAULT_LPK_BINDDN NULL
#define _DEFAULT_LPK_BINDPW NULL
#define _DEFAULT_LPK_SGROUP NULL
#define _DEFAULT_LPK_FILTER NULL
#define _DEFAULT_LPK_TLS -1
#define _DEFAULT_LPK_BTIMEOUT 10
#define _DEFAULT_LPK_STIMEOUT 10
#define _DEFAULT_LPK_LDP NULL
#define _DEFAULT_LPK_PUB "sshPublicKey"

/* flags */
#define FLAG_EMPTY	    0x00000000
#define FLAG_CONNECTED	    0x00000001

/* flag macros */
#define FLAG_SET_EMPTY(x)		x&=(FLAG_EMPTY)
#define FLAG_SET_CONNECTED(x)		x|=(FLAG_CONNECTED)
#define FLAG_SET_DISCONNECTED(x)	x&=~(FLAG_CONNECTED)

/* defines */
#define FAILURE -1
#define SUCCESS 0

/* 
 *
 * defined files path 
 * (should be relocated to pathnames.h,
 * if one day it's included within the tree) 
 *
 */
#define _PATH_LDAP_CONFIG_FILE "/etc/ldap.conf"

/* structures */
typedef struct ldap_options {
    int on;			/* Use it or NOT */
    LDAP * ld;			/* LDAP file desc */
    char * servers;		/* parsed servers for ldaplib failover handling */
    char * u_basedn;		/* user basedn */
    char * g_basedn;		/* group basedn */
    char * binddn;		/* binddn */
    char * bindpw;		/* bind password */
    char * sgroup;		/* server group */
    char * fgroup;		/* group filter */
    char * filter;		/* additional filter */
    char * l_conf;		/* use ldap.conf */
    int tls;			/* TLS only */
    struct timeval b_timeout;   /* bind timeout */
    struct timeval s_timeout;   /* search timeout */
    unsigned int flags;		/* misc flags (reconnection, future use?) */
    char * pub_key_attr;	/* Pubkey-Attribute */
} ldap_opt_t;

typedef struct ldap_keys {
    struct berval ** keys;	/* the public keys retrieved */
    unsigned int num;		/* number of keys */
} ldap_key_t;


/* function headers */
void ldap_close(ldap_opt_t *);
int ldap_connect(ldap_opt_t *);
char * ldap_parse_groups(const char *);
char * ldap_parse_servers(const char *);
void ldap_options_print(ldap_opt_t *);
void ldap_options_free(ldap_opt_t *);
void ldap_keys_free(ldap_key_t *);
int ldap_parse_lconf(ldap_opt_t *);
ldap_key_t * ldap_getuserkey(ldap_opt_t *, const char *);
int ldap_ismember(ldap_opt_t *, const char *);

#endif
