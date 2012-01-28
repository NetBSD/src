
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004
 *
 */

#ifndef _MEMMGR_H_
#define _MEMMGR_H_

/*
 * For each TSP context, there is one memTable, which holds a list of memEntry's,
 * each of which holds a pointer to some malloc'd memory that's been returned to
 * the user. The memTable also can point to other memTable's which would be
 * created if multiple TSP contexts were opened.
 *
 */

struct memEntry {
	void *memPointer;
	struct memEntry *nextEntry;
};

struct memTable {
	TSS_HCONTEXT tspContext;
	struct memEntry *entries;
	struct memTable *nextTable;
};

MUTEX_DECLARE_INIT(memtable_lock);

struct memTable *SpiMemoryTable = NULL;

#endif
