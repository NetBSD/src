/******************************************************************************
 *
 * Module Name: aslcache -- Local cache support for iASL
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2018, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#include "aslcompiler.h"

/*
 * Local caches. The caches are fully deleted after the compilation/disassembly
 * of each individual input file. Thus, individual allocations from the cache
 * memory do not need to be freed or even released back into the cache.
 *
 * See aslallocate.c for standard heap allocations.
 */


/*******************************************************************************
 *
 * FUNCTION:    UtLocalCacheCalloc
 *
 * PARAMETERS:  Length              - Size of buffer requested
 *
 * RETURN:      Pointer to the buffer. Aborts compiler on allocation failure
 *
 * DESCRIPTION: Allocate a string buffer. Bypass the local
 *              dynamic memory manager for performance reasons (This has a
 *              major impact on the speed of the compiler.)
 *
 ******************************************************************************/

char *
UtLocalCacheCalloc (
    UINT32                  Length)
{
    char                    *Buffer;
    ASL_CACHE_INFO          *Cache;
    UINT32                  CacheSize = ASL_STRING_CACHE_SIZE;


    if (Length > CacheSize)
    {
        CacheSize = Length;

        if (Gbl_StringCacheList)
        {
            Cache = UtLocalCalloc (sizeof (Cache->Next) + CacheSize);

            /* Link new cache buffer just following head of list */

            Cache->Next = Gbl_StringCacheList->Next;
            Gbl_StringCacheList->Next = Cache;

            /* Leave cache management pointers alone as they pertain to head */

            Gbl_StringCount++;
            Gbl_StringSize += Length;

            return (Cache->Buffer);
        }
    }

    if ((Gbl_StringCacheNext + Length) >= Gbl_StringCacheLast)
    {
        /* Allocate a new buffer */

        Cache = UtLocalCalloc (sizeof (Cache->Next) + CacheSize);

        /* Link new cache buffer to head of list */

        Cache->Next = Gbl_StringCacheList;
        Gbl_StringCacheList = Cache;

        /* Setup cache management pointers */

        Gbl_StringCacheNext = Cache->Buffer;
        Gbl_StringCacheLast = Gbl_StringCacheNext + CacheSize;
    }

    Gbl_StringCount++;
    Gbl_StringSize += Length;

    Buffer = Gbl_StringCacheNext;
    Gbl_StringCacheNext += Length;
    return (Buffer);
}


/*******************************************************************************
 *
 * FUNCTION:    UtParseOpCacheCalloc
 *
 * PARAMETERS:  None
 *
 * RETURN:      New parse op. Aborts on allocation failure
 *
 * DESCRIPTION: Allocate a new parse op for the parse tree. Bypass the local
 *              dynamic memory manager for performance reasons (This has a
 *              major impact on the speed of the compiler.)
 *
 ******************************************************************************/

ACPI_PARSE_OBJECT *
UtParseOpCacheCalloc (
    void)
{
    ASL_CACHE_INFO          *Cache;


    if (Gbl_ParseOpCacheNext >= Gbl_ParseOpCacheLast)
    {
        /* Allocate a new buffer */

        Cache = UtLocalCalloc (sizeof (Cache->Next) +
            (sizeof (ACPI_PARSE_OBJECT) * ASL_PARSEOP_CACHE_SIZE));

        /* Link new cache buffer to head of list */

        Cache->Next = Gbl_ParseOpCacheList;
        Gbl_ParseOpCacheList = Cache;

        /* Setup cache management pointers */

        Gbl_ParseOpCacheNext = ACPI_CAST_PTR (ACPI_PARSE_OBJECT, Cache->Buffer);
        Gbl_ParseOpCacheLast = Gbl_ParseOpCacheNext + ASL_PARSEOP_CACHE_SIZE;
    }

    Gbl_ParseOpCount++;
    return (Gbl_ParseOpCacheNext++);
}


/*******************************************************************************
 *
 * FUNCTION:    UtSubtableCacheCalloc - Data Table compiler
 *
 * PARAMETERS:  None
 *
 * RETURN:      Pointer to the buffer. Aborts on allocation failure
 *
 * DESCRIPTION: Allocate a subtable object buffer. Bypass the local
 *              dynamic memory manager for performance reasons (This has a
 *              major impact on the speed of the compiler.)
 *
 ******************************************************************************/

DT_SUBTABLE *
UtSubtableCacheCalloc (
    void)
{
    ASL_CACHE_INFO          *Cache;


    if (Gbl_SubtableCacheNext >= Gbl_SubtableCacheLast)
    {
        /* Allocate a new buffer */

        Cache = UtLocalCalloc (sizeof (Cache->Next) +
            (sizeof (DT_SUBTABLE) * ASL_SUBTABLE_CACHE_SIZE));

        /* Link new cache buffer to head of list */

        Cache->Next = Gbl_SubtableCacheList;
        Gbl_SubtableCacheList = Cache;

        /* Setup cache management pointers */

        Gbl_SubtableCacheNext = ACPI_CAST_PTR (DT_SUBTABLE, Cache->Buffer);
        Gbl_SubtableCacheLast = Gbl_SubtableCacheNext + ASL_SUBTABLE_CACHE_SIZE;
    }

    Gbl_SubtableCount++;
    return (Gbl_SubtableCacheNext++);
}


/*******************************************************************************
 *
 * FUNCTION:    UtFieldCacheCalloc - Data Table compiler
 *
 * PARAMETERS:  None
 *
 * RETURN:      Pointer to the buffer. Aborts on allocation failure
 *
 * DESCRIPTION: Allocate a field object buffer. Bypass the local
 *              dynamic memory manager for performance reasons (This has a
 *              major impact on the speed of the compiler.)
 *
 ******************************************************************************/

DT_FIELD *
UtFieldCacheCalloc (
    void)
{
    ASL_CACHE_INFO          *Cache;


    if (Gbl_FieldCacheNext >= Gbl_FieldCacheLast)
    {
        /* Allocate a new buffer */

        Cache = UtLocalCalloc (sizeof (Cache->Next) +
            (sizeof (DT_FIELD) * ASL_FIELD_CACHE_SIZE));

        /* Link new cache buffer to head of list */

        Cache->Next = Gbl_FieldCacheList;
        Gbl_FieldCacheList = Cache;

        /* Setup cache management pointers */

        Gbl_FieldCacheNext = ACPI_CAST_PTR (DT_FIELD, Cache->Buffer);
        Gbl_FieldCacheLast = Gbl_FieldCacheNext + ASL_FIELD_CACHE_SIZE;
    }

    Gbl_FieldCount++;
    return (Gbl_FieldCacheNext++);
}


/*******************************************************************************
 *
 * FUNCTION:    UtDeleteLocalCaches
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Delete all local cache buffer blocks
 *
 ******************************************************************************/

void
UtDeleteLocalCaches (
    void)
{
    UINT32                  BufferCount;
    ASL_CACHE_INFO          *Next;


    /*
     * Generic cache, arbitrary size allocations
     */
    BufferCount = 0;
    while (Gbl_StringCacheList)
    {
        Next = Gbl_StringCacheList->Next;
        ACPI_FREE (Gbl_StringCacheList);
        Gbl_StringCacheList = Next;
        BufferCount++;
    }

    DbgPrint (ASL_DEBUG_OUTPUT,
        "%u Strings (%u bytes), Buffer size: %u bytes, %u Buffers\n",
        Gbl_StringCount, Gbl_StringSize, ASL_STRING_CACHE_SIZE, BufferCount);

    /* Reset cache globals */

    Gbl_StringSize = 0;
    Gbl_StringCount = 0;
    Gbl_StringCacheNext = NULL;
    Gbl_StringCacheLast = NULL;


    /*
     * Parse Op cache
     */
    BufferCount = 0;
    while (Gbl_ParseOpCacheList)
    {
        Next = Gbl_ParseOpCacheList->Next;
        ACPI_FREE (Gbl_ParseOpCacheList);
        Gbl_ParseOpCacheList = Next;
        BufferCount++;
    }

    DbgPrint (ASL_DEBUG_OUTPUT,
        "%u ParseOps, Buffer size: %u ops (%u bytes), %u Buffers\n",
        Gbl_ParseOpCount, ASL_PARSEOP_CACHE_SIZE,
        (sizeof (ACPI_PARSE_OBJECT) * ASL_PARSEOP_CACHE_SIZE), BufferCount);

    /* Reset cache globals */

    Gbl_ParseOpCount = 0;
    Gbl_ParseOpCacheNext = NULL;
    Gbl_ParseOpCacheLast = NULL;
    Gbl_ParseTreeRoot = NULL;


    /*
     * Table Compiler - Field cache
     */
    BufferCount = 0;
    while (Gbl_FieldCacheList)
    {
        Next = Gbl_FieldCacheList->Next;
        ACPI_FREE (Gbl_FieldCacheList);
        Gbl_FieldCacheList = Next;
        BufferCount++;
    }

    DbgPrint (ASL_DEBUG_OUTPUT,
        "%u Fields, Buffer size: %u fields (%u bytes), %u Buffers\n",
        Gbl_FieldCount, ASL_FIELD_CACHE_SIZE,
        (sizeof (DT_FIELD) * ASL_FIELD_CACHE_SIZE), BufferCount);

    /* Reset cache globals */

    Gbl_FieldCount = 0;
    Gbl_FieldCacheNext = NULL;
    Gbl_FieldCacheLast = NULL;


    /*
     * Table Compiler - Subtable cache
     */
    BufferCount = 0;
    while (Gbl_SubtableCacheList)
    {
        Next = Gbl_SubtableCacheList->Next;
        ACPI_FREE (Gbl_SubtableCacheList);
        Gbl_SubtableCacheList = Next;
        BufferCount++;
    }

    DbgPrint (ASL_DEBUG_OUTPUT,
        "%u Subtables, Buffer size: %u subtables (%u bytes), %u Buffers\n",
        Gbl_SubtableCount, ASL_SUBTABLE_CACHE_SIZE,
        (sizeof (DT_SUBTABLE) * ASL_SUBTABLE_CACHE_SIZE), BufferCount);

    /* Reset cache globals */

    Gbl_SubtableCount = 0;
    Gbl_SubtableCacheNext = NULL;
    Gbl_SubtableCacheLast = NULL;
}
