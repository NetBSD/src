/*
   test_dict.c - simple test for the dict module
   This file is part of the nss-ldapd library.

   Copyright (C) 2007, 2008 Arthur de Jong

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
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "common/dict.h"
#include "compat/attrs.h"

/* Simple test that adds a few key/value pairs to the dict and the does
   most operations. */
static void test_simple(void)
{
  DICT *dict;
  const char *key;
  void *val;
  static char *value1="value1";
  static char *value2="value2";
  static char *replace2="replace2";
  /* initialize */
  dict=dict_new();
  /* store some entries */
  dict_put(dict,"key1",value1);
  dict_put(dict,"key2",value2);
  dict_put(dict,"key3",dict);
  dict_put(dict,"KEY2",replace2);
  /* check dictionary contents */
  val=dict_get(dict,"KeY1");
  assert(val==value1);
  val=dict_get(dict,"kEy2");
  assert(val==replace2);
  val=dict_get(dict,"KeY3");
  assert(val==dict);
  val=dict_get(dict,"key4");
  assert(val==NULL);
  /* remove a key */
  dict_put(dict,"kEy3",NULL);
  val=dict_get(dict,"keY3");
  assert(val==NULL);
  /* loop over dictionary contents */
  dict_loop_first(dict);
  while (dict_loop_next(dict,&key,&val)!=NULL)
  {
    assert(((val==value1)||(val==replace2)));
  }
  /* free dictionary */
  dict_free(dict);
}

/* Test to insert a large number of elements in the dict. */
static void test_lotsofelements(void)
{
  DICT *dict;
  char buf[80];
  int i,r;
  const char *key;
  void *val;
  /* initialize */
  dict=dict_new();
  /* insert a number of entries */
  for (i=0;i<1024;i++)
  {
    r=1+(int)(10000.0*(rand()/(RAND_MAX+1.0)));
    sprintf(buf,"test%04d",r);
    dict_put(dict,buf,&buf);
  }
  /* remove a number of entries */
  for (i=0;i<100;i++)
  {
    r=1+(int)(10000.0*(rand()/(RAND_MAX+1.0)));
    sprintf(buf,"test%04d",r);
    dict_put(dict,buf,NULL);
  }
  /* add some more entries */
  for (i=0;i<1024;i++)
  {
    r=1+(int)(10000.0*(rand()/(RAND_MAX+1.0)));
    sprintf(buf,"test%04d",r);
    dict_put(dict,buf,&buf);
  }
  /* loop over dictionary contents */
  dict_loop_first(dict);
  while (dict_loop_next(dict,&key,&val)!=NULL)
  {
    assert(val==buf);
  }
  /* free dictionary */
  dict_free(dict);
}

/* Test to insert a large number of elements in the dict. */
static void test_readelements(const char *fname)
{
  DICT *dict;
  char buf[80];
  FILE *fp;
  const char *key;
  void *val;
  /* initialize */
  dict=dict_new();
  /* read file and insert all entries */
  fp=fopen(fname,"r");
  assert(fp!=NULL);
  while (fgets(buf,sizeof(buf),fp)!=NULL)
  {
    /* strip newline */
    buf[strlen(buf)-1]='\0';
    dict_put(dict,buf,&buf);
  }
  fclose(fp);
  /* loop over dictionary contents */
  dict_loop_first(dict);
  while (dict_loop_next(dict,&key,&val)!=NULL)
  {
    assert(val==buf);
  }
  /* free dictionary */
  dict_free(dict);
}

static void test_countelements(int num)
{
  DICT *dict;
  char buf[80];
  int i,j,r;
  const char *key;
  void *val;
  /* initialize */
  dict=dict_new();
  /* insert a number of entries */
  for (i=0;i<num;i++)
  {
    r=1+(int)(10000.0*(rand()/(RAND_MAX+1.0)));
    sprintf(buf,"%04dx%04d",i,r);
    dict_put(dict,buf,&buf);
  }
  /* loop over dictionary contents */
  dict_loop_first(dict);
  i=0;
  while (dict_loop_next(dict,&key,&val)!=NULL)
  {
    assert(val==buf);
    i++;
  }
  /* we should have num elements */
  assert(i==num);
  /* free dictionary */
  dict_free(dict);
}

/* the main program... */
int main(int UNUSED(argc),char UNUSED(*argv[]))
{
  char *srcdir;
  char fname[100];
  /* build the name of the file */
  srcdir=getenv("srcdir");
  if (srcdir==NULL)
    strcpy(fname,"usernames.txt");
  else
    snprintf(fname,sizeof(fname),"%s/usernames.txt",srcdir);
  fname[sizeof(fname)-1]='\0';
  /* run the tests */
  test_simple();
  test_lotsofelements();
  test_readelements(fname);
  test_countelements(0);
  test_countelements(1);
  test_countelements(2);
  test_countelements(3);
  test_countelements(4);
  test_countelements(10);
  test_countelements(20);
  return 0;
}
