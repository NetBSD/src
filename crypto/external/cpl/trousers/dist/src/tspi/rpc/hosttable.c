
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
#include "tsplog.h"
#include "hosttable.h"
#include "obj.h"

struct host_table *ht = NULL;

TSS_RESULT
host_table_init()
{
	ht = calloc(1, sizeof(struct host_table));
	if (ht == NULL) {
		LogError("malloc of %zd bytes failed.", sizeof(struct host_table));
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	MUTEX_INIT(ht->lock);

	return TSS_SUCCESS;
}

#ifdef SOLARIS
#pragma init(_init)
void _init(void)
#else
void __attribute__ ((constructor)) my_init(void)
#endif
{
	host_table_init();
	__tspi_obj_list_init();
}

void
host_table_final()
{
	struct host_table_entry *hte, *next = NULL;

	MUTEX_LOCK(ht->lock);

	for (hte = ht->entries; hte; hte = next) {
		if (hte)
			next = hte->next;
		if (hte->hostname)
			free(hte->hostname);
		if (hte->comm.buf)
			free(hte->comm.buf);
		free(hte);
	}

	MUTEX_UNLOCK(ht->lock);

	free(ht);
	ht = NULL;
}

#ifdef SOLARIS
#pragma fini(_fini)
void _fini(void)
#else
void __attribute__ ((destructor)) my_fini(void)
#endif
{
	host_table_final();
}

TSS_RESULT
__tspi_add_table_entry(TSS_HCONTEXT tspContext, BYTE *host, int type, struct host_table_entry **ret)
{
	struct host_table_entry *entry, *tmp;

        entry = calloc(1, sizeof(struct host_table_entry));
        if (entry == NULL) {
                LogError("malloc of %zd bytes failed.", sizeof(struct host_table_entry));
                return TSPERR(TSS_E_OUTOFMEMORY);
        }

	entry->tspContext = tspContext;
        entry->hostname = host;
        entry->type = type;
        entry->comm.buf_size = TCSD_INIT_TXBUF_SIZE;
        entry->comm.buf = calloc(1, entry->comm.buf_size);
        if (entry->comm.buf == NULL) {
                LogError("malloc of %u bytes failed.", entry->comm.buf_size);
                free(entry);
                return TSPERR(TSS_E_OUTOFMEMORY);
        }
        MUTEX_INIT(entry->lock);

	MUTEX_LOCK(ht->lock);

	for (tmp = ht->entries; tmp; tmp = tmp->next) {
		if (tmp->tspContext == tspContext) {
			LogError("Tspi_Context_Connect attempted on an already connected context!");
			MUTEX_UNLOCK(ht->lock);
			free(entry->hostname);
			free(entry->comm.buf);
			free(entry);
			return TSPERR(TSS_E_CONNECTION_FAILED);
		}
	}

	if( ht->entries == NULL ) {
		ht->entries = entry;
	} else {
		for (tmp = ht->entries; tmp->next; tmp = tmp->next)
			;
		tmp->next = entry;
	}
	MUTEX_UNLOCK(ht->lock);

	*ret = entry;

	return TSS_SUCCESS;
}

void
remove_table_entry(TSS_HCONTEXT tspContext)
{
	struct host_table_entry *hte, *prev = NULL;

	MUTEX_LOCK(ht->lock);

	for (hte = ht->entries; hte; prev = hte, hte = hte->next) {
		if (hte->tspContext == tspContext) {
			if (prev != NULL)
				prev->next = hte->next;
			else
				ht->entries = hte->next;
			if (hte->hostname)
				free(hte->hostname);
			free(hte->comm.buf);
			free(hte);
			break;
		}
	}

	MUTEX_UNLOCK(ht->lock);
}

struct host_table_entry *
get_table_entry(TSS_HCONTEXT tspContext)
{
	struct host_table_entry *index = NULL;

	MUTEX_LOCK(ht->lock);

	for (index = ht->entries; index; index = index->next) {
		if (index->tspContext == tspContext)
			break;
	}

	if (index)
		MUTEX_LOCK(index->lock);

	MUTEX_UNLOCK(ht->lock);

	return index;
}

void
put_table_entry(struct host_table_entry *entry)
{
	if (entry)
		MUTEX_UNLOCK(entry->lock);
}

