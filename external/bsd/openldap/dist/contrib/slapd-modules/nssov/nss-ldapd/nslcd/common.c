/*
   common.c - common server code routines
   This file is part of the nss-ldapd library.

   Copyright (C) 2006 West Consulting
   Copyright (C) 2006, 2007 Arthur de Jong

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

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>

#include "nslcd.h"
#include "common.h"
#include "log.h"

/* simple wrapper around snptintf() to return non-0 in case
   of any failure (but always keep string 0-terminated) */
int mysnprintf(char *buffer,size_t buflen,const char *format, ...)
{
  int res;
  va_list ap;
  /* do snprintf */
  va_start(ap,format);
  res=vsnprintf(buffer,buflen,format,ap);
  /* NULL-terminate the string just to be on the safe side */
  buffer[buflen-1]='\0';
  /* check if the string was completely written */
  return ((res<0)||(((size_t)res)>=buflen));
}

const char *get_userpassword(MYLDAP_ENTRY *entry,const char *attr)
{
  const char **values;
  int i;
  /* get the entries */
  values=myldap_get_values(entry,attr);
  if ((values==NULL)||(values[0]==NULL))
    return NULL;
  /* go over the entries and return the remainder of the value if it
     starts with {crypt} or crypt$ */
  for (i=0;values[i]!=NULL;i++)
  {
    if (strncasecmp(values[i],"{crypt}",7)==0)
      return values[i]+7;
    if (strncasecmp(values[i],"crypt$",6)==0)
      return values[i]+6;
  }
  /* just return the first value completely */
  return values[0];
  /* TODO: support more password formats e.g. SMD5
    (which is $1$ but in a different format)
    (any code for this is more than welcome) */
}

/* this writes a single address to the stream */
int write_address(TFILE *fp,const char *addr)
{
  int32_t tmpint32;
  struct in_addr ipv4addr;
  struct in6_addr ipv6addr;
  /* try to parse the address as IPv4 first, fall back to IPv6 */
  if (inet_pton(AF_INET,addr,&ipv4addr)>0)
  {
    /* write address type */
    WRITE_INT32(fp,AF_INET);
    /* write the address length */
    WRITE_INT32(fp,sizeof(struct in_addr));
    /* write the address itself (in network byte order) */
    WRITE_TYPE(fp,ipv4addr,struct in_addr);
  }
  else if (inet_pton(AF_INET6,addr,&ipv6addr)>0)
  {
    /* write address type */
    WRITE_INT32(fp,AF_INET6);
    /* write the address length */
    WRITE_INT32(fp,sizeof(struct in6_addr));
    /* write the address itself (in network byte order) */
    WRITE_TYPE(fp,ipv6addr,struct in6_addr);
  }
  else
  {
    /* failure, log but write simple invalid address
       (otherwise the address list is messed up) */
    /* TODO: have error message in correct format */
    log_log(LOG_WARNING,"unparseble address: %s",addr);
    /* write an illegal address type */
    WRITE_INT32(fp,-1);
    /* write an emtpy address */
    WRITE_INT32(fp,0);
  }
  /* we're done */
  return 0;
}

int read_address(TFILE *fp,char *addr,int *addrlen,int *af)
{
  int32_t tmpint32;
  int len;
  /* read address family */
  READ_INT32(fp,*af);
  if ((*af!=AF_INET)&&(*af!=AF_INET6))
  {
    log_log(LOG_WARNING,"incorrect address family specified: %d",*af);
    return -1;
  }
  /* read address length */
  READ_INT32(fp,len);
  if ((len>*addrlen)||(len<=0))
  {
    log_log(LOG_WARNING,"address length incorrect: %d",len);
    return -1;
  }
  *addrlen=len;
  /* read address */
  READ(fp,addr,len);
  /* we're done */
  return 0;
}
