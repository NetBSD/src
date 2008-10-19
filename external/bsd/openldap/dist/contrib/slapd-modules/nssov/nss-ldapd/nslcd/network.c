/*
   network.c - network address entry lookup routines
   Parts of this file were part of the nss_ldap library (as ldap-network.c)
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

/* ( nisSchema.2.7 NAME 'ipNetwork' SUP top STRUCTURAL
 *   DESC 'Abstraction of a network. The distinguished value of
 *   MUST ( cn $ ipNetworkNumber )
 *   MAY ( ipNetmaskNumber $ l $ description $ manager ) )
 */

/* the search base for searches */
const char *network_base = NULL;

/* the search scope for searches */
int network_scope = LDAP_SCOPE_DEFAULT;

/* the basic search filter for searches */
const char *network_filter = "(objectClass=ipNetwork)";

/* the attributes used in searches */
const char *attmap_network_cn              = "cn";
const char *attmap_network_ipNetworkNumber = "ipNetworkNumber";
/*const char *attmap_network_ipNetmaskNumber = "ipNetmaskNumber"; */

/* the attribute list to request with searches */
static const char *network_attrs[3];

/* create a search filter for searching a network entry
   by name, return -1 on errors */
static int mkfilter_network_byname(const char *name,
                                   char *buffer,size_t buflen)
{
  char buf2[1024];
  /* escape attribute */
  if (myldap_escape(name,buf2,sizeof(buf2)))
    return -1;
  /* build filter */
  return mysnprintf(buffer,buflen,
                    "(&%s(%s=%s))",
                    network_filter,
                    attmap_network_cn,buf2);
}

static int mkfilter_network_byaddr(const char *name,
                                   char *buffer,size_t buflen)
{
  char buf2[1024];
  /* escape attribute */
  if (myldap_escape(name,buf2,sizeof(buf2)))
    return -1;
  /* build filter */
  return mysnprintf(buffer,buflen,
                    "(&%s(%s=%s))",
                    network_filter,
                    attmap_network_ipNetworkNumber,buf2);
}

static void network_init(void)
{
  /* set up base */
  if (network_base==NULL)
    network_base=nslcd_cfg->ldc_base;
  /* set up scope */
  if (network_scope==LDAP_SCOPE_DEFAULT)
    network_scope=nslcd_cfg->ldc_scope;
  /* set up attribute list */
  network_attrs[0]=attmap_network_cn;
  network_attrs[1]=attmap_network_ipNetworkNumber;
  network_attrs[2]=NULL;
}

/* write a single network entry to the stream */
static int write_network(TFILE *fp,MYLDAP_ENTRY *entry)
{
  int32_t tmpint32,tmp2int32,tmp3int32;
  int numaddr,i;
  const char *networkname;
  const char **networknames;
  const char **addresses;
  /* get the most canonical name */
  networkname=myldap_get_rdn_value(entry,attmap_network_cn);
  /* get the other names for the network */
  networknames=myldap_get_values(entry,attmap_network_cn);
  if ((networknames==NULL)||(networknames[0]==NULL))
  {
    log_log(LOG_WARNING,"network entry %s does not contain %s value",
                        myldap_get_dn(entry),attmap_network_cn);
    return 0;
  }
  /* if the networkname is not yet found, get the first entry from networknames */
  if (networkname==NULL)
    networkname=networknames[0];
  /* get the addresses */
  addresses=myldap_get_values(entry,attmap_network_ipNetworkNumber);
  if ((addresses==NULL)||(addresses[0]==NULL))
  {
    log_log(LOG_WARNING,"network entry %s does not contain %s value",
                        myldap_get_dn(entry),attmap_network_ipNetworkNumber);
    return 0;
  }
  /* write the entry */
  WRITE_INT32(fp,NSLCD_RESULT_SUCCESS);
  WRITE_STRING(fp,networkname);
  WRITE_STRINGLIST_EXCEPT(fp,networknames,networkname);
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
  network,byname,
  char name[256];
  char filter[1024];
  READ_STRING_BUF2(fp,name,sizeof(name));,
  log_log(LOG_DEBUG,"nslcd_network_byname(%s)",name);,
  NSLCD_ACTION_NETWORK_BYNAME,
  mkfilter_network_byname(name,filter,sizeof(filter)),
  write_network(fp,entry)
)

NSLCD_HANDLE(
  network,byaddr,
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
  log_log(LOG_DEBUG,"nslcd_network_byaddr(%s)",name);,
  NSLCD_ACTION_NETWORK_BYADDR,
  mkfilter_network_byaddr(name,filter,sizeof(filter)),
  write_network(fp,entry)
)

NSLCD_HANDLE(
  network,all,
  const char *filter;
  /* no parameters to read */,
  log_log(LOG_DEBUG,"nslcd_network_all()");,
  NSLCD_ACTION_NETWORK_ALL,
  (filter=network_filter,0),
  write_network(fp,entry)
)
