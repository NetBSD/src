/*
   service.c - NSS lookup functions for services database

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

#include <string.h>
#include <nss.h>
#include <errno.h>

#include "prototypes.h"
#include "common.h"
#include "compat/attrs.h"

static enum nss_status read_servent(
        TFILE *fp,struct servent *result,
        char *buffer,size_t buflen,int *errnop)
{
  int32_t tmpint32,tmp2int32,tmp3int32;
  size_t bufptr=0;
  READ_STRING_BUF(fp,result->s_name);
  READ_STRINGLIST_NULLTERM(fp,result->s_aliases);
  /* store port number in network byte order */
  READ_TYPE(fp,tmpint32,int32_t);
  result->s_port=ntohs((uint16_t)tmpint32);
  READ_STRING_BUF(fp,result->s_proto);
  /* we're done */
  return NSS_STATUS_SUCCESS;
}

enum nss_status _nss_ldap_getservbyname_r(const char *name,const char *protocol,struct servent *result,char *buffer,size_t buflen,int *errnop)
{
  NSS_BYGEN(NSLCD_ACTION_SERVICE_BYNAME,
            WRITE_STRING(fp,name);WRITE_STRING(fp,protocol),
            read_servent(fp,result,buffer,buflen,errnop));

}

enum nss_status _nss_ldap_getservbyport_r(int port,const char *protocol,struct servent *result,char *buffer,size_t buflen,int *errnop)
{
  NSS_BYGEN(NSLCD_ACTION_SERVICE_BYNUMBER,
            WRITE_INT32(fp,ntohs(port));WRITE_STRING(fp,protocol),
            read_servent(fp,result,buffer,buflen,errnop));
}

/* thread-local file pointer to an ongoing request */
static __thread TFILE *protoentfp;

enum nss_status _nss_ldap_setservent(int UNUSED(stayopen))
{
  NSS_SETENT(protoentfp);
}

enum nss_status _nss_ldap_getservent_r(struct servent *result,char *buffer,size_t buflen,int *errnop)
{
  NSS_GETENT(protoentfp,NSLCD_ACTION_SERVICE_ALL,
             read_servent(protoentfp,result,buffer,buflen,errnop));
}

enum nss_status _nss_ldap_endservent(void)
{
  NSS_ENDENT(protoentfp);
}
