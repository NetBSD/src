/*
   test_networks.c - simple tests of developed nss code

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

static void printnetwork(struct netent *network)
{
  int i;
  printf("struct netent {\n"
         "  n_name=\"%s\",\n",
         network->n_name);
  for (i=0;network->n_aliases[i]!=NULL;i++)
    printf("  n_aliases[%d]=\"%s\",\n",
           i,network->n_aliases[i]);
  printf("  n_aliases[%d]=NULL,\n",i);
  if (network->n_addrtype==AF_INET)
    printf("  n_addrtype=AF_INET,\n");
  else if (network->n_addrtype==AF_INET6)
    printf("  n_addrtype=AF_INET6,\n");
  else
    printf("  n_addrtype=%d,\n",network->n_addrtype);
  printf("  n_net=%s\n"
         "}\n",inet_ntoa(inet_makeaddr(network->n_net,0)));
}

/* the main program... */
int main(int argc,char *argv[])
{
  struct netent netresult;
  char buffer[1024];
  enum nss_status res;
  int errnocp,h_errnocp;

  /* test getnetbyname() */
  printf("\nTEST getnetbyname()\n");
  res=_nss_ldap_getnetbyname_r("west",&netresult,buffer,1024,&errnocp,&h_errnocp);
  printf("status=%s\n",nssstatus(res));
  if (res==NSS_STATUS_SUCCESS)
    printnetwork(&netresult);
  else
  {
    printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));
    printf("h_errno=%d:%s\n",(int)h_errnocp,hstrerror(h_errnocp));
  }

  /* test getnetbyaddr() */
  printf("\nTEST getnetbyaddr()\n");
  res=_nss_ldap_getnetbyaddr_r(inet_network("192.43.210.0"),AF_INET,&netresult,buffer,1024,&errnocp,&h_errnocp);
  printf("status=%s\n",nssstatus(res));
  if (res==NSS_STATUS_SUCCESS)
    printnetwork(&netresult);
  else
  {
    printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));
    printf("h_errno=%d:%s\n",(int)h_errnocp,hstrerror(h_errnocp));
  }

  /* test {set,get,end}netent() */
  printf("\nTEST {set,get,end}netent()\n");
  res=_nss_ldap_setnetent(1);
  printf("status=%s\n",nssstatus(res));
  while ((res=_nss_ldap_getnetent_r(&netresult,buffer,1024,&errnocp,&h_errnocp))==NSS_STATUS_SUCCESS)
  {
    printf("status=%s\n",nssstatus(res));
    printnetwork(&netresult);
  }
  printf("status=%s\n",nssstatus(res));
  printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));
  printf("h_errno=%d:%s\n",(int)h_errnocp,hstrerror(h_errnocp));
  res=_nss_ldap_endnetent();
  printf("status=%s\n",nssstatus(res));

  return 0;
}
