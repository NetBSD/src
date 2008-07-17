/*
   host.c - host name lookup routines
   Parts of this file were part of the nss_ldap library (as ldap-hosts.c)
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
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "common.h"
#include "log.h"
#include "myldap.h"
#include "cfg.h"
#include "attmap.h"

/* ( nisSchema.2.6 NAME 'ipHost' SUP top AUXILIARY
 *   DESC 'Abstraction of a host, an IP device. The distinguished
 *         value of the cn attribute denotes the host's canonical
 *         name. Device SHOULD be used as a structural class'
 *   MUST ( cn $ ipHostNumber )
 *   MAY ( l $ description $ manager ) )
 */

/* the search base for searches */
const char *host_base = NULL;

/* the search scope for searches */
int host_scope = LDAP_SCOPE_DEFAULT;

/* the basic search filter for searches */
const char *host_filter = "(objectClass=ipHost)";

/* the attributes to request with searches */
const char *attmap_host_cn            = "cn";
const char *attmap_host_ipHostNumber  = "ipHostNumber";

/* the attribute list to request with searches */
static const char *host_attrs[3];

/* create a search filter for searching a host entry
   by name, return -1 on errors */
static int mkfilter_host_byname(const char *name,
                                char *buffer,size_t buflen)
{
  char buf2[1024];
  /* escape attribute */
  if (myldap_escape(name,buf2,sizeof(buf2)))
    return -1;
  /* build filter */
  return mysnprintf(buffer,buflen,
                    "(&%s(%s=%s))",
                    host_filter,
                    attmap_host_cn,buf2);
}

static int mkfilter_host_byaddr(const char *name,
                                char *buffer,size_t buflen)
{
  char buf2[1024];
  /* escape attribute */
  if (myldap_escape(name,buf2,sizeof(buf2)))
    return -1;
  /* build filter */
  return mysnprintf(buffer,buflen,
                    "(&%s(%s=%s))",
                    host_filter,
                    attmap_host_ipHostNumber,buf2);
}

static void host_init(void)
{
  /* set up base */
  if (host_base==NULL)
    host_base=nslcd_cfg->ldc_base;
  /* set up scope */
  if (host_scope==LDAP_SCOPE_DEFAULT)
    host_scope=nslcd_cfg->ldc_scope;
  /* set up attribute list */
  host_attrs[0]=attmap_host_cn;
  host_attrs[1]=attmap_host_ipHostNumber;
  host_attrs[2]=NULL;
}

/* write a single host entry to the stream */
static int write_host(TFILE *fp,MYLDAP_ENTRY *entry)
{
  int32_t tmpint32,tmp2int32,tmp3int32;
  int numaddr,i;
  const char *hostname;
  const char **hostnames;
  const char **addresses;
  /* get the most canonical name */
  hostname=myldap_get_rdn_value(entry,attmap_host_cn);
  /* get the other names for the host */
  hostnames=myldap_get_values(entry,attmap_host_cn);
  if ((hostnames==NULL)||(hostnames[0]==NULL))
  {
    log_log(LOG_WARNING,"host entry %s does not contain %s value",
                        myldap_get_dn(entry),attmap_host_cn);
    return 0;
  }
  /* if the hostname is not yet found, get the first entry from hostnames */
  if (hostname==NULL)
    hostname=hostnames[0];
  /* get the addresses */
  addresses=myldap_get_values(entry,attmap_host_ipHostNumber);
  if ((addresses==NULL)||(addresses[0]==NULL))
  {
    log_log(LOG_WARNING,"host entry %s does not contain %s value",
                        myldap_get_dn(entry),attmap_host_ipHostNumber);
    return 0;
  }
  /* write the entry */
  WRITE_INT32(fp,NSLCD_RESULT_SUCCESS);
  WRITE_STRING(fp,hostname);
  WRITE_STRINGLIST_EXCEPT(fp,hostnames,hostname);
  for (numaddr=0;addresses[numaddr]!=NULL;numaddr++)
    /*noting*/ ;
  WRITE_INT32(fp,numaddr);
  for (i=0;i<numaddr;i++)
  {
    WRITE_ADDRESS(fp,addresses[i]);
  }
  return 0;
}

NSLCD_HANDLE(
  host,byname,
  char name[256];
  char filter[1024];
  READ_STRING_BUF2(fp,name,sizeof(name));,
  log_log(LOG_DEBUG,"nslcd_host_byname(%s)",name);,
  NSLCD_ACTION_HOST_BYNAME,
  mkfilter_host_byname(name,filter,sizeof(filter)),
  write_host(fp,entry)
)

NSLCD_HANDLE(
  host,byaddr,
  int af;
  char addr[64];
  int len=sizeof(addr);
  char name[1024];
  char filter[1024];
  READ_ADDRESS(fp,addr,len,af);
  /* translate the address to a string */
  if (inet_ntop(af,addr,name,sizeof(name))==NULL)
  {
    log_log(LOG_WARNING,"unable to convert address to string");
    return -1;
  },
  log_log(LOG_DEBUG,"nslcd_host_byaddr(%s)",name);,
  NSLCD_ACTION_HOST_BYADDR,
  mkfilter_host_byaddr(name,filter,sizeof(filter)),
  write_host(fp,entry)
)

NSLCD_HANDLE(
  host,all,
  const char *filter;
  /* no parameters to read */,
  log_log(LOG_DEBUG,"nslcd_host_all()");,
  NSLCD_ACTION_HOST_ALL,
  (filter=host_filter,0),
  write_host(fp,entry)
)
