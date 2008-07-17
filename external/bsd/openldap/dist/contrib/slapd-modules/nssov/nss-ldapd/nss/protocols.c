/*
   protocols.c - NSS lookup functions for protocol database

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

static enum nss_status read_protoent(
        TFILE *fp,struct protoent *result,
        char *buffer,size_t buflen,int *errnop)
{
  int32_t tmpint32,tmp2int32,tmp3int32;
  size_t bufptr=0;
  READ_STRING_BUF(fp,result->p_name);
  READ_STRINGLIST_NULLTERM(fp,result->p_aliases);
  READ_INT32(fp,result->p_proto);
  return NSS_STATUS_SUCCESS;
}

enum nss_status _nss_ldap_getprotobyname_r(const char *name,struct protoent *result,char *buffer,size_t buflen,int *errnop)
{
  NSS_BYNAME(NSLCD_ACTION_PROTOCOL_BYNAME,
             name,
             read_protoent(fp,result,buffer,buflen,errnop));
}

enum nss_status _nss_ldap_getprotobynumber_r(int number,struct protoent *result,char *buffer,size_t buflen,int *errnop)
{
  NSS_BYINT32(NSLCD_ACTION_PROTOCOL_BYNUMBER,
              number,
              read_protoent(fp,result,buffer,buflen,errnop));
}

/* thread-local file pointer to an ongoing request */
static __thread TFILE *protoentfp;

enum nss_status _nss_ldap_setprotoent(int UNUSED(stayopen))
{
  NSS_SETENT(protoentfp);
}

enum nss_status _nss_ldap_getprotoent_r(struct protoent *result,char *buffer,size_t buflen,int *errnop)
{
  NSS_GETENT(protoentfp,NSLCD_ACTION_PROTOCOL_ALL,
             read_protoent(protoentfp,result,buffer,buflen,errnop));
}

enum nss_status _nss_ldap_endprotoent(void)
{
  NSS_ENDENT(protoentfp);
}
