/*
   test_aliases.c - simple tests of developed nss code

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
#include "compat/attrs.h"

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

static void printalias(struct aliasent *alias)
{
  int i;
  printf("struct alias {\n"
         "  alias_name=\"%s\",\n"
         "  alias_members_len=%d,\n",
         alias->alias_name,(int)alias->alias_members_len);
  for (i=0;i<(int)alias->alias_members_len;i++)
    printf("  alias_members[%d]=\"%s\",\n",
           i,alias->alias_members[i]);
  printf("  alias_local=%d\n"
         "}\n",(int)alias->alias_local);
}

/* the main program... */
int main(int UNUSED(argc),char UNUSED(*argv[]))
{
  struct aliasent aliasresult;
  char buffer[1024];
  enum nss_status res;
  int errnocp;

  /* test getaliasbyname() */
  printf("\nTEST getaliasbyname()\n");
  res=_nss_ldap_getaliasbyname_r("wij@arthurenhella.demon.nl",&aliasresult,buffer,1024,&errnocp);
  printf("status=%s\n",nssstatus(res));
  if (res==NSS_STATUS_SUCCESS)
    printalias(&aliasresult);
  else
    printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));

  /* test {set,get,end}aliasent() */
  printf("\nTEST {set,get,end}aliasent()\n");
  res=_nss_ldap_setaliasent();
  printf("status=%s\n",nssstatus(res));
  while ((res=_nss_ldap_getaliasent_r(&aliasresult,buffer,1024,&errnocp))==NSS_STATUS_SUCCESS)
  {
    printf("status=%s\n",nssstatus(res));
    printalias(&aliasresult);
  }
  printf("status=%s\n",nssstatus(res));
  printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));
  res=_nss_ldap_endaliasent();
  printf("status=%s\n",nssstatus(res));

  return 0;
}
