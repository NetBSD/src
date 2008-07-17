/*
   common.h - common functions for NSS lookups

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

#ifndef _NSS_COMMON_H
#define _NSS_COMMON_H 1

#include <stdio.h>
#include <nss.h>

#include "nslcd.h"
#include "nslcd-common.h"
#include "compat/attrs.h"
#include "common/tio.h"

/* returns a socket to the server or NULL on error (see errno),
   socket should be closed with tio_close() */
TFILE *nslcd_client_open(void)
  MUST_USE;

/* These are macors for performing common operations in the nslcd
   request/response protocol, they are an extension for client
   applications to the macros defined in nslcd-common.h. */

/* Open a client socket. */
#define OPEN_SOCK(fp) \
  if ((fp=nslcd_client_open())==NULL) \
    { ERROR_OUT_OPENERROR }

/* Write a request header with a request code. */
#define WRITE_REQUEST(fp,req) \
  WRITE_INT32(fp,(int32_t)NSLCD_VERSION) \
  WRITE_INT32(fp,(int32_t)req)

/* Read a response header and check that the returned request
   code equals the expected code. */
#define READ_RESPONSEHEADER(fp,req) \
  READ_TYPE(fp,tmpint32,int32_t); \
  if (tmpint32!=(int32_t)NSLCD_VERSION) \
    { ERROR_OUT_READERROR(fp) } \
  READ_TYPE(fp,tmpint32,int32_t); \
  if (tmpint32!=(int32_t)(req)) \
    { ERROR_OUT_READERROR(fp) }

/* Read the response code (the result code of the query) from
   the stream. */
#define READ_RESPONSE_CODE(fp) \
  READ_TYPE(fp,tmpint32,int32_t); \
  if (tmpint32!=(int32_t)NSLCD_RESULT_SUCCESS) \
    { ERROR_OUT_NOSUCCESS(fp,tmpint32) }

/* These are macros for handling read and write problems, they are
   NSS specific due to the return code so are defined here. They
   genrally close the open file, set an error code and return with
   an error status. */

/* Macro is called to handle errors in opening a client connection. */
#define ERROR_OUT_OPENERROR \
  *errnop=ENOENT; \
  return (errno==EAGAIN)?NSS_STATUS_TRYAGAIN:NSS_STATUS_UNAVAIL;

/* Macro is called to handle errors on read operations. */
#define ERROR_OUT_READERROR(fp) \
  (void)tio_close(fp); \
  fp=NULL; \
  *errnop=ENOENT; \
  return NSS_STATUS_UNAVAIL;

/* Macro is called to handle problems with too small a buffer.
   This triggers the caller to call the function with a larger
   buffer (see NSS_GETENT below). */
#define ERROR_OUT_BUFERROR(fp) \
  *errnop=ERANGE; \
  return NSS_STATUS_TRYAGAIN;

/* This macro is called if there was a problem with a write
   operation. */
#define ERROR_OUT_WRITEERROR(fp) \
  ERROR_OUT_READERROR(fp)

/* This macro is called if the read status code is not
   NSLCD_RESULT_SUCCESS. */
#define ERROR_OUT_NOSUCCESS(fp,retv) \
  (void)tio_close(fp); \
  fp=NULL; \
  return NSS_STATUS_NOTFOUND;

/* The following macros to automatically generate get..byname(),
   get..bynumber(), setent(), getent() and endent() function
   bodies. These functions have very common code so this can
   easily be reused. */

/* This is a generic get..by..() generation macro. The action
   parameter is the NSLCD_ACTION_.. action, the param is the
   operation for writing the parameter and readfn is the function
   name for reading a single result entry. The function is assumed
   to have result, buffer, buflen and errnop parameters that define
   the result structure, the user buffer with length and the
   errno to return. This macro should be called through some of
   the customized ones below. */
#define NSS_BYGEN(action,writefn,readfn) \
  TFILE *fp; \
  int32_t tmpint32; \
  enum nss_status retv; \
  /* open socket and write request */ \
  OPEN_SOCK(fp); \
  WRITE_REQUEST(fp,action); \
  writefn; \
  WRITE_FLUSH(fp); \
  /* read response header */ \
  READ_RESPONSEHEADER(fp,action); \
  /* read response */ \
  READ_RESPONSE_CODE(fp); \
  retv=readfn; \
  /* close socket and we're done */ \
  if ((retv==NSS_STATUS_SUCCESS)||(retv==NSS_STATUS_TRYAGAIN)) \
    (void)tio_close(fp); \
  return retv;

/* This macro can be used to generate a get..byname() function
   body. */
#define NSS_BYNAME(action,name,readfn) \
  NSS_BYGEN(action,WRITE_STRING(fp,name),readfn)

/* This macro can be used to generate a get..by..() function
   body where the value that is the key has the specified type. */
#define NSS_BYTYPE(action,val,type,readfn) \
  NSS_BYGEN(action,WRITE_TYPE(fp,val,type),readfn)

/* This macro can be used to generate a get..by..() function
   body where the value should be passed as an int32_t. */
#define NSS_BYINT32(action,val,readfn) \
  NSS_BYGEN(action,WRITE_INT32(fp,val),readfn)

/* This macro generates a simple setent() function body. A stream
   is opened, a request is written and a check is done for
   a response header. */
#define NSS_SETENT(fp) \
  if (fp!=NULL) \
  { \
    (void)tio_close(fp); \
    fp=NULL; \
  } \
  return NSS_STATUS_SUCCESS;

/* This macro generates a getent() function body. A single entry
   is read with the readfn() function. */
#define NSS_GETENT(fp,action,readfn) \
  int32_t tmpint32; \
  enum nss_status retv; \
  /* check that we have a valid file descriptor */ \
  if (fp==NULL) \
  { \
    /* open a new stream and write the request */ \
    OPEN_SOCK(fp); \
    WRITE_REQUEST(fp,action); \
    WRITE_FLUSH(fp); \
    /* read response header */ \
    READ_RESPONSEHEADER(fp,action); \
  } \
  /* prepare for buffer errors */ \
  tio_mark(fp); \
  /* read a response */ \
  READ_RESPONSE_CODE(fp); \
  retv=readfn; \
  /* check read result */ \
  if (retv==NSS_STATUS_TRYAGAIN) \
  { \
    /* if we have a full buffer try to reset the stream */ \
    if (tio_reset(fp)) \
    { \
      tio_close(fp); \
      fp=NULL; \
      /* fail with permanent error to prevent retries */ \
      *errnop=EINVAL; \
      return NSS_STATUS_UNAVAIL; \
    } \
  } \
  else if (retv!=NSS_STATUS_SUCCESS) \
    fp=NULL; /* file should be closed by now */ \
  return retv;

/* This macro generates a endent() function body. This just closes
   the stream. */
#define NSS_ENDENT(fp) \
  if (fp!=NULL) \
  { \
    (void)tio_close(fp); \
    fp=NULL; \
  } \
  return NSS_STATUS_SUCCESS;

#endif /* not _NSS_COMMON_H */
