/*
   shadow.c - service entry lookup routines
   Parts of this file were part of the nss_ldap library (as ldap-spwd.c)
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
#include <string.h>

#include "common.h"
#include "log.h"
#include "myldap.h"
#include "cfg.h"
#include "attmap.h"

/* ( nisSchema.2.1 NAME 'shadowAccount' SUP top AUXILIARY
 *   DESC 'Additional attributes for shadow passwords'
 *   MUST uid
 *   MAY ( userPassword $ shadowLastChange $ shadowMin
 *         shadowMax $ shadowWarning $ shadowInactive $
 *         shadowExpire $ shadowFlag $ description ) )
 */

/* the search base for searches */
const char *shadow_base = NULL;

/* the search scope for searches */
int shadow_scope = LDAP_SCOPE_DEFAULT;

/* the basic search filter for searches */
const char *shadow_filter = "(objectClass=shadowAccount)";

/* the attributes to request with searches */
const char *attmap_shadow_uid              = "uid";
const char *attmap_shadow_userPassword     = "userPassword";
const char *attmap_shadow_shadowLastChange = "shadowLastChange";
const char *attmap_shadow_shadowMin        = "shadowMin";
const char *attmap_shadow_shadowMax        = "shadowMax";
const char *attmap_shadow_shadowWarning    = "shadowWarning";
const char *attmap_shadow_shadowInactive   = "shadowInactive";
const char *attmap_shadow_shadowExpire     = "shadowExpire";
const char *attmap_shadow_shadowFlag       = "shadowFlag";

/* default values for attributes */
static const char *default_shadow_userPassword     = "*"; /* unmatchable */
static const char *default_shadow_shadowLastChange = "-1";
static const char *default_shadow_shadowMin        = "-1";
static const char *default_shadow_shadowMax        = "-1";
static const char *default_shadow_shadowWarning    = "-1";
static const char *default_shadow_shadowInactive   = "-1";
static const char *default_shadow_shadowExpire     = "-1";
static const char *default_shadow_shadowFlag       = "0";

/* the attribute list to request with searches */
static const char *shadow_attrs[10];

static int mkfilter_shadow_byname(const char *name,
                                  char *buffer,size_t buflen)
{
  char buf2[1024];
  /* escape attribute */
  if(myldap_escape(name,buf2,sizeof(buf2)))
    return -1;
  /* build filter */
  return mysnprintf(buffer,buflen,
                    "(&%s(%s=%s))",
                    shadow_filter,
                    attmap_shadow_uid,buf2);
}

static void shadow_init(void)
{
  /* set up base */
  if (shadow_base==NULL)
    shadow_base=nslcd_cfg->ldc_base;
  /* set up scope */
  if (shadow_scope==LDAP_SCOPE_DEFAULT)
    shadow_scope=nslcd_cfg->ldc_scope;
  /* set up attribute list */
  shadow_attrs[0]=attmap_shadow_uid;
  shadow_attrs[1]=attmap_shadow_userPassword;
  shadow_attrs[2]=attmap_shadow_shadowLastChange;
  shadow_attrs[3]=attmap_shadow_shadowMax;
  shadow_attrs[4]=attmap_shadow_shadowMin;
  shadow_attrs[5]=attmap_shadow_shadowWarning;
  shadow_attrs[6]=attmap_shadow_shadowInactive;
  shadow_attrs[7]=attmap_shadow_shadowExpire;
  shadow_attrs[8]=attmap_shadow_shadowFlag;
  shadow_attrs[9]=NULL;
}

static long to_date(const char *date,const char *attr)
{
  char buffer[8];
  long value;
  char *tmp;
  size_t l;
  /* do some special handling for date values on AD */
  if (strcasecmp(attr,"pwdLastSet")==0)
  {
    /* we expect an AD 64-bit datetime value;
       we should do date=date/864000000000-134774
       but that causes problems on 32-bit platforms,
       first we devide by 1000000000 by stripping the
       last 9 digits from the string and going from there */
    l=strlen(date)-9;
    if (l>(sizeof(buffer)-1))
      return 0; /* error */
    strncpy(buffer,date,l);
    buffer[l]='\0';
    value=strtol(date,&tmp,0);
    if ((*date=='\0')||(*tmp!='\0'))
    {
      log_log(LOG_WARNING,"shadow entry contains non-numeric %s value",attr);
      return 0;
    }
    return value/864-134774;
    /* note that AD does not have expiry dates but a lastchangeddate
       and some value that needs to be added */
  }
  value=strtol(date,&tmp,0);
  if ((*date=='\0')||(*tmp!='\0'))
  {
    log_log(LOG_WARNING,"shadow entry contains non-numeric %s value",attr);
    return 0;
  }
  return value;
}

#ifndef UF_DONT_EXPIRE_PASSWD
#define UF_DONT_EXPIRE_PASSWD 0x10000
#endif

#define GET_OPTIONAL_LONG(var,att) \
  tmpvalues=myldap_get_values(entry,attmap_shadow_##att); \
  if ((tmpvalues==NULL)||(tmpvalues[0]==NULL)) \
    var=strtol(default_shadow_##att,NULL,0); \
  else \
  { \
    if (tmpvalues[1]!=NULL) \
    { \
      log_log(LOG_WARNING,"shadow entry %s contains multiple %s values", \
                          myldap_get_dn(entry),attmap_shadow_##att); \
    } \
    var=strtol(tmpvalues[0],&tmp,0); \
    if ((*(tmpvalues[0])=='\0')||(*tmp!='\0')) \
    { \
      log_log(LOG_WARNING,"shadow entry %s contains non-numeric %s value", \
                          myldap_get_dn(entry),attmap_shadow_##att); \
      return 0; \
    } \
  }

#define GET_OPTIONAL_DATE(var,att) \
  tmpvalues=myldap_get_values(entry,attmap_shadow_##att); \
  if ((tmpvalues==NULL)||(tmpvalues[0]==NULL)) \
    var=to_date(default_shadow_##att,attmap_shadow_##att); \
  else \
  { \
    if (tmpvalues[1]!=NULL) \
    { \
      log_log(LOG_WARNING,"shadow entry %s contains multiple %s values", \
                          myldap_get_dn(entry),attmap_shadow_##att); \
    } \
    var=to_date(tmpvalues[0],attmap_shadow_##att); \
  }

static int write_shadow(TFILE *fp,MYLDAP_ENTRY *entry,const char *requser)
{
  int32_t tmpint32;
  const char *tmparr[2];
  const char **tmpvalues;
  char *tmp;
  const char **usernames;
  const char *passwd;
  long lastchangedate;
  long mindays;
  long maxdays;
  long warndays;
  long inactdays;
  long expiredate;
  unsigned long flag;
  int i;
  /* get username */
  if (requser!=NULL)
  {
    usernames=tmparr;
    usernames[0]=requser;
    usernames[1]=NULL;
  }
  else
  {
    usernames=myldap_get_values(entry,attmap_shadow_uid);
    if ((usernames==NULL)||(usernames[0]==NULL))
    {
      log_log(LOG_WARNING,"passwd entry %s does not contain %s value",
                          myldap_get_dn(entry),attmap_shadow_uid);
      return 0;
    }
  }
  /* get password */
  passwd=get_userpassword(entry,attmap_shadow_userPassword);
  if (passwd==NULL)
    passwd=default_shadow_userPassword;
  /* get lastchange date */
  GET_OPTIONAL_DATE(lastchangedate,shadowLastChange);
  /* get mindays */
  GET_OPTIONAL_LONG(mindays,shadowMin);
  /* get maxdays */
  GET_OPTIONAL_LONG(maxdays,shadowMax);
  /* get warndays */
  GET_OPTIONAL_LONG(warndays,shadowWarning);
  /* get inactdays */
  GET_OPTIONAL_LONG(inactdays,shadowInactive);
  /* get expire date */
  GET_OPTIONAL_LONG(expiredate,shadowExpire);
  /* get flag */
  GET_OPTIONAL_LONG(flag,shadowFlag);
  /* if we're using AD handle the flag specially */
  if (strcasecmp(attmap_shadow_shadowLastChange,"pwdLastSet")==0)
  {
    if (flag&UF_DONT_EXPIRE_PASSWD)
      maxdays=99999;
    flag=0;
  }
  /* write the entries */
  for (i=0;usernames[i]!=NULL;i++)
  {
    WRITE_INT32(fp,NSLCD_RESULT_SUCCESS);
    WRITE_STRING(fp,usernames[i]);
    WRITE_STRING(fp,passwd);
    WRITE_INT32(fp,lastchangedate);
    WRITE_INT32(fp,mindays);
    WRITE_INT32(fp,maxdays);
    WRITE_INT32(fp,warndays);
    WRITE_INT32(fp,inactdays);
    WRITE_INT32(fp,expiredate);
    WRITE_INT32(fp,flag);
  }
  return 0;
}

NSLCD_HANDLE(
  shadow,byname,
  char name[256];
  char filter[1024];
  READ_STRING_BUF2(fp,name,sizeof(name));,
  log_log(LOG_DEBUG,"nslcd_shadow_byname(%s)",name);,
  NSLCD_ACTION_SHADOW_BYNAME,
  mkfilter_shadow_byname(name,filter,sizeof(filter)),
  write_shadow(fp,entry,name)
)

NSLCD_HANDLE(
  shadow,all,
  const char *filter;
  /* no parameters to read */,
  log_log(LOG_DEBUG,"nslcd_shadow_all()");,
  NSLCD_ACTION_SHADOW_ALL,
  (filter=shadow_filter,0),
  write_shadow(fp,entry,NULL)
)
