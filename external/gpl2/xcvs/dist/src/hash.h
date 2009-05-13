/*
 * Copyright (C) 1986-2005 The Free Software Foundation, Inc.
 *
 * Portions Copyright (C) 1998-2005 Derek Price, Ximbiot <http://ximbiot.com>,
 *                                  and others.
 *
 * Portions Copyright (C) 1992, Brian Berliner and Jeff Polk
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 */

/*
 * The number of buckets for the hash table contained in each list.  This
 * should probably be prime.
 */
#define HASHSIZE	151

/*
 * Types of nodes
 */
enum ntype
{
    NT_UNKNOWN, HEADER, ENTRIES, FILES, LIST, RCSNODE,
    RCSVERS, DIRS, UPDATE, LOCK, NDBMNODE, FILEATTR,
    VARIABLE, RCSFIELD, RCSCMPFLD
};
typedef enum ntype Ntype;

struct node
{
    Ntype type;
    struct node *next;
    struct node *prev;
    struct node *hashnext;
    struct node *hashprev;
    char *key;
    void *data;
    void (*delproc) (struct node *);
};
typedef struct node Node;

struct list
{
    Node *list;
    Node *hasharray[HASHSIZE];
    struct list *next;
};
typedef struct list List;

List *getlist (void);
Node *findnode (List *list, const char *key);
Node *findnode_fn (List *list, const char *key);
Node *getnode (void);
int insert_before (List *list, Node *marker, Node *p);
int addnode (List *list, Node *p);
int addnode_at_front (List *list, Node *p);
int walklist (List *list, int (*)(Node *n, void *closure), void *closure);
int list_isempty (List *list);
void removenode (Node *p);
void mergelists (List *dest, List **src);
void dellist (List **listp);
void delnode (Node *p);
void freenode (Node *p);
void sortlist (List *list, int (*)(const Node *, const Node *));
int fsortcmp (const Node *p, const Node *q);
