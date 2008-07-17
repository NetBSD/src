/*
   dict.c - dictionary functions
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

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdint.h>

#include "dict.h"

/*
   This module uses a hashtable to store it's key to value mappings. The
   structure is basically as follows:

   [struct dictionary]
     \- holds an array of pointers to a linked list of [struct dict_entry]
          \- each entry has a key/value mapping

   The hashmap can be resized when the total number of elements in the hashmap
   exceeds a certain load factor.

   All the keys are copied in a separate linked list of buffers where each new
   buffer that is allocated is larger than the previous one. The first buffer
   in the linked list is always the current one.

   Note that the initial sizes of hashtable and the loadfactor still need to
   be tuned to the use in this application.
*/

/* an entry stores one key/value pair */
struct dict_entry {
  uint32_t hash;      /* used for quick matching and rehashing */
  const char *key;    /* a reference to a copy of the key */
  void *value;        /* the stored value */
  struct dict_entry *next;
};

/* the initial size of the hashtable */
#define DICT_INITSIZE 7

/* load factor at which point to grow hashtable */
#define DICT_LOADPERCENTAGE 400

/* the dictionary is a hashtable */
struct dictionary {
  int size;                      /* size of the hashtable */
  int num;                       /* total number of keys stored */
  struct dict_entry **table;     /* the hashtable */
  int loop_idx;                  /* for looping */
  struct dict_entry *loop_entry; /* for looping */
};

/* Simple hash function that computes the hash value of a lower-cased
   string. */
static uint32_t stringhash(const char *str)
{
  uint32_t hash=0;
  while (*str!='\0')
    hash=3*hash+tolower(*str++);
  return hash;
}

/* Grow the hashtable. */
static void growhashtable(DICT *dict)
{
  int i;
  int newsize;
  struct dict_entry **newtable;
  struct dict_entry *entry,*tmp;
  newsize=dict->size*3+1;
  /* allocate room for new hashtable */
  newtable=(struct dict_entry **)malloc(newsize*sizeof(struct dict_entry *));
  if (newtable==NULL)
    return; /* allocating memory failed continue to fill the existing table */
  /* clear new table */
  for (i=0;i<newsize;i++)
    newtable[i]=NULL;
  /* copy old hashtable into new table */
  for (i=0;i<dict->size;i++)
  {
    /* go over elements in linked list */
    entry=dict->table[i];
    while (entry!=NULL)
    {
      tmp=entry;
      entry=entry->next;
      /* put in new position */
      tmp->next=newtable[tmp->hash%newsize];
      newtable[tmp->hash%newsize]=tmp;
    }
  }
  /* free the old hashtable */
  free(dict->table);
  /* put new hashtable in place */
  dict->size=newsize;
  dict->table=newtable;
}

DICT *dict_new(void)
{
  struct dictionary *dict;
  int i;
  /* allocate room for dictionary information */
  dict=(struct dictionary *)malloc(sizeof(struct dictionary));
  if (dict==NULL)
    return NULL;
  dict->size=DICT_INITSIZE;
  dict->num=0;
  /* allocate initial hashtable */
  dict->table=(struct dict_entry **)malloc(DICT_INITSIZE*sizeof(struct dict_entry *));
  if (dict->table==NULL)
  {
    free(dict);
    return NULL;
  }
  /* clear the hashtable */
  for (i=0;i<DICT_INITSIZE;i++)
    dict->table[i]=NULL;
  /* we're done */
  return dict;
}

void dict_free(DICT *dict)
{
  struct dict_entry *entry,*etmp;
  int i;
  /* free hashtable entries */
  for (i=0;i<dict->size;i++)
  {
    entry=dict->table[i];
    while (entry!=NULL)
    {
      etmp=entry;
      entry=entry->next;
      free(etmp);
    }
  }
  /* free the hashtable */
  free(dict->table);
  /* free dictionary struct itself */
  free(dict);
}

void *dict_get(DICT *dict,const char *key)
{
  uint32_t hash;
  struct dict_entry *entry;
  /* calculate the hash */
  hash=stringhash(key);
  /* loop over the linked list in the hashtable */
  for (entry=dict->table[hash%dict->size];entry!=NULL;entry=entry->next)
  {
    if ( (entry->hash==hash) &&
         (strcasecmp(entry->key,key)==0) )
      return entry->value;
  }
  /* no matches found */
  return NULL;
}

int dict_put(DICT *dict,const char *key,void *value)
{
  uint32_t hash;
  int l;
  char *buf;
  int idx;
  struct dict_entry *entry,*prev;
  /* check if we should grow the hashtable */
  if ( dict->num >= ((dict->size*DICT_LOADPERCENTAGE)/100) )
    growhashtable(dict);
  /* calculate the hash and position in the hashtable */
  hash=stringhash(key);
  idx=hash%dict->size;
  /* check if the entry is already present */
  for (entry=dict->table[idx],prev=NULL;
       entry!=NULL;
       prev=entry,entry=entry->next)
  {
    if ( (entry->hash==hash) &&
         (strcasecmp(entry->key,key)==0) )
    {
      /* check if we should unset the entry */
      if (value==NULL)
      {
        /* remove from linked list */
        if (prev==NULL)
          dict->table[idx]=entry->next;
        else
          prev->next=entry->next;
        /* free entry memory and register removal */
        free(entry);
        dict->num--;
        return 0;
      }
      /* just set the new value */
      entry->value=value;
      return 0;
    }
  }
  /* if entry should be unset we're done */
  if (value==NULL)
    return 0;
  /* entry is not present, make new entry */
  l=strlen(key)+1;
  buf=(char *)malloc(sizeof(struct dict_entry)+l);
  if (buf==NULL)
    return -1;
  entry=(struct dict_entry *)buf;
  buf+=sizeof(struct dict_entry);
  strcpy(buf,key);
  entry->hash=hash;
  entry->key=buf;
  entry->value=value;
  /* insert into hashtable/linked list */
  entry->next=dict->table[idx];
  dict->table[idx]=entry;
  /* increment number of stored items */
  dict->num++;
  return 0;
}

void dict_loop_first(DICT *dict)
{
  dict->loop_idx=0;
  dict->loop_entry=NULL;
}

const char *dict_loop_next(DICT *dict,const char **key,void **value)
{
  struct dict_entry *entry;
  /* find non-empty entry */
  while ( (dict->loop_idx<dict->size) && (dict->loop_entry==NULL) )
    dict->loop_entry=dict->table[dict->loop_idx++];
  if (dict->loop_entry==NULL)
    return NULL; /* no more entries to check */
  /* save current result and go to next entry */
  entry=dict->loop_entry;
  dict->loop_entry=entry->next;
  /* return results */
  if (key!=NULL)
    *key=entry->key;
  if (value!=NULL)
    *value=entry->value;
  return entry->key;
}
