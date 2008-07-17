/* nssov.c - nss-ldap overlay for slapd */
/* $OpenLDAP: pkg/ldap/contrib/slapd-modules/nssov/nssov.c,v 1.1.2.1 2008/07/08 18:53:57 quanah Exp $ */
/*
 * Copyright 2008 by Howard Chu, Symas Corp.
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
/*
 * This code references portions of the nss-ldapd package
 * written by Arthur de Jong. The nss-ldapd code was forked
 * from the nss-ldap library written by Luke Howard.
 */

#include "nssov.h"

#ifndef SLAPD_OVER_NSSOV
#define SLAPD_OVER_NSSOV SLAPD_MOD_DYNAMIC
#endif

#include "../slapd/config.h"	/* not nss-ldapd config.h */

#include "lutil.h"

#include <ac/errno.h>
#include <ac/unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

/* buffer sizes for I/O */
#define READBUFFER_MINSIZE 32
#define READBUFFER_MAXSIZE 64
#define WRITEBUFFER_MINSIZE 64
#define WRITEBUFFER_MAXSIZE 64*1024

/* Find the given attribute's value in the RDN of the DN */
int nssov_find_rdnval(struct berval *dn, AttributeDescription *ad, struct berval *value)
{
	struct berval rdn;
	char *next;

	BER_BVZERO(value);
	dnRdn( dn, &rdn );
	do {
		next = ber_bvchr( &rdn, '+' );
		if ( rdn.bv_val[ad->ad_cname.bv_len] == '=' &&
			!ber_bvcmp( &rdn, &ad->ad_cname )) {
			if ( next )
				rdn.bv_len = next - rdn.bv_val;
			value->bv_val = rdn.bv_val + ad->ad_cname.bv_len + 1;
			value->bv_len = rdn.bv_len - ad->ad_cname.bv_len - 1;
			break;
		}
		if ( !next )
			break;
		next++;
		rdn.bv_len -= next - rdn.bv_val;
		rdn.bv_val = next;
	} while (1);
}

/* create a search filter using a name that requires escaping */
int nssov_filter_byname(nssov_mapinfo *mi,int key,struct berval *name,struct berval *buf)
{
	char buf2[1024];
	struct berval bv2 = {sizeof(buf2),buf2};

	/* escape attribute */
	if (nssov_escape(name,&bv2))
		return -1;
	/* build filter */
	if (bv2.bv_len + mi->mi_filter.bv_len + mi->mi_attrs[key].an_desc->ad_cname.bv_len + 6 >
		buf->bv_len )
		return -1;
	buf->bv_len = snprintf(buf->bv_val, buf->bv_len, "(&%s(%s=%s))",
		mi->mi_filter.bv_val, mi->mi_attrs[key].an_desc->ad_cname.bv_val,
		bv2.bv_val );
	return 0;
}

/* create a search filter using a string converted from an int */
int nssov_filter_byid(nssov_mapinfo *mi,int key,struct berval *id,struct berval *buf)
{
	/* build filter */
	if (id->bv_len + mi->mi_filter.bv_len + mi->mi_attrs[key].an_desc->ad_cname.bv_len + 6 >
		buf->bv_len )
		return -1;
	buf->bv_len = snprintf(buf->bv_val, buf->bv_len, "(&%s(%s=%s))",
		mi->mi_filter.bv_val, mi->mi_attrs[key].an_desc->ad_cname.bv_val,
		id->bv_val );
	return 0;
}

void get_userpassword(struct berval *attr,struct berval *pw)
{
	int i;
	/* go over the entries and return the remainder of the value if it
		 starts with {crypt} or crypt$ */
	for (i=0;!BER_BVISNULL(&attr[i]);i++)
	{
		if (strncasecmp(attr[i].bv_val,"{crypt}",7)==0) {
			pw->bv_val = attr[i].bv_val + 7;
			pw->bv_len = attr[i].bv_len - 7;
			return;
		}
		if (strncasecmp(attr[i].bv_val,"crypt$",6)==0) {
			pw->bv_val = attr[i].bv_val + 6;
			pw->bv_len = attr[i].bv_len - 6;
			return;
		}
	}
	/* just return the first value completely */
	*pw = *attr;
	/* TODO: support more password formats e.g. SMD5
		(which is $1$ but in a different format)
		(any code for this is more than welcome) */
}

/* this writes a single address to the stream */
int write_address(TFILE *fp,struct berval *addr)
{
	int32_t tmpint32;
	struct in_addr ipv4addr;
	struct in6_addr ipv6addr;
	/* try to parse the address as IPv4 first, fall back to IPv6 */
	if (inet_pton(AF_INET,addr->bv_val,&ipv4addr)>0)
	{
		/* write address type */
		WRITE_INT32(fp,AF_INET);
		/* write the address length */
		WRITE_INT32(fp,sizeof(struct in_addr));
		/* write the address itself (in network byte order) */
		WRITE_TYPE(fp,ipv4addr,struct in_addr);
	}
	else if (inet_pton(AF_INET6,addr->bv_val,&ipv6addr)>0)
	{
		/* write address type */
		WRITE_INT32(fp,AF_INET6);
		/* write the address length */
		WRITE_INT32(fp,sizeof(struct in6_addr));
		/* write the address itself (in network byte order) */
		WRITE_TYPE(fp,ipv6addr,struct in6_addr);
	}
	else
	{
		/* failure, log but write simple invalid address
			 (otherwise the address list is messed up) */
		/* TODO: have error message in correct format */
		Debug(LDAP_DEBUG_ANY,"nssov: unparseable address: %s",addr->bv_val,0,0);
		/* write an illegal address type */
		WRITE_INT32(fp,-1);
		/* write an empty address */
		WRITE_INT32(fp,0);
	}
	/* we're done */
	return 0;
}

int read_address(TFILE *fp,char *addr,int *addrlen,int *af)
{
	int32_t tmpint32;
	int len;
	/* read address family */
	READ_INT32(fp,*af);
	if ((*af!=AF_INET)&&(*af!=AF_INET6))
	{
		Debug(LDAP_DEBUG_ANY,"nssov: incorrect address family specified: %d",*af,0,0);
		return -1;
	}
	/* read address length */
	READ_INT32(fp,len);
	if ((len>*addrlen)||(len<=0))
	{
		Debug(LDAP_DEBUG_ANY,"nssov: address length incorrect: %d",len,0,0);
		return -1;
	}
	*addrlen=len;
	/* read address */
	READ(fp,addr,len);
	/* we're done */
	return 0;
}

int nssov_escape(struct berval *src,struct berval *dst)
{
	size_t pos=0;
	int i;
	/* go over all characters in source string */
	for (i=0;i<src->bv_len;i++)
	{
		/* check if char will fit */
		if (pos>=(dst->bv_len-4))
			return -1;
		/* do escaping for some characters */
		switch (src->bv_val[i])
		{
			case '*':
				strcpy(dst->bv_val+pos,"\\2a");
				pos+=3;
				break;
			case '(':
				strcpy(dst->bv_val+pos,"\\28");
				pos+=3;
				break;
			case ')':
				strcpy(dst->bv_val+pos,"\\29");
				pos+=3;
				break;
			case '\\':
				strcpy(dst->bv_val+pos,"\\5c");
				pos+=3;
				break;
			default:
				/* just copy character */
				dst->bv_val[pos++]=src->bv_val[i];
				break;
		}
	}
	/* terminate destination string */
	dst->bv_val[pos]='\0';
	dst->bv_len = pos;
	return 0;
}

/* read the version information and action from the stream
   this function returns the read action in location pointer to by action */
static int read_header(TFILE *fp,int32_t *action)
{
  int32_t tmpint32;
  /* read the protocol version */
  READ_TYPE(fp,tmpint32,int32_t);
  if (tmpint32 != (int32_t)NSLCD_VERSION)
  {
    Debug( LDAP_DEBUG_TRACE,"nssov: wrong nslcd version id (%d)",(int)tmpint32,0,0);
    return -1;
  }
  /* read the request type */
  READ(fp,action,sizeof(int32_t));
  return 0;
}

/* read a request message, returns <0 in case of errors,
   this function closes the socket */
static void handleconnection(nssov_info *ni,int sock,Operation *op)
{
  TFILE *fp;
  int32_t action;
  struct timeval readtimeout,writetimeout;
  uid_t uid;
  gid_t gid;
  char authid[sizeof("gidNumber=4294967295+uidNumber=424967295,cn=peercred,cn=external,cn=auth")];

  /* log connection */
  if (lutil_getpeereid(sock,&uid,&gid))
    Debug( LDAP_DEBUG_TRACE,"nssov: connection from unknown client: %s",strerror(errno),0,0);
  else
    Debug( LDAP_DEBUG_TRACE,"nssov: connection from uid=%d gid=%d",
                      (int)uid,(int)gid,0);

  /* Should do authid mapping too */
  op->o_dn.bv_len = sprintf(authid,"gidNumber=%d+uidNumber=%d,cn=peercred,cn=external,cn=auth",
  	(int)uid, (int)gid );
  op->o_dn.bv_val = authid;
  op->o_ndn = op->o_dn;

  /* set the timeouts */
  readtimeout.tv_sec=0; /* clients should send their request quickly */
  readtimeout.tv_usec=500000;
  writetimeout.tv_sec=5; /* clients could be taking some time to process the results */
  writetimeout.tv_usec=0;
  /* create a stream object */
  if ((fp=tio_fdopen(sock,&readtimeout,&writetimeout,
                     READBUFFER_MINSIZE,READBUFFER_MAXSIZE,
                     WRITEBUFFER_MINSIZE,WRITEBUFFER_MAXSIZE))==NULL)
  {
    Debug( LDAP_DEBUG_ANY,"nssov: cannot create stream for writing: %s",strerror(errno),0,0);
    (void)close(sock);
    return;
  }
  /* read request */
  if (read_header(fp,&action))
  {
    (void)tio_close(fp);
    return;
  }
  /* handle request */
  switch (action)
  {
    case NSLCD_ACTION_ALIAS_BYNAME:     (void)nssov_alias_byname(ni,fp,op); break;
    case NSLCD_ACTION_ALIAS_ALL:        (void)nssov_alias_all(ni,fp,op); break;
    case NSLCD_ACTION_ETHER_BYNAME:     (void)nssov_ether_byname(ni,fp,op); break;
    case NSLCD_ACTION_ETHER_BYETHER:    (void)nssov_ether_byether(ni,fp,op); break;
    case NSLCD_ACTION_ETHER_ALL:        (void)nssov_ether_all(ni,fp,op); break;
    case NSLCD_ACTION_GROUP_BYNAME:     (void)nssov_group_byname(ni,fp,op); break;
    case NSLCD_ACTION_GROUP_BYGID:      (void)nssov_group_bygid(ni,fp,op); break;
    case NSLCD_ACTION_GROUP_BYMEMBER:   (void)nssov_group_bymember(ni,fp,op); break;
    case NSLCD_ACTION_GROUP_ALL:        (void)nssov_group_all(ni,fp,op); break;
    case NSLCD_ACTION_HOST_BYNAME:      (void)nssov_host_byname(ni,fp,op); break;
    case NSLCD_ACTION_HOST_BYADDR:      (void)nssov_host_byaddr(ni,fp,op); break;
    case NSLCD_ACTION_HOST_ALL:         (void)nssov_host_all(ni,fp,op); break;
    case NSLCD_ACTION_NETGROUP_BYNAME:  (void)nssov_netgroup_byname(ni,fp,op); break;
    case NSLCD_ACTION_NETWORK_BYNAME:   (void)nssov_network_byname(ni,fp,op); break;
    case NSLCD_ACTION_NETWORK_BYADDR:   (void)nssov_network_byaddr(ni,fp,op); break;
    case NSLCD_ACTION_NETWORK_ALL:      (void)nssov_network_all(ni,fp,op); break;
    case NSLCD_ACTION_PASSWD_BYNAME:    (void)nssov_passwd_byname(ni,fp,op); break;
    case NSLCD_ACTION_PASSWD_BYUID:     (void)nssov_passwd_byuid(ni,fp,op); break;
    case NSLCD_ACTION_PASSWD_ALL:       (void)nssov_passwd_all(ni,fp,op); break;
    case NSLCD_ACTION_PROTOCOL_BYNAME:  (void)nssov_protocol_byname(ni,fp,op); break;
    case NSLCD_ACTION_PROTOCOL_BYNUMBER:(void)nssov_protocol_bynumber(ni,fp,op); break;
    case NSLCD_ACTION_PROTOCOL_ALL:     (void)nssov_protocol_all(ni,fp,op); break;
    case NSLCD_ACTION_RPC_BYNAME:       (void)nssov_rpc_byname(ni,fp,op); break;
    case NSLCD_ACTION_RPC_BYNUMBER:     (void)nssov_rpc_bynumber(ni,fp,op); break;
    case NSLCD_ACTION_RPC_ALL:          (void)nssov_rpc_all(ni,fp,op); break;
    case NSLCD_ACTION_SERVICE_BYNAME:   (void)nssov_service_byname(ni,fp,op); break;
    case NSLCD_ACTION_SERVICE_BYNUMBER: (void)nssov_service_bynumber(ni,fp,op); break;
    case NSLCD_ACTION_SERVICE_ALL:      (void)nssov_service_all(ni,fp,op); break;
    case NSLCD_ACTION_SHADOW_BYNAME:    if (uid==0) (void)nssov_shadow_byname(ni,fp,op); break;
    case NSLCD_ACTION_SHADOW_ALL:       if (uid==0) (void)nssov_shadow_all(ni,fp,op); break;
    default:
      Debug( LDAP_DEBUG_ANY,"nssov: invalid request id: %d",(int)action,0,0);
      break;
  }
  /* we're done with the request */
  (void)tio_close(fp);
  return;
}

/* accept a connection on the socket */
static void *acceptconn(void *ctx, void *arg)
{
	nssov_info *ni = arg;
	Connection conn = {0};
	OperationBuffer opbuf;
	Operation *op;
	int csock;

	if ( slapd_shutdown )
		return NULL;

	{
		struct sockaddr_storage addr;
		socklen_t alen;
		int j;

		/* accept a new connection */
		alen=(socklen_t)sizeof(struct sockaddr_storage);
		csock=accept(ni->ni_socket,(struct sockaddr *)&addr,&alen);
		connection_client_enable(ni->ni_conn);
		if (csock<0)
		{
			if ((errno==EINTR)||(errno==EAGAIN)||(errno==EWOULDBLOCK))
			{
				Debug( LDAP_DEBUG_TRACE,"nssov: accept() failed (ignored): %s",strerror(errno),0,0);
				return;
			}
			Debug( LDAP_DEBUG_ANY,"nssov: accept() failed: %s",strerror(errno),0,0);
			return;
		}
		/* make sure O_NONBLOCK is not inherited */
		if ((j=fcntl(csock,F_GETFL,0))<0)
		{
			Debug( LDAP_DEBUG_ANY,"nssov: fcntl(F_GETFL) failed: %s",strerror(errno),0,0);
			if (close(csock))
				Debug( LDAP_DEBUG_ANY,"nssov: problem closing socket: %s",strerror(errno),0,0);
			return;
		}
		if (fcntl(csock,F_SETFL,j&~O_NONBLOCK)<0)
		{
			Debug( LDAP_DEBUG_ANY,"nssov: fcntl(F_SETFL,~O_NONBLOCK) failed: %s",strerror(errno),0,0);
			if (close(csock))
				Debug( LDAP_DEBUG_ANY,"nssov: problem closing socket: %s",strerror(errno),0,0);
			return;
		}
	}
	connection_fake_init( &conn, &opbuf, ctx );
	op=&opbuf.ob_op;
	op->o_bd = ni->ni_db;
	op->o_tag = LDAP_REQ_SEARCH;

	/* handle the connection */
	handleconnection(ni,csock,op);
}

static slap_verbmasks nss_svcs[] = {
	{ BER_BVC("alias"), NM_alias },
	{ BER_BVC("ether"), NM_ether },
	{ BER_BVC("group"), NM_group },
	{ BER_BVC("host"), NM_host },
	{ BER_BVC("netgroup"), NM_netgroup },
	{ BER_BVC("network"), NM_network },
	{ BER_BVC("passwd"), NM_passwd },
	{ BER_BVC("protocol"), NM_protocol },
	{ BER_BVC("rpc"), NM_rpc },
	{ BER_BVC("service"), NM_service },
	{ BER_BVC("shadow"), NM_shadow },
	{ BER_BVNULL, 0 }
};

enum {
	NSS_SSD=1,
	NSS_MAP
};

static ConfigDriver nss_cf_gen;

static ConfigTable nsscfg[] = {
	{ "nssov-ssd", "service> <url", 3, 3, 0, ARG_MAGIC|NSS_SSD,
		nss_cf_gen, "(OLcfgCtAt:3.1 NAME 'olcNssSsd' "
			"DESC 'URL for searches in a given service' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "nssov-map", "service> <orig> <new", 4, 4, 0, ARG_MAGIC|NSS_MAP,
		nss_cf_gen, "(OLcfgCtAt:3.2 NAME 'olcNssMap' "
			"DESC 'Map <service> lookups of <orig> attr to <new> attr' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ NULL, NULL, 0,0,0, ARG_IGNORED }
};

static ConfigOCs nssocs[] = {
	{ "( OLcfgCtOc:3.1 "
		"NAME 'olcNssOvConfig' "
		"DESC 'NSS lookup configuration' "
		"SUP olcOverlayConfig "
		"MAY ( olcNssSsd $ olcNssMap ) )",
		Cft_Overlay, nsscfg },
	{ NULL, 0, NULL }
};

static int
nss_cf_gen(ConfigArgs *c)
{
	slap_overinst *on = (slap_overinst *)c->bi;
	nssov_info *ni = on->on_bi.bi_private;
	nssov_mapinfo *mi;
	int i, j, rc = 0;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		switch(c->type) {
		case NSS_SSD:
			rc = 1;
			for (i=NM_alias;i<NM_NONE;i++) {
				struct berval scope;
				struct berval ssd;
				struct berval base;

				mi = &ni->ni_maps[i];

				/* ignore all-default services */
				if ( mi->mi_scope == LDAP_SCOPE_DEFAULT &&
					bvmatch( &mi->mi_filter, &mi->mi_filter0 ) &&
					BER_BVISNULL( &mi->mi_base ))
					continue;

				if ( BER_BVISNULL( &mi->mi_base ))
					base = ni->ni_db->be_nsuffix[0];
				else
					base = mi->mi_base;
				ldap_pvt_scope2bv(mi->mi_scope == LDAP_SCOPE_DEFAULT ?
					LDAP_SCOPE_SUBTREE : mi->mi_scope, &scope);
				ssd.bv_len = STRLENOF(" ldap:///???") + nss_svcs[i].word.bv_len +
					base.bv_len + scope.bv_len + mi->mi_filter.bv_len;
				ssd.bv_val = ch_malloc( ssd.bv_len + 1 );
				sprintf(ssd.bv_val, "%s ldap:///%s??%s?%s", nss_svcs[i].word.bv_val,
					base.bv_val, scope.bv_val, mi->mi_filter.bv_val );
				ber_bvarray_add( &c->rvalue_vals, &ssd );
				rc = 0;
			}
			break;
		case NSS_MAP:
			rc = 1;
			for (i=NM_alias;i<NM_NONE;i++) {
				int j;

				mi = &ni->ni_maps[i];
				for (j=0;!BER_BVISNULL(&mi->mi_attrkeys[j]);j++) {
					if ( ber_bvstrcasecmp(&mi->mi_attrkeys[j],
						&mi->mi_attrs[j].an_name)) {
						struct berval map;

						map.bv_len = nss_svcs[i].word.bv_len +
							mi->mi_attrkeys[j].bv_len +
							mi->mi_attrs->an_desc->ad_cname.bv_len + 2;
						map.bv_val = ch_malloc(map.bv_len + 1);
						sprintf(map.bv_val, "%s %s %s", nss_svcs[i].word.bv_val,
							mi->mi_attrkeys[j].bv_val, mi->mi_attrs->an_desc->ad_cname.bv_val );
						ber_bvarray_add( &c->rvalue_vals, &map );
						rc = 0;
					}
				}
			}
			break;
		}
		return rc;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		return 1;
	}
	switch( c->type ) {
	case NSS_SSD: {
		LDAPURLDesc *lud;

		i = verb_to_mask(c->argv[1], nss_svcs);
		if ( i == NM_NONE )
			return 1;

		mi = &ni->ni_maps[i];
		rc = ldap_url_parse(c->argv[2], &lud);
		if ( rc )
			return 1;
		do {
			struct berval base;
			/* Must be LDAP scheme */
			if (strcasecmp(lud->lud_scheme,"ldap")) {
				rc = 1;
				break;
			}
			/* Host part, attrs, and extensions must be empty */
			if (( lud->lud_host && *lud->lud_host ) ||
				lud->lud_attrs || lud->lud_exts ) {
				rc = 1;
				break;
			}
			ber_str2bv( lud->lud_dn,0,0,&base);
			rc = dnNormalize( 0,NULL,NULL,&base,&mi->mi_base,NULL);
			if ( rc )
				break;
			if ( lud->lud_filter ) {
				/* steal this */
				ber_str2bv( lud->lud_filter,0,0,&mi->mi_filter);
				lud->lud_filter = NULL;
			}
			mi->mi_scope = lud->lud_scope;
		} while(0);
		ldap_free_urldesc( lud );
		}
		break;
	case NSS_MAP:
		i = verb_to_mask(c->argv[1], nss_svcs);
		if ( i == NM_NONE )
			return 1;
		rc = 1;
		mi = &ni->ni_maps[i];
		for (j=0; !BER_BVISNULL(&mi->mi_attrkeys[j]); j++) {
			if (!strcasecmp(c->argv[2],mi->mi_attrkeys[j].bv_val)) {
				AttributeDescription *ad = NULL;
				const char *text;
				rc = slap_str2ad( c->argv[3], &ad, &text);
				if ( rc == 0 ) {
					mi->mi_attrs[j].an_desc = ad;
					mi->mi_attrs[j].an_name = ad->ad_cname;
				}
				break;
			}
		}
		break;
	}
	return rc;
}

static int
nssov_db_init(
	BackendDB *be,
	ConfigReply *cr )
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	nssov_info *ni;
	nssov_mapinfo *mi;
	int i, j;

	ni = ch_malloc( sizeof(nssov_info) );
	on->on_bi.bi_private = ni;

	/* set up map keys */
	nssov_alias_init(ni);
	nssov_ether_init(ni);
	nssov_group_init(ni);
	nssov_host_init(ni);
	nssov_netgroup_init(ni);
	nssov_network_init(ni);
	nssov_passwd_init(ni);
	nssov_protocol_init(ni);
	nssov_rpc_init(ni);
	nssov_service_init(ni);
	nssov_shadow_init(ni);

	ni->ni_db = be->bd_self;

	return 0;
}

static int
nssov_db_destroy(
	BackendDB *be,
	ConfigReply *cr )
{
}

static int
nssov_db_open(
	BackendDB *be,
	ConfigReply *cr )
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	nssov_info *ni = on->on_bi.bi_private;
	nssov_mapinfo *mi;

	int i, sock;
	struct sockaddr_un addr;

	/* Set default bases */
	for (i=0; i<NM_NONE; i++) {
		if ( BER_BVISNULL( &ni->ni_maps[i].mi_base )) {
			ber_dupbv( &ni->ni_maps[i].mi_base, &be->be_nsuffix[0] );
		}
		if ( ni->ni_maps[i].mi_scope == LDAP_SCOPE_DEFAULT )
			ni->ni_maps[i].mi_scope = LDAP_SCOPE_SUBTREE;
	}
	/* validate attribute maps */
	mi = ni->ni_maps;
	for ( i=0; i<NM_NONE; i++,mi++) {
		const char *text;
		int j;
		for (j=0; !BER_BVISNULL(&mi->mi_attrkeys[j]); j++) {
			/* skip attrs we already validated */
			if ( mi->mi_attrs[j].an_desc ) continue;
			if ( slap_bv2ad( &mi->mi_attrs[j].an_name,
				&mi->mi_attrs[j].an_desc, &text )) {
				Debug(LDAP_DEBUG_ANY,"nssov: invalid attr \"%s\": %s\n",
					mi->mi_attrs[j].an_name.bv_val, text, 0 );
				return -1;
			}
		}
		BER_BVZERO(&mi->mi_attrs[j].an_name);
		mi->mi_attrs[j].an_desc = NULL;
	}

	if ( slapMode & SLAP_SERVER_MODE ) {
		/* create a socket */
		if ( (sock=socket(PF_UNIX,SOCK_STREAM,0))<0 )
		{
			Debug(LDAP_DEBUG_ANY,"nssov: cannot create socket: %s",strerror(errno),0,0);
			return -1;
		}
		/* remove existing named socket */
		if (unlink(NSLCD_SOCKET)<0)
		{
			Debug( LDAP_DEBUG_TRACE,"nssov: unlink() of "NSLCD_SOCKET" failed (ignored): %s",
							strerror(errno),0,0);
		}
		/* create socket address structure */
		memset(&addr,0,sizeof(struct sockaddr_un));
		addr.sun_family=AF_UNIX;
		strncpy(addr.sun_path,NSLCD_SOCKET,sizeof(addr.sun_path));
		addr.sun_path[sizeof(addr.sun_path)-1]='\0';
		/* bind to the named socket */
		if (bind(sock,(struct sockaddr *)&addr,sizeof(struct sockaddr_un)))
		{
			Debug( LDAP_DEBUG_ANY,"nssov: bind() to "NSLCD_SOCKET" failed: %s",
							strerror(errno),0,0);
			if (close(sock))
				Debug( LDAP_DEBUG_ANY,"nssov: problem closing socket: %s",strerror(errno),0,0);
			return -1;
		}
		/* close the file descriptor on exit */
		if (fcntl(sock,F_SETFD,FD_CLOEXEC)<0)
		{
			Debug( LDAP_DEBUG_ANY,"nssov: fcntl(F_SETFL,O_NONBLOCK) failed: %s",strerror(errno),0,0);
			if (close(sock))
				Debug( LDAP_DEBUG_ANY,"nssov: problem closing socket: %s",strerror(errno),0,0);
			return -1;
		}
		/* set permissions of socket so anybody can do requests */
		/* Note: we use chmod() here instead of fchmod() because
			 fchmod does not work on sockets
			 http://www.opengroup.org/onlinepubs/009695399/functions/fchmod.html
			 http://lkml.org/lkml/2005/5/16/11 */
		if (chmod(NSLCD_SOCKET,(mode_t)0666))
		{
			Debug( LDAP_DEBUG_ANY,"nssov: chmod(0666) failed: %s",strerror(errno),0,0);
			if (close(sock))
				Debug( LDAP_DEBUG_ANY,"nssov: problem closing socket: %s",strerror(errno),0,0);
			return -1;
		}
		/* start listening for connections */
		if (listen(sock,SOMAXCONN)<0)
		{
			Debug( LDAP_DEBUG_ANY,"nssov: listen() failed: %s",strerror(errno),0,0);
			if (close(sock))
				Debug( LDAP_DEBUG_ANY,"nssov: problem closing socket: %s",strerror(errno),0,0);
			return -1;
		}
		ni->ni_socket = sock;
		ni->ni_conn = connection_client_setup( sock, acceptconn, ni );
	}

	return 0;
}

static int
nssov_db_close(
	BackendDB *be,
	ConfigReply *cr )
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	nssov_info *ni = on->on_bi.bi_private;

	/* close socket if it's still in use */
	if (ni->ni_socket >= 0);
	{
		if (close(ni->ni_socket))
			Debug( LDAP_DEBUG_ANY,"problem closing server socket (ignored): %s",strerror(errno),0,0);
		ni->ni_socket = -1;
	}
	/* remove existing named socket */
	if (unlink(NSLCD_SOCKET)<0)
	{
		Debug( LDAP_DEBUG_TRACE,"unlink() of "NSLCD_SOCKET" failed (ignored): %s",
			strerror(errno),0,0);
	}
}

static slap_overinst nssov;

int
nssov_initialize( void )
{
	int rc;

	nssov.on_bi.bi_type = "nssov";
	nssov.on_bi.bi_db_init = nssov_db_init;
	nssov.on_bi.bi_db_destroy = nssov_db_destroy;
	nssov.on_bi.bi_db_open = nssov_db_open;
	nssov.on_bi.bi_db_close = nssov_db_close;

	nssov.on_bi.bi_cf_ocs = nssocs;

	rc = config_register_schema( nsscfg, nssocs );
	if ( rc ) return rc;

	return overlay_register(&nssov);
}

#if SLAPD_OVER_NSSOV == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return nssov_initialize();
}
#endif
