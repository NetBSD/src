/*
   cfg.h - definition of configuration information
   This file contains parts that were part of the nss_ldap
   library which has been forked into the nss-ldapd library.

   Copyright (C) 1997-2005 Luke Howard
   Copyright (C) 2007 West Consulting
   Copyright (C) 2007, 2008 Arthur de Jong

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA
*/

#ifndef _CFG_H
#define _CFG_H

#include "compat/attrs.h"

/* maximum number of URIs */
#define NSS_LDAP_CONFIG_URI_MAX         31

enum ldap_ssl_options
{
  SSL_OFF,
  SSL_LDAPS,
  SSL_START_TLS
};

/* selectors for different maps */
enum ldap_map_selector
{
  LM_PASSWD,
  LM_SHADOW,
  LM_GROUP,
  LM_HOSTS,
  LM_SERVICES,
  LM_NETWORKS,
  LM_PROTOCOLS,
  LM_RPC,
  LM_ETHERS,
  LM_ALIASES,
  LM_NETGROUP,
  LM_NONE
};

struct myldap_uri
{
  char *uri;
  /* timestamp of last ok search */
  time_t lastok;
  /* timestamp of last ok search */
  time_t lastfail;
};

struct ldap_config
{
  /* the number of threads to start */
  int ldc_threads;
  /* NULL terminated list of URIs */
  struct myldap_uri ldc_uris[NSS_LDAP_CONFIG_URI_MAX+1];
  /* protocol version */
  int ldc_version;
  /* bind DN */
  char *ldc_binddn;
  /* bind cred */
  char *ldc_bindpw;
  /* sasl auth id */
  char *ldc_saslid;
  /* sasl security */
  char *ldc_sasl_secprops;
  /* do we use sasl when binding? */
  int ldc_usesasl;
  /* base DN, eg. dc=gnu,dc=org */
  char *ldc_base;
  /* scope for searches */
  int ldc_scope;
  /* dereference aliases/links */
  int ldc_deref;
  /* chase referrals */
  int ldc_referrals;
  /* bind timelimit */
  int ldc_bind_timelimit;
  /* search timelimit */
  int ldc_timelimit;
  /* idle timeout */
  int ldc_idle_timelimit;
  /* seconds to sleep; doubled until max */
  int ldc_reconnect_sleeptime;
  /* maximum seconds to sleep */
  int ldc_reconnect_maxsleeptime;
#ifdef LDAP_OPT_X_TLS
  /* SSL enabled */
  enum ldap_ssl_options ldc_ssl_on;
  /* SSL certificate path */
  char *ldc_sslpath;
  /* tls check peer */
  int ldc_tls_checkpeer;
  /* tls ca certificate dir */
  char *ldc_tls_cacertdir;
  /* tls ca certificate file */
  char *ldc_tls_cacertfile;
  /* tls randfile */
  char *ldc_tls_randfile;
  /* tls ciphersuite */
  char *ldc_tls_ciphers;
  /* tls certificate */
  char *ldc_tls_cert;
  /* tls key */
  char *ldc_tls_key;
#endif /* LDAP_OPT_X_TLS */
  /* whether the LDAP library should restart the select(2) system call when interrupted */
  int ldc_restart;
  /* set to a greater than 0 to enable handling of paged results with the specified size */
  int ldc_pagesize;
  /* LDAP debug level */
  int ldc_debug;
};

/* this is a pointer to the global configuration, it should be available
   once cfg_init() was called */
extern struct ldap_config *nslcd_cfg;

/* Initialize the configuration in nslcd_cfg. This method
   will read the default configuration file and call exit()
   if an error occurs. */
void cfg_init(const char *fname);

#endif /* _CFG_H */
