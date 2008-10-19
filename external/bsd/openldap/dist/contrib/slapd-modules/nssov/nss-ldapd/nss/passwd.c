/*
   passwd.c - NSS lookup functions for passwd database

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

static enum nss_status read_passwd(
        TFILE *fp,struct passwd *result,
        char *buffer,size_t buflen,int *errnop)
{
  int32_t tmpint32;
  size_t bufptr=0;
  READ_STRING_BUF(fp,result->pw_name);
  READ_STRING_BUF(fp,result->pw_passwd);
  READ_TYPE(fp,result->pw_uid,uid_t);
  READ_TYPE(fp,result->pw_gid,gid_t);
  READ_STRING_BUF(fp,result->pw_gecos);
  READ_STRING_BUF(fp,result->pw_dir);
  READ_STRING_BUF(fp,result->pw_shell);
  return NSS_STATUS_SUCCESS;
}

enum nss_status _nss_ldap_getpwnam_r(const char *name,struct passwd *result,char *buffer,size_t buflen,int *errnop)
{
  NSS_BYNAME(NSLCD_ACTION_PASSWD_BYNAME,
             name,
             read_passwd(fp,result,buffer,buflen,errnop));
}

enum nss_status _nss_ldap_getpwuid_r(uid_t uid,struct passwd *result,char *buffer,size_t buflen,int *errnop)
{
  NSS_BYTYPE(NSLCD_ACTION_PASSWD_BYUID,
             uid,uid_t,
             read_passwd(fp,result,buffer,buflen,errnop));
}

/* thread-local file pointer to an ongoing request */
static __thread TFILE *pwentfp;

/* open a connection to the nslcd and write the request */
enum nss_status _nss_ldap_setpwent(int UNUSED(stayopen))
{
  NSS_SETENT(pwentfp);
}

/* read password data from an opened stream */
enum nss_status _nss_ldap_getpwent_r(struct passwd *result,char *buffer,size_t buflen,int *errnop)
{
  NSS_GETENT(pwentfp,NSLCD_ACTION_PASSWD_ALL,
             read_passwd(pwentfp,result,buffer,buflen,errnop));
}

/* close the stream opened with setpwent() above */
enum nss_status _nss_ldap_endpwent(void)
{
  NSS_ENDENT(pwentfp);
}
