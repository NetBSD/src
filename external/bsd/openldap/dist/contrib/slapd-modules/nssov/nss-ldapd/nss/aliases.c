/*
   aliases.c - NSS lookup functions for aliases database

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

static enum nss_status read_aliasent(
        TFILE *fp,struct aliasent *result,
        char *buffer,size_t buflen,int *errnop)
{
  int32_t tmpint32,tmp2int32;
  size_t bufptr=0;
  /* auto-genereted read code */
  READ_STRING_BUF(fp,result->alias_name);
  READ_STRINGLIST_NUM(fp,result->alias_members,result->alias_members_len);
  /* fill in remaining gaps in struct */
  result->alias_local=0;
  /* we're done */
  return NSS_STATUS_SUCCESS;
}

enum nss_status _nss_ldap_getaliasbyname_r(
        const char *name,struct aliasent *result,
        char *buffer,size_t buflen,int *errnop)
{
  NSS_BYNAME(NSLCD_ACTION_ALIAS_BYNAME,
             name,
             read_aliasent(fp,result,buffer,buflen,errnop));
}

/* thread-local file pointer to an ongoing request */
static __thread TFILE *aliasentfp;

enum nss_status _nss_ldap_setaliasent(void)
{
  NSS_SETENT(aliasentfp);
}

enum nss_status _nss_ldap_getaliasent_r(struct aliasent *result,char *buffer,size_t buflen,int *errnop)
{
  NSS_GETENT(aliasentfp,NSLCD_ACTION_ALIAS_ALL,
             read_aliasent(aliasentfp,result,buffer,buflen,errnop));
}

enum nss_status _nss_ldap_endaliasent(void)
{
  NSS_ENDENT(aliasentfp);
}
