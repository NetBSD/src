/*
   test_services.c - simple tests of developed nss code

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

static void printserv(struct servent *serv)
{
  int i;
  printf("struct servent {\n"
         "  s_name=\"%s\",\n",
         serv->s_name);
  for (i=0;serv->s_aliases[i]!=NULL;i++)
    printf("  s_aliases[%d]=\"%s\",\n",
           i,serv->s_aliases[i]);
  printf("  s_aliases[%d]=NULL,\n"
         "  s_port=%d,\n"
         "  s_proto=\"%s\"\n"
         "}\n",i,(int)(ntohs(serv->s_port)),
         serv->s_proto);
}

/* the main program... */
int main(int argc,char *argv[])
{
  struct servent servresult;
  char buffer[1024];
  enum nss_status res;
  int errnocp;

  /* test getservbyname() */
  printf("\nTEST getservbyname()\n");
  res=_nss_ldap_getservbyname_r("srvfoo","udp",&servresult,buffer,1024,&errnocp);
  printf("status=%s\n",nssstatus(res));
  if (res==NSS_STATUS_SUCCESS)
    printserv(&servresult);
  else
    printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));

  /* test getservbyport() */
  printf("\nTEST getservbyport()\n");
  res=_nss_ldap_getservbyport_r(ntohs(9988),NULL,&servresult,buffer,1024,&errnocp);
  printf("status=%s\n",nssstatus(res));
  if (res==NSS_STATUS_SUCCESS)
    printserv(&servresult);
  else
    printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));

  /* test {set,get,end}servent() */
  printf("\nTEST {set,get,end}servent()\n");
  res=_nss_ldap_setservent(1);
  printf("status=%s\n",nssstatus(res));
  while ((res=_nss_ldap_getservent_r(&servresult,buffer,1024,&errnocp))==NSS_STATUS_SUCCESS)
  {
    printf("status=%s\n",nssstatus(res));
    printserv(&servresult);
  }
  printf("status=%s\n",nssstatus(res));
  printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));
  res=_nss_ldap_endservent();
  printf("status=%s\n",nssstatus(res));

  return 0;
}
