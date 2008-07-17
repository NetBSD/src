/*
   networks.c - NSS lookup functions for networks database

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

/* Redifine some ERROR_OUT macros as we also want to set h_errnop. */

#undef ERROR_OUT_OPENERROR
#define ERROR_OUT_OPENERROR \
  *errnop=ENOENT; \
  *h_errnop=HOST_NOT_FOUND; \
  return (errno==EAGAIN)?NSS_STATUS_TRYAGAIN:NSS_STATUS_UNAVAIL;

#undef ERROR_OUT_READERROR
#define ERROR_OUT_READERROR(fp) \
  (void)tio_close(fp); \
  fp=NULL; \
  *errnop=ENOENT; \
  *h_errnop=NO_RECOVERY; \
  return NSS_STATUS_UNAVAIL;

#undef ERROR_OUT_BUFERROR
#define ERROR_OUT_BUFERROR(fp) \
  (void)tio_close(fp); \
  fp=NULL; \
  *errnop=ERANGE; \
  *h_errnop=TRY_AGAIN; \
  return NSS_STATUS_TRYAGAIN;

#undef ERROR_OUT_WRITEERROR
#define ERROR_OUT_WRITEERROR(fp) \
  ERROR_OUT_READERROR(fp)

/* read a single network entry from the stream, ignoring entries
   that are not AF_INET (IPv4), result is stored in result */
static enum nss_status read_netent(
        TFILE *fp,struct netent *result,
        char *buffer,size_t buflen,int *errnop,int *h_errnop)
{
  int32_t tmpint32,tmp2int32,tmp3int32;
  int32_t numaddr,i;
  int readaf;
  size_t bufptr=0;
  enum nss_status retv=NSS_STATUS_NOTFOUND;
  /* read the network entry */
  READ_STRING_BUF(fp,result->n_name);
  READ_STRINGLIST_NULLTERM(fp,result->n_aliases);
  result->n_addrtype=AF_INET;
  /* read number of addresses to follow */
  READ_TYPE(fp,numaddr,int32_t);
  /* go through the address list and filter on af */
  i=0;
  while (--numaddr>=0)
  {
    /* read address family and size */
    READ_INT32(fp,readaf);
    READ_INT32(fp,tmp2int32);
    if ((readaf==AF_INET)&&(tmp2int32==4))
    {
      /* read address and translate to host byte order */
      READ_TYPE(fp,tmpint32,int32_t);
      result->n_net=ntohl((uint32_t)tmpint32);
      /* signal that we've read a proper entry */
      retv=NSS_STATUS_SUCCESS;
      /* don't return here to not upset the stream */
    }
    else
    {
      /* skip unsupported address families */
      SKIP(fp,tmpint32);
    }
  }
  return retv;
}

enum nss_status _nss_ldap_getnetbyname_r(const char *name,struct netent *result,char *buffer,size_t buflen,int *errnop,int *h_errnop)
{
  NSS_BYNAME(NSLCD_ACTION_NETWORK_BYNAME,
             name,
             read_netent(fp,result,buffer,buflen,errnop,h_errnop));
}

/* write an address value */
#define WRITE_ADDRESS(fp,af,len,addr) \
  WRITE_INT32(fp,af); \
  WRITE_INT32(fp,len); \
  WRITE(fp,addr,len);

/* Note: the af parameter is ignored and is assumed to be AF_INET */
/* TODO: implement handling of af parameter */
enum nss_status _nss_ldap_getnetbyaddr_r(uint32_t addr,int UNUSED(af),struct netent *result,char *buffer,size_t buflen,int *errnop,int *h_errnop)
{
  NSS_BYGEN(NSLCD_ACTION_NETWORK_BYADDR,
            WRITE_ADDRESS(fp,AF_INET,4,&addr),
            read_netent(fp,result,buffer,buflen,errnop,h_errnop))
}

/* thread-local file pointer to an ongoing request */
static __thread TFILE *netentfp;

enum nss_status _nss_ldap_setnetent(int UNUSED(stayopen))
{
  NSS_SETENT(netentfp);
}

enum nss_status _nss_ldap_getnetent_r(struct netent *result,char *buffer,size_t buflen,int *errnop,int *h_errnop)
{
  NSS_GETENT(netentfp,NSLCD_ACTION_NETWORK_ALL,
             read_netent(netentfp,result,buffer,buflen,errnop,h_errnop));
}

enum nss_status _nss_ldap_endnetent(void)
{
  NSS_ENDENT(netentfp);
}
