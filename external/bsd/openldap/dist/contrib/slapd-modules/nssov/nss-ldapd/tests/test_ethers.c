/*
   test_ethers.c - simple tests of developed nss code

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

static void printether(struct etherent *ether)
{
  printf("struct etherent {\n"
         "  e_name=\"%s\",\n"
         "  e_addr=%s\n"
         "}\n",
         ether->e_name,ether_ntoa(&(ether->e_addr)));
}

/* the main program... */
int main(int argc,char *argv[])
{
  struct etherent etherresult;
  char buffer[1024];
  enum nss_status res;
  int errnocp;

  /* test ether_hostton() */
  printf("\nTEST ether_hostton()\n");
  res=_nss_ldap_gethostton_r("spiritus",&etherresult,buffer,1024,&errnocp);
  printf("status=%s\n",nssstatus(res));
  if (res==NSS_STATUS_SUCCESS)
    printether(&etherresult);
  else
    printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));

  /* test ether_ntohost() */
  printf("\nTEST ether_ntohost()\n");
  res=_nss_ldap_getntohost_r(ether_aton("00:E0:4C:39:D3:6A"),
                             &etherresult,buffer,1024,&errnocp);
  printf("status=%s\n",nssstatus(res));
  if (res==NSS_STATUS_SUCCESS)
    printether(&etherresult);
  else
    printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));

  /* test {set,get,end}etherent() */
  printf("\nTEST {set,get,end}etherent()\n");
  res=_nss_ldap_setetherent(1);
  printf("status=%s\n",nssstatus(res));
  while ((res=_nss_ldap_getetherent_r(&etherresult,buffer,1024,&errnocp))==NSS_STATUS_SUCCESS)
  {
    printf("status=%s\n",nssstatus(res));
    printether(&etherresult);
  }
  printf("status=%s\n",nssstatus(res));
  printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));
  res=_nss_ldap_endetherent();
  printf("status=%s\n",nssstatus(res));

  return 0;
}
