//
// convert.c - Little-endian conversion
//
// Written by Eryk Vershen
//
// See comments in convert.h
//

/*
 * Copyright 1996,1997,1998 by Apple Computer, Inc.
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */

#ifdef __linux__
#include <endian.h>
#elif __NetBSD__
#include <machine/endian.h>
#else
#define LITTLE_ENDIAN 1234
#define BIG_ENDIAN 4321
#define BYTE_ORDER 4321
//#define BYTE_ORDER 1234
#endif

#include "convert.h"


//
// Defines
//


//
// Types
//


//
// Global Constants
//


//
// Global Variables
//


//
// Forward declarations
//
void reverse2(uint8_t *bytes);
void reverse4(uint8_t *bytes);


//
// Routines
//
int
convert_dpme(DPME *data, int to_cpu_form)
{
#if BYTE_ORDER == LITTLE_ENDIAN
    // Since we will toss the block if the signature doesn't match
    // we don't need to check the signature down here.
    reverse2((uint8_t *)&data->dpme_signature);
    reverse2((uint8_t *)&data->dpme_reserved_1);
    reverse4((uint8_t *)&data->dpme_map_entries);
    reverse4((uint8_t *)&data->dpme_pblock_start);
    reverse4((uint8_t *)&data->dpme_pblocks);
    reverse4((uint8_t *)&data->dpme_lblock_start);
    reverse4((uint8_t *)&data->dpme_lblocks);
    reverse4((uint8_t *)&data->dpme_flags);
    reverse4((uint8_t *)&data->dpme_boot_block);
    reverse4((uint8_t *)&data->dpme_boot_bytes);
    reverse4((uint8_t *)&data->dpme_load_addr);
    reverse4((uint8_t *)&data->dpme_load_addr_2);
    reverse4((uint8_t *)&data->dpme_goto_addr);
    reverse4((uint8_t *)&data->dpme_goto_addr_2);
    reverse4((uint8_t *)&data->dpme_checksum);
    convert_bzb((BZB *)data->dpme_bzb, to_cpu_form);
#endif
    return 0;
}


#if BYTE_ORDER == LITTLE_ENDIAN
int
convert_bzb(BZB *data, int to_cpu_form)
{
    // Since the data here varies according to the type of partition we
    // do not want to convert willy-nilly. We use the flag to determine
    // whether to check for the signature before or after we flip the bytes.
    if (to_cpu_form) {
	reverse4((uint8_t *)&data->bzb_magic);
	if (data->bzb_magic != BZBMAGIC) {
	    reverse4((uint8_t *)&data->bzb_magic);
	    if (data->bzb_magic != BZBMAGIC) {
		return 0;
	    }
	}
    } else {
	if (data->bzb_magic != BZBMAGIC) {
	    return 0;
	}
	reverse4((uint8_t *)&data->bzb_magic);
    }
    reverse2((uint8_t *)&data->bzb_inode);
    reverse4((uint8_t *)&data->bzb_flags);
    reverse4((uint8_t *)&data->bzb_tmade);
    reverse4((uint8_t *)&data->bzb_tmount);
    reverse4((uint8_t *)&data->bzb_tumount);
    return 0;
}
#endif


int
convert_block0(Block0 *data, int to_cpu_form)
{
#if BYTE_ORDER == LITTLE_ENDIAN
    DDMap *m;
    uint16_t count;
    int i;

    // Since this data is optional we do not want to convert willy-nilly.
    // We use the flag to determine whether to check for the signature
    // before or after we flip the bytes and to determine which form of
    // the count to use.
    if (to_cpu_form) {
	reverse2((uint8_t *)&data->sbSig);
	if (data->sbSig != BLOCK0_SIGNATURE) {
	    reverse2((uint8_t *)&data->sbSig);
	    if (data->sbSig != BLOCK0_SIGNATURE) {
		return 0;
	    }
	}
    } else {
	if (data->sbSig != BLOCK0_SIGNATURE) {
	    return 0;
	}
	reverse2((uint8_t *)&data->sbSig);
    }
    reverse2((uint8_t *)&data->sbBlkSize);
    reverse4((uint8_t *)&data->sbBlkCount);
    reverse2((uint8_t *)&data->sbDevType);
    reverse2((uint8_t *)&data->sbDevId);
    reverse4((uint8_t *)&data->sbData);
    if (to_cpu_form) {
	reverse2((uint8_t *)&data->sbDrvrCount);
	count = data->sbDrvrCount;
    } else {
	count = data->sbDrvrCount;
	reverse2((uint8_t *)&data->sbDrvrCount);
    }

    if (count > 0) {
	m = (DDMap *) data->sbMap;
	for (i = 0; i < count; i++) {
	    reverse4((uint8_t *)&m[i].ddBlock);
	    reverse2((uint8_t *)&m[i].ddSize);
	    reverse2((uint8_t *)&m[i].ddType);
	}
    }
#endif
    return 0;
}


void
reverse2(uint8_t *bytes)
{
    uint8_t t;

    t = *bytes;
    *bytes = bytes[1];
    bytes[1] = t;
}


void
reverse4(uint8_t *bytes)
{
    uint8_t t;

    t = *bytes;
    *bytes = bytes[3];
    bytes[3] = t;
    t = bytes[1];
    bytes[1] = bytes[2];
    bytes[2] = t;
}
