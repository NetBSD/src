/*
   group.c - NSS lookup functions for group database

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
#include <stdlib.h>

#include "prototypes.h"
#include "common.h"
#include "compat/attrs.h"

static enum nss_status read_group(
        TFILE *fp,struct group *result,
        char *buffer,size_t buflen,int *errnop)
{
  int32_t tmpint32,tmp2int32,tmp3int32;
  size_t bufptr=0;
  READ_STRING_BUF(fp,result->gr_name);
  READ_STRING_BUF(fp,result->gr_passwd);
  READ_TYPE(fp,result->gr_gid,gid_t);
  READ_STRINGLIST_NULLTERM(fp,result->gr_mem);
  return NSS_STATUS_SUCCESS;
}

/* read all group entries from the stream and add
   gids of these groups to the list */
static enum nss_status read_gids(
        TFILE *fp,gid_t skipgroup,long int *start,long int *size,
        gid_t **groupsp,long int limit,int *errnop)
{
  int32_t res=(int32_t)NSLCD_RESULT_SUCCESS;
  int32_t tmpint32,tmp2int32,tmp3int32;
  gid_t gid;
  gid_t *newgroups;
  long int newsize;
  /* loop over results */
  while (res==(int32_t)NSLCD_RESULT_SUCCESS)
  {
    /* skip group name */
    SKIP_STRING(fp);
    /* skip passwd entry */
    SKIP_STRING(fp);
    /* read gid */
    READ_TYPE(fp,gid,gid_t);
    /* skip members */
    SKIP_STRINGLIST(fp);
    /* only add the group to the list if it is not the specified group */
    if (gid!=skipgroup)
    {
      /* check if we reached the limit */
      if ( (limit>0) && (*start>=limit) )
        return NSS_STATUS_TRYAGAIN;
      /* check if our buffer is large enough */
      if ((*start)>=(*size))
      {
        /* for some reason Glibc expects us to grow the array (completely
           different from all other NSS functions) */
        /* calculate new size */
        newsize=2*(*size);
        if ( (limit>0) && (*start>=limit) )
          newsize=limit;
        /* allocate new memory */
        newgroups=realloc(*groupsp,newsize*sizeof(gid_t));
        if (newgroups==NULL)
          return NSS_STATUS_TRYAGAIN;
        *groupsp=newgroups;
        *size=newsize;
      }
      /* add gid to list */
      (*groupsp)[(*start)++]=gid;
    }
    /* read next response code
      (don't bail out on not success since we just want to build
      up a list) */
    READ_TYPE(fp,res,int32_t);
  }
  /* return the proper status code */
  return NSS_STATUS_SUCCESS;
}

enum nss_status _nss_ldap_getgrnam_r(const char *name,struct group *result,char *buffer,size_t buflen,int *errnop)
{
  NSS_BYNAME(NSLCD_ACTION_GROUP_BYNAME,
             name,
             read_group(fp,result,buffer,buflen,errnop));
}

enum nss_status _nss_ldap_getgrgid_r(gid_t gid,struct group *result,char *buffer,size_t buflen,int *errnop)
{
  NSS_BYTYPE(NSLCD_ACTION_GROUP_BYGID,
             gid,gid_t,
             read_group(fp,result,buffer,buflen,errnop));
}

/* this function returns a list of groups, documentation for the
   interface is scarce (any pointers are welcome) but this is
   what is assumed the parameters mean:

   user      IN     - the user name to find groups for
   skipgroup IN     - a group to not include in the list
   *start    IN/OUT - where to write in the array, is incremented
   *size     IN/OUT - the size of the supplied array (gid_t entries, not bytes)
   **groupsp IN/OUT - pointer to the array of returned groupids
   limit     IN     - the maxium size of the array
   *errnop   OUT    - for returning errno

   This function cannot grow the array if it becomes too large
   (and will return NSS_STATUS_TRYAGAIN on buffer problem)
   because it has no way of free()ing the buffer.
*/
enum nss_status _nss_ldap_initgroups_dyn(
        const char *user,gid_t skipgroup,long int *start,
        long int *size,gid_t **groupsp,long int limit,int *errnop)
{
  NSS_BYNAME(NSLCD_ACTION_GROUP_BYMEMBER,
             user,
             read_gids(fp,skipgroup,start,size,groupsp,limit,errnop));
}

/* thread-local file pointer to an ongoing request */
static __thread TFILE *grentfp;

enum nss_status _nss_ldap_setgrent(int UNUSED(stayopen))
{
  NSS_SETENT(grentfp);
}

enum nss_status _nss_ldap_getgrent_r(struct group *result,char *buffer,size_t buflen,int *errnop)
{
  NSS_GETENT(grentfp,NSLCD_ACTION_GROUP_ALL,
             read_group(grentfp,result,buffer,buflen,errnop));
}

enum nss_status _nss_ldap_endgrent(void)
{
  NSS_ENDENT(grentfp);
}
