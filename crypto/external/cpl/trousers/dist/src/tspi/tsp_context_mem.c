
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "memmgr.h"
#include "tsplog.h"
#include "obj.h"

static struct memTable *
__tspi_createTable()
{
	struct memTable *table = NULL;
	/*
	 * No table has yet been created to hold the memory allocations of
	 * this context, so we need to create one
	 */
	table = calloc(1, sizeof(struct memTable));
	if (table == NULL) {
		LogError("malloc of %zd bytes failed.", sizeof(struct memTable));
		return NULL;
	}
	return (table);
}

/* caller needs to lock memtable lock */
struct memTable *
getTable(TSS_HCONTEXT tspContext)
{
	struct memTable *tmp;

	for (tmp = SpiMemoryTable; tmp; tmp = tmp->nextTable)
		if (tmp->tspContext == tspContext)
			return tmp;

	return NULL;
}

/* caller needs to lock memtable lock */
static void
__tspi_addTable(struct memTable *new)
{
	struct memTable *tmp = SpiMemoryTable;

	/* base case, this is the first table */
	if (SpiMemoryTable == NULL) {
		SpiMemoryTable = new;
		return;
	}

	/* else add @new onto the end */
	for (; tmp; tmp = tmp->nextTable)
		if (tmp->nextTable == NULL) {
			tmp->nextTable = new;
			break;
		}
}

/* caller needs to lock memtable lock and be sure the context mem slot for
 * @tspContext exists before calling.
 */
void
__tspi_addEntry(TSS_HCONTEXT tspContext, struct memEntry *new)
{
	struct memTable *tmp = getTable(tspContext);
	struct memEntry *tmp_entry;

	if (tmp == NULL) {
		if ((tmp = __tspi_createTable()) == NULL)
			return;
		tmp->tspContext = tspContext;
		__tspi_addTable(tmp);
	}

	tmp_entry = tmp->entries;

	if (tmp->entries == NULL) {
		tmp->entries = new;
		return;
	}

	/* else tack @new onto the end */
	for (; tmp_entry; tmp_entry = tmp_entry->nextEntry) {
		if (tmp_entry->nextEntry == NULL) {
			tmp_entry->nextEntry = new;
			break;
		}
	}
}

/* caller needs to lock memtable lock */
TSS_RESULT
__tspi_freeTable(TSS_HCONTEXT tspContext)
{
	struct memTable *prev = NULL, *index = NULL, *next = NULL;
	struct memEntry *entry = NULL, *entry_next = NULL;

	for(index = SpiMemoryTable; index; index = index->nextTable) {
		next = index->nextTable;
		if (index->tspContext == tspContext) {
			for (entry = index->entries; entry; entry = entry_next) {
				/* this needs to be set before we do free(entry) */
				entry_next = entry->nextEntry;
				free(entry->memPointer);
				free(entry);
			}

			if (prev != NULL)
				prev->nextTable = next;
			else
				SpiMemoryTable = NULL;

			free(index);
			break;
		}
		prev = index;
	}

	return TSS_SUCCESS;
}

TSS_RESULT
__tspi_freeEntry(struct memTable *table, void *pointer)
{
	struct memEntry *index = NULL;
	struct memEntry *prev = NULL;
	struct memEntry *toKill = NULL;

	for (index = table->entries; index; prev = index, index = index->nextEntry) {
		if (index->memPointer == pointer) {
			toKill = index;
			if (prev == NULL)
				table->entries = toKill->nextEntry;
			else
				prev->nextEntry = toKill->nextEntry;

			free(pointer);
			free(toKill);
			return TSS_SUCCESS;
		}
	}

	return TSPERR(TSS_E_INVALID_RESOURCE);
}

TSS_RESULT
__tspi_add_mem_entry(TSS_HCONTEXT tspContext, void *allocd_mem)
{
	struct memEntry *newEntry = calloc(1, sizeof(struct memEntry));
	if (newEntry == NULL) {
		LogError("malloc of %zd bytes failed.", sizeof(struct memEntry));
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	newEntry->memPointer = allocd_mem;

	MUTEX_LOCK(memtable_lock);

	__tspi_addEntry(tspContext, newEntry);

	MUTEX_UNLOCK(memtable_lock);

	return TSS_SUCCESS;
}

/*
 * calloc_tspi will be called by functions outside of this file. All locking
 * is done here.
 */
void *
calloc_tspi(TSS_HCONTEXT tspContext, UINT32 howMuch)
{
	struct memTable *table = NULL;
	struct memEntry *newEntry = NULL;

	MUTEX_LOCK(memtable_lock);

	table = getTable(tspContext);
	if (table == NULL) {
		if ((table = __tspi_createTable()) == NULL) {
			MUTEX_UNLOCK(memtable_lock);
			return NULL;
		}
		table->tspContext = tspContext;
		__tspi_addTable(table);
	}

	newEntry = calloc(1, sizeof(struct memEntry));
	if (newEntry == NULL) {
		LogError("malloc of %zd bytes failed.", sizeof(struct memEntry));
		MUTEX_UNLOCK(memtable_lock);
		return NULL;
	}

	newEntry->memPointer = calloc(1, howMuch);
	if (newEntry->memPointer == NULL) {
		LogError("malloc of %d bytes failed.", howMuch);
		free(newEntry);
		MUTEX_UNLOCK(memtable_lock);
		return NULL;
	}

	/* this call must happen inside the lock or else another thread could
	 * remove the context mem slot, causing a segfault
	 */
	__tspi_addEntry(tspContext, newEntry);

	MUTEX_UNLOCK(memtable_lock);

	return newEntry->memPointer;
}

/*
 * free_tspi will be called by functions outside of this file. All locking
 * is done here.
 */
TSS_RESULT
free_tspi(TSS_HCONTEXT tspContext, void *memPointer)
{
	struct memTable *index;
	TSS_RESULT result;

	MUTEX_LOCK(memtable_lock);

	if (memPointer == NULL) {
		result = __tspi_freeTable(tspContext);
		MUTEX_UNLOCK(memtable_lock);
		return result;
	}

	if ((index = getTable(tspContext)) == NULL) {
		MUTEX_UNLOCK(memtable_lock);
		/* Tspi_Context_FreeMemory checks that the TSP context is good before calling us,
		 * so we can be sure that the problem is with memPointer */
		return TSPERR(TSS_E_INVALID_RESOURCE);
	}

	/* just free one entry */
	result = __tspi_freeEntry(index, memPointer);

	MUTEX_UNLOCK(memtable_lock);

	return result;
}
