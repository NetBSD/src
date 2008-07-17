/*
   ether.c - ethernet address entry lookup routines
   Parts of this file were part of the nss_ldap library (as ldap-ethers.c)
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
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif /* HAVE_STDINT_H */

#include "common.h"
#include "log.h"
#include "myldap.h"
#include "cfg.h"
#include "attmap.h"
#include "compat/ether.h"

/* ( nisSchema.2.11 NAME 'ieee802Device' SUP top AUXILIARY
 *   DESC 'A device with a MAC address; device SHOULD be
 *         used as a structural class'
 *   MAY macAddress )
 */

/* the search base for searches */
const char *ether_base = NULL;

/* the search scope for searches */
int ether_scope = LDAP_SCOPE_DEFAULT;

/* the basic search filter for searches */
const char *ether_filter = "(objectClass=ieee802Device)";

/* the attributes to request with searches */
const char *attmap_ether_cn          = "cn";
const char *attmap_ether_macAddress  = "macAddress";

/* the attribute list to request with searches */
static const char *ether_attrs[3];

/* create a search filter for searching an ethernet address
   by name, return -1 on errors */
static int mkfilter_ether_byname(const char *name,
                                 char *buffer,size_t buflen)
{
  char buf2[1024];
  /* escape attribute */
  if(myldap_escape(name,buf2,sizeof(buf2)))
    return -1;
  /* build filter */
  return mysnprintf(buffer,buflen,
                   "(&%s(%s=%s))",
                   ether_filter,
                   attmap_ether_cn,buf2);
}

static int mkfilter_ether_byether(const struct ether_addr *addr,
                                  char *buffer,size_t buflen)
{
  char buf2[20];
  /* transform into string */
  if (ether_ntoa_r(addr,buf2)==NULL)
    return -1;
  /* FIXME: this has a bug when the directory has 01:00:0e:...
            and we're looking for 1:0:e:... (leading zeros) */
  /* there should be no characters that need escaping */
  /* build filter */
  return mysnprintf(buffer,buflen,
                   "(&%s(%s=%s))",
                   ether_filter,
                   attmap_ether_macAddress,buf2);
}

static void ether_init(void)
{
  /* set up base */
  if (ether_base==NULL)
    ether_base=nslcd_cfg->ldc_base;
  /* set up scope */
  if (ether_scope==LDAP_SCOPE_DEFAULT)
    ether_scope=nslcd_cfg->ldc_scope;
  /* set up attribute list */
  ether_attrs[0]=attmap_ether_cn;
  ether_attrs[1]=attmap_ether_macAddress;
  ether_attrs[2]=NULL;
}

/* TODO: check for errors in aton() */
#define WRITE_ETHER(fp,addr) \
  ether_aton_r(addr,&tmpaddr); \
  WRITE_TYPE(fp,tmpaddr,uint8_t[6]);

static int write_ether(TFILE *fp,MYLDAP_ENTRY *entry,
                       const char *reqname,const char *reqether)
{
  int32_t tmpint32;
  struct ether_addr tmpaddr;
  const char *tmparr[2];
  const char **names,**ethers;
  int i,j;
  /* get the name of the ether entry */
  if (reqname!=NULL)
  {
    names=tmparr;
    names[0]=reqname;
    names[1]=NULL;
  }
  else
  {
    names=myldap_get_values(entry,attmap_ether_cn);
    if ((names==NULL)||(names[0]==NULL))
    {
      log_log(LOG_WARNING,"ether entry %s does not contain %s value",
                          myldap_get_dn(entry),attmap_ether_cn);
      return 0;
    }
  }
  /* get the addresses */
  if (reqether!=NULL)
  {
    ethers=tmparr;
    ethers[0]=reqether;
    ethers[1]=NULL;
  }
  else
  {
    ethers=myldap_get_values(entry,attmap_ether_macAddress);
    if ((ethers==NULL)||(ethers[0]==NULL))
    {
      log_log(LOG_WARNING,"ether entry %s does not contain %s value",
                          myldap_get_dn(entry),attmap_ether_macAddress);
      return 0;
    }
    /* TODO: move parsing of addresses up here */
  }
  /* write entries for all names and addresses */
  for (i=0;names[i]!=NULL;i++)
    for (j=0;ethers[j]!=NULL;j++)
    {
      WRITE_INT32(fp,NSLCD_RESULT_SUCCESS);
      WRITE_STRING(fp,names[i]);
      WRITE_ETHER(fp,ethers[j]);
    }
  return 0;
}

NSLCD_HANDLE(
  ether,byname,
  char name[256];
  char filter[1024];
  READ_STRING_BUF2(fp,name,sizeof(name));,
  log_log(LOG_DEBUG,"nslcd_ether_byname(%s)",name);,
  NSLCD_ACTION_ETHER_BYNAME,
  mkfilter_ether_byname(name,filter,sizeof(filter)),
  write_ether(fp,entry,name,NULL)
)

NSLCD_HANDLE(
  ether,byether,
  struct ether_addr addr;
  char ether[20];
  char filter[1024];
  READ_TYPE(fp,addr,uint8_t[6]);
  if (ether_ntoa_r(&addr,ether)==NULL)
    return -1;,
  log_log(LOG_DEBUG,"nslcd_ether_byether(%s)",ether);,
  NSLCD_ACTION_ETHER_BYETHER,
  mkfilter_ether_byether(&addr,filter,sizeof(filter)),
  write_ether(fp,entry,NULL,ether)
)

NSLCD_HANDLE(
  ether,all,
  const char *filter;
  /* no parameters to read */,
  log_log(LOG_DEBUG,"nslcd_ether_all()");,
  NSLCD_ACTION_ETHER_ALL,
  (filter=ether_filter,0),
  write_ether(fp,entry,NULL,NULL)
)
