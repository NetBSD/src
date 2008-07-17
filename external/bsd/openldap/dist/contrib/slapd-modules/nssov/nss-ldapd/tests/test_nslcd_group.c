/*
   test_nslcd_group.c - simple tests of developed lookup code

   Copyright (C) 2008 Arthur de Jong

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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include "common/tio.h"
#include "nslcd/myldap.h"

/* include group code because we want to test static methods */
#include "nslcd/group.c"

static void test_isvalidgroupname(void)
{
  assert(isvalidgroupname("foo"));
  assert(!isvalidgroupname("foo^"));
  assert(!isvalidgroupname("-foo"));
  assert(isvalidgroupname("foo-bar"));
}

static void test_group_all(MYLDAP_SESSION *session,TFILE *fp)
{
  MYLDAP_SEARCH *search;
  MYLDAP_ENTRY *entry;
  int rc;
  /* build the list of attributes */
  group_init();
  /* do the LDAP search */
  search=myldap_search(session,group_base,group_scope,group_filter,group_attrs);
  assert(search!=NULL);
  /* go over results */
  while ((entry=myldap_get_entry(search,&rc))!=NULL)
  {
    if (write_group(fp,entry,NULL,NULL,1,session))
      return;
  }
}

static void test_group_byname(MYLDAP_SESSION *session,TFILE *fp,const char *name)
{
  MYLDAP_SEARCH *search;
  MYLDAP_ENTRY *entry;
  int rc;
  char filter[1024];
  /* build the list of attributes */
  group_init();
  /* build the filter */
  mkfilter_group_byname(name,filter,sizeof(filter));
  /* do the LDAP search */
  search=myldap_search(session,group_base,group_scope,filter,group_attrs);
  assert(search!=NULL);
  /* go over results */
  while ((entry=myldap_get_entry(search,&rc))!=NULL)
  {
    if (write_group(fp,entry,NULL,NULL,1,session))
      return;
  }
}

static void initconfig(void)
{
  char *srcdir;
  char fname[100];
  /* build the name of the file to read */
  srcdir=getenv("srcdir");
  if (srcdir==NULL)
    strcpy(fname,"nss-ldapd-test.conf");
  else
    snprintf(fname,sizeof(fname),"%s/nss-ldapd-test.conf",srcdir);
  fname[sizeof(fname)-1]='\0';
  /* load config file */
  cfg_init(fname);
  /* partially initialize logging */
  log_setdefaultloglevel(LOG_DEBUG);
}

static TFILE *opendummyfile(void)
{
  int fd;
  struct timeval timeout;
  /* set the timeout */
  timeout.tv_sec=2;
  timeout.tv_usec=0;
  /* open the file for writing the result data */
  fd=open("/dev/null",O_RDWR,0);
  assert(fd>=0);
  return tio_fdopen(fd,&timeout,&timeout,1024,2*1024,1024,2*1024);
}

/* the main program... */
int main(int UNUSED(argc),char UNUSED(*argv[]))
{
  MYLDAP_SESSION *session;
  TFILE *fp;
  /* initialize configuration */
  initconfig();
  /* initialize session */
  session=myldap_create_session();
  assert(session!=NULL);
  /* get a stream */
  fp=opendummyfile();
  assert(fp!=NULL);
  /* perform tests */
  test_isvalidgroupname();
  test_group_byname(session,fp,"testgroup");
  test_group_byname(session,fp,"testgroup2");
  test_group_all(session,fp);
  /* close session */
  myldap_session_close(session);
  tio_close(fp);
  return 0;
}
