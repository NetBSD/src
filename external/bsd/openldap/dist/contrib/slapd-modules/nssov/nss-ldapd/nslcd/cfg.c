/*
   cfg.c - functions for configuration information
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#ifdef HAVE_GSSAPI_H
#include <gssapi.h>
#endif /* HAVE_GSSAPI_H */
#ifdef HAVE_GSSAPI_GSSAPI_KRB5_H
#include <gssapi/gssapi.h>
#include <gssapi/gssapi_krb5.h>
#endif /* HAVE_GSSAPI_GSSAPI_KRB5_H */

#include "common.h"
#include "log.h"
#include "cfg.h"
#include "attmap.h"

struct ldap_config *nslcd_cfg=NULL;

/* the maximum line length in the configuration file */
#define MAX_LINE_LENGTH          4096

/* the delimiters of tokens */
#define TOKEN_DELIM " \t\n\r"

/* set the configuration information to the defaults */
static void cfg_defaults(struct ldap_config *cfg)
{
  int i;
  memset(cfg,0,sizeof(struct ldap_config));
  cfg->ldc_threads=5;
  for (i=0;i<(NSS_LDAP_CONFIG_URI_MAX+1);i++)
  {
    cfg->ldc_uris[i].uri=NULL;
    cfg->ldc_uris[i].lastok=0;
    cfg->ldc_uris[i].lastfail=0;
  }
#ifdef LDAP_VERSION3
  cfg->ldc_version=LDAP_VERSION3;
#else /* LDAP_VERSION3 */
  cfg->ldc_version=LDAP_VERSION2;
#endif /* not LDAP_VERSION3 */
  cfg->ldc_binddn=NULL;
  cfg->ldc_bindpw=NULL;
  cfg->ldc_saslid=NULL;
  cfg->ldc_sasl_secprops=NULL;
  cfg->ldc_usesasl=0;
  cfg->ldc_base=NULL;
  cfg->ldc_scope=LDAP_SCOPE_SUBTREE;
  cfg->ldc_deref=LDAP_DEREF_NEVER;
  cfg->ldc_referrals=1;
  cfg->ldc_bind_timelimit=30;
  cfg->ldc_timelimit=LDAP_NO_LIMIT;
  cfg->ldc_idle_timelimit=0;
  cfg->ldc_reconnect_sleeptime=1;
  cfg->ldc_reconnect_maxsleeptime=30;
#ifdef LDAP_OPT_X_TLS
  cfg->ldc_ssl_on=SSL_OFF;
  cfg->ldc_sslpath=NULL;
  cfg->ldc_tls_checkpeer=-1;
  cfg->ldc_tls_cacertdir=NULL;
  cfg->ldc_tls_cacertfile=NULL;
  cfg->ldc_tls_randfile=NULL;
  cfg->ldc_tls_ciphers=NULL;
  cfg->ldc_tls_cert=NULL;
  cfg->ldc_tls_key=NULL;
#endif /* LDAP_OPT_X_TLS */
  cfg->ldc_restart=1;
  cfg->ldc_pagesize=0;
  cfg->ldc_debug=0;
}

/* simple strdup wrapper */
static char *xstrdup(const char *s)
{
  char *tmp;
  if (s==NULL)
  {
    log_log(LOG_CRIT,"xstrdup() called with NULL");
    exit(EXIT_FAILURE);
  }
  tmp=strdup(s);
  if (tmp==NULL)
  {
    log_log(LOG_CRIT,"strdup() failed to allocate memory");
    exit(EXIT_FAILURE);
  }
  return tmp;
}

/* add a single URI to the list of URIs in the configuration */
static void add_uri(const char *filename,int lnr,
                    struct ldap_config *cfg,const char *uri)
{
  int i;
  log_log(LOG_DEBUG,"add_uri(%s)",uri);
  /* find the place where to insert the URI */
  for (i=0;cfg->ldc_uris[i].uri!=NULL;i++)
    ;
  /* check for room */
  if (i>=NSS_LDAP_CONFIG_URI_MAX)
  {
    log_log(LOG_ERR,"%s:%d: maximum number of URIs exceeded",filename,lnr);
    exit(EXIT_FAILURE);
  }
  /* append URI to list */
  cfg->ldc_uris[i].uri=xstrdup(uri);
}

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif /* not HOST_NAME_MAX */

#ifdef HAVE_LDAP_DOMAIN2HOSTLIST
/* return the domain name of the current host
   we return part of the structure that is retured by gethostbyname()
   so there should be no need to free() this entry, however we should
   use the value before any other call to gethostbyname() */
static const char *cfg_getdomainname(const char *filename,int lnr)
{
  char hostname[HOST_NAME_MAX],*domain;
  struct hostent *host;
  /* lookup the hostname and with that the fqdn to extract the domain */
  if (gethostname(hostname,sizeof(hostname))<0)
  {
    log_log(LOG_ERR,"%s:%d: gethostname(): %s",filename,lnr,strerror(errno));
    exit(EXIT_FAILURE);
  }
  if ((host=gethostbyname(hostname))==NULL)
  {
    log_log(LOG_ERR,"%s:%d: gethostbyname(%s): %s",filename,lnr,hostname,hstrerror(h_errno));
    exit(EXIT_FAILURE);
  }
  /* TODO: this may fail if the fqdn is in h_aliases */
  if ((domain=strchr(host->h_name,'.'))==NULL)
  {
    log_log(LOG_ERR,"%s:%d: host name %s is not in fqdn form",filename,lnr,host->h_name);
    exit(EXIT_FAILURE);
  }
  /* we're done */
  return domain+1;
}

/* add URIs by doing DNS queries for SRV records */
static void add_uris_from_dns(const char *filename,int lnr,
                              struct ldap_config *cfg)
{
  int ret=0;
  const char *domain;
  char *hostlist=NULL,*nxt;
  char buf[HOST_NAME_MAX+sizeof("ldap://")];
  domain=cfg_getdomainname(filename,lnr);
  ret=ldap_domain2hostlist(domain,&hostlist);
  /* FIXME: have better error handling */
  if ((hostlist==NULL)||(*hostlist=='\0'))
  {
    log_log(LOG_ERR,"%s:%d: no servers found in DNS zone %s",filename,lnr,domain);
    exit(EXIT_FAILURE);
  }
  /* hostlist is a space-separated list of host names that we use to build
     URIs */
  while(hostlist!=NULL)
  {
    /* find the next space and split the string there */
    nxt=strchr(hostlist,' ');
    if (nxt!=NULL)
    {
      *nxt='\0';
      nxt++;
    }
    /* add the URI */
    mysnprintf(buf,sizeof(buf),"ldap://%s",hostlist);
    log_log(LOG_DEBUG,"add_uris_from_dns(): found uri: %s",buf);
    add_uri(filename,lnr,cfg,buf);
    /* get next entry from list */
    hostlist=nxt;
  }
}
#endif /* HAVE_LDAP_DOMAIN2HOSTLIST */

static int parse_boolean(const char *filename,int lnr,const char *value)
{
  if ( (strcasecmp(value,"on")==0) ||
       (strcasecmp(value,"yes")==0) ||
       (strcasecmp(value,"true")==0) ||
       (strcasecmp(value,"1")==0) )
    return 1;
  else if ( (strcasecmp(value,"off")==0) ||
            (strcasecmp(value,"no")==0) ||
            (strcasecmp(value,"false")==0) ||
            (strcasecmp(value,"0")==0) )
    return 0;
  else
  {
    log_log(LOG_ERR,"%s:%d: not a boolean argument: '%s'",filename,lnr,value);
    exit(EXIT_FAILURE);
  }
}

static int parse_scope(const char *filename,int lnr,const char *value)
{
  if ( (strcasecmp(value,"sub")==0) || (strcasecmp(value,"subtree")==0) )
    return LDAP_SCOPE_SUBTREE;
  else if ( (strcasecmp(value,"one")==0) || (strcasecmp(value,"onelevel")==0) )
    return LDAP_SCOPE_ONELEVEL;
  else if (strcasecmp(value,"base")==0)
    return LDAP_SCOPE_BASE;
  else
  {
    log_log(LOG_ERR,"%s:%d: not a scope argument: '%s'",filename,lnr,value);
    exit(EXIT_FAILURE);
  }
}

/* This function works like strtok() except that the original string is
   not modified and a pointer within str to where the next token begins
   is returned (this can be used to pass to the function on the next
   iteration). If no more tokens are found or the token will not fit in
   the buffer, NULL is returned. */
static char *get_token(char **line,char *buf,size_t buflen)
{
  size_t len;
  if ((line==NULL)||(*line==NULL)||(**line=='\0')||(buf==NULL))
    return NULL;
  /* find the beginning and length of the token */
  *line+=strspn(*line,TOKEN_DELIM);
  len=strcspn(*line,TOKEN_DELIM);
  /* check if there is a token */
  if (len==0)
  {
    *line=NULL;
    return NULL;
  }
  /* limit the token length */
  if (len>=buflen)
    len=buflen-1;
  /* copy the token */
  strncpy(buf,*line,len);
  buf[len]='\0';
  /* skip to the next token */
  *line+=len;
  *line+=strspn(*line,TOKEN_DELIM);
  /* return the token */
  return buf;
}

static enum ldap_map_selector parse_map(const char *value)
{
  if ( (strcasecmp(value,"alias")==0) || (strcasecmp(value,"aliases")==0) )
    return LM_ALIASES;
  else if ( (strcasecmp(value,"ether")==0) || (strcasecmp(value,"ethers")==0) )
    return LM_ETHERS;
  else if (strcasecmp(value,"group")==0)
    return LM_GROUP;
  else if ( (strcasecmp(value,"host")==0) || (strcasecmp(value,"hosts")==0) )
    return LM_HOSTS;
  else if (strcasecmp(value,"netgroup")==0)
    return LM_NETGROUP;
  else if ( (strcasecmp(value,"network")==0) || (strcasecmp(value,"networks")==0) )
    return LM_NETWORKS;
  else if (strcasecmp(value,"passwd")==0)
    return LM_PASSWD;
  else if ( (strcasecmp(value,"protocol")==0) || (strcasecmp(value,"protocols")==0) )
    return LM_PROTOCOLS;
  else if (strcasecmp(value,"rpc")==0)
    return LM_RPC;
  else if ( (strcasecmp(value,"service")==0) || (strcasecmp(value,"services")==0) )
    return LM_SERVICES;
  else if (strcasecmp(value,"shadow")==0)
    return LM_SHADOW;
  else
    return LM_NONE;
}

/* check to see if the line begins with a named map */
static enum ldap_map_selector get_map(char **line)
{
  char token[32];
  char *old;
  enum ldap_map_selector map;
  /* get the token */
  old=*line;
  if (get_token(line,token,sizeof(token))==NULL)
    return LM_NONE;
  /* find the map if any */
  map=parse_map(token);
  /* unknown map, return to the previous state */
  if (map==LM_NONE)
    *line=old;
  return map;
}

/* check that the condition is true and otherwise log an error
   and bail out */
static inline void check_argumentcount(const char *filename,int lnr,
                                       const char *keyword,int condition)
{
  if (!condition)
  {
    log_log(LOG_ERR,"%s:%d: %s: wrong number of arguments",filename,lnr,keyword);
    exit(EXIT_FAILURE);
  }
}

static void get_int(const char *filename,int lnr,
                    const char *keyword,char **line,
                    int *var)
{
  /* TODO: refactor to have less overhead */
  char token[32];
  check_argumentcount(filename,lnr,keyword,get_token(line,token,sizeof(token))!=NULL);
  /* TODO: replace with correct numeric parse */
  *var=atoi(token);
}

static void get_boolean(const char *filename,int lnr,
                        const char *keyword,char **line,
                        int *var)
{
  /* TODO: refactor to have less overhead */
  char token[32];
  check_argumentcount(filename,lnr,keyword,get_token(line,token,sizeof(token))!=NULL);
  *var=parse_boolean(filename,lnr,token);
}

static void get_strdup(const char *filename,int lnr,
                       const char *keyword,char **line,
                       char **var)
{
  /* TODO: refactor to have less overhead */
  char token[64];
  check_argumentcount(filename,lnr,keyword,get_token(line,token,sizeof(token))!=NULL);
  if ((*var==NULL)||(strcmp(*var,token)!=0))
  {
    /* Note: we have a memory leak here if a single mapping is changed
             multiple times in one config (deemed not a problem) */
    *var=xstrdup(token);
  }
}

static void get_restdup(const char *filename,int lnr,
                        const char *keyword,char **line,
                        char **var)
{
  check_argumentcount(filename,lnr,keyword,(*line!=NULL)&&(**line!='\0'));
  if ((*var==NULL)||(strcmp(*var,*line)!=0))
  {
    /* Note: we have a memory leak here if a single mapping is changed
             multiple times in one config (deemed not a problem) */
    *var=xstrdup(*line);
  }
  *line=NULL;
}

static void get_eol(const char *filename,int lnr,
                    const char *keyword,char **line)
{
  if ((line!=NULL)&&(*line!=NULL)&&(**line!='\0'))
  {
    log_log(LOG_ERR,"%s:%d: %s: too may arguments",filename,lnr,keyword);
    exit(EXIT_FAILURE);
  }
}

static void parse_krb5_ccname_statement(const char *filename,int lnr,
                                        const char *keyword,char *line)
{
  char token[80];
  const char *ccname;
  const char *ccfile;
  size_t ccenvlen;
  char *ccenv;
#ifdef HAVE_GSS_KRB5_CCACHE_NAME
  OM_uint32 minor_status;
# endif /* HAVE_GSS_KRB5_CCACHE_NAME */
  /* get token */
  check_argumentcount(filename,lnr,keyword,
      (get_token(&line,token,sizeof(token))!=NULL)&&(*line=='\0'));
  /* set default kerberos ticket cache for SASL-GSSAPI */
  ccname=token;
  /* check that cache exists and is readable if it is a file */
  if ( (strncasecmp(ccname,"FILE:",sizeof("FILE:")-1)==0) ||
       (strncasecmp(ccname,"WRFILE:",sizeof("WRFILE:")-1)==0))
  {
    ccfile=strchr(ccname,':')+1;
    if (access(ccfile,R_OK)!=0)
    {
      log_log(LOG_ERR,"%s:%d: error accessing %s: %s",filename,lnr,ccfile,strerror(errno));
      exit(EXIT_FAILURE);
    }
  }
  /* set the environment variable (we have a memory leak if this option
     is set multiple times) */
  ccenvlen=strlen(ccname)+sizeof("KRB5CCNAME=");
  ccenv=(char *)malloc(ccenvlen);
  if (ccenv==NULL)
  {
    log_log(LOG_CRIT,"malloc() failed to allocate memory");
    exit(EXIT_FAILURE);
  }
  mysnprintf(ccenv,ccenvlen,"KRB5CCNAME=%s",ccname);
  putenv(ccenv);
#ifdef HAVE_GSS_KRB5_CCACHE_NAME
  /* set the name with gss_krb5_ccache_name() */
  if (gss_krb5_ccache_name(&minor_status,ccname,NULL)!=GSS_S_COMPLETE)
  {
    log_log(LOG_ERR,"%s:%d: unable to set default credential cache: %s",filename,lnr,ccname);
    exit(EXIT_FAILURE);
  }
# endif /* HAVE_GSS_KRB5_CCACHE_NAME */
}

/* assigns the base to the specified variable doing domain expansion
   and a simple check to avoid overwriting duplicate values */
static void set_base(const char *filename,int lnr,
                     const char *value,const char **var)
{
#ifdef HAVE_LDAP_DOMAIN2DN
  char *domaindn=NULL;
#endif /* HAVE_LDAP_DOMAIN2DN */
  /* if the base is "DOMAIN" use the domain name */
  if (strcasecmp(value,"domain")==0)
  {
#ifdef HAVE_LDAP_DOMAIN2DN
    ldap_domain2dn(cfg_getdomainname(filename,lnr),&domaindn);
    log_log(LOG_DEBUG,"set_base(): setting base to %s from domain",domaindn);
    value=domaindn;
#else /* not HAVE_LDAP_DOMAIN2DN */
    log_log(LOG_ERR,"%s:%d: value %s not supported on platform",filename,lnr,value);
    exit(EXIT_FAILURE);
#endif /* not HAVE_LDAP_DOMAIN2DN */
  }
  /* check if the value will be changed */
  if ((*var==NULL)||(strcmp(*var,value)!=0))
  {
    /* Note: we have a memory leak here if a single mapping is changed
             multiple times in one config (deemed not a problem) */
    *var=xstrdup(value);
  }
}

static void parse_base_statement(const char *filename,int lnr,
                                 const char *keyword,char *line,
                                 struct ldap_config *cfg)
{
  const char **var;
  var=base_get_var(get_map(&line));
  if (var==NULL)
    var=(const char **)&(cfg->ldc_base);
  check_argumentcount(filename,lnr,keyword,(line!=NULL)&&(*line!='\0'));
  set_base(filename,lnr,line,var);
}

static void parse_scope_statement(const char *filename,int lnr,
                                  const char *keyword,char *line,
                                  struct ldap_config *cfg)
{
  int *var;
  var=scope_get_var(get_map(&line));
  if (var==NULL)
    var=&cfg->ldc_scope;
  check_argumentcount(filename,lnr,keyword,(line!=NULL)&&(*line!='\0'));
  *var=parse_scope(filename,lnr,line);
}

static void parse_filter_statement(const char *filename,int lnr,
                                   const char *keyword,char *line)
{
  const char **var;
  const char *map=line;
  var=filter_get_var(get_map(&line));
  if (var==NULL)
  {
    log_log(LOG_ERR,"%s:%d: unknown map: '%s'",filename,lnr,map);
    exit(EXIT_FAILURE);
  }
  check_argumentcount(filename,lnr,keyword,(line!=NULL)&&(*line!='\0'));
  /* check if the value will be changed */
  if (strcmp(*var,line)!=0)
  {
    /* Note: we have a memory leak here if a single mapping is changed
             multiple times in one config (deemed not a problem) */
    *var=xstrdup(line);
  }
}

/* this function modifies the statement argument passed */
static void parse_map_statement(const char *filename,int lnr,
                                const char *keyword,char *line)
{
  enum ldap_map_selector map;
  const char **var;
  char oldatt[32], newatt[32];
  /* get the map */
  if ((map=get_map(&line))==LM_NONE)
  {
    log_log(LOG_ERR,"%s:%d: unknown map: '%s'",filename,lnr,line);
    exit(EXIT_FAILURE);
  }
  /* read the other tokens */
  check_argumentcount(filename,lnr,keyword,
      (get_token(&line,oldatt,sizeof(oldatt))!=NULL)&&
      (get_token(&line,newatt,sizeof(newatt))!=NULL));
  /* check that there are no more tokens left on the line */
  get_eol(filename,lnr,keyword,&line);
  /* get the attribute variable to set */
  var=attmap_get_var(map,oldatt);
  if (var==NULL)
  {
    log_log(LOG_ERR,"%s:%d: unknown attribute to map: '%s'",filename,lnr,oldatt);
    exit(EXIT_FAILURE);
  }
  /* check if the value will be changed */
  if ( (*var==NULL) || (strcmp(*var,newatt)!=0) )
  {
    /* Note: we have a memory leak here if a single mapping is changed
             multiple times in one config (deemed not a problem) */
    *var=xstrdup(newatt);
  }
}

static void cfg_read(const char *filename,struct ldap_config *cfg)
{
  FILE *fp;
  int lnr=0;
  char linebuf[MAX_LINE_LENGTH];
  char *line;
  char keyword[32];
  char token[64];
  int i;
  /* open config file */
  if ((fp=fopen(filename,"r"))==NULL)
  {
    log_log(LOG_ERR,"cannot open config file (%s): %s",filename,strerror(errno));
    exit(EXIT_FAILURE);
  }
  /* read file and parse lines */
  while (fgets(linebuf,MAX_LINE_LENGTH,fp)!=NULL)
  {
    lnr++;
    line=linebuf;
    /* strip newline */
    i=(int)strlen(line);
    if ((i<=0)||(line[i-1]!='\n'))
    {
      log_log(LOG_ERR,"%s:%d: line too long or last line missing newline",filename,lnr);
      exit(EXIT_FAILURE);
    }
    line[i-1]='\0';
    /* ignore comment lines */
    if (line[0]=='#')
      continue;
    /* strip trailing spaces */
    for (i--;(i>0)&&isspace(line[i-1]);i--)
      line[i-1]='\0';
    /* get keyword from line and ignore empty lines */
    if (get_token(&line,keyword,sizeof(keyword))==NULL)
      continue;
    /* runtime options */
    if (strcasecmp(keyword,"threads")==0)
    {
      get_int(filename,lnr,keyword,&line,&cfg->ldc_threads);
      get_eol(filename,lnr,keyword,&line);
    }
    /* general connection options */
    else if (strcasecmp(keyword,"uri")==0)
    {
      check_argumentcount(filename,lnr,keyword,(line!=NULL)&&(*line!='\0'));
      while (get_token(&line,token,sizeof(token))!=NULL)
      {
        if (strcasecmp(token,"dns")==0)
        {
#ifdef HAVE_LDAP_DOMAIN2HOSTLIST
          add_uris_from_dns(filename,lnr,cfg);
#else /* not HAVE_LDAP_DOMAIN2HOSTLIST */
          log_log(LOG_ERR,"%s:%d: value %s not supported on platform",filename,lnr,token);
          exit(EXIT_FAILURE);
#endif /* not HAVE_LDAP_DOMAIN2HOSTLIST */
        }
        else
          add_uri(filename,lnr,cfg,token);
      }
    }
    else if (strcasecmp(keyword,"ldap_version")==0)
    {
      get_int(filename,lnr,keyword,&line,&cfg->ldc_version);
      get_eol(filename,lnr,keyword,&line);
    }
    else if (strcasecmp(keyword,"binddn")==0)
    {
      get_restdup(filename,lnr,keyword,&line,&cfg->ldc_binddn);
    }
    else if (strcasecmp(keyword,"bindpw")==0)
    {
      get_restdup(filename,lnr,keyword,&line,&cfg->ldc_bindpw);
    }
    /* SASL authentication options */
    else if (strcasecmp(keyword,"sasl_authid")==0)
    {
      log_log(LOG_WARNING,"%s:%d: option %s is currently untested (please report any successes)",filename,lnr,keyword);
      get_strdup(filename,lnr,keyword,&line,&cfg->ldc_saslid);
      get_eol(filename,lnr,keyword,&line);
    }
    else if (strcasecmp(keyword,"sasl_secprops")==0)
    {
      log_log(LOG_WARNING,"%s:%d: option %s is currently untested (please report any successes)",filename,lnr,keyword);
      get_strdup(filename,lnr,keyword,&line,&cfg->ldc_sasl_secprops);
      get_eol(filename,lnr,keyword,&line);
    }
    else if (strcasecmp(keyword,"use_sasl")==0)
    {
      log_log(LOG_WARNING,"%s:%d: option %s is currently untested (please report any successes)",filename,lnr,keyword);
      get_boolean(filename,lnr,keyword,&line,&cfg->ldc_usesasl);
      get_eol(filename,lnr,keyword,&line);
    }
    /* Kerberos authentication options */
    else if (strcasecmp(keyword,"krb5_ccname")==0)
    {
      parse_krb5_ccname_statement(filename,lnr,keyword,line);
    }
    /* search/mapping options */
    else if (strcasecmp(keyword,"base")==0)
    {
      parse_base_statement(filename,lnr,keyword,line,cfg);
    }
    else if (strcasecmp(keyword,"scope")==0)
    {
      parse_scope_statement(filename,lnr,keyword,line,cfg);
    }
    else if (strcasecmp(keyword,"deref")==0)
    {
      check_argumentcount(filename,lnr,keyword,
          (get_token(&line,token,sizeof(token))!=NULL));
      if (strcasecmp(token,"never")==0)
        cfg->ldc_deref=LDAP_DEREF_NEVER;
      else if (strcasecmp(token,"searching")==0)
        cfg->ldc_deref=LDAP_DEREF_SEARCHING;
      else if (strcasecmp(token,"finding")==0)
        cfg->ldc_deref=LDAP_DEREF_FINDING;
      else if (strcasecmp(token,"always")==0)
        cfg->ldc_deref=LDAP_DEREF_ALWAYS;
      else
      {
        log_log(LOG_ERR,"%s:%d: wrong argument: '%s'",filename,lnr,token);
        exit(EXIT_FAILURE);
      }
      get_eol(filename,lnr,keyword,&line);
    }
    else if (strcasecmp(keyword,"referrals")==0)
    {
      get_boolean(filename,lnr,keyword,&line,&cfg->ldc_referrals);
      get_eol(filename,lnr,keyword,&line);
    }
    else if (strcasecmp(keyword,"filter")==0)
    {
      parse_filter_statement(filename,lnr,keyword,line);
    }
    else if (strcasecmp(keyword,"map")==0)
    {
      parse_map_statement(filename,lnr,keyword,line);
    }
    /* timing/reconnect options */
    else if (strcasecmp(keyword,"bind_timelimit")==0)
    {
      get_int(filename,lnr,keyword,&line,&cfg->ldc_bind_timelimit);
      get_eol(filename,lnr,keyword,&line);
    }
    else if (strcasecmp(keyword,"timelimit")==0)
    {
      get_int(filename,lnr,keyword,&line,&cfg->ldc_timelimit);
      get_eol(filename,lnr,keyword,&line);
    }
    else if (strcasecmp(keyword,"idle_timelimit")==0)
    {
      get_int(filename,lnr,keyword,&line,&cfg->ldc_idle_timelimit);
      get_eol(filename,lnr,keyword,&line);
    }
    else if (strcasecmp(keyword,"reconnect_tries")==0)
      log_log(LOG_WARNING,"%s:%d: option %s has been removed and will be ignored",filename,lnr,keyword);
    else if (!strcasecmp(keyword,"reconnect_sleeptime"))
    {
      get_int(filename,lnr,keyword,&line,&cfg->ldc_reconnect_sleeptime);
      get_eol(filename,lnr,keyword,&line);
    }
    else if (strcasecmp(keyword,"reconnect_maxsleeptime")==0)
    {
      get_int(filename,lnr,keyword,&line,&cfg->ldc_reconnect_maxsleeptime);
      get_eol(filename,lnr,keyword,&line);
    }
#ifdef LDAP_OPT_X_TLS
    /* SSL/TLS options */
    else if (strcasecmp(keyword,"ssl")==0)
    {
      log_log(LOG_WARNING,"%s:%d: option %s is currently untested (please report any successes)",filename,lnr,keyword);
      check_argumentcount(filename,lnr,keyword,
          (get_token(&line,token,sizeof(token))!=NULL));
      if (strcasecmp(token,"start_tls")==0)
        cfg->ldc_ssl_on=SSL_START_TLS;
      else if (parse_boolean(filename,lnr,token))
        cfg->ldc_ssl_on=SSL_LDAPS;
      get_eol(filename,lnr,keyword,&line);
    }
    else if (strcasecmp(keyword,"sslpath")==0)
    {
      log_log(LOG_WARNING,"%s:%d: option %s is currently untested (please report any successes)",filename,lnr,keyword);
      get_strdup(filename,lnr,keyword,&line,&cfg->ldc_sslpath);
      get_eol(filename,lnr,keyword,&line);
      /* TODO: check that the path is valid */
    }
    else if (strcasecmp(keyword,"tls_checkpeer")==0)
    {
      log_log(LOG_WARNING,"%s:%d: option %s is currently untested (please report any successes)",filename,lnr,keyword);
      get_boolean(filename,lnr,keyword,&line,&cfg->ldc_tls_checkpeer);
      get_eol(filename,lnr,keyword,&line);
    }
    else if (strcasecmp(keyword,"tls_cacertdir")==0)
    {
      log_log(LOG_WARNING,"%s:%d: option %s is currently untested (please report any successes)",filename,lnr,keyword);
      get_strdup(filename,lnr,keyword,&line,&cfg->ldc_tls_cacertdir);
      get_eol(filename,lnr,keyword,&line);
      /* TODO: check that the path is valid */
    }
    else if (strcasecmp(keyword,"tls_cacertfile")==0)
    {
      log_log(LOG_WARNING,"%s:%d: option %s is currently untested (please report any successes)",filename,lnr,keyword);
      get_strdup(filename,lnr,keyword,&line,&cfg->ldc_tls_cacertfile);
      get_eol(filename,lnr,keyword,&line);
      /* TODO: check that the path is valid */
    }
    else if (strcasecmp(keyword,"tls_randfile")==0)
    {
      log_log(LOG_WARNING,"%s:%d: option %s is currently untested (please report any successes)",filename,lnr,keyword);
      get_strdup(filename,lnr,keyword,&line,&cfg->ldc_tls_randfile);
      get_eol(filename,lnr,keyword,&line);
      /* TODO: check that the path is valid */
    }
    else if (strcasecmp(keyword,"tls_ciphers")==0)
    {
      log_log(LOG_WARNING,"%s:%d: option %s is currently untested (please report any successes)",filename,lnr,keyword);
      get_restdup(filename,lnr,keyword,&line,&cfg->ldc_tls_ciphers);
    }
    else if (strcasecmp(keyword,"tls_cert")==0)
    {
      log_log(LOG_WARNING,"%s:%d: option %s is currently untested (please report any successes)",filename,lnr,keyword);
      get_strdup(filename,lnr,keyword,&line,&cfg->ldc_tls_cert);
      get_eol(filename,lnr,keyword,&line);
      /* TODO: check that the path is valid */
    }
    else if (strcasecmp(keyword,"tls_key")==0)
    {
      log_log(LOG_WARNING,"%s:%d: option %s is currently untested (please report any successes)",filename,lnr,keyword);
      get_strdup(filename,lnr,keyword,&line,&cfg->ldc_tls_key);
      get_eol(filename,lnr,keyword,&line);
      /* TODO: check that the path is valid */
    }
#endif /* LDAP_OPT_X_TLS */
    /* other options */
    else if (strcasecmp(keyword,"restart")==0)
    {
      log_log(LOG_WARNING,"%s:%d: option %s is currently untested (and may be removed in an upcoming release)",filename,lnr,keyword);
      get_boolean(filename,lnr,keyword,&line,&cfg->ldc_restart);
      get_eol(filename,lnr,keyword,&line);
    }
    else if (strcasecmp(keyword,"pagesize")==0)
    {
      get_int(filename,lnr,keyword,&line,&cfg->ldc_pagesize);
      get_eol(filename,lnr,keyword,&line);
    }
    /* fallthrough */
    else
    {
      log_log(LOG_ERR,"%s:%d: unknown keyword: '%s'",filename,lnr,keyword);
      exit(EXIT_FAILURE);
    }
  }
  /* we're done reading file, close */
  fclose(fp);
}

void cfg_init(const char *fname)
{
  /* check if we were called before */
  if (nslcd_cfg!=NULL)
  {
    log_log(LOG_CRIT,"cfg_init() may only be called once");
    exit(EXIT_FAILURE);
  }
  /* allocate the memory (this memory is not freed anywhere) */
  nslcd_cfg=(struct ldap_config *)malloc(sizeof(struct ldap_config));
  if (nslcd_cfg==NULL)
  {
    log_log(LOG_CRIT,"malloc() failed to allocate memory");
    exit(EXIT_FAILURE);
  }
  /* clear configuration */
  cfg_defaults(nslcd_cfg);
  /* read configfile */
  cfg_read(fname,nslcd_cfg);
  /* do some sanity checks */
  if (nslcd_cfg->ldc_uris[0].uri==NULL)
  {
    log_log(LOG_ERR,"no URIs defined in config");
    exit(EXIT_FAILURE);
  }
}
