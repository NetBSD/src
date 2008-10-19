/*
   alias.c - alias entry lookup routines
   Parts of this file were part of the nss_ldap library (as ldap-alias.c)
   which has been forked into the nss-ldapd library.

   Copyright (C) 1997-2005 Luke Howard
   Copyright (C) 2006 West Consulting
   Copyright (C) 2006, 2007 Arthur de Jong

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

/* Vendor-specific attributes and object classes.
 * (Mainly from Sun.)
 * ( 1.3.6.1.4.1.42.2.27.1.2.5 NAME 'nisMailAlias' SUP top STRUCTURAL
 *   DESC 'NIS mail alias'
 *   MUST cn
 *   MAY rfc822MailMember )
 */

/* the search base for searches */
const char *alias_base = NULL;

/* the search scope for searches */
int alias_scope = LDAP_SCOPE_DEFAULT;

/* the basic search filter for searches */
const char *alias_filter = "(objectClass=nisMailAlias)";

/* the attributes to request with searches */
const char *attmap_alias_cn               = "cn";
const char *attmap_alias_rfc822MailMember = "rfc822MailMember";

/* the attribute list to request with searches */
static const char *alias_attrs[3];

/* create a search filter for searching an alias by name,
   return -1 on errors */
static int mkfilter_alias_byname(const char *name,
                                 char *buffer,size_t buflen)
{
  char buf2[1024];
  /* escape attribute */
  if (myldap_escape(name,buf2,sizeof(buf2)))
    return -1;
  /* build filter */
  return mysnprintf(buffer,buflen,
                    "(&%s(%s=%s))",
                    alias_filter,
                    attmap_alias_cn,buf2);
}

static void alias_init(void)
{
  /* set up base */
  if (alias_base==NULL)
    alias_base=nslcd_cfg->ldc_base;
  /* set up scope */
  if (alias_scope==LDAP_SCOPE_DEFAULT)
    alias_scope=nslcd_cfg->ldc_scope;
  /* set up attribute list */
  alias_attrs[0]=attmap_alias_cn;
  alias_attrs[1]=attmap_alias_rfc822MailMember;
  alias_attrs[2]=NULL;
}

static int write_alias(TFILE *fp,MYLDAP_ENTRY *entry,const char *reqalias)
{
  int32_t tmpint32,tmp2int32,tmp3int32;
  const char *tmparr[2];
  const char **names,**members;
  int i;
  /* get the name of the alias */
  if (reqalias!=NULL)
  {
    names=tmparr;
    names[0]=reqalias;
    names[1]=NULL;
  }
  else
  {
    names=myldap_get_values(entry,attmap_alias_cn);
    if ((names==NULL)||(names[0]==NULL))
    {
      log_log(LOG_WARNING,"alias entry %s does not contain %s value",
                          myldap_get_dn(entry),attmap_alias_cn);
      return 0;
    }
  }
  /* get the members of the alias */
  members=myldap_get_values(entry,attmap_alias_rfc822MailMember);
  /* for each name, write an entry */
  for (i=0;names[i]!=NULL;i++)
  {
    WRITE_INT32(fp,NSLCD_RESULT_SUCCESS);
    WRITE_STRING(fp,names[i]);
    WRITE_STRINGLIST(fp,members);
  }
  return 0;
}

NSLCD_HANDLE(
  alias,byname,
  char name[256];
  char filter[1024];
  READ_STRING_BUF2(fp,name,sizeof(name));,
  log_log(LOG_DEBUG,"nslcd_alias_byname(%s)",name);,
  NSLCD_ACTION_ALIAS_BYNAME,
  mkfilter_alias_byname(name,filter,sizeof(filter)),
  write_alias(fp,entry,name)
)

NSLCD_HANDLE(
  alias,all,
  const char *filter;
  /* no parameters to read */,
  log_log(LOG_DEBUG,"nslcd_alias_all()");,
  NSLCD_ACTION_ALIAS_ALL,
  (filter=alias_filter,0),
  write_alias(fp,entry,NULL)
)
