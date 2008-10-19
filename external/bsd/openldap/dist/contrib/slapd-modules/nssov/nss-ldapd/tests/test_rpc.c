/*
   test_rpc.c - simple tests of developed nss code

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

static void printrpc(struct rpcent *rpc)
{
  int i;
  printf("struct rpcent {\n"
         "  r_name=\"%s\",\n",
         rpc->r_name);
  for (i=0;rpc->r_aliases[i]!=NULL;i++)
    printf("  r_aliases[%d]=\"%s\",\n",
           i,rpc->r_aliases[i]);
  printf("  r_aliases[%d]=NULL,\n"
         "  r_number=%d\n"
         "}\n",i,(int)(rpc->r_number));
}

/* the main program... */
int main(int argc,char *argv[])
{
  struct rpcent rpcresult;
  char buffer[1024];
  enum nss_status res;
  int errnocp;

  /* test getrpcbyname() */
  printf("\nTEST getrpcbyname()\n");
  res=_nss_ldap_getrpcbyname_r("rpcfoo",&rpcresult,buffer,1024,&errnocp);
  printf("status=%s\n",nssstatus(res));
  if (res==NSS_STATUS_SUCCESS)
    printrpc(&rpcresult);
  else
    printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));

  /* test getrpcbynumber() */
  printf("\nTEST getrpcbynumber()\n");
  res=_nss_ldap_getrpcbynumber_r(7899,&rpcresult,buffer,1024,&errnocp);
  printf("status=%s\n",nssstatus(res));
  if (res==NSS_STATUS_SUCCESS)
    printrpc(&rpcresult);
  else
    printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));

  /* test {set,get,end}rpcent() */
  printf("\nTEST {set,get,end}rpcent()\n");
  res=_nss_ldap_setrpcent(1);
  printf("status=%s\n",nssstatus(res));
  while ((res=_nss_ldap_getrpcent_r(&rpcresult,buffer,1024,&errnocp))==NSS_STATUS_SUCCESS)
  {
    printf("status=%s\n",nssstatus(res));
    printrpc(&rpcresult);
  }
  printf("status=%s\n",nssstatus(res));
  printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));
  res=_nss_ldap_endrpcent();
  printf("status=%s\n",nssstatus(res));

  return 0;
}
