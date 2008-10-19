/*
   common.h - common server code routines
   This file is part of the nss-ldapd library.

   Copyright (C) 2006 West Consulting
   Copyright (C) 2006, 2007, 2008 Arthur de Jong

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

#ifndef _SERVER_COMMON_H
#define _SERVER_COMMON_H 1

#include "nslcd.h"
#include "nslcd-common.h"
#include "common/tio.h"
#include "compat/attrs.h"
#include "myldap.h"

/* macros for basic read and write operations, the following
   ERROR_OUT* marcos define the action taken on errors
   the stream is not closed because the caller closes the
   stream */

#define ERROR_OUT_WRITEERROR(fp) \
  log_log(LOG_WARNING,"error writing to client"); \
  return -1;

#define ERROR_OUT_READERROR(fp) \
  log_log(LOG_WARNING,"error reading from client"); \
  return -1;

#define ERROR_OUT_BUFERROR(fp) \
  log_log(LOG_WARNING,"client supplied argument too large"); \
  return -1;

/* a simple wrapper around snprintf,
   returns 0 if ok, -1 on error */
int mysnprintf(char *buffer,size_t buflen,const char *format, ...)
  LIKE_PRINTF(3,4);

/* This tries to get the user password attribute from the entry.
   It will try to return an encrypted password as it is used in /etc/passwd,
   /etc/group or /etc/shadow depending upon what is in the directory.
   This function will return NULL if no passwd is found and will return the
   literal value in the directory if conversion is not possible. */
const char *get_userpassword(MYLDAP_ENTRY *entry,const char *attr);

/* write out an address, parsing the addr value */
int write_address(TFILE *fp,const char *addr);

/* a helper macro to write out addresses and bail out on errors */
#define WRITE_ADDRESS(fp,addr) \
  if (write_address(fp,addr)) \
    return -1;

/* read an address from the stream */
int read_address(TFILE *fp,char *addr,int *addrlen,int *af);

/* helper macro to read an address from the stream */
#define READ_ADDRESS(fp,addr,len,af) \
  len=(int)sizeof(addr); \
  if (read_address(fp,addr,&(len),&(af))) \
    return -1;

/* checks to see if the specified string is a valid username */
MUST_USE int isvalidusername(const char *name);

/* transforms the DN info a uid doing an LDAP lookup if needed */
MUST_USE char *dn2uid(MYLDAP_SESSION *session,const char *dn,char *buf,size_t buflen);

/* transforms the uid into a DN by doing an LDAP lookup */
MUST_USE char *uid2dn(MYLDAP_SESSION *session,const char *uid,char *buf,size_t buflen);

/* these are the different functions that handle the database
   specific actions, see nslcd.h for the action descriptions */
int nslcd_alias_byname(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_alias_all(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_ether_byname(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_ether_byether(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_ether_all(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_group_byname(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_group_bygid(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_group_bymember(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_group_all(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_host_byname(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_host_byaddr(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_host_all(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_netgroup_byname(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_network_byname(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_network_byaddr(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_network_all(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_passwd_byname(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_passwd_byuid(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_passwd_all(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_protocol_byname(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_protocol_bynumber(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_protocol_all(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_rpc_byname(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_rpc_bynumber(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_rpc_all(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_service_byname(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_service_bynumber(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_service_all(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_shadow_byname(TFILE *fp,MYLDAP_SESSION *session);
int nslcd_shadow_all(TFILE *fp,MYLDAP_SESSION *session);

/* macro for generating service handling code */
#define NSLCD_HANDLE(db,fn,readfn,logcall,action,mkfilter,writefn) \
  int nslcd_##db##_##fn(TFILE *fp,MYLDAP_SESSION *session) \
  { \
    /* define common variables */ \
    int32_t tmpint32; \
    MYLDAP_SEARCH *search; \
    MYLDAP_ENTRY *entry; \
    int rc; \
    /* read request parameters */ \
    readfn; \
    /* log call */ \
    logcall; \
    /* write the response header */ \
    WRITE_INT32(fp,NSLCD_VERSION); \
    WRITE_INT32(fp,action); \
    /* prepare the search filter */ \
    if (mkfilter) \
    { \
      log_log(LOG_WARNING,"nslcd_" __STRING(db) "_" __STRING(fn) "(): filter buffer too small"); \
      return -1; \
    } \
    /* build the list of attributes */ \
    db##_init(); \
    /* do the LDAP search */ \
    if ((search=myldap_search(session,db##_base,db##_scope,filter,db##_attrs))==NULL) \
      return -1; \
    /* go over results */ \
    while ((entry=myldap_get_entry(search,&rc))!=NULL) \
    { \
      if (writefn) \
        return -1; \
    } \
    /* write the final result code */ \
    if (rc==LDAP_SUCCESS) \
    { \
      WRITE_INT32(fp,NSLCD_RESULT_END); \
    } \
    return 0; \
  }

#endif /* not _SERVER_COMMON_H */
