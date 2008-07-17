/*
   service.c - service entry lookup routines
   Parts of this file were part of the nss_ldap library (as ldap-service.c)
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

/* ( nisSchema.2.3 NAME 'ipService' SUP top STRUCTURAL
 *   DESC 'Abstraction an Internet Protocol service.
 *         Maps an IP port and protocol (such as tcp or udp)
 *         to one or more names; the distinguished value of
 *         the cn attribute denotes the service's canonical
 *         name'
 *   MUST ( cn $ ipServicePort $ ipServiceProtocol )
 *   MAY ( description ) )
 */

/* the search base for searches */
const char *service_base = NULL;

/* the search scope for searches */
int service_scope = LDAP_SCOPE_DEFAULT;

/* the basic search filter for searches */
const char *service_filter = "(objectClass=ipService)";

/* the attributes to request with searches */
const char *attmap_service_cn                = "cn";
const char *attmap_service_ipServicePort     = "ipServicePort";
const char *attmap_service_ipServiceProtocol = "ipServiceProtocol";

/* the attribute list to request with searches */
static const char *service_attrs[4];

static int mkfilter_service_byname(const char *name,
                                   const char *protocol,
                                   char *buffer,size_t buflen)
{
  char buf2[1024],buf3[1024];
  /* escape attributes */
  if (myldap_escape(name,buf2,sizeof(buf2)))
    return -1;
  if (*protocol!='\0')
    if (myldap_escape(protocol,buf3,sizeof(buf3)))
      return -1;
  /* build filter */
  if (*protocol!='\0')
    return mysnprintf(buffer,buflen,
                      "(&%s(%s=%s)(%s=%s))",
                      service_filter,
                      attmap_service_cn,buf2,
                      attmap_service_ipServiceProtocol,buf3);
  else
    return mysnprintf(buffer,buflen,
                      "(&%s(%s=%s))",
                      service_filter,
                      attmap_service_cn,buf2);
}

static int mkfilter_service_bynumber(int number,
                                     const char *protocol,
                                     char *buffer,size_t buflen)
{
  char buf3[1024];
  /* escape attribute */
  if (*protocol!='\0')
    if (myldap_escape(protocol,buf3,sizeof(buf3)))
      return -1;
  /* build filter */
  if (*protocol!='\0')
    return mysnprintf(buffer,buflen,
                      "(&%s(%s=%d)(%s=%s))",
                      service_filter,
                      attmap_service_ipServicePort,number,
                      attmap_service_ipServiceProtocol,buf3);
  else
    return mysnprintf(buffer,buflen,
                      "(&%s(%s=%d))",
                      service_filter,
                      attmap_service_ipServicePort,number);
}

static void service_init(void)
{
  /* set up base */
  if (service_base==NULL)
    service_base=nslcd_cfg->ldc_base;
  /* set up scope */
  if (service_scope==LDAP_SCOPE_DEFAULT)
    service_scope=nslcd_cfg->ldc_scope;
  /* set up attribute list */
  service_attrs[0]=attmap_service_cn;
  service_attrs[1]=attmap_service_ipServicePort;
  service_attrs[2]=attmap_service_ipServiceProtocol;
  service_attrs[3]=NULL;
}

static int write_service(TFILE *fp,MYLDAP_ENTRY *entry,const char *reqprotocol)
{
  int32_t tmpint32,tmp2int32,tmp3int32;
  const char *name;
  const char **aliases;
  const char **ports;
  const char **protocols;
  const char *tmparr[2];
  char *tmp;
  int port;
  int i;
  /* get the most canonical name */
  name=myldap_get_rdn_value(entry,attmap_service_cn);
  /* get the other names for the service entries */
  aliases=myldap_get_values(entry,attmap_service_cn);
  if ((aliases==NULL)||(aliases[0]==NULL))
  {
    log_log(LOG_WARNING,"service entry %s does not contain %s value",
                        myldap_get_dn(entry),attmap_service_cn);
    return 0;
  }
  /* if the service name is not yet found, get the first entry */
  if (name==NULL)
    name=aliases[0];
  /* get the service number */
  ports=myldap_get_values(entry,attmap_service_ipServicePort);
  if ((ports==NULL)||(ports[0]==NULL))
  {
    log_log(LOG_WARNING,"service entry %s does not contain %s value",
                        myldap_get_dn(entry),attmap_service_ipServicePort);
    return 0;
  }
  else if (ports[1]!=NULL)
  {
    log_log(LOG_WARNING,"service entry %s contains multiple %s values",
                        myldap_get_dn(entry),attmap_service_ipServicePort);
  }
  port=(int)strtol(ports[0],&tmp,0);
  if ((*(ports[0])=='\0')||(*tmp!='\0'))
  {
    log_log(LOG_WARNING,"service entry %s contains non-numeric %s value",
                        myldap_get_dn(entry),attmap_service_ipServicePort);
    return 0;
  }
  /* get protocols */
  if ((reqprotocol!=NULL)&&(*reqprotocol!='\0'))
  {
    protocols=tmparr;
    protocols[0]=reqprotocol;
    protocols[1]=NULL;
  }
  else
  {
    protocols=myldap_get_values(entry,attmap_service_ipServiceProtocol);
    if ((protocols==NULL)||(protocols[0]==NULL))
    {
      log_log(LOG_WARNING,"service entry %s does not contain %s value",
                          myldap_get_dn(entry),attmap_service_ipServiceProtocol);
      return 0;
    }
  }
  /* write the entries */
  for (i=0;protocols[i]!=NULL;i++)
  {
    WRITE_INT32(fp,NSLCD_RESULT_SUCCESS);
    WRITE_STRING(fp,name);
    WRITE_STRINGLIST_EXCEPT(fp,aliases,name);
    WRITE_INT32(fp,port);
    WRITE_STRING(fp,protocols[i]);
  }
  return 0;
}

NSLCD_HANDLE(
  service,byname,
  char name[256];
  char protocol[256];
  char filter[1024];
  READ_STRING_BUF2(fp,name,sizeof(name));
  READ_STRING_BUF2(fp,protocol,sizeof(protocol));,
  log_log(LOG_DEBUG,"nslcd_service_byname(%s,%s)",name,protocol);,
  NSLCD_ACTION_SERVICE_BYNAME,
  mkfilter_service_byname(name,protocol,filter,sizeof(filter)),
  write_service(fp,entry,protocol)
)

NSLCD_HANDLE(
  service,bynumber,
  int number;
  char protocol[256];
  char filter[1024];
  READ_INT32(fp,number);
  READ_STRING_BUF2(fp,protocol,sizeof(protocol));,
  log_log(LOG_DEBUG,"nslcd_service_bynumber(%d,%s)",number,protocol);,
  NSLCD_ACTION_SERVICE_BYNUMBER,
  mkfilter_service_bynumber(number,protocol,filter,sizeof(filter)),
  write_service(fp,entry,protocol)
)

NSLCD_HANDLE(
  service,all,
  const char *filter;
  /* no parameters to read */,
  log_log(LOG_DEBUG,"nslcd_service_all()");,
  NSLCD_ACTION_SERVICE_ALL,
  (filter=service_filter,0),
  write_service(fp,entry,NULL)
)
