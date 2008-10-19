/*
   ethers.c - NSS lookup functions for ethers database

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

static enum nss_status read_etherent(
        TFILE *fp,struct etherent *result,
        char *buffer,size_t buflen,int *errnop)
{
  int32_t tmpint32;
  size_t bufptr=0;
  READ_STRING_BUF(fp,result->e_name);
  READ_TYPE(fp,result->e_addr,uint8_t[6]);
  return NSS_STATUS_SUCCESS;
}

/* map a hostname to the corresponding ethernet address */
enum nss_status _nss_ldap_gethostton_r(
        const char *name,struct etherent *result,
        char *buffer,size_t buflen,int *errnop)
{
  NSS_BYNAME(NSLCD_ACTION_ETHER_BYNAME,
             name,
             read_etherent(fp,result,buffer,buflen,errnop));
}

/* map an ethernet address to the corresponding hostname */
enum nss_status _nss_ldap_getntohost_r(
        const struct ether_addr *addr,struct etherent *result,
        char *buffer,size_t buflen,int *errnop)
{
  NSS_BYTYPE(NSLCD_ACTION_ETHER_BYETHER,
             *addr,uint8_t[6],
             read_etherent(fp,result,buffer,buflen,errnop));
}

/* thread-local file pointer to an ongoing request */
static __thread TFILE *etherentfp;

enum nss_status _nss_ldap_setetherent(int UNUSED(stayopen))
{
  NSS_SETENT(etherentfp);
}

enum nss_status _nss_ldap_getetherent_r(
        struct etherent *result,
        char *buffer,size_t buflen,int *errnop)
{
  NSS_GETENT(etherentfp,NSLCD_ACTION_ETHER_ALL,
             read_etherent(etherentfp,result,buffer,buflen,errnop));
}

enum nss_status _nss_ldap_endetherent(void)
{
  NSS_ENDENT(etherentfp);
}
