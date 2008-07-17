/*
   test_hosts.c - simple tests of developed nss code

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

static void printhost(struct hostent *host)
{
  int i,j;
  char buffer[1024];
  printf("struct hostent {\n"
         "  h_name=\"%s\",\n",
         host->h_name);
  for (i=0;host->h_aliases[i]!=NULL;i++)
    printf("  h_aliases[%d]=\"%s\",\n",
           i,host->h_aliases[i]);
  printf("  h_aliases[%d]=NULL,\n",i);
  if (host->h_addrtype==AF_INET)
    printf("  h_addrtype=AF_INET,\n");
  else if (host->h_addrtype==AF_INET6)
    printf("  h_addrtype=AF_INET6,\n");
  else
    printf("  h_addrtype=%d,\n",host->h_addrtype);
  printf("  h_length=%d,\n",host->h_length);
  for (i=0;host->h_addr_list[i]!=NULL;i++)
  {
    if (inet_ntop(host->h_addrtype,host->h_addr_list[i],
                  buffer,1024)!=NULL)
    {
      printf("  h_addr_list[%d]=%s,\n",i,buffer);
    }
    else
    {
      printf("  h_addr_list[%d]=",i);
      for (j=0;j<host->h_length;j++)
        printf("%02x",(int)((const uint8_t*)host->h_addr_list[i])[j]);
      printf(",\n");
    }
  }
  printf("  h_addr_list[%d]=NULL\n"
         "}\n",i);
}

/* the main program... */
int main(int argc,char *argv[])
{
  struct hostent hostresult;
  char buffer[1024];
  enum nss_status res;
  int errnocp,h_errnocp;
  char address[1024];

  /* test gethostbyname2(AF_INET) */
  printf("\nTEST gethostbyname2(AF_INET)\n");
  res=_nss_ldap_gethostbyname2_r("oostc",AF_INET,&hostresult,buffer,1024,&errnocp,&h_errnocp);
  printf("status=%s\n",nssstatus(res));
  if (res==NSS_STATUS_SUCCESS)
    printhost(&hostresult);
  else
  {
    printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));
    printf("h_errno=%d:%s\n",(int)h_errnocp,hstrerror(h_errnocp));
  }

  /* test gethostbyname2(AF_INET6) */
/* this is currently unsupported
  printf("\nTEST gethostbyname2(AF_INET6)\n");
  res=_nss_ldap_gethostbyname2_r("appelscha",AF_INET6,&hostresult,buffer,1024,&errnocp,&h_errnocp);
  printf("status=%s\n",nssstatus(res));
  if (res==NSS_STATUS_SUCCESS)
    printhost(&hostresult);
  else
  {
    printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));
    printf("h_errno=%d:%s\n",(int)h_errnocp,hstrerror(h_errnocp));
  }
*/

  /* test gethostbyaddr(AF_INET) */
  printf("\nTEST gethostbyaddr(AF_INET)\n");
  inet_pton(AF_INET,"192.43.210.81",address);
  res=_nss_ldap_gethostbyaddr_r(address,sizeof(struct in_addr),AF_INET,
                                &hostresult,buffer,1024,&errnocp,&h_errnocp);
  printf("status=%s\n",nssstatus(res));
  if (res==NSS_STATUS_SUCCESS)
    printhost(&hostresult);
  else
  {
    printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));
    printf("h_errno=%d:%s\n",(int)h_errnocp,hstrerror(h_errnocp));
  }

  /* test gethostbyaddr(AF_INET6) */
/* this is currently unsupported
  printf("\nTEST gethostbyaddr(AF_INET6)\n");
  inet_pton(AF_INET6,"2001:200:0:8002:203:47ff:fea5:3085",address);
  res=_nss_ldap_gethostbyaddr_r(address,sizeof(struct in6_addr),AF_INET6,
                                &hostresult,buffer,1024,&errnocp,&h_errnocp);
  printf("status=%s\n",nssstatus(res));
  if (res==NSS_STATUS_SUCCESS)
    printhost(&hostresult);
  else
  {
    printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));
    printf("h_errno=%d:%s\n",(int)h_errnocp,hstrerror(h_errnocp));
  }
*/

  /* test {set,get,end}hostent() */
  printf("\nTEST {set,get,end}hostent()\n");
  res=_nss_ldap_sethostent(1);
  printf("status=%s\n",nssstatus(res));
  while ((res=_nss_ldap_gethostent_r(&hostresult,buffer,1024,&errnocp,&h_errnocp))==NSS_STATUS_SUCCESS)
  {
    printf("status=%s\n",nssstatus(res));
    printhost(&hostresult);
  }
  printf("status=%s\n",nssstatus(res));
  printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));
  printf("h_errno=%d:%s\n",(int)h_errnocp,hstrerror(h_errnocp));
  res=_nss_ldap_endhostent();
  printf("status=%s\n",nssstatus(res));

  return 0;
}
