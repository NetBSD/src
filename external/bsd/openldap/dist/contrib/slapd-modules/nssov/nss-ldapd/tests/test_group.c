/*
   test_group.c - simple tests of developed nss code

   Copyright (C) 2006 West Consulting
   Copyright (C) 2006 Arthur de Jong

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
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "nss/prototypes.h"

static char *nssstatus(enum nss_status retv)
{
  switch(retv)
  {
    case NSS_STATUS_TRYAGAIN: return "NSS_STATUS_TRYAGAIN";
    case NSS_STATUS_UNAVAIL:  return "NSS_STATUS_UNAVAIL";
    case NSS_STATUS_NOTFOUND: return "NSS_STATUS_NOTFOUND";
    case NSS_STATUS_SUCCESS:  return "NSS_STATUS_SUCCESS";
    case NSS_STATUS_RETURN:   return "NSS_STATUS_RETURN";
    default:                  return "NSS_STATUS_**ILLEGAL**";
  }
}

static void printgroup(struct group *group)
{
  int i;
  printf("struct group {\n"
         "  gr_name=\"%s\",\n"
         "  gr_passwd=\"%s\",\n"
         "  gr_gid=%d,\n",
         group->gr_name,group->gr_passwd,(int)group->gr_gid);
  for (i=0;group->gr_mem[i]!=NULL;i++)
    printf("  gr_mem[%d]=\"%s\",\n",
           i,group->gr_mem[i]);
  printf("  gr_mem[%d]=NULL\n"
         "}\n",i);
}

/* the main program... */
int main(int argc,char *argv[])
{
  struct group groupresult;
  char buffer[32*1024];
  enum nss_status res;
  int errnocp=0;
  long int start=0,size=40;
  gid_t *gidlist=(gid_t *)buffer;

  /* test getgrnam() */
  printf("\nTEST getgrnam()\n");
  res=_nss_ldap_getgrnam_r("largegroup",&groupresult,buffer,sizeof(buffer),&errnocp);
  printf("status=%s\n",nssstatus(res));
  if (res==NSS_STATUS_SUCCESS)
    printgroup(&groupresult);
  else
    printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));

  /* test getgrgid() */
  printf("\nTEST getgrgid()\n");
  res=_nss_ldap_getgrgid_r(100,&groupresult,buffer,sizeof(buffer),&errnocp);
  printf("status=%s\n",nssstatus(res));
  if (res==NSS_STATUS_SUCCESS)
    printgroup(&groupresult);
  else
    printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));

  /* test initgroups() */
  printf("\nTEST initgroups()\n");
  res=_nss_ldap_initgroups_dyn("arthur",10,&start,&size,&gidlist,size,&errnocp);
  printf("status=%s\n",nssstatus(res));
  if (res==NSS_STATUS_SUCCESS)
  {
    for (size=0;size<start;size++)
    {
      printf("gidlist[%d]=%d\n",(int)size,(int)gidlist[size]);
    }
  }
  else
    printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));

  /* test {set,get,end}grent() */
  printf("\nTEST {set,get,end}grent()\n");
  res=_nss_ldap_setgrent(1);
  printf("status=%s\n",nssstatus(res));
  while ((res=_nss_ldap_getgrent_r(&groupresult,buffer,sizeof(buffer),&errnocp))==NSS_STATUS_SUCCESS)
  {
    printf("status=%s\n",nssstatus(res));
    printgroup(&groupresult);
  }
  printf("status=%s\n",nssstatus(res));
  printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));
  res=_nss_ldap_endgrent();
  printf("status=%s\n",nssstatus(res));

  return 0;
}
