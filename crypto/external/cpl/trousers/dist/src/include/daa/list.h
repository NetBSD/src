
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004
 *
 */

#ifndef LIST_H_
#define LIST_H_

/* simple linked list template */
struct _list_t {
  void *obj;
  struct _list_t *next; // pointer to next node
};

typedef struct _list_t node_t; // each link is a list "node"

typedef struct {
	node_t *head; // pointer to first node
	node_t *current;
	node_t *previous;
} list_struct;

typedef list_struct* list_ptr;
typedef list_struct list_t[1];


list_ptr list_new();

void list_add(list_ptr list, void * obj);

void list_dump(list_ptr list);

void list_freeall(list_ptr list);

#endif /*LIST_H_*/
