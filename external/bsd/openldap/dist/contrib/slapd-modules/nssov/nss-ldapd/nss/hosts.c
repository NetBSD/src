/*
   hosts.c - NSS lookup functions for hosts database

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

/* read a single host entry from the stream, filtering on the
   specified address family, result is stored in result
   it will an empty entry if no addresses in the address family
   were available */
static enum nss_status read_hostent(
        TFILE *fp,int af,struct hostent *result,
        char *buffer,size_t buflen,int *errnop,int *h_errnop)
{
  int32_t tmpint32,tmp2int32,tmp3int32;
  int32_t numaddr;
  int i;
  int readaf;
  size_t bufptr=0;
  /* read the host entry */
  READ_STRING_BUF(fp,result->h_name);
  READ_STRINGLIST_NULLTERM(fp,result->h_aliases);
  result->h_addrtype=af;
  result->h_length=0;
  /* read number of addresses to follow */
  READ_INT32(fp,numaddr);
  /* allocate memory for array */
  /* Note: this may allocate too much memory (e.g. also for
           address records of other address families) but
           this is a simple way to do it */
  BUF_ALLOC(fp,result->h_addr_list,char *,numaddr+1);
  /* go through the address list and filter on af */
  i=0;
  while (--numaddr>=0)
  {
    /* read address family and size */
    READ_INT32(fp,readaf);
    READ_INT32(fp,tmp2int32);
    if (readaf==af)
    {
      /* read the address */
      result->h_length=tmp2int32;
      READ_BUF(fp,result->h_addr_list[i++],tmp2int32);
    }
    else
    {
      SKIP(fp,tmpint32);
    }
  }
  /* null-terminate address list */
  result->h_addr_list[i]=NULL;
  return NSS_STATUS_SUCCESS;
}

/* this is a wrapper around read_hostent() that does error handling
   if the read address list does not contain any addresses for the
   specified address familiy */
static enum nss_status read_hostent_erronempty(
        TFILE *fp,int af,struct hostent *result,
        char *buffer,size_t buflen,int *errnop,int *h_errnop)
{
  enum nss_status retv;
  retv=read_hostent(fp,af,result,buffer,buflen,errnop,h_errnop);
  /* check result */
  if (retv!=NSS_STATUS_SUCCESS)
    return retv;
  /* check empty address list
     (note that we cannot do this in the read_hostent() function as closing
     the socket there will cause problems with the {set,get,end}ent() functions
     below)
  */
  if (result->h_addr_list[0]==NULL)
  {
    *errnop=ENOENT;
    *h_errnop=NO_ADDRESS;
    (void)tio_close(fp);
    return NSS_STATUS_NOTFOUND;
  }
  return NSS_STATUS_SUCCESS;
}

/* this is a wrapper around read_hostent() that skips to the
   next address if the address list does not contain any addresses for the
   specified address familiy */
static enum nss_status read_hostent_nextonempty(
        TFILE *fp,int af,struct hostent *result,
        char *buffer,size_t buflen,int *errnop,int *h_errnop)
{
  int32_t tmpint32;
  enum nss_status retv;
  /* check until we read an non-empty entry */
  do
  {
    /* read a host entry */
    retv=read_hostent(fp,af,result,buffer,buflen,errnop,h_errnop);
    /* check result */
    if (retv!=NSS_STATUS_SUCCESS)
      return retv;
    /* skip to the next entry if we read an empty address */
    if (result->h_addr_list[0]==NULL)
    {
      retv=NSS_STATUS_NOTFOUND;
      READ_RESPONSE_CODE(fp);
    }
    /* do another loop run if we read an empty address */
  }
  while (retv!=NSS_STATUS_SUCCESS);
  return NSS_STATUS_SUCCESS;
}

/* this function looks up a single host entry and returns all the addresses
   associated with the host in a single address familiy
   name            - IN  - hostname to lookup
   af              - IN  - address familty to present results for
   result          - OUT - entry found
   buffer,buflen   - OUT - buffer to store allocated stuff on
   errnop,h_errnop - OUT - for reporting errors */
enum nss_status _nss_ldap_gethostbyname2_r(
        const char *name,int af,struct hostent *result,
        char *buffer,size_t buflen,int *errnop,int *h_errnop)
{
  NSS_BYNAME(NSLCD_ACTION_HOST_BYNAME,
             name,
             read_hostent_erronempty(fp,af,result,buffer,buflen,errnop,h_errnop));
}

/* this function just calls the gethostbyname2() variant with the address
   familiy set */
enum nss_status _nss_ldap_gethostbyname_r(
        const char *name,struct hostent *result,
        char *buffer,size_t buflen,int *errnop,int *h_errnop)
{
  return _nss_ldap_gethostbyname2_r(name,AF_INET,result,buffer,buflen,errnop,h_errnop);
}

/* write an address value */
#define WRITE_ADDRESS(fp,af,len,addr) \
  WRITE_INT32(fp,af); \
  WRITE_INT32(fp,len); \
  WRITE(fp,addr,len);

/* this function looks up a single host entry and returns all the addresses
   associated with the host in a single address familiy
   addr            - IN  - the address to look up
   len             - IN  - the size of the addr struct
   af              - IN  - address familty the address is specified as
   result          - OUT - entry found
   buffer,buflen   - OUT - buffer to store allocated stuff on
   errnop,h_errnop - OUT - for reporting errors */
enum nss_status _nss_ldap_gethostbyaddr_r(
        const void *addr,socklen_t len,int af,struct hostent *result,
        char *buffer,size_t buflen,int *errnop,int *h_errnop)
{
  NSS_BYGEN(NSLCD_ACTION_HOST_BYADDR,
            WRITE_ADDRESS(fp,af,len,addr),
            read_hostent_erronempty(fp,af,result,buffer,buflen,errnop,h_errnop))
}

/* thread-local file pointer to an ongoing request */
static __thread TFILE *hostentfp;

enum nss_status _nss_ldap_sethostent(int UNUSED(stayopen))
{
  NSS_SETENT(hostentfp);
}

/* this function only returns addresses of the AF_INET address family */
enum nss_status _nss_ldap_gethostent_r(
        struct hostent *result,
        char *buffer,size_t buflen,int *errnop,int *h_errnop)
{
  NSS_GETENT(hostentfp,NSLCD_ACTION_HOST_ALL,
             read_hostent_nextonempty(hostentfp,AF_INET,result,buffer,buflen,errnop,h_errnop));
}

enum nss_status _nss_ldap_endhostent(void)
{
  NSS_ENDENT(hostentfp);
}
