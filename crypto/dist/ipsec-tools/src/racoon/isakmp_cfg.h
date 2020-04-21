/*	$NetBSD: isakmp_cfg.h,v 1.6.88.2 2020/04/21 19:37:54 martin Exp $	*/

/*	$KAME$ */

/*
 * Copyright (C) 2004 Emmanuel Dreyfus
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

#ifdef HAVE_LIBPAM
#include <security/pam_appl.h>
#endif

/* 
 * XXX don't forget to update 
 * src/racoon/handler.c:exclude_cfg_addr()
 * if you add IPv6 capability
 */

/* Attribute types */
#define INTERNAL_IP4_ADDRESS        1
#define INTERNAL_IP4_NETMASK        2
#define INTERNAL_IP4_DNS            3
#define INTERNAL_IP4_NBNS           4
#define INTERNAL_ADDRESS_EXPIRY     5
#define INTERNAL_IP4_DHCP           6
#define APPLICATION_VERSION         7
#define INTERNAL_IP6_ADDRESS        8
#define INTERNAL_IP6_NETMASK        9
#define INTERNAL_IP6_DNS           10
#define INTERNAL_IP6_NBNS          11
#define INTERNAL_IP6_DHCP          12
#define INTERNAL_IP4_SUBNET        13
#define SUPPORTED_ATTRIBUTES       14
#define INTERNAL_IP6_SUBNET        15

/* For APPLICATION_VERSION */
#define ISAKMP_CFG_RACOON_VERSION "racoon / IPsec-tools"

/* For the wins servers -- XXX find the value somewhere ? */
#define MAXWINS 4

/* 
 * Global configuration for ISAKMP mode confiration address allocation 
 * Read from the mode_cfg section of racoon.conf
 */
struct isakmp_cfg_port {
	char	used;
#ifdef HAVE_LIBPAM
	pam_handle_t *pam;
#endif
};

struct isakmp_cfg_config {
	in_addr_t		network4;
	in_addr_t		netmask4;
	in_addr_t		dns4[MAXNS];
	int			dns4_index;
	in_addr_t		nbns4[MAXWINS];
	int			nbns4_index;
	struct isakmp_cfg_port 	*port_pool;
	int			authsource;
	int			groupsource;
	char			**grouplist;
	int			groupcount;
	int			confsource;
	int			accounting;
	size_t			pool_size;
	int			auth_throttle;
	/* XXX move this to a unity specific sub-structure */
	char			default_domain[MAXPATHLEN + 1];
	char			motd[MAXPATHLEN + 1];
	struct unity_netentry	*splitnet_list;
	int			splitnet_count;
	int			splitnet_type;
	char 			*splitdns_list;
	int			splitdns_len;
	int			pfs_group;
	int			save_passwd;
};

/* For utmp updating */
#define TERMSPEC	"vpn%d"

/* For authsource */
#define ISAKMP_CFG_AUTH_SYSTEM	0
#define ISAKMP_CFG_AUTH_RADIUS	1
#define ISAKMP_CFG_AUTH_PAM	2
#define ISAKMP_CFG_AUTH_LDAP	4

/* For groupsource */
#define ISAKMP_CFG_GROUP_SYSTEM	0
#define ISAKMP_CFG_GROUP_LDAP	1

/* For confsource */
#define ISAKMP_CFG_CONF_LOCAL	0
#define ISAKMP_CFG_CONF_RADIUS	1
#define ISAKMP_CFG_CONF_LDAP	2

/* For accounting */
#define ISAKMP_CFG_ACCT_NONE	0
#define ISAKMP_CFG_ACCT_RADIUS	1
#define ISAKMP_CFG_ACCT_PAM	2
#define ISAKMP_CFG_ACCT_LDAP	3
#define ISAKMP_CFG_ACCT_SYSTEM	4

/* For pool_size */
#define ISAKMP_CFG_MAX_CNX	255

/* For motd */
#define ISAKMP_CFG_MOTD	"/etc/motd"

/* For default domain */
#define ISAKMP_CFG_DEFAULT_DOMAIN ""

extern struct isakmp_cfg_config isakmp_cfg_config;

/*
 * ISAKMP mode config state 
 */
#define LOGINLEN 31
struct isakmp_cfg_state {
	int flags;			/* See below */
	unsigned int port;		/* address index */
	char login[LOGINLEN + 1];	/* login */
	struct in_addr addr4;		/* IPv4 address */
	struct in_addr mask4;		/* IPv4 netmask */
	struct in_addr dns4[MAXNS];	/* IPv4 DNS (when client only) */
	int dns4_index;			/* Number of IPv4 DNS (client only) */
	struct in_addr wins4[MAXWINS];	/* IPv4 WINS (when client only) */
	int wins4_index;		/* Number of IPv4 WINS (client only) */
	char default_domain[MAXPATHLEN + 1];	/* Default domain recieved */
	struct unity_netentry 
	    *split_include; 		/* UNITY_SPLIT_INCLUDE */
	int include_count;		/* Number of SPLIT_INCLUDES */
	struct unity_netentry 
	    *split_local;		/* UNITY_LOCAL_LAN */
	int local_count;		/* Number of SPLIT_LOCAL */
	struct xauth_state xauth;	/* Xauth state, if revelant */		
	struct isakmp_ivm *ivm;		/* XXX Use iph1's ivm? */
	u_int32_t last_msgid;           /* Last message-ID */
};

/* flags */
#define ISAKMP_CFG_VENDORID_XAUTH	0x01	/* Supports Xauth */
#define ISAKMP_CFG_VENDORID_UNITY	0x02	/* Cisco Unity compliant */
#define ISAKMP_CFG_PORT_ALLOCATED	0x04	/* Port allocated */
#define ISAKMP_CFG_ADDR4_EXTERN		0x08	/* Address from external config  */
#define ISAKMP_CFG_MASK4_EXTERN		0x10	/* Netmask from external config */
#define ISAKMP_CFG_ADDR4_LOCAL		0x20	/* Address from local pool */
#define ISAKMP_CFG_MASK4_LOCAL		0x40	/* Netmask from local pool */
#define ISAKMP_CFG_GOT_ADDR4		0x80	/* Client got address */
#define ISAKMP_CFG_GOT_MASK4		0x100	/* Client got mask */
#define ISAKMP_CFG_GOT_DNS4		0x200	/* Client got DNS */
#define ISAKMP_CFG_GOT_WINS4		0x400	/* Client got WINS */
#define ISAKMP_CFG_DELETE_PH1		0x800	/* phase 1 should be deleted */
#define ISAKMP_CFG_GOT_DEFAULT_DOMAIN	0x1000	/* Client got default domain */
#define ISAKMP_CFG_GOT_SPLIT_INCLUDE	0x2000	/* Client got a split network config */
#define ISAKMP_CFG_GOT_SPLIT_LOCAL	0x4000	/* Client got a split LAN config */

struct isakmp_pl_attr;
struct ph1handle;
struct isakmp_ivm;
void isakmp_cfg_r(struct ph1handle *, vchar_t *);
int isakmp_cfg_attr_r(struct ph1handle *, u_int32_t, struct isakmp_pl_attr *);
int isakmp_cfg_reply(struct ph1handle *, struct isakmp_pl_attr *);
int isakmp_cfg_request(struct ph1handle *, struct isakmp_pl_attr *);
int isakmp_cfg_set(struct ph1handle *, struct isakmp_pl_attr *);
int isakmp_cfg_send(struct ph1handle *, vchar_t *, u_int32_t, int, int);
struct isakmp_ivm *isakmp_cfg_newiv(struct ph1handle *, u_int32_t);
void isakmp_cfg_rmstate(struct ph1handle *);
struct isakmp_cfg_state *isakmp_cfg_mkstate(void);
vchar_t *isakmp_cfg_copy(struct ph1handle *, struct isakmp_data *);
vchar_t *isakmp_cfg_short(struct ph1handle *, struct isakmp_data *, int);
vchar_t *isakmp_cfg_varlen(struct ph1handle *, struct isakmp_data *, char *, size_t);
vchar_t *isakmp_cfg_string(struct ph1handle *, struct isakmp_data *, char *);
int isakmp_cfg_getconfig(struct ph1handle *);
int isakmp_cfg_setenv(struct ph1handle *, char ***, int *);

int isakmp_cfg_resize_pool(int);
int isakmp_cfg_getport(struct ph1handle *);
int isakmp_cfg_putport(struct ph1handle *, unsigned int);
int isakmp_cfg_init(int);
#define ISAKMP_CFG_INIT_COLD	1
#define ISAKMP_CFG_INIT_WARM	0

#ifdef HAVE_LIBRADIUS
struct rad_handle;
extern struct rad_handle *radius_acct_state;
int isakmp_cfg_radius_common(struct rad_handle *, int); 
#endif

#ifdef HAVE_LIBPAM
int isakmp_cfg_accounting_pam(int, int);
void cleanup_pam(int);
#endif

int isakmp_cfg_accounting_system(int, struct sockaddr *, char *, int);
