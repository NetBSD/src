/*
   myldap.c - simple interface to do LDAP requests
   Parts of this file were part of the nss_ldap library (as ldap-nss.c)
   which has been forked into the nss-ldapd library.

   Copyright (C) 1997-2006 Luke Howard
   Copyright (C) 2006, 2007 West Consulting
   Copyright (C) 2006, 2007, 2008 Arthur de Jong

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

/*
   This library expects to use an LDAP library to provide the real
   functionality and only provides a convenient wrapper.
   Some pointers for more information on the LDAP API:
     http://tools.ietf.org/id/draft-ietf-ldapext-ldap-c-api-05.txt
     http://www.mozilla.org/directory/csdk-docs/function.htm
     http://publib.boulder.ibm.com/infocenter/iseries/v5r3/topic/apis/dirserv1.htm
     http://www.openldap.org/software/man.cgi?query=ldap
*/

#include "config.h"

/* also include deprecated LDAP functions for now */
#define LDAP_DEPRECATED 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <lber.h>
#include <ldap.h>
#ifdef HAVE_LDAP_SSL_H
#include <ldap_ssl.h>
#endif
#ifdef HAVE_GSSLDAP_H
#include <gssldap.h>
#endif
#ifdef HAVE_GSSSASL_H
#include <gsssasl.h>
#endif
/* Try to handle systems with both SASL libraries installed */
#if defined(HAVE_SASL_SASL_H) && defined(HAVE_SASL_AUXPROP_REQUEST)
#include <sasl/sasl.h>
#elif defined(HAVE_SASL_H)
#include <sasl.h>
#endif
#include <ctype.h>
#include <pthread.h>

#include "myldap.h"
#include "compat/pagectrl.h"
#include "common.h"
#include "log.h"
#include "cfg.h"
#include "attmap.h"
#include "common/set.h"

/* compatibility macros */
#ifndef LDAP_CONST
#define LDAP_CONST const
#endif /* not LDAP_CONST */
#ifndef LDAP_MSG_ONE
#define LDAP_MSG_ONE 0x00
#endif /* not LDAP_MSG_ONE */

/* the maximum number of searches per session */
#define MAX_SEARCHES_IN_SESSION 4

/* This refers to a current LDAP session that contains the connection
   information. */
struct ldap_session
{
  /* the connection */
  LDAP *ld;
  /* timestamp of last activity */
  time_t lastactivity;
  /* index into ldc_uris: currently connected LDAP uri */
  int current_uri;
  /* a list of searches registered with this session */
  struct myldap_search *searches[MAX_SEARCHES_IN_SESSION];
};

/* A search description set as returned by myldap_search(). */
struct myldap_search
{
  /* reference to the session */
  MYLDAP_SESSION *session;
  /* indicator that the search is still valid */
  int valid;
  /* the parameters descibing the search */
  const char *base;
  int scope;
  const char *filter;
  char **attrs;
  /* a pointer to the current result entry, used for
     freeing resource allocated with that entry */
  MYLDAP_ENTRY *entry;
  /* LDAP message id for the search, -1 indicates absense of an active search */
  int msgid;
  /* the last result that was returned by ldap_result() */
  LDAPMessage *msg;
  /* cookie for paged searches */
  struct berval *cookie;
};

/* The maximum number of calls to myldap_get_values() that may be
   done per returned entry. */
#define MAX_ATTRIBUTES_PER_ENTRY 16

/* The maximum number of ranged attribute values that may be stoted
   per entry. */
#define MAX_RANGED_ATTRIBUTES_PER_ENTRY 2

/* A single entry from the LDAP database as returned by
   myldap_get_entry(). */
struct myldap_entry
{
  /* reference to the search to be used to get parameters
     (e.g. LDAP connection) for other calls */
  MYLDAP_SEARCH *search;
  /* the DN */
  const char *dn;
  /* a cached version of the exploded rdn */
  char **exploded_rdn;
  /* a cache of attribute to value list */
  char **attributevalues[MAX_ATTRIBUTES_PER_ENTRY];
  /* a reference to ranged attribute values so we can free() them later on */
  char **rangedattributevalues[MAX_RANGED_ATTRIBUTES_PER_ENTRY];
};

static MYLDAP_ENTRY *myldap_entry_new(MYLDAP_SEARCH *search)
{
  MYLDAP_ENTRY *entry;
  int i;
  /* Note: as an alternative we could embed the myldap_entry into the
     myldap_search struct to save on malloc() and free() calls. */
  /* allocate new entry */
  entry=(MYLDAP_ENTRY *)malloc(sizeof(struct myldap_entry));
  if (entry==NULL)
  {
    log_log(LOG_CRIT,"myldap_entry_new(): malloc() failed to allocate memory");
    exit(EXIT_FAILURE);
  }
  /* fill in fields */
  entry->search=search;
  entry->dn=NULL;
  entry->exploded_rdn=NULL;
  for (i=0;i<MAX_ATTRIBUTES_PER_ENTRY;i++)
    entry->attributevalues[i]=NULL;
  for (i=0;i<MAX_RANGED_ATTRIBUTES_PER_ENTRY;i++)
    entry->rangedattributevalues[i]=NULL;
  /* return the fresh entry */
  return entry;
}

static void myldap_entry_free(MYLDAP_ENTRY *entry)
{
  int i;
  /* free the DN */
  if (entry->dn!=NULL)
    ldap_memfree((char *)entry->dn);
  /* free the exploded RDN */
  if (entry->exploded_rdn!=NULL)
    ldap_value_free(entry->exploded_rdn);
  /* free all attribute values */
  for (i=0;i<MAX_ATTRIBUTES_PER_ENTRY;i++)
    if (entry->attributevalues[i]!=NULL)
      ldap_value_free(entry->attributevalues[i]);
  /* free all ranged attribute values */
  for (i=0;i<MAX_RANGED_ATTRIBUTES_PER_ENTRY;i++)
    if (entry->rangedattributevalues[i]!=NULL)
      free(entry->rangedattributevalues[i]);
  /* we don't need the result anymore, ditch it. */
  ldap_msgfree(entry->search->msg);
  entry->search->msg=NULL;
  /* free the actual memory for the struct */
  free(entry);
}

static MYLDAP_SEARCH *myldap_search_new(
        MYLDAP_SESSION *session,
        const char *base,int scope,const char *filter,const char **attrs)
{
  char *buffer;
  MYLDAP_SEARCH *search;
  int i;
  size_t sz;
  /* figure out size for new memory block to allocate
     this has the advantage that we can free the whole lot with one call */
  sz=sizeof(struct myldap_search);
  sz+=strlen(base)+1+strlen(filter)+1;
  for (i=0;attrs[i]!=NULL;i++)
    sz+=strlen(attrs[i])+1;
  sz+=(i+1)*sizeof(char *);
  /* allocate new results memory region */
  buffer=(char *)malloc(sz);
  if (buffer==NULL)
  {
    log_log(LOG_CRIT,"myldap_search_new(): malloc() failed to allocate memory");
    exit(EXIT_FAILURE);
  }
  /* initialize struct */
  search=(MYLDAP_SEARCH *)(void *)(buffer);
  buffer+=sizeof(struct myldap_search);
  /* save pointer to session */
  search->session=session;
  /* flag as valid search */
  search->valid=1;
  /* initialize array of attributes */
  search->attrs=(char **)(void *)buffer;
  buffer+=(i+1)*sizeof(char *);
  /* copy base */
  strcpy(buffer,base);
  search->base=buffer;
  buffer+=strlen(base)+1;
  /* just plainly store scope */
  search->scope=scope;
  /* copy filter */
  strcpy(buffer,filter);
  search->filter=buffer;
  buffer+=strlen(filter)+1;
  /* copy attributes themselves */
  for (i=0;attrs[i]!=NULL;i++)
  {
    strcpy(buffer,attrs[i]);
    search->attrs[i]=buffer;
    buffer+=strlen(attrs[i])+1;
  }
  search->attrs[i]=NULL;
  /* initialize context */
  search->cookie=NULL;
  search->msg=NULL;
  search->msgid=-1;
  /* clear result entry */
  search->entry=NULL;
  /* return the new search struct */
  return search;
}

static void myldap_search_free(MYLDAP_SEARCH *search)
{
  /* free any search entries */
  if (search->entry!=NULL)
    myldap_entry_free(search->entry);
  /* clean up cookie */
  if (search->cookie!=NULL)
    ber_bvfree(search->cookie);
  /* free read messages */
  if (search->msg!=NULL)
    ldap_msgfree(search->msg);
  /* free the storage we allocated */
  free(search);
}

static MYLDAP_SESSION *myldap_session_new(void)
{
  MYLDAP_SESSION *session;
  int i;
  /* allocate memory for the session storage */
  session=(struct ldap_session *)malloc(sizeof(struct ldap_session));
  if (session==NULL)
  {
    log_log(LOG_CRIT,"myldap_session_new(): malloc() failed to allocate memory");
    exit(EXIT_FAILURE);
  }
  /* initialize the session */
  session->ld=NULL;
  session->lastactivity=0;
  session->current_uri=0;
  for (i=0;i<MAX_SEARCHES_IN_SESSION;i++)
    session->searches[i]=NULL;
  /* return the new session */
  return session;
}

PURE static inline int is_valid_session(MYLDAP_SESSION *session)
{
  return (session!=NULL);
}

PURE static inline int is_open_session(MYLDAP_SESSION *session)
{
  return is_valid_session(session)&&(session->ld!=NULL);
}

/* note that this does not check the valid flag of the search */
PURE static inline int is_valid_search(MYLDAP_SEARCH *search)
{
  return (search!=NULL)&&is_open_session(search->session);
}

PURE static inline int is_valid_entry(MYLDAP_ENTRY *entry)
{
  return (entry!=NULL)&&is_valid_search(entry->search)&&(entry->search->msg!=NULL);
}

#ifdef HAVE_SASL_INTERACT_T
/* this is registered with ldap_sasl_interactive_bind_s() in do_bind() */
static int do_sasl_interact(LDAP UNUSED(*ld),unsigned UNUSED(flags),void *defaults,void *_interact)
{
  char *authzid=(char *)defaults;
  sasl_interact_t *interact=(sasl_interact_t *)_interact;
  while (interact->id!=SASL_CB_LIST_END)
  {
    if (interact->id!=SASL_CB_USER)
      return LDAP_PARAM_ERROR;
    if (authzid!=NULL)
    {
      interact->result=authzid;
      interact->len=strlen(authzid);
    }
    else if (interact->defresult!=NULL)
    {
      interact->result=interact->defresult;
      interact->len=strlen(interact->defresult);
    }
    else
    {
      interact->result="";
      interact->len=0;
    }
    interact++;
  }
  return LDAP_SUCCESS;
}
#endif /* HAVE_SASL_INTERACT_T */

#define LDAP_SET_OPTION(ld,option,invalue) \
  rc=ldap_set_option(ld,option,invalue); \
  if (rc!=LDAP_SUCCESS) \
  { \
    log_log(LOG_ERR,"ldap_set_option("__STRING(option)") failed: %s",ldap_err2string(rc)); \
    return rc; \
  }

/* This function performs the authentication phase of opening a connection.
   This returns an LDAP result code. */
static int do_bind(MYLDAP_SESSION *session,const char *uri)
{
  int rc;
#ifndef HAVE_SASL_INTERACT_T
  struct berval cred;
#endif /* not HAVE_SASL_INTERACT_T */
  /* check if StartTLS is requested */
  if (nslcd_cfg->ldc_ssl_on==SSL_START_TLS)
  {
    rc=ldap_start_tls_s(session->ld,NULL,NULL);
    if (rc!=LDAP_SUCCESS)
    {
      log_log(LOG_WARNING,"ldap_start_tls_s() failed: %s: %s",
                          ldap_err2string(rc),strerror(errno));
      return rc;
    }
  }
  /* TODO: store this information in the session */
  if (!nslcd_cfg->ldc_usesasl)
  {
    /* do a simple bind */
    if (nslcd_cfg->ldc_binddn)
      log_log(LOG_DEBUG,"simple bind to %s as %s",uri,nslcd_cfg->ldc_binddn);
    else
      log_log(LOG_DEBUG,"simple anonymous bind to %s",uri);
    return ldap_simple_bind_s(session->ld,nslcd_cfg->ldc_binddn,nslcd_cfg->ldc_bindpw);
  }
  else
  {
    /* do a SASL bind */
    log_log(LOG_DEBUG,"SASL bind to %s as %s",uri,nslcd_cfg->ldc_binddn);
    if (nslcd_cfg->ldc_sasl_secprops!=NULL)
    {
      LDAP_SET_OPTION(session->ld,LDAP_OPT_X_SASL_SECPROPS,(void *)nslcd_cfg->ldc_sasl_secprops);
    }
#ifdef HAVE_SASL_INTERACT_T
    return ldap_sasl_interactive_bind_s(session->ld,nslcd_cfg->ldc_binddn,"GSSAPI",NULL,NULL,
                                    LDAP_SASL_QUIET,
                                    do_sasl_interact,(void *)nslcd_cfg->ldc_saslid);
#else /* HAVE_SASL_INTERACT_T */
    cred.bv_val=nslcd_cfg->ldc_saslid;
    cred.bv_len=strlen(nslcd_cfg->ldc_saslid);
    return ldap_sasl_bind_s(session->ld,nslcd_cfg->ldc_binddn,"GSSAPI",&cred,NULL,NULL,NULL);
#endif /* not HAVE_SASL_INTERACT_T */
  }
}

#ifdef HAVE_LDAP_SET_REBIND_PROC
/* This function is called by the LDAP library when chasing referrals.
   It is configured with the ldap_set_rebind_proc() below. */
static int do_rebind(LDAP *UNUSED(ld),LDAP_CONST char *url,
                     ber_tag_t UNUSED(request),
                     ber_int_t UNUSED(msgid),void *arg)
{
  log_log(LOG_DEBUG,"rebinding to %s",url);
  return do_bind((MYLDAP_SESSION *)arg,url);
}
#endif /* HAVE_LDAP_SET_REBIND_PROC */

/* This function sets a number of properties on the connection, based
   what is configured in the configfile. This function returns an
   LDAP status code. */
static int do_set_options(MYLDAP_SESSION *session)
{
  int rc;
  struct timeval tv;
#ifdef LDAP_OPT_X_TLS
  int tls=LDAP_OPT_X_TLS_HARD;
#endif /* LDAP_OPT_X_TLS */
  /* turn on debugging */
  if (nslcd_cfg->ldc_debug)
  {
#ifdef LBER_OPT_DEBUG_LEVEL
    rc=ber_set_option(NULL,LBER_OPT_DEBUG_LEVEL,&nslcd_cfg->ldc_debug);
    if (rc!=LDAP_SUCCESS)
    {
      log_log(LOG_ERR,"ber_set_option(LBER_OPT_DEBUG_LEVEL) failed: %s",ldap_err2string(rc));
      return rc;
    }
#endif /* LBER_OPT_DEBUG_LEVEL */
#ifdef LDAP_OPT_DEBUG_LEVEL
    LDAP_SET_OPTION(NULL,LDAP_OPT_DEBUG_LEVEL,&nslcd_cfg->ldc_debug);
#endif /* LDAP_OPT_DEBUG_LEVEL */
  }
#ifdef HAVE_LDAP_SET_REBIND_PROC
  /* the rebind function that is called when chasing referrals, see
     http://publib.boulder.ibm.com/infocenter/iseries/v5r3/topic/apis/ldap_set_rebind_proc.htm
     http://www.openldap.org/software/man.cgi?query=ldap_set_rebind_proc&manpath=OpenLDAP+2.4-Release */
  /* TODO: probably only set this if we should chase referrals */
  rc=ldap_set_rebind_proc(session->ld,do_rebind,session);
  if (rc!=LDAP_SUCCESS)
  {
    log_log(LOG_ERR,"ldap_set_rebind_proc() failed: %s",ldap_err2string(rc));
    return rc;
  }
#endif /* HAVE_LDAP_SET_REBIND_PROC */
  /* set the protocol version to use */
  LDAP_SET_OPTION(session->ld,LDAP_OPT_PROTOCOL_VERSION,&nslcd_cfg->ldc_version);
  /* set some other options */
  LDAP_SET_OPTION(session->ld,LDAP_OPT_DEREF,&nslcd_cfg->ldc_deref);
  LDAP_SET_OPTION(session->ld,LDAP_OPT_TIMELIMIT,&nslcd_cfg->ldc_timelimit);
  tv.tv_sec=nslcd_cfg->ldc_bind_timelimit;
  tv.tv_usec=0;
#ifdef LDAP_OPT_TIMEOUT
  LDAP_SET_OPTION(session->ld,LDAP_OPT_TIMEOUT,&tv);
#endif /* LDAP_OPT_TIMEOUT */
#ifdef LDAP_OPT_NETWORK_TIMEOUT
  LDAP_SET_OPTION(session->ld,LDAP_OPT_NETWORK_TIMEOUT,&tv);
#endif /* LDAP_OPT_NETWORK_TIMEOUT */
#ifdef LDAP_X_OPT_CONNECT_TIMEOUT
  LDAP_SET_OPTION(session->ld,LDAP_X_OPT_CONNECT_TIMEOUT,&tv);
#endif /* LDAP_X_OPT_CONNECT_TIMEOUT */
  LDAP_SET_OPTION(session->ld,LDAP_OPT_REFERRALS,nslcd_cfg->ldc_referrals?LDAP_OPT_ON:LDAP_OPT_OFF);
  LDAP_SET_OPTION(session->ld,LDAP_OPT_RESTART,nslcd_cfg->ldc_restart?LDAP_OPT_ON:LDAP_OPT_OFF);
#ifdef LDAP_OPT_X_TLS
  /* if SSL is desired, then enable it */
  if (nslcd_cfg->ldc_ssl_on==SSL_LDAPS)
  {
    /* use tls */
    LDAP_SET_OPTION(session->ld,LDAP_OPT_X_TLS,&tls);
    /* rand file */
    if (nslcd_cfg->ldc_tls_randfile!=NULL)
    {
      LDAP_SET_OPTION(session->ld,LDAP_OPT_X_TLS_RANDOM_FILE,nslcd_cfg->ldc_tls_randfile);
    }
    /* ca cert file */
    if (nslcd_cfg->ldc_tls_cacertfile!=NULL)
    {
      LDAP_SET_OPTION(session->ld,LDAP_OPT_X_TLS_CACERTFILE,nslcd_cfg->ldc_tls_cacertfile);
    }
    /* ca cert directory */
    if (nslcd_cfg->ldc_tls_cacertdir!=NULL)
    {
      LDAP_SET_OPTION(session->ld,LDAP_OPT_X_TLS_CACERTDIR,nslcd_cfg->ldc_tls_cacertdir);
    }
    /* require cert? */
    if (nslcd_cfg->ldc_tls_checkpeer>-1)
    {
      LDAP_SET_OPTION(session->ld,LDAP_OPT_X_TLS_REQUIRE_CERT,&nslcd_cfg->ldc_tls_checkpeer);
    }
    /* set cipher suite, certificate and private key */
    if (nslcd_cfg->ldc_tls_ciphers!=NULL)
    {
      LDAP_SET_OPTION(session->ld,LDAP_OPT_X_TLS_CIPHER_SUITE,nslcd_cfg->ldc_tls_ciphers);
    }
    /* set certificate */
    if (nslcd_cfg->ldc_tls_cert!=NULL)
    {
      LDAP_SET_OPTION(session->ld,LDAP_OPT_X_TLS_CERTFILE,nslcd_cfg->ldc_tls_cert);
    }
    /* set up key */
    if (nslcd_cfg->ldc_tls_key!=NULL)
    {
      LDAP_SET_OPTION(session->ld,LDAP_OPT_X_TLS_KEYFILE,nslcd_cfg->ldc_tls_key);
    }
  }
#endif /* LDAP_OPT_X_TLS */
  /* if nothing above failed, everything should be fine */
  return LDAP_SUCCESS;
}

/* close the connection to the server and invalidate any running searches */
static void do_close(MYLDAP_SESSION *session)
{
  int i;
  int rc;
  /* if we had reachability problems with the server close the connection */
  if (session->ld!=NULL)
  {
    /* go over the other searches and partially close them */
    for (i=0;i<MAX_SEARCHES_IN_SESSION;i++)
    {
      if (session->searches[i]!=NULL)
      {
        /* free any messages (because later ld is no longer valid) */
        if (session->searches[i]->msg!=NULL)
        {
          ldap_msgfree(session->searches[i]->msg);
          session->searches[i]->msg=NULL;
        }
        /* abandon the search if there were more results to fetch */
        if (session->searches[i]->msgid!=-1)
        {
          if (ldap_abandon(session->searches[i]->session->ld,session->searches[i]->msgid))
          {
            if (ldap_get_option(session->ld,LDAP_OPT_ERROR_NUMBER,&rc)==LDAP_SUCCESS)
              rc=LDAP_OTHER;
            log_log(LOG_WARNING,"ldap_abandon() failed to abandon search: %s",ldap_err2string(rc));
          }
          session->searches[i]->msgid=-1;
        }
        /* flag the search as invalid */
        session->searches[i]->valid=0;
      }
    }
    /* close the connection to the server */
    rc=ldap_unbind(session->ld);
    session->ld=NULL;
    if (rc!=LDAP_SUCCESS)
      log_log(LOG_WARNING,"ldap_unbind() failed: %s",ldap_err2string(rc));
  }
}

/* This checks the timeout value of the session and closes the connection
   to the LDAP server if the timeout has expired and there are no pending
   searches. */
static void myldap_session_check(MYLDAP_SESSION *session)
{
  int i;
  int runningsearches=0;
  time_t current_time;
  /* check parameters */
  if (!is_valid_session(session))
  {
    log_log(LOG_ERR,"myldap_session_check(): invalid parameter passed");
    errno=EINVAL;
    return;
  }
  /* check if we should time out the connection */
  if ((session->ld!=NULL)&&(nslcd_cfg->ldc_idle_timelimit>0))
  {
    /* check if we have any running searches */
    for (i=0;i<MAX_SEARCHES_IN_SESSION;i++)
    {
      if ((session->searches[i]!=NULL)&&(session->searches[i]->valid))
      {
        runningsearches=1;
        break;
      }
    }
    /* only consider timeout if we have no running searches */
    if (!runningsearches)
    {
      time(&current_time);
      if ((session->lastactivity+nslcd_cfg->ldc_idle_timelimit)<current_time)
      {
        log_log(LOG_DEBUG,"do_open(): idle_timelimit reached");
        do_close(session);
      }
    }
  }
}

/* This opens connection to an LDAP server, sets all connection options
   and binds to the server. This returns an LDAP status code. */
static int do_open(MYLDAP_SESSION *session)
{
  int rc,rc2;
  int sd=-1;
  int off=0;
  /* check if the idle time for the connection has expired */
  myldap_session_check(session);
  /* if the connection is still there (ie. ldap_unbind() wasn't
     called) then we can return the cached connection */
  if (session->ld!=NULL)
    return LDAP_SUCCESS;
  /* we should build a new session now */
  session->ld=NULL;
  session->lastactivity=0;
  /* open the connection */
  rc=ldap_initialize(&(session->ld),nslcd_cfg->ldc_uris[session->current_uri].uri);
  if (rc!=LDAP_SUCCESS)
  {
    log_log(LOG_WARNING,"ldap_initialize(%s) failed: %s: %s",
                        nslcd_cfg->ldc_uris[session->current_uri].uri,
                        ldap_err2string(rc),strerror(errno));
    if (session->ld!=NULL)
    {
      rc2=ldap_unbind(session->ld);
      session->ld=NULL;
      if (rc2!=LDAP_SUCCESS)
        log_log(LOG_WARNING,"ldap_unbind() failed: %s",ldap_err2string(rc2));
    }
    return rc;
  }
  else if (session->ld==NULL)
  {
    log_log(LOG_WARNING,"ldap_initialize() returned NULL");
    return LDAP_LOCAL_ERROR;
  }
  /* set the options for the connection */
  rc=do_set_options(session);
  if (rc!=LDAP_SUCCESS)
  {
    rc2=ldap_unbind(session->ld);
    session->ld=NULL;
    if (rc2!=LDAP_SUCCESS)
      log_log(LOG_WARNING,"ldap_unbind() failed: %s",ldap_err2string(rc2));
    return rc;
  }
  /* bind to the server */
  rc=do_bind(session,nslcd_cfg->ldc_uris[session->current_uri].uri);
  if (rc!=LDAP_SUCCESS)
  {
    /* log actual LDAP error code */
    log_log(LOG_WARNING,"failed to bind to LDAP server %s: %s: %s",
                        nslcd_cfg->ldc_uris[session->current_uri].uri,
                        ldap_err2string(rc),strerror(errno));
    rc2=ldap_unbind(session->ld);
    session->ld=NULL;
    if (rc2!=LDAP_SUCCESS)
      log_log(LOG_WARNING,"ldap_unbind() failed: %s",ldap_err2string(rc2));
    return rc;
  }
  /* disable keepalive on the LDAP connection socket (why?) */
  if (ldap_get_option(session->ld,LDAP_OPT_DESC,&sd)==LDAP_SUCCESS)
  {
    /* ignore errors */
    (void)setsockopt(sd,SOL_SOCKET,SO_KEEPALIVE,(void *)&off,sizeof(off));
  }
  /* update last activity and finish off state */
  time(&(session->lastactivity));
  log_log(LOG_INFO,"connected to LDAP server %s",
                   nslcd_cfg->ldc_uris[session->current_uri].uri);
  return LDAP_SUCCESS;
}

static MYLDAP_SEARCH *do_try_search(
        MYLDAP_SESSION *session,
        const char *base,int scope,const char *filter,const char **attrs)
{
  int rc;
  LDAPControl *serverCtrls[2];
  LDAPControl **pServerCtrls;
  int msgid;
  MYLDAP_SEARCH *search;
  int i;
  /* ensure that we have an open connection */
  rc=do_open(session);
  if (rc!=LDAP_SUCCESS)
    return NULL;
  /* if we're using paging, build a page control */
  if ((nslcd_cfg->ldc_pagesize>0)&&(scope!=LDAP_SCOPE_BASE))
  {
    rc=ldap_create_page_control(session->ld,nslcd_cfg->ldc_pagesize,
                                NULL,0,&serverCtrls[0]);
    if (rc==LDAP_SUCCESS)
    {
      serverCtrls[1]=NULL;
      pServerCtrls=serverCtrls;
    }
    else
    {
      log_log(LOG_WARNING,"ldap_create_page_control() failed: %s",ldap_err2string(rc));
      /* clear error flag */
      rc=LDAP_SUCCESS;
      ldap_set_option(session->ld,LDAP_OPT_ERROR_NUMBER,&rc);
      pServerCtrls=NULL;
    }
  }
  else
    pServerCtrls=NULL;
  /* perform the search */
  rc=ldap_search_ext(session->ld,
                     base,scope,filter,(char **)attrs,
                     0,pServerCtrls,NULL,NULL,
                     LDAP_NO_LIMIT,&msgid);
  /* free the controls if we had them */
  if (pServerCtrls!=NULL)
  {
    ldap_control_free(serverCtrls[0]);
    serverCtrls[0]=NULL;
  }
  /* handle errors */
  if (rc!=LDAP_SUCCESS)
  {
    log_log(LOG_WARNING,"ldap_search_ext() failed: %s",ldap_err2string(rc));
    return NULL;
  }
  /* update the last activity on the connection */
  time(&(session->lastactivity));
  /* allocate a new search entry */
  search=myldap_search_new(session,base,scope,filter,attrs);
  /* save msgid */
  search->msgid=msgid;
  /* find a place in the session where we can register our search */
  for (i=0;(session->searches[i]!=NULL)&&(i<MAX_SEARCHES_IN_SESSION);i++)
    ;
  if (i>=MAX_SEARCHES_IN_SESSION)
  {
    log_log(LOG_ERR,"myldap_search(): too many searches registered with session (max %d)",
                    MAX_SEARCHES_IN_SESSION);
    myldap_search_close(search);
    return NULL;
  }
  /* regsiter search with the session so we can free it later on */
  session->searches[i]=search;
  /* return the new search */
  return search;
}

MYLDAP_SESSION *myldap_create_session(void)
{
  return myldap_session_new();
}

void myldap_session_cleanup(MYLDAP_SESSION *session)
{
  int i;
  /* check parameter */
  if (!is_valid_session(session))
  {
    log_log(LOG_ERR,"myldap_session_cleanup(): invalid session passed");
    return;
  }
  /* go over all searches in the session and close them */
  for (i=0;i<MAX_SEARCHES_IN_SESSION;i++)
  {
    if (session->searches[i]!=NULL)
    {
      myldap_search_close(session->searches[i]);
      session->searches[i]=NULL;
    }
  }
}

void myldap_session_close(MYLDAP_SESSION *session)
{
  /* check parameter */
  if (!is_valid_session(session))
  {
    log_log(LOG_ERR,"myldap_session_cleanup(): invalid session passed");
    return;
  }
  /* close pending searches */
  myldap_session_cleanup(session);
  /* close any open connections */
  do_close(session);
  /* free allocated memory */
  free(session);
}

/* mutex for updating the times in the uri */
pthread_mutex_t uris_mutex = PTHREAD_MUTEX_INITIALIZER;

MYLDAP_SEARCH *myldap_search(
        MYLDAP_SESSION *session,
        const char *base,int scope,const char *filter,const char **attrs)
{
  MYLDAP_SEARCH *search;
  int sleeptime=0;
  int start_uri;
  time_t endtime;
  time_t nexttry;
  time_t t;
  /* check parameters */
  if (!is_valid_session(session)||(base==NULL)||(filter==NULL)||(attrs==NULL))
  {
    log_log(LOG_ERR,"myldap_search(): invalid parameter passed");
    errno=EINVAL;
    return NULL;
  }
  /* log the call */
  log_log(LOG_DEBUG,"myldap_search(base=\"%s\", filter=\"%s\")",
                    base,filter);
  /* keep trying until we time out */
  endtime=time(NULL)+nslcd_cfg->ldc_reconnect_maxsleeptime;
  nexttry=endtime;
  while (1)
  {
    /* try each configured URL once */
    pthread_mutex_lock(&uris_mutex);
    start_uri=session->current_uri;
    do
    {
      /* only try if we haven't just had an error and it was a long tme
         since the last ok */
      if ( ( ( nslcd_cfg->ldc_uris[session->current_uri].lastfail -
               nslcd_cfg->ldc_uris[session->current_uri].lastok ) < nslcd_cfg->ldc_reconnect_maxsleeptime) ||
           ( time(NULL) >= (nslcd_cfg->ldc_uris[session->current_uri].lastfail+nslcd_cfg->ldc_reconnect_maxsleeptime) ) )
      {
        pthread_mutex_unlock(&uris_mutex);
        /* try to start the search */
        search=do_try_search(session,base,scope,filter,attrs);
        if (search!=NULL)
        {
          /* update ok time and return search handle */
          pthread_mutex_lock(&uris_mutex);
          nslcd_cfg->ldc_uris[session->current_uri].lastok=time(NULL);
          pthread_mutex_unlock(&uris_mutex);
          return search;
        }
        /* close the current connection */
        do_close(session);
        /* update time of failure and figure out when we should retry */
        pthread_mutex_lock(&uris_mutex);
        t=time(NULL);
        nslcd_cfg->ldc_uris[session->current_uri].lastfail=t;
        t+=nslcd_cfg->ldc_reconnect_sleeptime;
        if (t<nexttry)
          nexttry=t;
      }
      else if (nslcd_cfg->ldc_uris[session->current_uri].lastfail>0)
      {
        /* we are in a hard fail state, figure out when we can retry */
        t=(nslcd_cfg->ldc_uris[session->current_uri].lastfail+nslcd_cfg->ldc_reconnect_maxsleeptime);
        if (t<nexttry)
          nexttry=t;
      }
      /* try the next URI (with wrap-around) */
      session->current_uri++;
      if (nslcd_cfg->ldc_uris[session->current_uri].uri==NULL)
        session->current_uri=0;
    }
    while (session->current_uri!=start_uri);
    pthread_mutex_unlock(&uris_mutex);
    /* see if it is any use sleeping */
    if (nexttry>=endtime)
    {
      log_log(LOG_ERR,"no available LDAP server found");
      return NULL;
    }
    /* sleep between tries */
    sleeptime=nexttry-time(NULL);
    if (sleeptime>0)
    {
      log_log(LOG_WARNING,"no available LDAP server found, sleeping %d seconds",sleeptime);
      (void)sleep(sleeptime);
    }
    nexttry=time(NULL)+nslcd_cfg->ldc_reconnect_maxsleeptime;
  }
}

void myldap_search_close(MYLDAP_SEARCH *search)
{
  int i;
  if (!is_valid_search(search))
    return;
  /* free any messages */
  if (search->msg!=NULL)
  {
    ldap_msgfree(search->msg);
    search->msg=NULL;
  }
  /* abandon the search if there were more results to fetch */
  if (search->msgid!=-1)
  {
    ldap_abandon(search->session->ld,search->msgid);
    search->msgid=-1;
  }
  /* find the reference to this search in the session */
  for (i=0;i<MAX_SEARCHES_IN_SESSION;i++)
  {
    if (search->session->searches[i]==search)
      search->session->searches[i]=NULL;
  }
  /* free this search */
  myldap_search_free(search);
}

MYLDAP_ENTRY *myldap_get_entry(MYLDAP_SEARCH *search,int *rcp)
{
  int rc;
  int parserc;
  int msgid;
  struct timeval tv,*tvp;
  LDAPControl **resultcontrols;
  LDAPControl *serverctrls[2];
  ber_int_t count;
  /* check parameters */
  if (!is_valid_search(search))
  {
    log_log(LOG_ERR,"myldap_get_entry(): invalid search passed");
    errno=EINVAL;
    if (rcp!=NULL)
      *rcp=LDAP_OPERATIONS_ERROR;
    return NULL;
  }
  /* check if the connection wasn't closed in another search */
  if (!search->valid)
  {
    log_log(LOG_WARNING,"myldap_get_entry(): connection was closed");
    myldap_search_close(search);
    if (rcp!=NULL)
      *rcp=LDAP_SERVER_DOWN;
    return NULL;
  }
  /* set up a timelimit value for operations */
  if (nslcd_cfg->ldc_timelimit==LDAP_NO_LIMIT)
    tvp=NULL;
  else
  {
    tv.tv_sec=nslcd_cfg->ldc_timelimit;
    tv.tv_usec=0;
    tvp=&tv;
  }
  /* if we have an existing result entry, free it */
  if (search->entry!=NULL)
  {
    myldap_entry_free(search->entry);
    search->entry=NULL;
  }
  /* try to parse results until we have a final error or ok */
  while (1)
  {
    /* free the previous message if there was any */
    if (search->msg!=NULL)
    {
      ldap_msgfree(search->msg);
      search->msg=NULL;
    }
    /* get the next result */
    rc=ldap_result(search->session->ld,search->msgid,LDAP_MSG_ONE,tvp,&(search->msg));
    /* handle result */
    switch (rc)
    {
      case -1:
        /* we have an error condition, try to get error code */
        if (ldap_get_option(search->session->ld,LDAP_OPT_ERROR_NUMBER,&rc)!=LDAP_SUCCESS)
          rc=LDAP_UNAVAILABLE;
        log_log(LOG_ERR,"ldap_result() failed: %s",ldap_err2string(rc));
        /* close connection on connection problems */
        if ((rc==LDAP_UNAVAILABLE)||(rc==LDAP_SERVER_DOWN))
          do_close(search->session);
        /* close search */
        myldap_search_close(search);
        if (rcp!=NULL)
          *rcp=rc;
        return NULL;
      case 0:
        /* the timeout expired */
        log_log(LOG_ERR,"ldap_result() timed out");
        myldap_search_close(search);
        if (rcp!=NULL)
          *rcp=LDAP_TIMELIMIT_EXCEEDED;
        return NULL;
      case LDAP_RES_SEARCH_ENTRY:
        /* we have a normal search entry, update timestamp and return result */
        time(&(search->session->lastactivity));
        search->entry=myldap_entry_new(search);
        if (rcp!=NULL)
          *rcp=LDAP_SUCCESS;
        return search->entry;
      case LDAP_RES_SEARCH_RESULT:
        /* we have a search result, parse it */
        resultcontrols=NULL;
        if (search->cookie!=NULL)
        {
          ber_bvfree(search->cookie);
          search->cookie=NULL;
        }
        /* NB: this frees search->msg */
        parserc=ldap_parse_result(search->session->ld,search->msg,&rc,NULL,
                                  NULL,NULL,&resultcontrols,1);
        search->msg=NULL;
        /* check for errors during parsing */
        if ((parserc!=LDAP_SUCCESS)&&(parserc!=LDAP_MORE_RESULTS_TO_RETURN))
        {
          if (resultcontrols!=NULL)
            ldap_controls_free(resultcontrols);
          log_log(LOG_ERR,"ldap_parse_result() failed: %s",ldap_err2string(parserc));
          myldap_search_close(search);
          if (rcp!=NULL)
            *rcp=parserc;
          return NULL;
        }
        /* check for errors in message */
        if ((rc!=LDAP_SUCCESS)&&(rc!=LDAP_MORE_RESULTS_TO_RETURN))
        {
          if (resultcontrols!=NULL)
            ldap_controls_free(resultcontrols);
          log_log(LOG_ERR,"ldap_result() failed: %s",ldap_err2string(rc));
          /* close connection on connection problems */
          if ((rc==LDAP_UNAVAILABLE)||(rc==LDAP_SERVER_DOWN))
            do_close(search->session);
          myldap_search_close(search);
          if (rcp!=NULL)
            *rcp=rc;
          return NULL;
        }
        /* handle result controls */
        if (resultcontrols!=NULL)
        {
          /* see if there are any more pages to come */
          rc=ldap_parse_page_control(search->session->ld,
                                    resultcontrols,&count,
                                    &(search->cookie));
          if (rc!=LDAP_SUCCESS)
          {
            log_log(LOG_WARNING,"ldap_parse_page_control() failed: %s",
                                ldap_err2string(rc));
            /* clear error flag */
            rc=LDAP_SUCCESS;
            ldap_set_option(search->session->ld,LDAP_OPT_ERROR_NUMBER,&rc);
          }
          /* TODO: handle the above return code?? */
          ldap_controls_free(resultcontrols);
        }
        search->msgid=-1;
        /* check if there are more pages to come */
        if ((search->cookie==NULL)||(search->cookie->bv_len==0))
        {
          log_log(LOG_DEBUG,"ldap_result(): end of results");
          /* we are at the end of the search, no more results */
          myldap_search_close(search);
          if (rcp!=NULL)
            *rcp=LDAP_SUCCESS;
          return NULL;
        }
        /* try the next page */
        serverctrls[0]=NULL;
        serverctrls[1]=NULL;
        rc=ldap_create_page_control(search->session->ld,
                                    nslcd_cfg->ldc_pagesize,
                                    search->cookie,0,&serverctrls[0]);
        if (rc!=LDAP_SUCCESS)
        {
          if (serverctrls[0]!=NULL)
            ldap_control_free(serverctrls[0]);
          log_log(LOG_WARNING,"ldap_create_page_control() failed: %s",
                              ldap_err2string(rc));
          myldap_search_close(search);
          if (rcp!=NULL)
            *rcp=rc;
          return NULL;
        }
        /* set up a new search for the next page */
        rc=ldap_search_ext(search->session->ld,
                           search->base,search->scope,search->filter,
                           search->attrs,0,serverctrls,NULL,NULL,
                           LDAP_NO_LIMIT,&msgid);
        ldap_control_free(serverctrls[0]);
        if (rc!=LDAP_SUCCESS)
        {
          log_log(LOG_WARNING,"ldap_search_ext() failed: %s",
                              ldap_err2string(rc));
          /* close connection on connection problems */
          if ((rc==LDAP_UNAVAILABLE)||(rc==LDAP_SERVER_DOWN))
            do_close(search->session);
          myldap_search_close(search);
          if (rcp!=NULL)
            *rcp=rc;
          return NULL;
        }
        search->msgid=msgid;
        /* we continue with another pass */
        break;
      case LDAP_RES_SEARCH_REFERENCE:
        break; /* just ignore search references */
      default:
        log_log(LOG_WARNING,"ldap_result() returned unexpected result type");
        myldap_search_close(search);
        if (rcp!=NULL)
          *rcp=LDAP_PROTOCOL_ERROR;
        return NULL;
    }
  }
}

/* Get the DN from the entry. This function only returns NULL (and sets
   errno) if an incorrect entry is passed. If the DN value cannot be
   retreived "unknown" is returned instead. */
const char *myldap_get_dn(MYLDAP_ENTRY *entry)
{
  int rc;
  /* check parameters */
  if (!is_valid_entry(entry))
  {
    log_log(LOG_ERR,"myldap_get_dn(): invalid result entry passed");
    errno=EINVAL;
    return "unknown";
  }
  /* if we don't have it yet, retreive it */
  if ((entry->dn==NULL)&&(entry->search->valid))
  {
    entry->dn=ldap_get_dn(entry->search->session->ld,entry->search->msg);
    if (entry->dn==NULL)
    {
      if (ldap_get_option(entry->search->session->ld,LDAP_OPT_ERROR_NUMBER,&rc)!=LDAP_SUCCESS)
        rc=LDAP_UNAVAILABLE;
      log_log(LOG_WARNING,"ldap_get_dn() returned NULL: %s",ldap_err2string(rc));
      /* close connection on connection problems */
      if ((rc==LDAP_UNAVAILABLE)||(rc==LDAP_SERVER_DOWN))
        do_close(entry->search->session);
    }
  }
  /* if we still don't have it, return unknown */
  if (entry->dn==NULL)
    return "unknown";
  /* return it */
  return entry->dn;
}

/* Return a buffer that is an a list of strings that can be freed
   with a single call to free(). This function frees the set. */
static char **set2values(SET *set)
{
  char *buf;
  char **values;
  const char *val;
  size_t sz;
  int num;
  /* check set */
  if (set==NULL)
    return NULL;
  /* count number of entries and needed space */
  sz=0;
  num=1;
  set_loop_first(set);
  while ((val=set_loop_next(set))!=NULL)
  {
    num++;
    sz+=strlen(val)+1;
  }
  /* allocate the needed memory */
  buf=(char *)malloc(num*sizeof(char *)+sz);
  if (buf==NULL)
  {
    log_log(LOG_CRIT,"set2values(): malloc() failed to allocate memory");
    set_free(set);
    return NULL;
  }
  values=(char **)buf;
  buf+=num*sizeof(char *);
  /* copy set into buffer */
  sz=0;
  num=0;
  set_loop_first(set);
  while ((val=set_loop_next(set))!=NULL)
  {
    strcpy(buf,val);
    values[num++]=buf;
    buf+=strlen(buf)+1;
  }
  values[num]=NULL;
  /* we're done */
  set_free(set);
  return values;
}

/* Perform ranged retreival of attributes.
   http://msdn.microsoft.com/en-us/library/aa367017(vs.85).aspx
   http://www.tkk.fi/cc/docs/kerberos/draft-kashi-incremental-00.txt */
static char **myldap_get_ranged_values(MYLDAP_ENTRY *entry,const char *attr)
{
  char **values;
  char *attn;
  const char *attrs[2];
  BerElement *ber;
  int i;
  int startat=0,nxt=0;
  char attbuf[80];
  const char *dn=myldap_get_dn(entry);
  MYLDAP_SESSION *session=entry->search->session;
  MYLDAP_SEARCH *search=NULL;
  SET *set=NULL;
  /* build the attribute name to find */
  if (mysnprintf(attbuf,sizeof(attbuf),"%s;range=0-*",attr))
    return NULL;
  /* keep doing lookups untul we can't get any more results */
  while (1)
  {
    /* go over all attributes to find the ranged attribute */
    ber=NULL;
    attn=ldap_first_attribute(entry->search->session->ld,entry->search->msg,&ber);
    values=NULL;
    while (attn!=NULL)
    {
      if (strncasecmp(attn,attbuf,strlen(attbuf)-1)==0)
      {
        log_log(LOG_DEBUG,"found ranged results %s",attn);
        nxt=atoi(attn+strlen(attbuf)-1)+1;
        values=ldap_get_values(entry->search->session->ld,entry->search->msg,attn);
        ldap_memfree(attn);
        break;
      }
      /* free old attribute name and get next one */
      ldap_memfree(attn);
      attn=ldap_next_attribute(entry->search->session->ld,entry->search->msg,ber);
    }
    ber_free(ber,0);
    /* see if we found any values */
    if ((values==NULL)||(*values==NULL))
      break;
    /* allocate memory */
    if (set==NULL)
    {
      set=set_new();
      if (set==NULL)
      {
        ldap_value_free(values);
        log_log(LOG_CRIT,"myldap_get_ranged_values(): set_new() failed to allocate memory");
        return NULL;
      }
    }
    /* add to the set */
    for (i=0;values[i]!=NULL;i++)
      set_add(set,values[i]);
    /* free results */
    ldap_value_free(values);
    /* check if we should start a new search */
    if (nxt<=startat)
      break;
    startat=nxt;
    /* build attributes for a new search */
    if (mysnprintf(attbuf,sizeof(attbuf),"%s;range=%d-*",attr,startat))
      break;
    attrs[0]=attbuf;
    attrs[1]=NULL;
    /* close the previous search, if any */
    if (search!=NULL)
      myldap_search_close(search);
    /* start the new search */
    search=myldap_search(session,dn,LDAP_SCOPE_BASE,"(objectClass=*)",attrs);
    if (search==NULL)
      break;
    entry=myldap_get_entry(search,NULL);
    if (entry==NULL)
      break;
  }
  /* close any started searches */
  if (search!=NULL)
    myldap_search_close(search);
  return set2values(set);
}

/* Simple wrapper around ldap_get_values(). */
const char **myldap_get_values(MYLDAP_ENTRY *entry,const char *attr)
{
  char **values;
  int rc;
  int i;
  /* check parameters */
  if (!is_valid_entry(entry))
  {
    log_log(LOG_ERR,"myldap_get_values(): invalid result entry passed");
    errno=EINVAL;
    return NULL;
  }
  else if (attr==NULL)
  {
    log_log(LOG_ERR,"myldap_get_values(): invalid attribute name passed");
    errno=EINVAL;
    return NULL;
  }
  if (!entry->search->valid)
    return NULL; /* search has been stopped */
  /* get from LDAP */
  values=ldap_get_values(entry->search->session->ld,entry->search->msg,attr);
  if (values==NULL)
  {
    if (ldap_get_option(entry->search->session->ld,LDAP_OPT_ERROR_NUMBER,&rc)!=LDAP_SUCCESS)
      rc=LDAP_UNAVAILABLE;
    /* ignore decoding errors as they are just nonexisting attribute values */
    if (rc==LDAP_DECODING_ERROR)
    {
      rc=LDAP_SUCCESS;
      ldap_set_option(entry->search->session->ld,LDAP_OPT_ERROR_NUMBER,&rc);
    }
    else if (rc==LDAP_SUCCESS)
    {
      /* we have a success code but no values, let's try to get ranged
         values */
      values=myldap_get_ranged_values(entry,attr);
      if (values==NULL)
        return NULL;
      /* store values entry so we can free it later on */
      for (i=0;i<MAX_RANGED_ATTRIBUTES_PER_ENTRY;i++)
        if (entry->rangedattributevalues[i]==NULL)
        {
          entry->rangedattributevalues[i]=values;
          return (const char **)values;
        }
      /* we found no room to store the values */
      log_log(LOG_ERR,"ldap_get_values() couldn't store results, increase MAX_RANGED_ATTRIBUTES_PER_ENTRY");
      free(values);
      return NULL;
    }
    else
      log_log(LOG_WARNING,"ldap_get_values() of attribute \"%s\" on entry \"%s\" returned NULL: %s",
                          attr,myldap_get_dn(entry),ldap_err2string(rc));
    return NULL;
  }
  /* store values entry so we can free it later on */
  for (i=0;i<MAX_ATTRIBUTES_PER_ENTRY;i++)
    if (entry->attributevalues[i]==NULL)
    {
      entry->attributevalues[i]=values;
      return (const char **)values;
    }
  /* we found no room to store the entry */
  log_log(LOG_ERR,"ldap_get_values() couldn't store results, increase MAX_ATTRIBUTES_PER_ENTRY");
  ldap_value_free(values);
  return NULL;
}

/* Go over the entries in exploded_rdn and see if any start with
   the requested attribute. Return a reference to the value part of
   the DN (does not modify exploded_rdn). */
static const char *find_rdn_value(char **exploded_rdn,const char *attr)
{
  int i,j;
  int l;
  if (exploded_rdn==NULL)
    return NULL;
  /* go over all RDNs */
  l=strlen(attr);
  for (i=0;exploded_rdn[i]!=NULL;i++)
  {
    /* check that RDN starts with attr */
    if (strncasecmp(exploded_rdn[i],attr,l)!=0)
      continue;
    /* skip spaces */
    for (j=l;isspace(exploded_rdn[i][j]);j++)
      /* nothing here */;
    /* ensure that we found an equals sign now */
    if (exploded_rdn[i][j]!='=')
    j++;
    /* skip more spaces */
    for (j++;isspace(exploded_rdn[i][j]);j++)
      /* nothing here */;
    /* ensure that we're not at the end of the string */
    if (exploded_rdn[i][j]=='\0')
      continue;
    /* we found our value */
    return exploded_rdn[i]+j;
  }
  /* fail */
  return NULL;
}

/* explode the first part of DN into parts
   (e.g. "cn=Test", "uid=test")
   The returned value should be freed with ldap_value_free(). */
static char **get_exploded_rdn(const char *dn)
{
  char **exploded_dn;
  char **exploded_rdn;
  /* check if we have a DN */
  if ((dn==NULL)||(strcasecmp(dn,"unknown")==0))
    return NULL;
  /* explode dn into { "uid=test", "ou=people", ..., NULL } */
  exploded_dn=ldap_explode_dn(dn,0);
  if ((exploded_dn==NULL)||(exploded_dn[0]==NULL))
  {
    log_log(LOG_WARNING,"ldap_explode_dn(%s) returned NULL: %s",
                        dn,strerror(errno));
    return NULL;
  }
  /* explode rdn (first part of exploded_dn),
      e.g. "cn=Test User+uid=testusr" into
     { "cn=Test User", "uid=testusr", NULL } */
  exploded_rdn=ldap_explode_rdn(exploded_dn[0],0);
  if ((exploded_rdn==NULL)||(exploded_rdn[0]==NULL))
  {
    log_log(LOG_WARNING,"ldap_explode_rdn(%s) returned NULL: %s",
                        exploded_dn[0],strerror(errno));
    if (exploded_rdn!=NULL)
      ldap_value_free(exploded_rdn);
    ldap_value_free(exploded_dn);
    return NULL;
  }
  ldap_value_free(exploded_dn);
  return exploded_rdn;
}

const char *myldap_get_rdn_value(MYLDAP_ENTRY *entry,const char *attr)
{
  /* check parameters */
  if (!is_valid_entry(entry))
  {
    log_log(LOG_ERR,"myldap_get_rdn_value(): invalid result entry passed");
    errno=EINVAL;
    return NULL;
  }
  else if (attr==NULL)
  {
    log_log(LOG_ERR,"myldap_get_rdn_value(): invalid attribute name passed");
    errno=EINVAL;
    return NULL;
  }
  /* check if entry contains exploded_rdn */
  if (entry->exploded_rdn==NULL)
  {
    entry->exploded_rdn=get_exploded_rdn(myldap_get_dn(entry));
    if (entry->exploded_rdn==NULL)
      return NULL;
  }
  /* find rnd value */
  return find_rdn_value(entry->exploded_rdn,attr);
}

const char *myldap_cpy_rdn_value(const char *dn,const char *attr,
                                 char *buf,size_t buflen)
{
  char **exploded_rdn;
  const char *value;
  /* explode dn into { "cn=Test", "uid=test", NULL } */
  exploded_rdn=get_exploded_rdn(dn);
  if (exploded_rdn==NULL)
    return NULL;
  /* see if we have a match */
  value=find_rdn_value(exploded_rdn,attr);
  /* if we have something store it in the buffer */
  if ((value!=NULL)&&(strlen(value)<buflen))
    strcpy(buf,value);
  else
    value=NULL;
  /* free allocated stuff */
  ldap_value_free(exploded_rdn);
  /* check if we have something to return */
  return (value!=NULL)?buf:NULL;
}

int myldap_has_objectclass(MYLDAP_ENTRY *entry,const char *objectclass)
{
  const char **values;
  int i;
  if ((!is_valid_entry(entry))||(objectclass==NULL))
  {
    log_log(LOG_ERR,"myldap_has_objectclass(): invalid argument passed");
    errno=EINVAL;
    return 0;
  }
  values=myldap_get_values(entry,"objectClass");
  if (values==NULL)
    return 0;
  for (i=0;values[i]!=NULL;i++)
  {
    if (strcasecmp(values[i],objectclass)==0)
      return -1;
  }
  return 0;
}

int myldap_escape(const char *src,char *buffer,size_t buflen)
{
  size_t pos=0;
  /* go over all characters in source string */
  for (;*src!='\0';src++)
  {
    /* check if char will fit */
    if (pos>=(buflen+4))
      return -1;
    /* do escaping for some characters */
    switch (*src)
    {
      case '*':
        strcpy(buffer+pos,"\\2a");
        pos+=3;
        break;
      case '(':
        strcpy(buffer+pos,"\\28");
        pos+=3;
        break;
      case ')':
        strcpy(buffer+pos,"\\29");
        pos+=3;
        break;
      case '\\':
        strcpy(buffer+pos,"\\5c");
        pos+=3;
        break;
      default:
        /* just copy character */
        buffer[pos++]=*src;
        break;
    }
  }
  /* terminate destination string */
  buffer[pos]='\0';
  return 0;
}
