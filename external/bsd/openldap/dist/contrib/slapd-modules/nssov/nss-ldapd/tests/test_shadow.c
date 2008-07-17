/*
   test_shadow.c - simple tests of developed nss code

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

static void printshadow(struct spwd *shadow)
{
  printf("struct spwd {\n"
         "  sp_namp=\"%s\",\n"
         "  sp_pwdp=\"%s\",\n"
         "  sp_lstchg=%ld,\n"
         "  sp_min=%ld,\n"
         "  sp_max=%ld,\n"
         "  sp_warn=%ld,\n"
         "  sp_inact=%ld,\n"
         "  sp_expire=%ld,\n"
         "  sp_flag=%lu\n"
         "}\n",
         shadow->sp_namp,shadow->sp_pwdp,shadow->sp_lstchg,
         shadow->sp_min,shadow->sp_max,shadow->sp_warn,
         shadow->sp_inact,shadow->sp_expire,shadow->sp_flag);
}

/* the main program... */
int main(int argc,char *argv[])
{
  struct spwd shadowresult;
  char buffer[1024];
  enum nss_status res;
  int errnocp;

  /* test getspnam() */
  printf("\nTEST getspnam()\n");
  res=_nss_ldap_getspnam_r("arthur",&shadowresult,buffer,1024,&errnocp);
  printf("status=%s\n",nssstatus(res));
  if (res==NSS_STATUS_SUCCESS)
    printshadow(&shadowresult);
  else
    printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));

  /* test {set,get,end}spent() */
  printf("\nTEST {set,get,end}spent()\n");
  res=_nss_ldap_setspent(1);
  printf("status=%s\n",nssstatus(res));
  while ((res=_nss_ldap_getspent_r(&shadowresult,buffer,1024,&errnocp))==NSS_STATUS_SUCCESS)
  {
    printf("status=%s\n",nssstatus(res));
    printshadow(&shadowresult);
  }
  printf("status=%s\n",nssstatus(res));
  printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));
  res=_nss_ldap_endspent();
  printf("status=%s\n",nssstatus(res));

  return 0;
}
