/*
   test_netgroup.c - simple tests of developed nss code

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

static void printnetgroup(struct __netgrent *netgroup)
{
  printf("struct __netgrent {\n");
  if (netgroup->type==triple_val)
  {
    printf("  type=triple_val,\n");
    if (netgroup->val.triple.host==NULL)
      printf("  val.triple.host=NULL,\n");
    else
      printf("  val.triple.host=\"%s\",\n",netgroup->val.triple.host);
    if (netgroup->val.triple.user==NULL)
      printf("  val.triple.user=NULL,\n");
    else
      printf("  val.triple.user=\"%s\",\n",netgroup->val.triple.user);
    if (netgroup->val.triple.domain==NULL)
      printf("  val.triple.domain=NULL,\n");
    else
      printf("  val.triple.domain=\"%s\",\n",netgroup->val.triple.domain);
  }
  else
  {
    printf("  type=triple_val,\n"
           "  val.group=\"%s\",\n",
           netgroup->val.group);
  }
  printf("  ...\n"
         "}\n");
}

/* the main program... */
int main(int argc,char *argv[])
{
  struct __netgrent netgroupresult;
  char buffer[1024];
  enum nss_status res;
  int errnocp;

  /* test {set,get,end}netgrent() */
  printf("\nTEST {set,get,end}netgrent()\n");
  res=_nss_ldap_setnetgrent("westcomp",&netgroupresult);
  printf("status=%s\n",nssstatus(res));
  while ((_nss_ldap_getnetgrent_r(&netgroupresult,buffer,1024,&errnocp))==NSS_STATUS_SUCCESS)
  {
    printf("status=%s\n",nssstatus(res));
    printnetgroup(&netgroupresult);
  }
  printf("status=%s\n",nssstatus(res));
  printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));
  res=_nss_ldap_endnetgrent(&netgroupresult);
  printf("status=%s\n",nssstatus(res));

  return 0;
}
