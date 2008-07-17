/*
   passwd.c - password entry lookup routines
   Parts of this file were part of the nss_ldap library (as ldap-pwd.c)
   which has been forked into the nss-ldapd library.

   Copyright (C) 1997-2005 Luke Howard
   Copyright (C) 2006 West Consulting
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>

#include "common.h"
#include "log.h"
#include "myldap.h"
#include "cfg.h"
#include "attmap.h"
#include "common/dict.h"

/* ( nisSchema.2.0 NAME 'posixAccount' SUP top AUXILIARY
 *   DESC 'Abstraction of an account with POSIX attributes'
 *   MUST ( cn $ uid $ uidNumber $ gidNumber $ homeDirectory )
 *   MAY ( userPassword $ loginShell $ gecos $ description ) )
 */

/* the search base for searches */
const char *passwd_base = NULL;

/* the search scope for searches */
int passwd_scope = LDAP_SCOPE_DEFAULT;

/* the basic search filter for searches */
const char *passwd_filter = "(objectClass=posixAccount)";

/* the attributes used in searches */
const char *attmap_passwd_uid           = "uid";
const char *attmap_passwd_userPassword  = "userPassword";
const char *attmap_passwd_uidNumber     = "uidNumber";
const char *attmap_passwd_gidNumber     = "gidNumber";
const char *attmap_passwd_gecos         = "gecos";
const char *attmap_passwd_cn            = "cn";
const char *attmap_passwd_homeDirectory = "homeDirectory";
const char *attmap_passwd_loginShell    = "loginShell";

/* default values for attributes */
static const char *default_passwd_userPassword     = "*"; /* unmatchable */
static const char *default_passwd_homeDirectory    = "";
static const char *default_passwd_loginShell       = "";

/* the attribute list to request with searches */
static const char *passwd_attrs[10];

/* create a search filter for searching a passwd entry
   by name, return -1 on errors */
static int mkfilter_passwd_byname(const char *name,
                                  char *buffer,size_t buflen)
{
  char buf2[1024];
  /* escape attribute */
  if(myldap_escape(name,buf2,sizeof(buf2)))
    return -1;
  /* build filter */
  return mysnprintf(buffer,buflen,
                    "(&%s(%s=%s))",
                    passwd_filter,
                    attmap_passwd_uid,buf2);
}

/* create a search filter for searching a passwd entry
   by uid, return -1 on errors */
static int mkfilter_passwd_byuid(uid_t uid,
                                 char *buffer,size_t buflen)
{
  return mysnprintf(buffer,buflen,
                    "(&%s(%s=%d))",
                    passwd_filter,
                    attmap_passwd_uidNumber,uid);
}

static void passwd_init(void)
{
  /* set up base */
  if (passwd_base==NULL)
    passwd_base=nslcd_cfg->ldc_base;
  /* set up scope */
  if (passwd_scope==LDAP_SCOPE_DEFAULT)
    passwd_scope=nslcd_cfg->ldc_scope;
  /* set up attribute list */
  passwd_attrs[0]=attmap_passwd_uid;
  passwd_attrs[1]=attmap_passwd_userPassword;
  passwd_attrs[2]=attmap_passwd_uidNumber;
  passwd_attrs[3]=attmap_passwd_gidNumber;
  passwd_attrs[4]=attmap_passwd_cn;
  passwd_attrs[5]=attmap_passwd_homeDirectory;
  passwd_attrs[6]=attmap_passwd_loginShell;
  passwd_attrs[7]=attmap_passwd_gecos;
  passwd_attrs[8]="objectClass";
  passwd_attrs[9]=NULL;
}

/*
   Checks to see if the specified name is a valid user name.

   This test is based on the definition from POSIX (IEEE Std 1003.1, 2004, 3.426 User Name
   and 3.276 Portable Filename Character Set):
   http://www.opengroup.org/onlinepubs/009695399/basedefs/xbd_chap03.html#tag_03_426
   http://www.opengroup.org/onlinepubs/009695399/basedefs/xbd_chap03.html#tag_03_276

   The standard defines user names valid if they contain characters from
   the set [A-Za-z0-9._-] where the hyphen should not be used as first
   character. As an extension this test allows the dolar '$' sign as the last
   character to support Samba special accounts.
*/
int isvalidusername(const char *name)
{
  int i;
  if ((name==NULL)||(name[0]=='\0'))
    return 0;
  /* check first character */
  if ( ! ( (name[0]>='A' && name[0] <= 'Z') ||
           (name[0]>='a' && name[0] <= 'z') ||
           (name[0]>='0' && name[0] <= '9') ||
           name[0]=='.' || name[0]=='_' ) )
    return 0;
  /* check other characters */
  for (i=1;name[i]!='\0';i++)
  {
    if ( name[i]=='$' )
    {
      /* if the char is $ we require it to be the last char */
      if (name[i+1]!='\0')
        return 0;
    }
    else if ( ! ( (name[i]>='A' && name[i] <= 'Z') ||
                  (name[i]>='a' && name[i] <= 'z') ||
                  (name[i]>='0' && name[i] <= '9') ||
                  name[i]=='.' || name[i]=='_'  || name[i]=='-') )
      return 0;
  }
  /* no test failed so it must be good */
  return -1;
}

/* the cache that is used in dn2uid() */
static pthread_mutex_t dn2uid_cache_mutex=PTHREAD_MUTEX_INITIALIZER;
static DICT *dn2uid_cache=NULL;
struct dn2uid_cache_entry
{
  time_t timestamp;
  char *uid;
};
#define DN2UID_CACHE_TIMEOUT (15*60)

/* Perform an LDAP lookup to translate the DN into a uid.
   This function either returns NULL or a strdup()ed string. */
static char *lookup_dn2uid(MYLDAP_SESSION *session,const char *dn)
{
  MYLDAP_SEARCH *search;
  MYLDAP_ENTRY *entry;
  static const char *attrs[2];
  int rc;
  const char **values;
  char *uid;
  /* we have to look up the entry */
  attrs[0]=attmap_passwd_uid;
  attrs[1]=NULL;
  search=myldap_search(session,dn,LDAP_SCOPE_BASE,passwd_filter,attrs);
  if (search==NULL)
  {
    log_log(LOG_WARNING,"lookup of user %s failed",dn);
    return NULL;
  }
  entry=myldap_get_entry(search,&rc);
  if (entry==NULL)
  {
    if (rc!=LDAP_SUCCESS)
      log_log(LOG_WARNING,"lookup of user %s failed: %s",dn,ldap_err2string(rc));
    return NULL;
  }
  /* get uid (just use first one) */
  values=myldap_get_values(entry,attmap_passwd_uid);
  /* check the result for presence and validity */
  if ((values!=NULL)&&(values[0]!=NULL)&&isvalidusername(values[0]))
    uid=strdup(values[0]);
  else
    uid=NULL;
  myldap_search_close(search);
  return uid;
}

char *dn2uid(MYLDAP_SESSION *session,const char *dn,char *buf,size_t buflen)
{
  struct dn2uid_cache_entry *cacheentry=NULL;
  char *uid;
  /* check for empty string */
  if ((dn==NULL)||(*dn=='\0'))
    return NULL;
  /* try to look up uid within DN string */
  if (myldap_cpy_rdn_value(dn,attmap_passwd_uid,buf,buflen)!=NULL)
  {
    /* check if it is valid */
    if (!isvalidusername(buf))
      return NULL;
    return buf;
  }
  /* see if we have a cached entry */
  pthread_mutex_lock(&dn2uid_cache_mutex);
  if (dn2uid_cache==NULL)
    dn2uid_cache=dict_new();
  if ((dn2uid_cache!=NULL) && ((cacheentry=dict_get(dn2uid_cache,dn))!=NULL))
  {
    /* if the cached entry is still valid, return that */
    if (time(NULL) < (cacheentry->timestamp+DN2UID_CACHE_TIMEOUT))
    {
      if ((cacheentry->uid!=NULL)&&(strlen(cacheentry->uid)<buflen))
        strcpy(buf,cacheentry->uid);
      else
        buf=NULL;
      pthread_mutex_unlock(&dn2uid_cache_mutex);
      return buf;
    }
    /* leave the entry intact, just replace the uid below */
  }
  pthread_mutex_unlock(&dn2uid_cache_mutex);
  /* look up the uid using an LDAP query */
  uid=lookup_dn2uid(session,dn);
  /* store the result in the cache */
  pthread_mutex_lock(&dn2uid_cache_mutex);
  if (cacheentry==NULL)
  {
    /* allocate a new entry in the cache */
    cacheentry=(struct dn2uid_cache_entry *)malloc(sizeof(struct dn2uid_cache_entry));
    if (cacheentry!=NULL)
      dict_put(dn2uid_cache,dn,cacheentry);
  }
  else if (cacheentry->uid!=NULL)
    free(cacheentry->uid);
  /* update the cache entry */
  if (cacheentry!=NULL)
  {
    cacheentry->timestamp=time(NULL);
    cacheentry->uid=uid;
  }
  pthread_mutex_unlock(&dn2uid_cache_mutex);
  /* copy the result into the buffer */
  if ((uid!=NULL)&&(strlen(uid)<buflen))
    strcpy(buf,uid);
  else
    buf=NULL;
  return buf;
}

char *uid2dn(MYLDAP_SESSION *session,const char *uid,char *buf,size_t buflen)
{
  MYLDAP_SEARCH *search;
  MYLDAP_ENTRY *entry;
  static const char *attrs[1];
  int rc;
  const char *dn;
  char filter[1024];
  /* if it isn't a valid username, just bail out now */
  if (!isvalidusername(uid))
    return NULL;
  /* set up attributes (we don't care, we just want the DN) */
  attrs[0]=NULL;
  /* initialize default base, scrope, etc */
  passwd_init();
  /* we have to look up the entry */
  mkfilter_passwd_byname(uid,filter,sizeof(filter));
  search=myldap_search(session,passwd_base,passwd_scope,filter,attrs);
  if (search==NULL)
    return NULL;
  entry=myldap_get_entry(search,&rc);
  if (entry==NULL)
    return NULL;
  /* get DN */
  dn=myldap_get_dn(entry);
  if (strcasecmp(dn,"unknown")==0)
  {
    myldap_search_close(search);
    return NULL;
  }
  /* copy into buffer */
  if (strlen(dn)<buflen)
    strcpy(buf,dn);
  else
    buf=NULL;
  myldap_search_close(search);
  return buf;
}

/* the maximum number of uidNumber attributes per entry */
#define MAXUIDS_PER_ENTRY 5

static int write_passwd(TFILE *fp,MYLDAP_ENTRY *entry,const char *requser,
                        const uid_t *requid)
{
  int32_t tmpint32;
  const char *tmparr[2];
  const char **tmpvalues;
  char *tmp;
  const char **usernames;
  const char *passwd;
  uid_t uids[MAXUIDS_PER_ENTRY];
  int numuids;
  gid_t gid;
  const char *gecos;
  const char *homedir;
  const char *shell;
  int i,j;
  /* get the usernames for this entry */
  if (requser!=NULL)
  {
    usernames=tmparr;
    usernames[0]=requser;
    usernames[1]=NULL;
  }
  else
  {
    usernames=myldap_get_values(entry,attmap_passwd_uid);
    if ((usernames==NULL)||(usernames[0]==NULL))
    {
      log_log(LOG_WARNING,"passwd entry %s does not contain %s value",
                          myldap_get_dn(entry),attmap_passwd_uid);
      return 0;
    }
  }
  /* get the password for this entry */
  if (myldap_has_objectclass(entry,"shadowAccount"))
  {
    /* if the entry has a shadowAccount entry, point to that instead */
    passwd="x";
  }
  else
  {
    passwd=get_userpassword(entry,attmap_passwd_userPassword);
    if (passwd==NULL)
      passwd=default_passwd_userPassword;
  }
  /* get the uids for this entry */
  if (requid!=NULL)
  {
    uids[0]=*requid;
    numuids=1;
  }
  else
  {
    tmpvalues=myldap_get_values(entry,attmap_passwd_uidNumber);
    if ((tmpvalues==NULL)||(tmpvalues[0]==NULL))
    {
      log_log(LOG_WARNING,"passwd entry %s does not contain %s value",
                          myldap_get_dn(entry),attmap_passwd_uidNumber);
      return 0;
    }
    for (numuids=0;(numuids<=MAXUIDS_PER_ENTRY)&&(tmpvalues[numuids]!=NULL);numuids++)
    {
      uids[numuids]=(uid_t)strtol(tmpvalues[numuids],&tmp,0);
      if ((*(tmpvalues[numuids])=='\0')||(*tmp!='\0'))
      {
        log_log(LOG_WARNING,"passwd entry %s contains non-numeric %s value",
                            myldap_get_dn(entry),attmap_passwd_uidNumber);
        return 0;
      }
    }
  }
  /* get the gid for this entry */
  tmpvalues=myldap_get_values(entry,attmap_passwd_gidNumber);
  if ((tmpvalues==NULL)||(tmpvalues[0]==NULL))
  {
    log_log(LOG_WARNING,"passwd entry %s does not contain %s value",
                        myldap_get_dn(entry),attmap_passwd_gidNumber);
    return 0;
  }
  else if (tmpvalues[1]!=NULL)
  {
    log_log(LOG_WARNING,"passwd entry %s contains multiple %s values",
                        myldap_get_dn(entry),attmap_passwd_gidNumber);
  }
  gid=(gid_t)strtol(tmpvalues[0],&tmp,0);
  if ((*(tmpvalues[0])=='\0')||(*tmp!='\0'))
  {
    log_log(LOG_WARNING,"passwd entry %s contains non-numeric %s value",
                        myldap_get_dn(entry),attmap_passwd_gidNumber);
    return 0;
  }
  /* get the gecos for this entry (fall back to cn) */
  tmpvalues=myldap_get_values(entry,attmap_passwd_gecos);
  if ((tmpvalues==NULL)||(tmpvalues[0]==NULL))
    tmpvalues=myldap_get_values(entry,attmap_passwd_cn);
  if ((tmpvalues==NULL)||(tmpvalues[0]==NULL))
  {
    log_log(LOG_WARNING,"passwd entry %s does not contain %s or %s value",
                        myldap_get_dn(entry),attmap_passwd_gecos,attmap_passwd_cn);
    return 0;
  }
  else if (tmpvalues[1]!=NULL)
  {
    log_log(LOG_WARNING,"passwd entry %s contains multiple %s or %s values",
                        myldap_get_dn(entry),attmap_passwd_gecos,attmap_passwd_cn);
  }
  gecos=tmpvalues[0];
  /* get the home directory for this entry */
  tmpvalues=myldap_get_values(entry,attmap_passwd_homeDirectory);
  if ((tmpvalues==NULL)||(tmpvalues[0]==NULL))
  {
    log_log(LOG_WARNING,"passwd entry %s does not contain %s value",
                        myldap_get_dn(entry),attmap_passwd_homeDirectory);
    homedir=default_passwd_homeDirectory;
  }
  else
  {
    if (tmpvalues[1]!=NULL)
    {
      log_log(LOG_WARNING,"passwd entry %s contains multiple %s values",
                          myldap_get_dn(entry),attmap_passwd_homeDirectory);
    }
    homedir=tmpvalues[0];
    if (*homedir=='\0')
      homedir=default_passwd_homeDirectory;
  }
  /* get the shell for this entry */
  tmpvalues=myldap_get_values(entry,attmap_passwd_loginShell);
  if ((tmpvalues==NULL)||(tmpvalues[0]==NULL))
  {
    shell=default_passwd_loginShell;
  }
  else
  {
    if (tmpvalues[1]!=NULL)
    {
      log_log(LOG_WARNING,"passwd entry %s contains multiple %s values",
                          myldap_get_dn(entry),attmap_passwd_loginShell);
    }
    shell=tmpvalues[0];
    if (*shell=='\0')
      shell=default_passwd_loginShell;
  }
  /* write the entries */
  for (i=0;usernames[i]!=NULL;i++)
  {
    if (!isvalidusername(usernames[i]))
    {
      log_log(LOG_WARNING,"passwd entry %s contains invalid user name: \"%s\"",
                          myldap_get_dn(entry),usernames[i]);
    }
    else
    {
      for (j=0;j<numuids;j++)
      {
        WRITE_INT32(fp,NSLCD_RESULT_SUCCESS);
        WRITE_STRING(fp,usernames[i]);
        WRITE_STRING(fp,passwd);
        WRITE_TYPE(fp,uids[j],uid_t);
        WRITE_TYPE(fp,gid,gid_t);
        WRITE_STRING(fp,gecos);
        WRITE_STRING(fp,homedir);
        WRITE_STRING(fp,shell);
      }
    }
  }
  return 0;
}

NSLCD_HANDLE(
  passwd,byname,
  char name[256];
  char filter[1024];
  READ_STRING_BUF2(fp,name,sizeof(name));
  if (!isvalidusername(name)) {
    log_log(LOG_WARNING,"nslcd_passwd_byname(%s): invalid user name",name);
    return -1;
  },
  log_log(LOG_DEBUG,"nslcd_passwd_byname(%s)",name);,
  NSLCD_ACTION_PASSWD_BYNAME,
  mkfilter_passwd_byname(name,filter,sizeof(filter)),
  write_passwd(fp,entry,name,NULL)
)

NSLCD_HANDLE(
  passwd,byuid,
  uid_t uid;
  char filter[1024];
  READ_TYPE(fp,uid,uid_t);,
  log_log(LOG_DEBUG,"nslcd_passwd_byuid(%d)",(int)uid);,
  NSLCD_ACTION_PASSWD_BYUID,
  mkfilter_passwd_byuid(uid,filter,sizeof(filter)),
  write_passwd(fp,entry,NULL,&uid)
)

NSLCD_HANDLE(
  passwd,all,
  const char *filter;
  /* no parameters to read */,
  log_log(LOG_DEBUG,"nslcd_passwd_all()");,
  NSLCD_ACTION_PASSWD_ALL,
  (filter=passwd_filter,0),
  write_passwd(fp,entry,NULL,NULL)
)
