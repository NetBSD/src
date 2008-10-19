/*
   test_passwd.c - simple tests of developed nss code

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

static void printpasswd(struct passwd *pw)
{
  printf("struct passwd {\n"
         "  pw_name=\"%s\",\n"
         "  pw_passwd=\"%s\",\n"
         "  pw_uid=%d,\n"
         "  pw_gid=%d,\n"
         "  pw_gecos=\"%s\",\n"
         "  pw_dir=\"%s\",\n"
         "  pw_shell=\"%s\"\n"
         "}\n",pw->pw_name,pw->pw_passwd,
         (int)(pw->pw_uid),(int)(pw->pw_gid),
         pw->pw_gecos,pw->pw_dir,pw->pw_shell);
}

/* the main program... */
int main(int argc,char *argv[])
{
  struct passwd passwdresult;
  char buffer[1024];
  enum nss_status res;
  int errnocp;

  /* test getpwnam() */
  printf("\nTEST getpwnam()\n");
  res=_nss_ldap_getpwnam_r("arthur",&passwdresult,buffer,1024,&errnocp);
  printf("status=%s\n",nssstatus(res));
  if (res==NSS_STATUS_SUCCESS)
    printpasswd(&passwdresult);
  else
    printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));

  /* test getpwnam() with non-existing user */
  printf("\nTEST getpwnam() with non-existing user\n");
  res=_nss_ldap_getpwnam_r("nonexist",&passwdresult,buffer,1024,&errnocp);
  printf("status=%s\n",nssstatus(res));
  if (res==NSS_STATUS_SUCCESS)
    printpasswd(&passwdresult);
  else
    printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));

  /* test getpwuid() */
  printf("\nTEST getpwuid()\n");
  res=_nss_ldap_getpwuid_r(1004,&passwdresult,buffer,1024,&errnocp);
  printf("status=%s\n",nssstatus(res));
  if (res==NSS_STATUS_SUCCESS)
    printpasswd(&passwdresult);
  else
    printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));

  /* test {set,get,end}pwent() */
  printf("\nTEST {set,get,end}pwent()\n");
  res=_nss_ldap_setpwent(1);
  printf("status=%s\n",nssstatus(res));
  while ((res=_nss_ldap_getpwent_r(&passwdresult,buffer,1024,&errnocp))==NSS_STATUS_SUCCESS)
  {
    printf("status=%s\n",nssstatus(res));
    printpasswd(&passwdresult);
  }
  printf("status=%s\n",nssstatus(res));
  printf("errno=%d:%s\n",(int)errnocp,strerror(errnocp));
  res=_nss_ldap_endpwent();
  printf("status=%s\n",nssstatus(res));

  return 0;
}
