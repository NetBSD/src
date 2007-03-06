/*	$NetBSD: libhfsp.c,v 1.1.1.1 2007/03/06 00:10:38 dillo Exp $	*/

/*-
 * Copyright (c) 2005, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Yevgeny Binder and Dieter Baron.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */                                     

/*
 *  All functions and variable types have the prefix "hfsp_". All constants
 *  have the prefix "HFSP_".
 *
 *  Naming convention for functions which read/write raw, linear data
 *	into/from a structured form:
 *
 *  hfsp_read/write[d][a]_foo_bar
 *      [d] - read/write from/to [d]isk instead of a memory buffer
 *      [a] - [a]llocate output buffer instead of using an existing one
 *            (not applicable for writing functions)
 *
 *  Most functions do not have either of these options, so they will read from
 *	or write to a memory buffer, which has been previously allocated by the
 *	caller.
 */

#include "libhfsp.h"

/* global private file/folder keys */
hfsp_catalog_key_t hfsp_gMetadataDirectoryKey; /* contains HFS+ inodes */
hfsp_catalog_key_t hfsp_gJournalInfoBlockFileKey;
hfsp_catalog_key_t hfsp_gJournalBufferFileKey;
hfsp_catalog_key_t* hfsp_gPrivateObjectKeys[4] = {
	&hfsp_gMetadataDirectoryKey,
	&hfsp_gJournalInfoBlockFileKey,
	&hfsp_gJournalBufferFileKey,
	NULL};


extern uint16_t be16tohp(void** inout_ptr);
extern uint32_t be32tohp(void** inout_ptr);
extern uint64_t be64tohp(void** inout_ptr);

int hfsplib_create_casefolding_table(void);

#ifdef DLO_DEBUG
#include <stdio.h>
void
dlo_print_key(hfsp_catalog_key_t *key)
{
	int i;
	
	printf("%ld:[", (long)key->parent_cnid);
	for (i=0; i<key->name.length; i++) {
		if (key->name.unicode[i] < 256
		    && isprint(key->name.unicode[i]))
			putchar(key->name.unicode[i]);
		else
			printf("<%04x>", key->name.unicode[i]);
	}		    
	printf("]");
}
#endif

void
hfsplib_init(hfsp_callbacks* in_callbacks)
{
	unichar_t	temp[256];
	
	if(in_callbacks!=NULL)
		memcpy(&hfsp_gcb, in_callbacks, sizeof(hfsp_callbacks));
	
	hfsp_gcft = NULL;
	
	/*
	 * Create keys for the HFS+ "private" files so we can reuse them whenever
	 * we perform a user-visible operation, such as listing directory contents.
	 */
		
#define ATOU(str, len) /* quick & dirty ascii-to-unicode conversion */ \
	do{ int i; for(i=0; i<len; i++) temp[i]=str[i]; } \
	while( /*CONSTCOND*/ 0)
	
	ATOU("\0\0\0\0HFS+ Private Data", 21);
	hfsplib_make_catalog_key(HFSP_CNID_ROOT_FOLDER, 21, temp, 
		&hfsp_gMetadataDirectoryKey);

	ATOU(".journal_info_block", 19);
	hfsplib_make_catalog_key(HFSP_CNID_ROOT_FOLDER, 19, temp, 
		&hfsp_gJournalInfoBlockFileKey);

	ATOU(".journal", 8);
	hfsplib_make_catalog_key(HFSP_CNID_ROOT_FOLDER, 8, temp, 
		&hfsp_gJournalBufferFileKey);

#undef ATOU
}

void
hfsplib_done(void)
{
	hfsp_callback_args	cbargs;
	
	if(hfsp_gcft!=NULL) {
		hfsplib_init_cbargs(&cbargs);
		hfsplib_free(hfsp_gcft, &cbargs);
		hfsp_gcft = NULL;
	}

	return;
}

void
hfsplib_init_cbargs(hfsp_callback_args* ptr)
{
	memset(ptr, 0, sizeof(hfsp_callback_args));
}

#if 0
#pragma mark -
#pragma mark High-Level Routines
#endif

int
hfsplib_open_volume(
	const char* in_device,
	uint64_t in_offset, /* given in BYTES, not BLOCKS */
	int in_readonly,
	hfsp_volume* out_vol,
	hfsp_callback_args* cbargs)
{
	hfsp_node_descriptor_t nd; /* node descriptor for some node we're reading */
	hfsp_catalog_key_t		rootkey;
	hfsp_thread_record_t	rootthread;
	uint16_t*	node_rec_sizes;
	void*		buffer;
	void*		buffer2;	/* used as temporary pointer for realloc() */
	void**		node_recs;
	int			result;
	
	result = 1;
	buffer = NULL;
	node_recs = NULL;
	node_rec_sizes = NULL;

	if(in_device==NULL || out_vol==NULL)
		return 1;

	out_vol->readonly = in_readonly;
	
	if(hfsplib_openvoldevice(out_vol, in_device, in_offset, cbargs) != 0)
		HFSP_LIBERR("could not open device");

	/*
	 *	Read the volume header.
	 */
	buffer = hfsplib_malloc(sizeof(hfsp_volume_header_t), cbargs);
	if(buffer==NULL)
		HFSP_LIBERR("could not allocate volume header");
	
	if(hfsplib_readd(out_vol, buffer, sizeof(hfsp_volume_header_t),
		HFSP_VOLUME_HEAD_RESERVE_SIZE, cbargs)!=0)
		HFSP_LIBERR("could not read volume header");
	if(hfsplib_read_volume_header(buffer, &(out_vol->vh))==0)
		HFSP_LIBERR("could not parse volume header");
	
	/*
	 * Check the volume signature to see if this is a legitimate HFS+ or HFSX
	 * volume. If so, set the key comparison function pointers appropriately.
	 */
	switch(out_vol->vh.signature)
	{
		case HFSP_SIG_HFSP:
			out_vol->keycmp = hfsplib_compare_catalog_keys_cf;
			break;
		
		case HFSP_SIG_HFSX:
			out_vol->keycmp = NULL; /* will be set below */
			break;
			
		case HFSP_SIG_HFS:
			HFSP_LIBERR("HFS volumes and HFS+ volumes with HFS wrappers are"
						"not currently supported");
			break;
			
		default:
			HFSP_LIBERR("unrecognized volume format");
	}


	/*
	 *	Read the catalog header.
	 */
	buffer2 = hfsplib_realloc(buffer,
		out_vol->vh.catalog_file.extents[0].block_count *
		out_vol->vh.block_size, cbargs);
	if(buffer2==NULL)
		HFSP_LIBERR("could not allocate catalog header node");
	buffer = buffer2;
	
	/*	We don't use hfsplib_readd_with_extents() here because we don't know
	 *	the size of this node ahead of time. Besides, we only need the first
	 *	extent in order to get the header and root nodes. */
	if(hfsplib_readd(out_vol, buffer,
		out_vol->vh.catalog_file.extents[0].block_count*out_vol->vh.block_size,
		out_vol->vh.catalog_file.extents[0].start_block*out_vol->vh.block_size,
		cbargs) != 0)
		HFSP_LIBERR("could not read catalog header node");
	
	if(hfsplib_reada_node(buffer, &nd, &node_recs, &node_rec_sizes,
		HFSP_CATALOG_FILE, out_vol, cbargs)==0)
		HFSP_LIBERR("could not read catalog header node");

	if(hfsplib_read_header_node(node_recs, node_rec_sizes, nd.num_recs,
		&out_vol->chr, NULL, NULL)==0)
		HFSP_LIBERR("could not parse catalog header node");
	
	/* If this is an HFSX volume, the catalog header specifies the type of
	 * key comparison method (case-folding or binary compare) we should use. */
	if(out_vol->keycmp == NULL)
	{
		if(out_vol->chr.keycomp_type == HFSP_KEY_CASEFOLD)
			out_vol->keycmp = hfsplib_compare_catalog_keys_cf;
		else if(out_vol->chr.keycomp_type == HFSP_KEY_BINARY)
			out_vol->keycmp = hfsplib_compare_catalog_keys_bc;
		else
			HFSP_LIBERR("undefined key compare method");
	}
	
	/*
	 *	Read the extent overflow header.
	 */
	buffer2 = hfsplib_realloc(buffer,
		out_vol->vh.extents_file.extents[0].block_count *
		out_vol->vh.block_size, cbargs);
	if(buffer2==NULL)
		HFSP_LIBERR("could not allocate extent header node");
	buffer = buffer2;

	/*	We don't use hfsplib_readd_with_extents() here because we don't know
	 *	the size of this node ahead of time. Besides, we only need the first
	 *	extent in order to get the header and root nodes. */
	if(hfsplib_readd(out_vol, buffer,
		out_vol->vh.extents_file.extents[0].block_count*out_vol->vh.block_size,
		out_vol->vh.extents_file.extents[0].start_block*out_vol->vh.block_size,
		cbargs) != 0)
		HFSP_LIBERR("could not read extent header node");
	
	if(hfsplib_reada_node(buffer, &nd, &node_recs, &node_rec_sizes,
		HFSP_EXTENTS_FILE, out_vol, cbargs)==0)
		HFSP_LIBERR("could not read extent header node");

	if(hfsplib_read_header_node(node_recs, node_rec_sizes, nd.num_recs,
		&out_vol->ehr, NULL, NULL)==0)
		HFSP_LIBERR("could not parse extent header node");
		
	/*
	 * Read the journal info block and journal header (if volume journaled).
	 */
	if(out_vol->vh.attributes & (1<<HFSP_VOL_JOURNALED))
	{
		/* journal info block */
		buffer2 = hfsplib_realloc(buffer, sizeof(hfsp_journal_info_t), cbargs);
		if(buffer2==NULL)
			HFSP_LIBERR("could not allocate journal info block");
		buffer = buffer2;
	
		if(hfsplib_readd(out_vol, buffer, sizeof(hfsp_journal_info_t),
			out_vol->vh.journal_info_block * out_vol->vh.block_size,
			cbargs) != 0)
			HFSP_LIBERR("could not read journal info block");
		
		if(hfsplib_read_journal_info(buffer, &out_vol->jib)==0)
			HFSP_LIBERR("could not parse journal info block");
					
		/* journal header */
		buffer2 = hfsplib_realloc(buffer, sizeof(hfsp_journal_header_t),cbargs);
		if(buffer2==NULL)
			HFSP_LIBERR("could not allocate journal header");
		buffer = buffer2;
	
		if(hfsplib_readd(out_vol, buffer, sizeof(hfsp_journal_header_t),
			out_vol->jib.offset, cbargs) != 0)
			HFSP_LIBERR("could not read journal header");
		
		if(hfsplib_read_journal_header(buffer, &out_vol->jh)==0)
			HFSP_LIBERR("could not parse journal header");

		out_vol->journaled = 1;
	}
	else
	{
		out_vol->journaled = 0;
	}
	
	/*
	 * If this volume uses case-folding comparison and the folding table hasn't
	 * been created yet, do that here. (We don't do this in hfsplib_init()
	 * because the table is large and we might never even need to use it.)
	 */
	if(out_vol->keycmp==hfsplib_compare_catalog_keys_cf && hfsp_gcft==NULL)
		result = hfsplib_create_casefolding_table();
	else
		result = 0;
		
	/*
	 * Find and store the volume name.
	 */	
	if(hfsplib_make_catalog_key(HFSP_CNID_ROOT_FOLDER, 0, NULL, &rootkey)==0)
		HFSP_LIBERR("could not make root search key");
	
	if(hfsplib_find_catalog_record_with_key(out_vol, &rootkey,
		(hfsp_catalog_keyed_record_t*)&rootthread, cbargs)!=0)
		HFSP_LIBERR("could not find root parent");

	memcpy(&out_vol->name, &rootthread.name, sizeof(hfsp_unistr255_t));
	

	/* FALLTHROUGH */
error:	
	if(buffer!=NULL)
		hfsplib_free(buffer, cbargs);

	hfsplib_free_recs(&node_recs, &node_rec_sizes, &nd.num_recs, cbargs);
		
	return result;
}

void
hfsplib_close_volume(hfsp_volume* in_vol, hfsp_callback_args* cbargs)
{
	if(in_vol==NULL)
		return;
		
	hfsplib_closevoldevice(in_vol, cbargs);
}

int
hfsplib_path_to_cnid(hfsp_volume* in_vol,
	hfsp_cnid_t in_cnid,
	char** out_unicode,
	uint16_t* out_length,
	hfsp_callback_args* cbargs)
{
	hfsp_thread_record_t	parent_thread;
	hfsp_cnid_t	parent_cnid, child_cnid;
	char*		newpath;
	char*		path;
	int			path_offset = 0;
	int			result;
	uint16_t*	ptr;	/* dummy var */
	uint16_t	uchar;	/* dummy var */
	uint16_t	total_path_length;
		
	if(in_vol==NULL || in_cnid==0 || out_unicode==NULL || out_length==NULL)
		return 1;
		
	result = 1;
	*out_unicode = NULL;
	*out_length = 0;
	path = NULL;
	total_path_length = 0;
	
	path = hfsplib_malloc(514, cbargs); /* 256 unichars plus a forward slash */
	if(path==NULL)
		return 1;

	child_cnid = in_cnid;
	parent_cnid = child_cnid; /* skips loop in case in_cnid is root id */
	while(parent_cnid != HFSP_CNID_ROOT_FOLDER
		&& parent_cnid != HFSP_CNID_ROOT_PARENT)
	{
		if(child_cnid!=in_cnid)
		{
			newpath = hfsplib_realloc(path, 514 + total_path_length*2, cbargs);

			if(newpath==NULL)
				goto exit;
			path = newpath;

			memmove(path + 514, path + path_offset, total_path_length*2);
		}

		parent_cnid = hfsplib_find_parent_thread(in_vol, child_cnid,
			&parent_thread, cbargs);
		if(parent_cnid==0)
			goto exit;

		path_offset = 512 - parent_thread.name.length*2;

		memcpy(path + path_offset, parent_thread.name.unicode,
			parent_thread.name.length*2);
		
		/*	Add a forward slash. The unicode string was specified in big endian
		 *	format, so convert to core format if necessary. */
		path[512]=0x00;
		path[513]=0x2F;
		
		ptr = (uint16_t*)path + 256;
		uchar = be16tohp((void*)&ptr);
		*(ptr-1) = uchar;

		total_path_length += parent_thread.name.length + 1;

		child_cnid = parent_cnid;
	}
	
	/*
	 *	At this point, 'path' holds a sequence of unicode characters which
	 *	represent the absolute path to the given cnid. This string is missing
	 *	a terminating null char and an initial forward slash that represents
	 *	the root of the filesystem. It most likely also has extra space in
	 *	the beginning, due to the fact that we reserve 512 bytes for each path
	 *	component and won't usually use all that space. So, we allocate the
	 *	final string based on the actual length of the absolute path, plus four
	 *	additional bytes (two unichars) for the forward slash and the null char.
	 */
	
	*out_unicode = hfsplib_malloc((total_path_length+2)*2, cbargs);
	if(*out_unicode == NULL)
		goto exit;
	
	/* copy only the bytes that are actually used */
	memcpy(*out_unicode+2, path + path_offset, total_path_length*2);

	/* insert forward slash at start */
	(*out_unicode)[0] = 0x00;
	(*out_unicode)[1] = 0x2F;
	ptr = (uint16_t*)*out_unicode;
	uchar = be16tohp((void*)&ptr);
	*(ptr-1) = uchar;

	/* insert null char at end */
	(*out_unicode)[total_path_length*2+2] = 0x00;
	(*out_unicode)[total_path_length*2+3] = 0x00;
	
	*out_length = total_path_length + 1 /* extra for forward slash */ ;

	result = 0;
	
exit:
	if(path!=NULL)
		hfsplib_free(path, cbargs);
		
	return result;
}

hfsp_cnid_t
hfsplib_find_parent_thread(
	hfsp_volume* in_vol,
	hfsp_cnid_t in_child,
	hfsp_thread_record_t* out_thread,
	hfsp_callback_args* cbargs)
{	
	hfsp_catalog_key_t	childkey;

	if(in_vol==NULL || in_child==0 || out_thread==NULL)
		return 0;
	
	if(hfsplib_make_catalog_key(in_child, 0, NULL, &childkey)==0)
		return 0;
	
	if(hfsplib_find_catalog_record_with_key(in_vol, &childkey,
		(hfsp_catalog_keyed_record_t*)out_thread, cbargs)!=0)
		return 0;
		
	return out_thread->parent_cnid;
}

/*
 * hfsplib_find_catalog_record_with_cnid()
 *
 * Looks up a catalog record by calling hfsplib_find_parent_thread() and
 * hfsplib_find_catalog_record_with_key(). out_key may be NULL; if not, the key
 * corresponding to this cnid is stuffed in it. Returns 0 on success.
 */
int
hfsplib_find_catalog_record_with_cnid(
	hfsp_volume* in_vol,
	hfsp_cnid_t in_cnid,
	hfsp_catalog_keyed_record_t* out_rec,
	hfsp_catalog_key_t* out_key,
	hfsp_callback_args* cbargs)
{
	hfsp_cnid_t					parentcnid;
	hfsp_thread_record_t		parentthread;
	hfsp_catalog_key_t			key;
	
	if(in_vol==NULL || in_cnid==0 || out_rec==NULL)
		return 0;

	parentcnid =
		hfsplib_find_parent_thread(in_vol, in_cnid, &parentthread, cbargs);
	if(parentcnid == 0)
		HFSP_LIBERR("could not find parent thread for cnid %i", in_cnid);

	if(hfsplib_make_catalog_key(parentthread.parent_cnid,
		parentthread.name.length, parentthread.name.unicode, &key) == 0)
		HFSP_LIBERR("could not make catalog search key");
	
	if(out_key!=NULL)
		memcpy(out_key, &key, sizeof(key));

	return hfsplib_find_catalog_record_with_key(in_vol, &key, out_rec, cbargs);
	
error:
	return 1;
}

/* Returns 0 on success, 1 on error, and -1 if record was not found. */
int
hfsplib_find_catalog_record_with_key(
	hfsp_volume* in_vol,
	hfsp_catalog_key_t* in_key,
	hfsp_catalog_keyed_record_t* out_rec,
	hfsp_callback_args* cbargs)
{
	hfsp_node_descriptor_t			nd;
	hfsp_extent_descriptor_t*		extents;
	hfsp_catalog_keyed_record_t		lastrec;
	hfsp_catalog_key_t*	curkey;
	void**				recs;
	void*				buffer;
	uint64_t			bytesread;
	uint32_t			curnode;
	uint16_t*			recsizes;
	uint16_t			numextents;
	uint16_t			recnum;
	int16_t				leaftype;
	int					keycompare;
	int					result;

	if(in_key==NULL || out_rec==NULL || in_vol==NULL)
		return 1;
	
	result = 1;
	buffer = NULL;
	curkey = NULL;
	extents = NULL;
	recs = NULL;
	recsizes = NULL;
	
	/* The key takes up over half a kb of ram, which is a lot for the BSD
	 * kernel stack. So allocate it in the heap instead to play it safe. */
	curkey = hfsplib_malloc(sizeof(hfsp_catalog_key_t), cbargs);
	if(curkey==NULL)
		HFSP_LIBERR("could not allocate catalog search key");
	
	buffer = hfsplib_malloc(in_vol->chr.node_size, cbargs);
	if(buffer==NULL)
		HFSP_LIBERR("could not allocate node buffer");

	numextents = hfsplib_get_file_extents(in_vol, HFSP_CNID_CATALOG,
		HFSP_DATAFORK, &extents, cbargs);
	if(numextents==0)
		HFSP_LIBERR("could not locate fork extents");

	nd.num_recs = 0;
	curnode = in_vol->chr.root_node;

#ifdef DLO_DEBUG
	printf("-> key ");
	dlo_print_key(in_key);
	printf("\n");
#endif
	
	do
	{
#ifdef DLO_DEBUG
		printf("--> node %d\n", curnode);
#endif

		if(hfsplib_readd_with_extents(in_vol, buffer, 
			&bytesread,in_vol->chr.node_size, curnode * in_vol->chr.node_size, 
			extents, numextents, cbargs)!=0)
			HFSP_LIBERR("could not read catalog node #%i", curnode);

		if(hfsplib_reada_node(buffer, &nd, &recs, &recsizes, HFSP_CATALOG_FILE,
			in_vol, cbargs)==0)
			HFSP_LIBERR("could not parse catalog node #%i", curnode);

		for(recnum=0; recnum<nd.num_recs; recnum++)
		{
			leaftype = nd.kind;
			if(hfsplib_read_catalog_keyed_record(recs[recnum], out_rec,
				&leaftype, curkey, in_vol)==0)
				HFSP_LIBERR("could not read catalog record #%i",recnum);

#ifdef DLO_DEBUG
			printf("---> record %d: ", recnum);
			dlo_print_key(curkey);
			fflush(stdout);
#endif
			keycompare = in_vol->keycmp(in_key, curkey);
#ifdef DLO_DEBUG
			printf(" %c\n",
			       keycompare < 0 ? '<'
			       : keycompare == 0 ? '=' : '>');
#endif
			
			if(keycompare < 0)
			{
				/* Check if key is less than *every* record, which should never
				 * happen if the volume is consistent and the key legit. */
				if(recnum==0)
					HFSP_LIBERR("all records greater than key");
				
				/* Otherwise, we've found the first record that exceeds our key,
				 * so retrieve the previous record, which is still less... */
				memcpy(out_rec, &lastrec,
					sizeof(hfsp_catalog_keyed_record_t));

				/* ...unless this is a leaf node, which means we've gone from
				 * a key which is smaller than the search key, in the previous
				 * loop, to a key which is larger, in this loop, and that
				 * implies that our search key does not exist on the volume. */
				if(nd.kind==HFSP_LEAFNODE)
					result = -1;

				break;
			}
			else if(keycompare == 0)
			{
				/* If leaf node, found an exact match. */
				result = 0;
				break;
			}
			else if(recnum==nd.num_recs-1 && keycompare > 0)
			{
				/* If leaf node, we've reached the last record with no match,
				 * which means this key is not present on the volume. */
				result = -1;
				break;
			}

			memcpy(&lastrec, out_rec, sizeof(hfsp_catalog_keyed_record_t));
		}
		
		if(nd.kind==HFSP_INDEXNODE)
			curnode = out_rec->child;
		else if(nd.kind==HFSP_LEAFNODE)
			break;
			
		hfsplib_free_recs(&recs, &recsizes, &nd.num_recs, cbargs);
	}
	while(nd.kind!=HFSP_LEAFNODE);
	
	/* FALLTHROUGH */
error:
	if(extents!=NULL)
		hfsplib_free(extents, cbargs);
	hfsplib_free_recs(&recs, &recsizes, &nd.num_recs, cbargs);
	if(curkey!=NULL)
		hfsplib_free(curkey, cbargs);		
	if(buffer!=NULL)
		hfsplib_free(buffer, cbargs);

	return result;
}

/* returns 0 on success */
/* XXX Need to look this over and make sure it gracefully handles cases where
 * XXX the key is not found. */
int
hfsplib_find_extent_record_with_key(hfsp_volume* in_vol,
	hfsp_extent_key_t* in_key,
	hfsp_extent_record_t* out_rec,
	hfsp_callback_args* cbargs)
{
	hfsp_node_descriptor_t		nd;
	hfsp_extent_descriptor_t*	extents;
	hfsp_extent_record_t		lastrec;
	hfsp_extent_key_t	curkey;
	void**				recs;
	void*				buffer;
	uint64_t			bytesread;
	uint32_t			curnode;
	uint16_t*			recsizes;
	uint16_t			numextents;
	uint16_t			recnum;
	int					keycompare;
	int					result;
	
	if(in_vol==NULL || in_key==NULL || out_rec==NULL)
		return 1;
		
	result = 1;
	buffer = NULL;
	extents = NULL;
	recs = NULL;
	recsizes = NULL;
	
	buffer = hfsplib_malloc(in_vol->ehr.node_size, cbargs);
	if(buffer==NULL)
		HFSP_LIBERR("could not allocate node buffer");
	
	numextents = hfsplib_get_file_extents(in_vol, HFSP_CNID_EXTENTS,
		HFSP_DATAFORK, &extents, cbargs);
	if(numextents==0)
		HFSP_LIBERR("could not locate fork extents");

	nd.num_recs = 0;
	curnode = in_vol->ehr.root_node;
	
	do
	{
		hfsplib_free_recs(&recs, &recsizes, &nd.num_recs, cbargs);
		recnum = 0;

		if(hfsplib_readd_with_extents(in_vol, buffer, &bytesread, 
			in_vol->ehr.node_size, curnode * in_vol->ehr.node_size, extents, 
			numextents, cbargs)!=0)
			HFSP_LIBERR("could not read extents overflow node #%i", curnode);
		
		if(hfsplib_reada_node(buffer, &nd, &recs, &recsizes, HFSP_EXTENTS_FILE,
			in_vol, cbargs)==0)
			HFSP_LIBERR("could not parse extents overflow node #%i",curnode);

		for(recnum=0; recnum<nd.num_recs; recnum++)
		{
			memcpy(&lastrec, out_rec, sizeof(hfsp_extent_record_t));
		
			if(hfsplib_read_extent_record(recs[recnum], out_rec, nd.kind,
				&curkey, in_vol)==0)
				HFSP_LIBERR("could not read extents record #%i",recnum);

			keycompare = hfsplib_compare_extent_keys(in_key, &curkey);
			if(keycompare < 0)
			{
				/* this should never happen for any legitimate key */
				if(recnum==0)
					return 1;
					
				memcpy(out_rec, &lastrec, sizeof(hfsp_extent_record_t));

				break;
			}
			else if(keycompare == 0 ||
				(recnum==nd.num_recs-1 && keycompare > 0))
				break;
		}
		
		if(nd.kind==HFSP_INDEXNODE)
			curnode = *((uint32_t *)out_rec); /* out_rec is a node ptr in this case */
		else if(nd.kind==HFSP_LEAFNODE)
			break;
		else
		    HFSP_LIBERR("unknwon node type for extents overflow node #%i",curnode);
	}
	while(nd.kind!=HFSP_LEAFNODE);
	
	result = 0;

	/* FALLTHROUGH */

error:
	if(buffer!=NULL)
		hfsplib_free(buffer, cbargs);
	if(extents!=NULL)
		hfsplib_free(extents, cbargs);
	hfsplib_free_recs(&recs, &recsizes, &nd.num_recs, cbargs);
		
	return result;	
}

/* out_extents may be NULL. */
uint16_t
hfsplib_get_file_extents(hfsp_volume* in_vol,
	hfsp_cnid_t in_cnid,
	uint8_t in_forktype,
	hfsp_extent_descriptor_t** out_extents,
	hfsp_callback_args* cbargs)
{
	hfsp_extent_descriptor_t*	dummy;
	hfsp_extent_key_t		extentkey;
	hfsp_file_record_t		file;
	hfsp_catalog_key_t		filekey;
	hfsp_thread_record_t	fileparent;
	hfsp_fork_t				fork;
	hfsp_extent_record_t	nextextentrec;
	uint32_t	numblocks;
	uint16_t	numextents, n;
	
	if(in_vol==NULL || in_cnid==0)
		return 0;
	
	if(out_extents!=NULL)
	{
		*out_extents = hfsplib_malloc(sizeof(hfsp_extent_descriptor_t), cbargs);
		if(*out_extents==NULL)
			return 0;
	}
	
	switch(in_cnid)
	{
		case HFSP_CNID_CATALOG:
			fork = in_vol->vh.catalog_file;
			break;
		
		case HFSP_CNID_EXTENTS:
			fork = in_vol->vh.extents_file;
			break;
		
		case HFSP_CNID_ALLOCATION:
			fork = in_vol->vh.allocation_file;
			break;
		
		case HFSP_CNID_ATTRIBUTES:
			fork = in_vol->vh.attributes_file;
			break;
		
		case HFSP_CNID_STARTUP:
			fork = in_vol->vh.startup_file;
			break;
			
		default:
			if(hfsplib_find_parent_thread(in_vol, in_cnid, &fileparent,
				cbargs)==0)
				goto error;
			
			if(hfsplib_make_catalog_key(fileparent.parent_cnid,
				fileparent.name.length, fileparent.name.unicode, &filekey)==0)
				goto error;
					
			if(hfsplib_find_catalog_record_with_key(in_vol, &filekey,
				(hfsp_catalog_keyed_record_t*)&file, cbargs)!=0)
				goto error;
			
			/* only files have extents, not folders or threads */
			if(file.rec_type!=HFSP_REC_FILE)
				goto error;
			
			if(in_forktype==HFSP_DATAFORK)
				fork = file.data_fork;
			else if(in_forktype==HFSP_RSRCFORK)
				fork = file.rsrc_fork;
	}
	
	numextents = 0;
	numblocks = 0;
	memcpy(&nextextentrec, &fork.extents, sizeof(hfsp_extent_record_t));

	while(1)
	{
		for(n=0; n<8; n++)
		{
			if(nextextentrec[n].block_count==0)
				break;
				
			numblocks += nextextentrec[n].block_count;
		}

		if(out_extents!=NULL)
		{
			dummy = hfsplib_realloc(*out_extents,
			    (numextents+n) * sizeof(hfsp_extent_descriptor_t),
			    cbargs);
			if(dummy==NULL)
				goto error;
			*out_extents = dummy;
			
			memcpy(*out_extents + numextents,
			    &nextextentrec, n*sizeof(hfsp_extent_descriptor_t));
		}
		numextents += n;

		if(numblocks >= fork.total_blocks)
			break;
			
		if(hfsplib_make_extent_key(in_cnid, in_forktype, numblocks,
			&extentkey)==0)
			goto error;
		
		if(hfsplib_find_extent_record_with_key(in_vol, &extentkey,
			&nextextentrec, cbargs)!=0)
			goto error;
	}

	goto exit;
	
error:
	if(out_extents!=NULL && *out_extents!=NULL)
	{
		hfsplib_free(*out_extents, cbargs);
		*out_extents = NULL;
	}
	return 0;
	
exit:
	return numextents;
}

/*
 * hfsplib_get_directory_contents()
 *
 * Finds the immediate children of a given directory CNID and places their 
 * CNIDs in an array allocated here. The first child is found by doing a
 * catalog search that only compares parent CNIDs (ignoring file/folder names)
 * and skips over thread records. Then the remaining children are listed in 
 * ascending order by name, according to the HFS+ spec, so just read off each
 * successive leaf node until a different parent CNID is found.
 * 
 * If out_childnames is not NULL, it will be allocated and set to an array of
 * hfsp_unistr255_t's which correspond to the name of the child with that same
 * index.
 *
 * out_children may be NULL.
 *
 * Returns 0 on success.
 */
int
hfsplib_get_directory_contents(
	hfsp_volume* in_vol,
	hfsp_cnid_t in_dir,
	hfsp_catalog_keyed_record_t** out_children,
	hfsp_unistr255_t** out_childnames,
	uint32_t* out_numchildren,
	hfsp_callback_args* cbargs)
{
	hfsp_node_descriptor_t			nd;
	hfsp_extent_descriptor_t*		extents;
	hfsp_catalog_keyed_record_t		currec;
	hfsp_catalog_key_t	curkey;
	void**				recs;
	void*				buffer;
	void*				ptr; /* temporary pointer for realloc() */
	uint64_t			bytesread;
	uint32_t			curnode;
	uint32_t			lastnode;
	uint16_t*			recsizes;
	uint16_t			numextents;
	uint16_t			recnum;
	int16_t				leaftype;
	int					keycompare;
	int					result;

	if(in_vol==NULL || in_dir==0 || out_numchildren==NULL)
		return 1;
		
	result = 1;
	buffer = NULL;
	extents = NULL;
	lastnode = 0;
	recs = NULL;
	recsizes = NULL;
	*out_numchildren = 0;
	if(out_children!=NULL)
		*out_children = NULL;
	if(out_childnames!=NULL)
		*out_childnames = NULL;

	buffer = hfsplib_malloc(in_vol->chr.node_size, cbargs);
	if(buffer==NULL)
		HFSP_LIBERR("could not allocate node buffer");

	numextents = hfsplib_get_file_extents(in_vol, HFSP_CNID_CATALOG,
		HFSP_DATAFORK, &extents, cbargs);
	if(numextents==0)
		HFSP_LIBERR("could not locate fork extents");

	nd.num_recs = 0;
	curnode = in_vol->chr.root_node;
	
	while(1)
	{
		hfsplib_free_recs(&recs, &recsizes, &nd.num_recs, cbargs);
		recnum = 0;

		if(hfsplib_readd_with_extents(in_vol, buffer, &bytesread, 
			in_vol->chr.node_size, curnode * in_vol->chr.node_size, extents, 
			numextents, cbargs)!=0)
			HFSP_LIBERR("could not read catalog node #%i", curnode);

		if(hfsplib_reada_node(buffer, &nd, &recs, &recsizes, HFSP_CATALOG_FILE,
			in_vol, cbargs)==0)
			HFSP_LIBERR("could not parse catalog node #%i", curnode);

		for(recnum=0; recnum<nd.num_recs; recnum++)
		{
			leaftype = nd.kind; /* needed b/c leaftype might be modified now */
			if(hfsplib_read_catalog_keyed_record(recs[recnum], &currec,
				&leaftype, &curkey, in_vol)==0)
				HFSP_LIBERR("could not read cat record %i:%i", curnode, recnum);

			if(nd.kind==HFSP_INDEXNODE)
			{
				keycompare = in_dir - curkey.parent_cnid;
				if(keycompare < 0)
				{
					/* Check if key is less than *every* record, which should 
					 * never happen if the volume and key are good. */
					if(recnum==0)
						HFSP_LIBERR("all records greater than key");
					
					/* Otherwise, we've found the first record that exceeds our 
					 * key, so retrieve the previous, lesser record. */
					curnode = lastnode;
					break;
				}
				else if(keycompare == 0)
				{
					/*
					 * Normally, if we were doing a typical catalog lookup with
					 * both a parent cnid AND a name, keycompare==0 would be an
					 * exact match. However, since we are ignoring object names
					 * in this case and only comparing parent cnids, a direct
					 * match on only a parent cnid could mean that we've found
					 * an object with that parent cnid BUT which is NOT the
					 * first object (according to the HFS+ spec) with that
					 * parent cnid. Thus, when we find a parent cnid match, we 
					 * still go back to the previously found leaf node and start
					 * checking it for a possible prior instance of an object
					 * with our desired parent cnid.
					 */
					curnode = lastnode;
					break;					 
				}
				else if (recnum==nd.num_recs-1 && keycompare > 0)
				{
					/* Descend to child node if we found an exact match, or if
					 * this is the last pointer record. */
					curnode = currec.child;
					break;
				}
				
				lastnode = currec.child;
			}
			else
			{
				/*
				 * We have now descended down the hierarchy of index nodes into
				 * the leaf node that contains the first catalog record with a
				 * matching parent CNID. Since all leaf nodes are chained
				 * through their flink/blink, we can simply walk forward through
				 * this chain, copying every matching non-thread record, until 
				 * we hit a record with a different parent CNID. At that point,
				 * we've retrieved all of our directory's items, if any.
				 */
				curnode = nd.flink;

				if(curkey.parent_cnid<in_dir)
					continue;
				else if(curkey.parent_cnid==in_dir)
				{
					/* Hide files/folders which are supposed to be invisible
					 * to users, according to the hfs+ spec. */
					if(hfsplib_is_private_file(&curkey))
						continue;
						
					/* leaftype has now been set to the catalog record type */
					if(leaftype==HFSP_REC_FLDR || leaftype==HFSP_REC_FILE)
					{
						(*out_numchildren)++;
						
						if(out_children!=NULL)
						{
							ptr = hfsplib_realloc(*out_children, 
								*out_numchildren *
								sizeof(hfsp_catalog_keyed_record_t), cbargs);
							if(ptr==NULL)
								HFSP_LIBERR("could not allocate child record");
							*out_children = ptr;
							
							memcpy(&((*out_children)[*out_numchildren-1]), 
								&currec, sizeof(hfsp_catalog_keyed_record_t));
						}

						if(out_childnames!=NULL)
						{
							ptr = hfsplib_realloc(*out_childnames,
								*out_numchildren * sizeof(hfsp_unistr255_t),
								cbargs);
							if(ptr==NULL)
								HFSP_LIBERR("could not allocate child name");
							*out_childnames = ptr;

							memcpy(&((*out_childnames)[*out_numchildren-1]), 
								&curkey.name, sizeof(hfsp_unistr255_t));
						}
					}
				} else {
					result = 0;
					/* We have just now passed the last item in the desired
					 * folder (or the folder was empty), so exit. */
					goto exit;
				}
			}
		}
	}

	result = 0;

	goto exit;	

error:
	if(out_children!=NULL && *out_children!=NULL)
		hfsplib_free(*out_children, cbargs);
	if(out_childnames!=NULL && *out_childnames!=NULL)
		hfsplib_free(*out_childnames, cbargs);
		
	/* FALLTHROUGH */

exit:
	if(extents!=NULL)
		hfsplib_free(extents, cbargs);
	hfsplib_free_recs(&recs, &recsizes, &nd.num_recs, cbargs);
	if(buffer!=NULL)
		hfsplib_free(buffer, cbargs);

	return result;
}

int
hfsplib_is_journal_clean(hfsp_volume* in_vol)
{
	if(in_vol==NULL)
		return 0;
		
	/* return true if no journal */
	if(!(in_vol->vh.attributes & (1<<HFSP_VOL_JOURNALED)))
		return 1;
		
	return (in_vol->jh.start == in_vol->jh.end);
}

/*
 * hfsplib_is_private_file()
 *
 * Given a file/folder's key and parent CNID, determines if it should be hidden
 * from the user (e.g., the journal header file or the HFS+ Private Data folder)
 */
int
hfsplib_is_private_file(hfsp_catalog_key_t *filekey)
{
	hfsp_catalog_key_t* curkey = NULL;
	int i = 0;
	
	/*
	 * According to the HFS+ spec to date, all special objects are located in
	 * the root directory of the volume, so don't bother going further if the
	 * requested object is not.
	 */
	if(filekey->parent_cnid != HFSP_CNID_ROOT_FOLDER)
		return 0;
		
	while((curkey = hfsp_gPrivateObjectKeys[i]) != NULL)
	{
		/* XXX Always use binary compare here, or use volume's specific key 
		 * XXX comparison routine? */
		if(filekey->name.length == curkey->name.length
			&& memcmp(filekey->name.unicode, curkey->name.unicode,
				2 * curkey->name.length)==0)
			return 1;
			
		i++;
	}
	
	return 0;
}


/* bool
hfsplib_is_journal_valid(hfsp_volume* in_vol)
{
	- check magic numbers
	- check Other Things
}*/

#if 0
#pragma mark -
#pragma mark Major Structures
#endif

/*
 *	hfsplib_read_volume_header()
 *	
 *	Reads in_bytes, formats the data appropriately, and places the result
 *	in out_header, which is assumed to be previously allocated. Returns number
 *	of bytes read, 0 if failed.
 */

size_t
hfsplib_read_volume_header(void* in_bytes, hfsp_volume_header_t* out_header)
{
	void*	ptr;
	size_t	last_bytes_read;
	int		i;
	
	if(in_bytes==NULL || out_header==NULL)
		return 0;
	
	ptr = in_bytes;
	
	out_header->signature = be16tohp(&ptr);
	out_header->version = be16tohp(&ptr);
	out_header->attributes = be32tohp(&ptr);
	out_header->last_mounting_version = be32tohp(&ptr);
	out_header->journal_info_block = be32tohp(&ptr);

	out_header->date_created = be32tohp(&ptr);
	out_header->date_modified = be32tohp(&ptr);
	out_header->date_backedup = be32tohp(&ptr);
	out_header->date_checked = be32tohp(&ptr);

	out_header->file_count = be32tohp(&ptr);
	out_header->folder_count = be32tohp(&ptr);

	out_header->block_size = be32tohp(&ptr);
	out_header->total_blocks = be32tohp(&ptr);
	out_header->free_blocks = be32tohp(&ptr);
	out_header->next_alloc_block = be32tohp(&ptr);
	out_header->rsrc_clump_size = be32tohp(&ptr);
	out_header->data_clump_size = be32tohp(&ptr);
	out_header->next_cnid = be32tohp(&ptr);

	out_header->write_count = be32tohp(&ptr);
	out_header->encodings = be64tohp(&ptr);

	for(i=0;i<8;i++)
		out_header->finder_info[i] = be32tohp(&ptr);

	if((last_bytes_read = hfsplib_read_fork_descriptor(ptr,
		&out_header->allocation_file))==0)
		return 0;
	ptr = (uint8_t*)ptr + last_bytes_read;
	
	if((last_bytes_read = hfsplib_read_fork_descriptor(ptr,
		&out_header->extents_file))==0)
		return 0;
	ptr = (uint8_t*)ptr + last_bytes_read;
	
	if((last_bytes_read = hfsplib_read_fork_descriptor(ptr,
		&out_header->catalog_file))==0)
		return 0;
	ptr = (uint8_t*)ptr + last_bytes_read;
	
	if((last_bytes_read = hfsplib_read_fork_descriptor(ptr,
		&out_header->attributes_file))==0)
		return 0;
	ptr = (uint8_t*)ptr + last_bytes_read;
	
	if((last_bytes_read = hfsplib_read_fork_descriptor(ptr,
		&out_header->startup_file))==0)
		return 0;
	ptr = (uint8_t*)ptr + last_bytes_read;
	
	return ((uint8_t*)ptr - (uint8_t*)in_bytes);
}

/*
 *	hfsplib_reada_node()
 *	
 *	Given the pointer to and size of a buffer containing the entire, raw
 *	contents of any b-tree node from the disk, this function will:
 *
 *		1.	determine the type of node and read its contents
 *		2.	allocate memory for each record and fill it appropriately
 *		3.	set out_record_ptrs_array to point to an array (which it allocates)
 *			which has out_node_descriptor->num_recs many pointers to the
 *			records themselves
 *		4.	allocate out_record_ptr_sizes_array and fill it with the sizes of
 *			each record
 *		5.	return the number of bytes read (i.e., the size of the node)
 *			or 0 on failure
 *	
 *	out_node_descriptor must be allocated by the caller and may not be NULL.
 *
 *	out_record_ptrs_array and out_record_ptr_sizes_array must both be specified,
 *	or both be NULL if the caller is not interested in reading the records.
 *
 *	out_record_ptr_sizes_array may be NULL if the caller is not interested in
 *	reading the records, but must not be NULL if out_record_ptrs_array is not.
 *
 *	in_parent_file is HFSP_CATALOG_FILE, HFSP_EXTENTS_FILE, or
 *	HFSP_ATTRIBUTES_FILE, depending on the special file in which this node
 *	resides.
 *
 *	inout_volume must have its catnodesize or extnodesize field (depending on
 *	the parent file) set to the correct value if this is an index, leaf, or map
 *	node. If this is a header node, the field will be set to its correct value.
 */
size_t
hfsplib_reada_node(void* in_bytes,
	hfsp_node_descriptor_t* out_node_descriptor,
	void** out_record_ptrs_array[],
	uint16_t* out_record_ptr_sizes_array[],
	hfsp_btree_file_type in_parent_file,
	hfsp_volume* inout_volume,
	hfsp_callback_args* cbargs)
{
	void*		ptr;
	uint16_t*	rec_offsets;
	size_t		last_bytes_read;
	uint16_t	nodesize;
	uint16_t	numrecords;
	uint16_t	free_space_offset;	/* offset to free space in node */
	int			keysizefieldsize;
	int			i;
	
	numrecords = 0;
	rec_offsets = NULL;
	if(out_record_ptrs_array!=NULL)
		*out_record_ptrs_array = NULL;
	if(out_record_ptr_sizes_array!=NULL)
		*out_record_ptr_sizes_array = NULL;
	
	if(in_bytes==NULL || inout_volume==NULL || out_node_descriptor==NULL
		|| (out_record_ptrs_array==NULL && out_record_ptr_sizes_array!=NULL)
		|| (out_record_ptrs_array!=NULL && out_record_ptr_sizes_array==NULL) )
		goto error;
		
	ptr = in_bytes;
	
	out_node_descriptor->flink = be32tohp(&ptr);
	out_node_descriptor->blink = be32tohp(&ptr);
	out_node_descriptor->kind = *(((int8_t*)ptr));
	ptr = (uint8_t*)ptr + 1;
	out_node_descriptor->height = *(((uint8_t*)ptr));
	ptr = (uint8_t*)ptr + 1;
	out_node_descriptor->num_recs = be16tohp(&ptr);
	out_node_descriptor->reserved = be16tohp(&ptr);
	
	numrecords = out_node_descriptor->num_recs;
	
	/*
	 *	To go any further, we will need to know the size of this node, as well
	 *	as the width of keyed records' key_len parameters for this btree. If
	 *	this is an index, leaf, or map node, inout_volume already has the node
	 *	size set in its catnodesize or extnodesize field and the key length set
	 *	in the catkeysizefieldsize or extkeysizefieldsize for catalog files and
	 *	extent files, respectively. However, if this is a header node, this
	 *	information has not yet been determined, so this is the place to do it.
	 */
	if(out_node_descriptor->kind == HFSP_HEADERNODE)
	{
		hfsp_header_record_t	hr;
		void*		header_rec_offset[1];
		uint16_t	header_rec_size[1];
		
		/* sanity check to ensure this is a good header node */
		if(numrecords!=3)
			HFSP_LIBERR("header node does not have exactly 3 records");
		
		header_rec_offset[0] = ptr;
		header_rec_size[0] = sizeof(hfsp_header_record_t);
		
		last_bytes_read = hfsplib_read_header_node(header_rec_offset,
			header_rec_size, 1, &hr, NULL, NULL);
		if(last_bytes_read==0)
			HFSP_LIBERR("could not read header node");
		
		switch(in_parent_file)
		{
			case HFSP_CATALOG_FILE:
				inout_volume->chr.node_size = hr.node_size;
				inout_volume->catkeysizefieldsize =
					(hr.attributes & HFSP_BIG_KEYS_MASK) ?
						sizeof(uint16_t):sizeof(uint8_t);
				break;
				
			case HFSP_EXTENTS_FILE:
				inout_volume->ehr.node_size = hr.node_size;
				inout_volume->extkeysizefieldsize =
					(hr.attributes & HFSP_BIG_KEYS_MASK) ?
						sizeof(uint16_t):sizeof(uint8_t);
				break;
				
			case HFSP_ATTRIBUTES_FILE:
			default:
				HFSP_LIBERR("invalid parent file type specified");
				/* NOTREACHED */
		}
	}
	
	switch(in_parent_file)
	{
		case HFSP_CATALOG_FILE:
			nodesize = inout_volume->chr.node_size;
			keysizefieldsize = inout_volume->catkeysizefieldsize;
			break;
			
		case HFSP_EXTENTS_FILE:
			nodesize = inout_volume->ehr.node_size;
			keysizefieldsize = inout_volume->extkeysizefieldsize;
			break;
			
		case HFSP_ATTRIBUTES_FILE:
		default:
			HFSP_LIBERR("invalid parent file type specified");
			/* NOTREACHED */
	}
	
	/*
	 *	Don't care about records so just exit after getting the node descriptor.
	 *	Note: This happens after the header node code, and not before it, in
	 *	case the caller calls this function and ignores the record data just to
	 *	get at the node descriptor, but then tries to call it again on a non-
	 *	header node without first setting inout_volume->cat/extnodesize.
	 */
	if(out_record_ptrs_array==NULL)
		return ((uint8_t*)ptr - (uint8_t*)in_bytes);
	
	rec_offsets = hfsplib_malloc(numrecords * sizeof(uint16_t), cbargs);
	*out_record_ptr_sizes_array =
		hfsplib_malloc(numrecords * sizeof(uint16_t), cbargs);
	if(rec_offsets==NULL || *out_record_ptr_sizes_array==NULL)
		HFSP_LIBERR("could not allocate node record offsets");

	*out_record_ptrs_array = hfsplib_malloc(numrecords * sizeof(void*), cbargs);
	if(*out_record_ptrs_array==NULL)
		HFSP_LIBERR("could not allocate node records");
	
	last_bytes_read = hfsplib_reada_node_offsets((uint8_t*)in_bytes + nodesize -
			numrecords * sizeof(uint16_t), rec_offsets);
	if(last_bytes_read==0)
		HFSP_LIBERR("could not read node record offsets");
	
	/*	The size of the last record (i.e. the first one listed in the offsets)
	 *	must be determined using the offset to the node's free space. */
	free_space_offset = be16toh(*(uint16_t*)((uint8_t*)in_bytes + nodesize -
			(numrecords+1) * sizeof(uint16_t)));

	(*out_record_ptr_sizes_array)[numrecords-1] =
		free_space_offset - rec_offsets[0];
	for(i=1;i<numrecords;i++)
	{
		(*out_record_ptr_sizes_array)[numrecords-i-1] = 
			rec_offsets[i-1] - rec_offsets[i];
	}

	for(i=0;i<numrecords;i++)
	{
		(*out_record_ptrs_array)[i] =
			hfsplib_malloc((*out_record_ptr_sizes_array)[i], cbargs);

		if((*out_record_ptrs_array)[i]==NULL)
			HFSP_LIBERR("could not allocate node record #%i",i);
		
		/*	
		 *	If this is a keyed node (i.e., a leaf or index node), there are two
		 *	boundary rules that each record must obey:
		 *
		 *		1.	A pad byte must be placed between the key and data if the
		 *			size of the key plus the size of the key_len field is odd.
		 *
		 *		2.	A pad byte must be placed after the data if the data size
		 *			is odd.
		 *	
		 *	So in the first case we increment the starting point of the data
		 *	and correspondingly decrement the record size. In the second case
		 *	we decrement the record size.
		 */			
		if(out_node_descriptor->kind == HFSP_LEAFNODE ||
		   out_node_descriptor->kind == HFSP_INDEXNODE)
		{
			hfsp_catalog_key_t	reckey;
			uint16_t			rectype;
			
			rectype = out_node_descriptor->kind;
			last_bytes_read = hfsplib_read_catalog_keyed_record(ptr, NULL,
				&rectype, &reckey, inout_volume);
			if(last_bytes_read==0)
				HFSP_LIBERR("could not read node record");
			
			if((reckey.key_len + keysizefieldsize) % 2 == 1)
			{
				ptr = (uint8_t*)ptr + 1;
				(*out_record_ptr_sizes_array)[i]--;
			}
				
			if((*out_record_ptr_sizes_array)[i] % 2 == 1)
				(*out_record_ptr_sizes_array)[i]--;
		}

		memcpy((*out_record_ptrs_array)[i], ptr,
				(*out_record_ptr_sizes_array)[i]);
		ptr = (uint8_t*)ptr + (*out_record_ptr_sizes_array)[i];
	}

	goto exit;
	
error:
	hfsplib_free_recs(out_record_ptrs_array, out_record_ptr_sizes_array,
		&numrecords, cbargs);

	ptr = in_bytes;
	
	/* warn("error occurred in hfsplib_reada_node()"); */
	
	/* FALLTHROUGH */
	
exit:	
	if(rec_offsets!=NULL)
		hfsplib_free(rec_offsets, cbargs);
		
	return ((uint8_t*)ptr - (uint8_t*)in_bytes);
}

/*
 *	hfsplib_reada_node_offsets()
 *	
 *	Sets out_offset_array to contain the offsets to each record in the node,
 *	in reverse order. Does not read the free space offset.
 */
size_t
hfsplib_reada_node_offsets(void* in_bytes, uint16_t* out_offset_array)
{
	void*		ptr;
	
	if(in_bytes==NULL || out_offset_array==NULL)
		return 0;
	
	ptr = in_bytes;

	/*
	 *	The offset for record 0 (which is the very last offset in the node) is
	 *	always equal to 14, the size of the node descriptor. So, once we hit
	 *	offset=14, we know this is the last offset. In this way, we don't need
	 *	to know the number of records beforehand.
	*/
	out_offset_array--;
	do
	{
		out_offset_array++;
		*out_offset_array = be16tohp(&ptr);
	}
	while(*out_offset_array != (uint16_t)14);
	
	return ((uint8_t*)ptr - (uint8_t*)in_bytes);
}

/*	hfsplib_read_header_node()
 *	
 *	out_header_record and/or out_map_record may be NULL if the caller doesn't
 *	care about their contents.
 */
size_t
hfsplib_read_header_node(void** in_recs,
	uint16_t* in_rec_sizes,
	uint16_t in_num_recs,
	hfsp_header_record_t* out_hr,
	void* out_userdata,
	void* out_map)
{
	void*	ptr;
	int		i;
	
	if(in_recs==NULL || in_rec_sizes==NULL)
		return 0;
	
	if(out_hr!=NULL)
	{
		ptr = in_recs[0];
		
		out_hr->tree_depth = be16tohp(&ptr);
		out_hr->root_node = be32tohp(&ptr);
		out_hr->leaf_recs = be32tohp(&ptr);
		out_hr->first_leaf = be32tohp(&ptr);
		out_hr->last_leaf = be32tohp(&ptr);
		out_hr->node_size = be16tohp(&ptr);
		out_hr->max_key_len = be16tohp(&ptr);
		out_hr->total_nodes = be32tohp(&ptr);
		out_hr->free_nodes = be32tohp(&ptr);
		out_hr->reserved = be16tohp(&ptr);
		out_hr->clump_size = be32tohp(&ptr);
		out_hr->btree_type = *(((uint8_t*)ptr));
		ptr = (uint8_t*)ptr + 1;
		out_hr->keycomp_type = *(((uint8_t*)ptr));
		ptr = (uint8_t*)ptr + 1;
		out_hr->attributes = be32tohp(&ptr);
		for(i=0;i<16;i++)
			out_hr->reserved2[i] = be32tohp(&ptr);
	}
	
	if(out_userdata!=NULL)
	{
		memcpy(out_userdata, in_recs[1], in_rec_sizes[1]);
	}
	ptr = (uint8_t*)ptr + in_rec_sizes[1];	/* size of user data record */

	if(out_map!=NULL)
	{
		memcpy(out_map, in_recs[2], in_rec_sizes[2]);
	}
	ptr = (uint8_t*)ptr + in_rec_sizes[2];	/* size of map record */

	return ((uint8_t*)ptr - (uint8_t*)in_recs[0]);
}

/*
 *	hfsplib_read_catalog_keyed_record()
 *	
 *	out_recdata can be NULL. inout_rectype must be set to either HFSP_LEAFNODE
 *	or HFSP_INDEXNODE upon calling this function, and will be set by the
 *	function to one of HFSP_REC_FLDR, HFSP_REC_FILE, HFSP_REC_FLDR_THREAD, or
 *	HFSP_REC_FLDR_THREAD upon return if the node is a leaf node. If it is an
 *	index node, inout_rectype will not be changed.
 */
size_t
hfsplib_read_catalog_keyed_record(
	void* in_bytes,
	hfsp_catalog_keyed_record_t* out_recdata,
	int16_t* inout_rectype,
	hfsp_catalog_key_t* out_key,
	hfsp_volume* in_volume)
{
	void*		ptr;
	size_t		last_bytes_read;
	
	if(in_bytes==NULL || out_key==NULL || inout_rectype==NULL)
		return 0;
		
	ptr = in_bytes;

	/*	For HFS+, the key length is always a 2-byte number. This is indicated
	 *	by the HFSP_BIG_KEYS_MASK bit in the attributes field of the catalog
	 *	header record. However, we just assume this bit is set, since all HFS+
	 *	volumes should have it set anyway. */
	if(in_volume->catkeysizefieldsize == sizeof(uint16_t))
		out_key->key_len = be16tohp(&ptr);
	else if (in_volume->catkeysizefieldsize == sizeof(uint8_t)) {
		out_key->key_len = *(((uint8_t*)ptr));
		ptr = (uint8_t*)ptr + 1;
	}
	
	out_key->parent_cnid = be32tohp(&ptr);

	last_bytes_read = hfsplib_read_unistr255(ptr, &out_key->name);
	if(last_bytes_read==0)
		return 0;
	ptr = (uint8_t*)ptr + last_bytes_read;

	/* don't waste time if the user just wanted the key and/or record type */
	if(out_recdata==NULL)
	{
		if(*inout_rectype == HFSP_LEAFNODE)
			*inout_rectype = be16tohp(&ptr);
		else if(*inout_rectype != HFSP_INDEXNODE)
			return 0;	/* should not happen if we were given valid arguments */

		return ((uint8_t*)ptr - (uint8_t*)in_bytes);
	}

	if(*inout_rectype == HFSP_INDEXNODE)
	{
		out_recdata->child = be32tohp(&ptr);
	}
	else
	{
		/* first need to determine what kind of record this is */
		*inout_rectype = be16tohp(&ptr);
		out_recdata->type = *inout_rectype;
		
		switch(out_recdata->type)
		{
			case HFSP_REC_FLDR:
			{
				out_recdata->folder.flags = be16tohp(&ptr);
				out_recdata->folder.valence = be32tohp(&ptr);
				out_recdata->folder.cnid = be32tohp(&ptr);
				out_recdata->folder.date_created = be32tohp(&ptr);
				out_recdata->folder.date_content_mod = be32tohp(&ptr);
				out_recdata->folder.date_attrib_mod = be32tohp(&ptr);
				out_recdata->folder.date_accessed = be32tohp(&ptr);
				out_recdata->folder.date_backedup = be32tohp(&ptr);
	
				last_bytes_read = hfsplib_read_bsd_data(ptr,
					&out_recdata->folder.bsd);
				if(last_bytes_read==0)
					return 0;
				ptr = (uint8_t*)ptr + last_bytes_read;
	
				last_bytes_read = hfsplib_read_folder_userinfo(ptr,
					&out_recdata->folder.user_info);
				if(last_bytes_read==0)
					return 0;
				ptr = (uint8_t*)ptr + last_bytes_read;
	
				last_bytes_read = hfsplib_read_folder_finderinfo(ptr,
					&out_recdata->folder.finder_info);
				if(last_bytes_read==0)
					return 0;
				ptr = (uint8_t*)ptr + last_bytes_read;
	
				out_recdata->folder.text_encoding = be32tohp(&ptr);
				out_recdata->folder.reserved = be32tohp(&ptr);
			}
			break;
			
			case HFSP_REC_FILE:
			{
				out_recdata->file.flags = be16tohp(&ptr);
				out_recdata->file.reserved = be32tohp(&ptr);
				out_recdata->file.cnid = be32tohp(&ptr);
				out_recdata->file.date_created = be32tohp(&ptr);
				out_recdata->file.date_content_mod = be32tohp(&ptr);
				out_recdata->file.date_attrib_mod = be32tohp(&ptr);
				out_recdata->file.date_accessed = be32tohp(&ptr);
				out_recdata->file.date_backedup = be32tohp(&ptr);
	
				last_bytes_read = hfsplib_read_bsd_data(ptr,
					&out_recdata->file.bsd);
				if(last_bytes_read==0)
					return 0;
				ptr = (uint8_t*)ptr + last_bytes_read;
	
				last_bytes_read = hfsplib_read_file_userinfo(ptr,
					&out_recdata->file.user_info);
				if(last_bytes_read==0)
					return 0;
				ptr = (uint8_t*)ptr + last_bytes_read;
	
				last_bytes_read = hfsplib_read_file_finderinfo(ptr,
					&out_recdata->file.finder_info);
				if(last_bytes_read==0)
					return 0;
				ptr = (uint8_t*)ptr + last_bytes_read;
	
				out_recdata->file.text_encoding = be32tohp(&ptr);
				out_recdata->file.reserved2 = be32tohp(&ptr);
	
				last_bytes_read = hfsplib_read_fork_descriptor(ptr,
					&out_recdata->file.data_fork);
				if(last_bytes_read==0)
					return 0;
				ptr = (uint8_t*)ptr + last_bytes_read;
	
				last_bytes_read = hfsplib_read_fork_descriptor(ptr,
					&out_recdata->file.rsrc_fork);
				if(last_bytes_read==0)
					return 0;
				ptr = (uint8_t*)ptr + last_bytes_read;
			}
			break;
			
			case HFSP_REC_FLDR_THREAD:
			case HFSP_REC_FILE_THREAD:
			{
				out_recdata->thread.reserved = be16tohp(&ptr);
				out_recdata->thread.parent_cnid = be32tohp(&ptr);
	
				last_bytes_read = hfsplib_read_unistr255(ptr,
					&out_recdata->thread.name);
				if(last_bytes_read==0)
					return 0;
				ptr = (uint8_t*)ptr + last_bytes_read;
			}
			break;
			
			default:
				return 1;
				/* NOTREACHED */
		}
	}
	
	return ((uint8_t*)ptr - (uint8_t*)in_bytes);
}

/* out_rec may be NULL */
size_t
hfsplib_read_extent_record(
	void* in_bytes,
	hfsp_extent_record_t* out_rec,
	hfsp_node_kind in_nodekind,
	hfsp_extent_key_t* out_key,
	hfsp_volume* in_volume)
{
	void*		ptr;
	size_t		last_bytes_read;
	
	if(in_bytes==NULL || out_key==NULL
		|| (in_nodekind!=HFSP_LEAFNODE && in_nodekind!=HFSP_INDEXNODE))
		return 0;
		
	ptr = in_bytes;

	/*	For HFS+, the key length is always a 2-byte number. This is indicated
	 *	by the HFSP_BIG_KEYS_MASK bit in the attributes field of the extent
	 *	overflow header record. However, we just assume this bit is set, since
	 *	all HFS+ volumes should have it set anyway. */
	if(in_volume->extkeysizefieldsize == sizeof(uint16_t))
		out_key->key_length = be16tohp(&ptr);
	else if (in_volume->extkeysizefieldsize == sizeof(uint8_t)) {
		out_key->key_length = *(((uint8_t*)ptr));
		ptr = (uint8_t*)ptr + 1;
	}
	
	out_key->fork_type = *(((uint8_t*)ptr));
	ptr = (uint8_t*)ptr + 1;
	out_key->padding = *(((uint8_t*)ptr));
	ptr = (uint8_t*)ptr + 1;
	out_key->file_cnid = be32tohp(&ptr);
	out_key->start_block = be32tohp(&ptr);

	/* don't waste time if the user just wanted the key */
	if(out_rec==NULL)
		return ((uint8_t*)ptr - (uint8_t*)in_bytes);

	if(in_nodekind==HFSP_LEAFNODE)
	{
		last_bytes_read = hfsplib_read_extent_descriptors(ptr, out_rec);
		if(last_bytes_read==0)
			return 0;
		ptr = (uint8_t*)ptr + last_bytes_read;
	}
	else
	{
		/* XXX: this is completely bogus */
                /*      (uint32_t*)*out_rec = be32tohp(&ptr); */
	    uint32_t *ptr_32 = (uint32_t *)out_rec;
		*ptr_32 = be32tohp(&ptr);
	        /* (*out_rec)[0].start_block = be32tohp(&ptr); */
	}
	
	return ((uint8_t*)ptr - (uint8_t*)in_bytes);
}

void
hfsplib_free_recs(
	void*** inout_node_recs,
	uint16_t** inout_rec_sizes,
	uint16_t* inout_num_recs,
	hfsp_callback_args* cbargs)
{
	uint16_t	i;
	
	if(inout_num_recs==NULL || *inout_num_recs==0)
		return;
	
	if(inout_node_recs!=NULL && *inout_node_recs!=NULL)
	{
		for(i=0;i<*inout_num_recs;i++)
		{
			if((*inout_node_recs)[i]!=NULL)
			{
				hfsplib_free((*inout_node_recs)[i], cbargs);
				(*inout_node_recs)[i] = NULL;
			}		
		}

		hfsplib_free(*inout_node_recs, cbargs);
		*inout_node_recs = NULL;
	}
	
	if(inout_rec_sizes!=NULL && *inout_rec_sizes!=NULL)
	{
		hfsplib_free(*inout_rec_sizes, cbargs);
		*inout_rec_sizes = NULL;
	}
	
	*inout_num_recs = 0;
}

#if 0
#pragma mark -
#pragma mark Individual Fields
#endif

size_t
hfsplib_read_fork_descriptor(void* in_bytes, hfsp_fork_t* out_forkdata)
{
	void*	ptr;
	size_t	last_bytes_read;
	
	if(in_bytes==NULL || out_forkdata==NULL)
		return 0;
	
	ptr = in_bytes;
	
	out_forkdata->logical_size = be64tohp(&ptr);
	out_forkdata->clump_size = be32tohp(&ptr);
	out_forkdata->total_blocks = be32tohp(&ptr);
	
	if((last_bytes_read = hfsplib_read_extent_descriptors(ptr,
		&out_forkdata->extents))==0)
		return 0;
	ptr = (uint8_t*)ptr + last_bytes_read;

	return ((uint8_t*)ptr - (uint8_t*)in_bytes);
}

size_t
hfsplib_read_extent_descriptors(
	void* in_bytes,
	hfsp_extent_record_t* out_extentrecord)
{
	void*	ptr;
	int		i;
	
	if(in_bytes==NULL || out_extentrecord==NULL)
		return 0;
	
	ptr = in_bytes;
		
	for(i=0;i<8;i++)
	{
		(((hfsp_extent_descriptor_t*)*out_extentrecord)[i]).start_block =
			be32tohp(&ptr);
		(((hfsp_extent_descriptor_t*)*out_extentrecord)[i]).block_count =
			be32tohp(&ptr);
	}
	
	return ((uint8_t*)ptr - (uint8_t*)in_bytes);
}

size_t
hfsplib_read_unistr255(void* in_bytes, hfsp_unistr255_t* out_string)
{
	void*		ptr;
	uint16_t	i, length;
	
	if(in_bytes==NULL || out_string==NULL)
		return 0;
		
	ptr = in_bytes;
	
	length = be16tohp(&ptr);
	if(length>255)
		length = 255; /* hfs+ folder/file names have a limit of 255 chars */
	out_string->length = length;
		
	for(i=0; i<length; i++)
	{
		out_string->unicode[i] = be16tohp(&ptr);
	}
	
	return ((uint8_t*)ptr - (uint8_t*)in_bytes);
}

size_t
hfsplib_read_bsd_data(void* in_bytes, hfsp_bsd_data_t* out_perms)
{
	void*	ptr;
	
	if(in_bytes==NULL || out_perms==NULL)
		return 0;
		
	ptr = in_bytes;

	out_perms->owner_id = be32tohp(&ptr);
	out_perms->group_id = be32tohp(&ptr);
	out_perms->admin_flags = *(((uint8_t*)ptr));
	ptr = (uint8_t*)ptr + 1;
	out_perms->owner_flags = *(((uint8_t*)ptr));
	ptr = (uint8_t*)ptr + 1;
	out_perms->file_mode = be16tohp(&ptr);
	out_perms->special.inode_num = be32tohp(&ptr); /* this field is a union */

	return ((uint8_t*)ptr - (uint8_t*)in_bytes);
}

size_t
hfsplib_read_file_userinfo(void* in_bytes, hfsp_macos_file_info_t* out_info)
{
	void*	ptr;
	
	if(in_bytes==NULL || out_info==NULL)
		return 0;
		
	ptr = in_bytes;
	
	out_info->file_type = be32tohp(&ptr);
	out_info->file_creator = be32tohp(&ptr);
	out_info->finder_flags = be16tohp(&ptr);
	out_info->location.v = be16tohp(&ptr);
	out_info->location.h = be16tohp(&ptr);
	out_info->reserved = be16tohp(&ptr);
	
	return ((uint8_t*)ptr - (uint8_t*)in_bytes);
}

size_t
hfsplib_read_file_finderinfo(
	void* in_bytes,
	hfsp_macos_extended_file_info_t* out_info)
{
	void*	ptr;
	
	if(in_bytes==NULL || out_info==NULL)
		return 0;
		
	ptr = in_bytes;
	
#if 0
	#pragma warn Fill in with real code!
#endif
	/* FIXME: Fill in with real code! */
	memset(out_info, 0, sizeof(*out_info));
	ptr = (uint8_t*)ptr + sizeof(hfsp_macos_extended_file_info_t);
	
	return ((uint8_t*)ptr - (uint8_t*)in_bytes);
}

size_t
hfsplib_read_folder_userinfo(void* in_bytes, hfsp_macos_folder_info_t* out_info)
{
	void*	ptr;
	
	if(in_bytes==NULL || out_info==NULL)
		return 0;
		
	ptr = in_bytes;
	
#if 0
	#pragma warn Fill in with real code!
#endif
	/* FIXME: Fill in with real code! */
	memset(out_info, 0, sizeof(*out_info));
	ptr = (uint8_t*)ptr + sizeof(hfsp_macos_folder_info_t);
	
	return ((uint8_t*)ptr - (uint8_t*)in_bytes);
}

size_t
hfsplib_read_folder_finderinfo(
	void* in_bytes,
	hfsp_macos_extended_folder_info_t* out_info)
{
	void*	ptr;
	
	if(in_bytes==NULL || out_info==NULL)
		return 0;
		
	ptr = in_bytes;
	
#if 0
	#pragma warn Fill in with real code!
#endif
	/* FIXME: Fill in with real code! */
	memset(out_info, 0, sizeof(*out_info));
	ptr = (uint8_t*)ptr + sizeof(hfsp_macos_extended_folder_info_t);
	
	return ((uint8_t*)ptr - (uint8_t*)in_bytes);
}

size_t
hfsplib_read_journal_info(void* in_bytes, hfsp_journal_info_t* out_info)
{
	void*	ptr;
	int		i;
	
	if(in_bytes==NULL || out_info==NULL)
		return 0;
		
	ptr = in_bytes;
	
	out_info->flags = be32tohp(&ptr);
	for(i=0; i<8; i++)
	{
		out_info->device_signature[i] = be32tohp(&ptr);
	}
	out_info->offset = be64tohp(&ptr);
	out_info->size = be64tohp(&ptr);
	for(i=0; i<32; i++)
	{
		out_info->reserved[i] = be64tohp(&ptr);
	}
	
	return ((uint8_t*)ptr - (uint8_t*)in_bytes);
}

size_t
hfsplib_read_journal_header(void* in_bytes, hfsp_journal_header_t* out_header)
{
	void*	ptr;
	
	if(in_bytes==NULL || out_header==NULL)
		return 0;
		
	ptr = in_bytes;

	out_header->magic = be32tohp(&ptr);
	out_header->endian = be32tohp(&ptr);
	out_header->start = be64tohp(&ptr);
	out_header->end = be64tohp(&ptr);
	out_header->size = be64tohp(&ptr);
	out_header->blocklist_header_size = be32tohp(&ptr);
	out_header->checksum = be32tohp(&ptr);
	out_header->journal_header_size = be32tohp(&ptr);

	return ((uint8_t*)ptr - (uint8_t*)in_bytes);
}

#if 0
#pragma mark -
#pragma mark Disk Access
#endif

/*
 *	hfsplib_readd_with_extents()
 *	
 *	This function reads the contents of a file from the volume, given an array
 *	of extent descriptors which specify where every extent of the file is
 *	located (in addition to the usual pread() arguments). out_bytes is presumed
 *  to exist and be large enough to hold in_length number of bytes. Returns 0
 *	on success.
 */
int
hfsplib_readd_with_extents(
	hfsp_volume*	in_vol,
	void*		out_bytes,
	uint64_t*	out_bytesread,
	uint64_t	in_length,
	uint64_t	in_offset,
	hfsp_extent_descriptor_t in_extents[],
	uint16_t	in_numextents,
	hfsp_callback_args*	cbargs)
{
	uint64_t	ext_length, last_offset;
	uint16_t	i;
	int			error;
	
	if(in_vol==NULL || out_bytes==NULL || in_extents==NULL || in_numextents==0
		|| out_bytesread==NULL)
		return -1;
	
	*out_bytesread = 0;
	last_offset = 0;

	for(i=0; i<in_numextents; i++)
	{
		if(in_extents[i].block_count==0)
			continue;

		ext_length = in_extents[i].block_count * in_vol->vh.block_size;

		if(in_offset < last_offset+ext_length
			&& in_offset+in_length >= last_offset)
		{
			uint64_t	isect_start, isect_end;
			
			isect_start = max(in_offset, last_offset);
			isect_end = min(in_offset+in_length, last_offset+ext_length);
			error = hfsplib_readd(in_vol, out_bytes, isect_end-isect_start,
				isect_start - last_offset + (uint64_t)in_extents[i].start_block
					* in_vol->vh.block_size, cbargs);
					
			if(error!=0)
				return error;
				
			*out_bytesread += isect_end-isect_start;
			out_bytes = (uint8_t*)out_bytes + isect_end-isect_start;
		}

		last_offset += ext_length;
	}
	
	
	return 0;
}

#if 0
#pragma mark -
#pragma mark Callback Wrappers
#endif

void
hfsplib_error(const char* in_format, const char* in_file, int in_line, ...)
{
	va_list		ap;
	
	if(in_format==NULL)
		return;
		
	if(hfsp_gcb.error!=NULL)
	{
		va_start(ap, in_line);
	
		hfsp_gcb.error(in_format, in_file, in_line, ap);
	
		va_end(ap);
	}
}

void*
hfsplib_malloc(size_t size, hfsp_callback_args* cbargs)
{
	if(hfsp_gcb.allocmem!=NULL)
		return hfsp_gcb.allocmem(size, cbargs);
		
	return NULL;
}

void*
hfsplib_realloc(void* ptr, size_t size, hfsp_callback_args* cbargs)
{
	if(hfsp_gcb.reallocmem!=NULL)
		return hfsp_gcb.reallocmem(ptr, size, cbargs);

	return NULL;
}

void
hfsplib_free(void* ptr, hfsp_callback_args* cbargs)
{
	if(hfsp_gcb.freemem!=NULL && ptr!=NULL)
		hfsp_gcb.freemem(ptr, cbargs);
}

int
hfsplib_openvoldevice(
	hfsp_volume* in_vol,
	const char* in_device,
	uint64_t in_offset,
	hfsp_callback_args* cbargs)
{
	if(hfsp_gcb.openvol!=NULL && in_device!=NULL)
		return hfsp_gcb.openvol(in_vol, in_device, in_offset, cbargs);

	return 1;
}

void
hfsplib_closevoldevice(hfsp_volume* in_vol, hfsp_callback_args* cbargs)
{
	if(hfsp_gcb.closevol!=NULL)
		hfsp_gcb.closevol(in_vol, cbargs);
}

int
hfsplib_readd(
	hfsp_volume* in_vol,
	void* out_bytes,
	uint64_t in_length,
	uint64_t in_offset,
	hfsp_callback_args* cbargs)
{
	if(in_vol==NULL || out_bytes==NULL)
		return -1;
	
	if(hfsp_gcb.read!=NULL)
		return hfsp_gcb.read(in_vol, out_bytes, in_length, in_offset, cbargs);
		
	return -1;
}

#if 0
#pragma mark -
#pragma mark Other
#endif

/* returns key length */
uint16_t
hfsplib_make_catalog_key(
	hfsp_cnid_t in_parent_cnid,
	uint16_t in_name_len,
	unichar_t* in_unicode,
	hfsp_catalog_key_t* out_key)
{
	if(in_parent_cnid==0 || (in_name_len>0 && in_unicode==NULL) || out_key==0)
		return 0;
	
	if(in_name_len>255)
		in_name_len = 255;
		
	out_key->key_len = 6 + 2 * in_name_len;
	out_key->parent_cnid = in_parent_cnid;
	out_key->name.length = in_name_len;
	if(in_name_len>0)
		memcpy(&out_key->name.unicode, in_unicode, in_name_len*2);
		
	return out_key->key_len;
}

/* returns key length */
uint16_t
hfsplib_make_extent_key(
	hfsp_cnid_t in_cnid,
	uint8_t in_forktype,
	uint32_t in_startblock,
	hfsp_extent_key_t* out_key)
{
	if(in_cnid==0 || out_key==0)
		return 0;
	
	out_key->key_length = HFSP_MAX_EXT_KEY_LEN;
	out_key->fork_type = in_forktype;
	out_key->padding = 0;
	out_key->file_cnid = in_cnid;
	out_key->start_block = in_startblock;
		
	return out_key->key_length;
}

/* case-folding */
int
hfsplib_compare_catalog_keys_cf (
	const void *ap,
	const void *bp)
{
	const hfsp_catalog_key_t	*a, *b;
	unichar_t	ac, bc; /* current character from a, b */
	unichar_t	lc; /* lowercase version of current character */
	uint8_t		apos, bpos; /* current character indices */

	a = (const hfsp_catalog_key_t*)ap;
	b = (const hfsp_catalog_key_t*)bp;
	
	if(a->parent_cnid != b->parent_cnid)
	{
		return (a->parent_cnid - b->parent_cnid);
	}
	else
	{
		/*
		 * The following code implements the pseudocode suggested by
		 * the HFS+ technote.
		 */

/*
 * XXX These need to be revised to be endian-independent!
 */
#define hbyte(x) ((x) >> 8)
#define lbyte(x) ((x) & 0x00FF)

		apos = bpos = 0;
		while(1)
		{
			/* get next valid character from a */
			for (lc=0; lc == 0 && apos < a->name.length; apos++) {
				ac = a->name.unicode[apos];
				lc = hfsp_gcft[hbyte(ac)];
				if(lc==0)
					lc = ac;
				else
					lc = hfsp_gcft[lc + lbyte(ac)];
			};
			ac=lc;

			/* get next valid character from b */
			for (lc=0; lc == 0 && bpos < b->name.length; bpos++) {
				bc = b->name.unicode[bpos];
				lc = hfsp_gcft[hbyte(bc)];
				if(lc==0)
					lc = bc;
				else
					lc = hfsp_gcft[lc + lbyte(bc)];
			};
			bc=lc;

			/* on end of string ac/bc are 0, otherwise > 0 */
			if (ac != bc || (ac == 0  && bc == 0))
				return ac - bc;
		}
#undef hbyte
#undef lbyte
	}
}

/* binary compare (i.e., not case folding) */
int
hfsplib_compare_catalog_keys_bc (
	const void *a,
	const void *b)
{
	if(((const hfsp_catalog_key_t*)a)->parent_cnid
		== ((const hfsp_catalog_key_t*)b)->parent_cnid)
	{
		if(((const hfsp_catalog_key_t*)a)->name.length == 0 &&
			((const hfsp_catalog_key_t*)b)->name.length == 0)
			return 0;

		if(((const hfsp_catalog_key_t*)a)->name.length == 0)
			return -1;
		if(((const hfsp_catalog_key_t*)b)->name.length == 0)
			return 1;

		/* FIXME: This does a byte-per-byte comparison, whereas the HFS spec
		 * mandates a uint16_t chunk comparison. */
		return memcmp(((const hfsp_catalog_key_t*)a)->name.unicode,
			((const hfsp_catalog_key_t*)b)->name.unicode,
			min(((const hfsp_catalog_key_t*)a)->name.length,
				((const hfsp_catalog_key_t*)b)->name.length));
	}
	else
	{
		return (((const hfsp_catalog_key_t*)a)->parent_cnid - 
				((const hfsp_catalog_key_t*)b)->parent_cnid);
	}
}

int
hfsplib_compare_extent_keys (
	const void *a,
	const void *b)
{
	/*
	 *	Comparison order, in descending importance:
	 *
	 *		CNID -> fork type -> start block
	 */

	if(((const hfsp_extent_key_t*)a)->file_cnid
		== ((const hfsp_extent_key_t*)b)->file_cnid)
	{
		if(((const hfsp_extent_key_t*)a)->fork_type
			== ((const hfsp_extent_key_t*)b)->fork_type)
		{
			if(((const hfsp_extent_key_t*)a)->start_block
				== ((const hfsp_extent_key_t*)b)->start_block)
			{
				return 0;
			}
			else
			{
				return (((const hfsp_extent_key_t*)a)->start_block - 
						((const hfsp_extent_key_t*)b)->start_block);
			}
		}
		else
		{
			return (((const hfsp_extent_key_t*)a)->fork_type - 
					((const hfsp_extent_key_t*)b)->fork_type);
		}
	}
	else
	{
		return (((const hfsp_extent_key_t*)a)->file_cnid - 
				((const hfsp_extent_key_t*)b)->file_cnid);
	}
}

/* 1+10 tables of 16 rows and 16 columns, each 2 bytes wide = 5632 bytes */
int
hfsplib_create_casefolding_table(void)
{
	hfsp_callback_args	cbargs;
	unichar_t*	t; /* convenience */
	uint16_t	s; /* current subtable * 256 */
	uint16_t	i; /* current subtable index (0 to 255) */
	
	if(hfsp_gcft!=NULL)
		return 0; /* no sweat, table already exists */
	
	hfsplib_init_cbargs(&cbargs);
	hfsp_gcft = hfsplib_malloc(5632, &cbargs);
	if(hfsp_gcft==NULL)
		HFSP_LIBERR("could not allocate case folding table");
	
	t = hfsp_gcft;	 /* easier to type :) */
	
	/*
	 * high byte indices
	 */
	s = 0 * 256;
	memset(t, 0x00, 512);
	t[s+  0] = 0x0100;
	t[s+  1] = 0x0200;
	t[s+  3] = 0x0300;
	t[s+  4] = 0x0400;
	t[s+  5] = 0x0500;
	t[s+ 16] = 0x0600;
	t[s+ 32] = 0x0700;
	t[s+ 33] = 0x0800;
	t[s+254] = 0x0900;
	t[s+255] = 0x0a00;
	
	/*
	 * table 1 (high byte 0x00)
	 */
	s = 1 * 256;
	for(i=0; i<65; i++)
		t[s+i] = i;
	t[s+  0] = 0xffff;
	for(i=65; i<91; i++)
		t[s+i] = i + 0x20;
	for(i=91; i<256; i++)
		t[s+i] = i;
	t[s+198] = 0x00e6;
	t[s+208] = 0x00f0;
	t[s+216] = 0x00f8;
	t[s+222] = 0x00fe;

	/*
	 * table 2 (high byte 0x01)
	 */
	s = 2 * 256;
	for(i=0; i<256; i++)
		t[s+i] = i + 0x0100;
	t[s+ 16] = 0x0111;
	t[s+ 38] = 0x0127;
	t[s+ 50] = 0x0133;
	t[s+ 63] = 0x0140;
	t[s+ 65] = 0x0142;
	t[s+ 74] = 0x014b;
	t[s+ 82] = 0x0153;
	t[s+102] = 0x0167;
	t[s+129] = 0x0253;
	t[s+130] = 0x0183;
	t[s+132] = 0x0185;
	t[s+134] = 0x0254;
	t[s+135] = 0x0188;
	t[s+137] = 0x0256;
	t[s+138] = 0x0257;
	t[s+139] = 0x018c;
	t[s+142] = 0x01dd;
	t[s+143] = 0x0259;
	t[s+144] = 0x025b;
	t[s+145] = 0x0192;
	t[s+147] = 0x0260;
	t[s+148] = 0x0263;
	t[s+150] = 0x0269;
	t[s+151] = 0x0268;
	t[s+152] = 0x0199;
	t[s+156] = 0x026f;
	t[s+157] = 0x0272;
	t[s+159] = 0x0275;
	t[s+162] = 0x01a3;
	t[s+164] = 0x01a5;
	t[s+167] = 0x01a8;
	t[s+169] = 0x0283;
	t[s+172] = 0x01ad;
	t[s+174] = 0x0288;
	t[s+177] = 0x028a;
	t[s+178] = 0x028b;
	t[s+179] = 0x01b4;
	t[s+181] = 0x01b6;
	t[s+183] = 0x0292;
	t[s+184] = 0x01b9;
	t[s+188] = 0x01bd;
	t[s+196] = 0x01c6;
	t[s+197] = 0x01c6;
	t[s+199] = 0x01c9;
	t[s+200] = 0x01c9;
	t[s+202] = 0x01cc;
	t[s+203] = 0x01cc;
	t[s+228] = 0x01e5;
	t[s+241] = 0x01f3;
	t[s+242] = 0x01f3;

	/*
	 * table 3 (high byte 0x03)
	 */
	s = 3 * 256;
	for(i=0; i<145; i++)
		t[s+i] = i + 0x0300;
	for(i=145; i<170; i++)
		t[s+i] = i + 0x0320;
	t[s+162] = 0x03a2;
	for(i=170; i<256; i++)
		t[s+i] = i + 0x0300;

	for(i=226; i<239; i+=2)
		t[s+i] = i + 0x0301;

	/*
	 * table 4 (high byte 0x04)
	 */
	s = 4 * 256;
	for(i=0; i<16; i++)
		t[s+i] = i + 0x0400;
	t[s+  2] = 0x0452;
	t[s+  4] = 0x0454;
	t[s+  5] = 0x0455;
	t[s+  6] = 0x0456;
	t[s+  8] = 0x0458;
	t[s+  9] = 0x0459;
	t[s+ 10] = 0x045a;
	t[s+ 11] = 0x045b;
	t[s+ 15] = 0x045f;

	for(i=16; i<48; i++)
		t[s+i] = i + 0x0420;
	t[s+ 25] = 0x0419;
	for(i=48; i<256; i++)
		t[s+i] = i + 0x0400;
	t[s+195] = 0x04c4;
	t[s+199] = 0x04c8;
	t[s+203] = 0x04cc;

	for(i=96; i<129; i+=2)
		t[s+i] = i + 0x0401;
	t[s+118] = 0x0476;
	for(i=144; i<191; i+=2)
		t[s+i] = i + 0x0401;

	/*
	 * table 5 (high byte 0x05)
	 */
	s = 5 * 256;
	for(i=0; i<49; i++)
		t[s+i] = i + 0x0500;
	for(i=49; i<87; i++)
		t[s+i] = i + 0x0530;
	for(i=87; i<256; i++)
		t[s+i] = i + 0x0500;
	
	/*
	 * table 6 (high byte 0x10)
	 */
	s = 6 * 256;
	for(i=0; i<160; i++)
		t[s+i] = i + 0x1000;
	for(i=160; i<198; i++)
		t[s+i] = i + 0x1030;
	for(i=198; i<256; i++)
		t[s+i] = i + 0x1000;
	
	/*
	 * table 7 (high byte 0x20)
	 */
	s = 7 * 256;
	for(i=0; i<256; i++)
		t[s+i] = i + 0x2000;
	{
		uint8_t zi[15] = { 12,  13,  14,  15,
						   42,  43,  44,  45,  46,
						  106, 107, 108, 109, 110, 111};

		for(i=0; i<15; i++)
			t[s+zi[i]] = 0x0000;
	}

	/*
	 * table 8 (high byte 0x21)
	 */
	s = 8 * 256;
	for(i=0; i<96; i++)
		t[s+i] = i + 0x2100;
	for(i=96; i<112; i++)
		t[s+i] = i + 0x2110;
	for(i=112; i<256; i++)
		t[s+i] = i + 0x2100;

	/*
	 * table 9 (high byte 0xFE)
	 */
	s = 9 * 256;
	for(i=0; i<256; i++)
		t[s+i] = i + 0xFE00;
	t[s+255] = 0x0000;

	/*
	 * table 10 (high byte 0xFF)
	 */
	s = 10 * 256;
	for(i=0; i<33; i++)
		t[s+i] = i + 0xFF00;
	for(i=33; i<59; i++)
		t[s+i] = i + 0xFF20;
	for(i=59; i<256; i++)
		t[s+i] = i + 0xFF00;
	
	return 0;
	
error:
	return 1;
}

int
hfsplib_get_hardlink(hfsp_volume *vol, uint32_t inode_num,
		     hfsp_catalog_keyed_record_t *rec,
		     hfsp_callback_args *cbargs)
{
	hfsp_catalog_keyed_record_t metadata;
	hfsp_catalog_key_t key;
	char name[16];
	unichar_t name_uni[16];
	int i, len;

	/* XXX: cache this */
	if (hfsplib_find_catalog_record_with_key(vol,
						 &hfsp_gMetadataDirectoryKey,
						 &metadata, cbargs) != 0
		|| metadata.type != HFSP_REC_FLDR)
		return -1;

	len = snprintf(name, sizeof(name), "iNode%d", inode_num);
	for (i=0; i<len; i++)
		name_uni[i] = name[i];
	
	if (hfsplib_make_catalog_key(metadata.folder.cnid, len, name_uni,
				     &key) == 0)
		return -1;

	return hfsplib_find_catalog_record_with_key(vol, &key, rec, cbargs);
}
