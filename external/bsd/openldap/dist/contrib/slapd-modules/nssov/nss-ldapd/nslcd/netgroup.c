/*
   netgroup.c - netgroup lookup routines
   Parts of this file were part of the nss_ldap library (as ldap-netgrp.c)
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
#include <stdarg.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/param.h>
#include <string.h>

#include "common.h"
#include "log.h"
#include "myldap.h"
#include "cfg.h"
#include "attmap.h"

/* ( nisSchema.2.8 NAME 'nisNetgroup' SUP top STRUCTURAL
 *   DESC 'Abstraction of a netgroup. May refer to other netgroups'
 *   MUST cn
 *   MAY ( nisNetgroupTriple $ memberNisNetgroup $ description ) )
 */

/* the search base for searches */
const char *netgroup_base = NULL;

/* the search scope for searches */
int netgroup_scope = LDAP_SCOPE_DEFAULT;

/* the basic search filter for searches */
const char *netgroup_filter = "(objectClass=nisNetgroup)";

/* the attributes to request with searches */
const char *attmap_netgroup_cn              = "cn";
const char *attmap_netgroup_nisNetgroupTriple = "nisNetgroupTriple";
const char *attmap_netgroup_memberNisNetgroup = "memberNisNetgroup";

/* the attribute list to request with searches */
static const char *netgroup_attrs[4];

static int mkfilter_netgroup_byname(const char *name,
                                    char *buffer,size_t buflen)
{
  char buf2[1024];
  /* escape attribute */
  if (myldap_escape(name,buf2,sizeof(buf2)))
    return -1;
  /* build filter */
  return mysnprintf(buffer,buflen,
                    "(&%s(%s=%s))",
                    netgroup_filter,
                    attmap_netgroup_cn,buf2);
}

static void netgroup_init(void)
{
  /* set up base */
  if (netgroup_base==NULL)
    netgroup_base=nslcd_cfg->ldc_base;
  /* set up scope */
  if (netgroup_scope==LDAP_SCOPE_DEFAULT)
    netgroup_scope=nslcd_cfg->ldc_scope;
  /* set up attribute list */
  netgroup_attrs[0]=attmap_netgroup_cn;
  netgroup_attrs[1]=attmap_netgroup_nisNetgroupTriple;
  netgroup_attrs[2]=attmap_netgroup_memberNisNetgroup;
  netgroup_attrs[3]=NULL;
}

static int write_string_stripspace_len(TFILE *fp,const char *str,int len)
{
  int32_t tmpint32;
  int i,j;
  DEBUG_PRINT("WRITE_STRING: var="__STRING(str)" string=\"%s\"",str);
  if (str==NULL)
  {
    WRITE_INT32(fp,0);
  }
  else
  {
    /* skip leading spaces */
    for (i=0;(str[i]!='\0')&&(isspace(str[i]));i++)
      /* nothing else to do */ ;
    /* skip trailing spaces */
    for (j=len;(j>i)&&(isspace(str[j-1]));j--)
      /* nothing else to do */ ;
    /* write length of string */
    WRITE_INT32(fp,j-i);
    /* write string itself */
    if (j>i)
    {
      WRITE(fp,str+i,j-i);
    }
  }
  /* we're done */
  return 0;
}

#define WRITE_STRING_STRIPSPACE_LEN(fp,str,len) \
  if (write_string_stripspace_len(fp,str,len)) \
    return -1;

#define WRITE_STRING_STRIPSPACE(fp,str) \
  WRITE_STRING_STRIPSPACE_LEN(fp,str,strlen(str))

static int write_netgroup_triple(TFILE *fp,const char *triple)
{
  int32_t tmpint32;
  int i;
  int hostb,hoste,userb,usere,domainb,domaine;
  /* skip leading spaces */
  for (i=0;(triple[i]!='\0')&&(isspace(triple[i]));i++)
    /* nothing else to do */ ;
  /* we should have a bracket now */
  if (triple[i]!='(')
  {
    log_log(LOG_WARNING,"write_netgroup_triple(): entry does not begin with '(' (entry skipped)");
    return 0;
  }
  i++;
  hostb=i;
  /* find comma (end of host string) */
  for (;(triple[i]!='\0')&&(triple[i]!=',');i++)
    /* nothing else to do */ ;
  if (triple[i]!=',')
  {
    log_log(LOG_WARNING,"write_netgroup_triple(): missing ',' (entry skipped)");
    return 0;
  }
  hoste=i;
  i++;
  userb=i;
  /* find comma (end of user string) */
  for (;(triple[i]!='\0')&&(triple[i]!=',');i++)
    /* nothing else to do */ ;
  if (triple[i]!=',')
  {
    log_log(LOG_WARNING,"write_netgroup_triple(): missing ',' (entry skipped)");
    return 0;
  }
  usere=i;
  i++;
  domainb=i;
  /* find closing bracket (end of domain string) */
  for (;(triple[i]!='\0')&&(triple[i]!=')');i++)
    /* nothing else to do */ ;
  if (triple[i]!=')')
  {
    log_log(LOG_WARNING,"write_netgroup_triple(): missing ')' (entry skipped)");
    return 0;
  }
  domaine=i;
  i++;
  /* skip trailing spaces */
  for (;(triple[i]!='\0')&&(isspace(triple[i]));i++)
    /* nothing else to do */ ;
  /* if anything is left in the string we have a problem */
  if (triple[i]!='\0')
  {
    log_log(LOG_WARNING,"write_netgroup_triple(): string contains trailing data (entry skipped)");
    return 0;
  }
  /* write strings */
  WRITE_INT32(fp,NSLCD_RESULT_SUCCESS);
  WRITE_INT32(fp,NETGROUP_TYPE_TRIPLE);
  WRITE_STRING_STRIPSPACE_LEN(fp,triple+hostb,hoste-hostb)
  WRITE_STRING_STRIPSPACE_LEN(fp,triple+userb,usere-userb)
  WRITE_STRING_STRIPSPACE_LEN(fp,triple+domainb,domaine-domainb)
  /* we're done */
  return 0;
}

#define WRITE_NETGROUP_TRIPLE(fp,triple) \
  if (write_netgroup_triple(fp,triple)) \
    return -1;

static int write_netgroup(TFILE *fp,MYLDAP_ENTRY *entry)
{
  int32_t tmpint32;
  int i;
  const char **triples;
  const char **members;
  /* get the netgroup triples and member */
  triples=myldap_get_values(entry,attmap_netgroup_nisNetgroupTriple);
  members=myldap_get_values(entry,attmap_netgroup_memberNisNetgroup);
  /* write the netgroup triples */
  if (triples!=NULL)
    for (i=0;triples[i]!=NULL;i++)
    {
      WRITE_NETGROUP_TRIPLE(fp,triples[i]);
    }
  /* write netgroup members */
  if (members!=NULL)
    for (i=0;members[i]!=NULL;i++)
    {
      /* write the result code */
      WRITE_INT32(fp,NSLCD_RESULT_SUCCESS);
      /* write triple indicator */
      WRITE_INT32(fp,NETGROUP_TYPE_NETGROUP);
      /* write netgroup name */
      WRITE_STRING_STRIPSPACE(fp,members[i]);
    }
  /* we're done */
  return 0;
}

NSLCD_HANDLE(
  netgroup,byname,
  char name[256];
  char filter[1024];
  READ_STRING_BUF2(fp,name,sizeof(name));,
  log_log(LOG_DEBUG,"nslcd_netgroup_byname(%s)",name);,
  NSLCD_ACTION_NETGROUP_BYNAME,
  mkfilter_netgroup_byname(name,filter,sizeof(filter)),
  write_netgroup(fp,entry)
)
