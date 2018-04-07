/******************************************************************************
 *
 * Module Name: aslallocate -- Local memory allocation
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
 * Local heap allocation wrappers. See aslcache.c for allocation from local
 * cache alloctions
 */


/*******************************************************************************
 *
 * FUNCTION:    UtLocalCalloc
 *
 * PARAMETERS:  Size                - Bytes to be allocated
 *
 * RETURN:      Pointer to the allocated memory. If this function returns
 *              (the compiler is not aborted), the pointer is guaranteed to
 *              be valid.
 *
 * DESCRIPTION: Allocate zero-initialized memory. The point of this function
 *              is to abort the compile on an allocation failure, on the
 *              assumption that nothing more can be accomplished.
 *
 * NOTE:        For allocation from the local caches, see aslcache.c
 *
 ******************************************************************************/

void *
UtLocalCalloc (
    UINT32                  Size)
{
    void                    *Allocated;


    Allocated = ACPI_ALLOCATE_ZEROED (Size);
    if (!Allocated)
    {
        AslCommonError (ASL_ERROR, ASL_MSG_MEMORY_ALLOCATION,
            Gbl_CurrentLineNumber, Gbl_LogicalLineNumber,
            Gbl_InputByteCount, Gbl_CurrentColumn,
            Gbl_Files[ASL_FILE_INPUT].Filename, NULL);

        CmCleanupAndExit ();
        exit (1);
    }

    TotalAllocations++;
    TotalAllocated += Size;
    return (Allocated);
}


/******************************************************************************
 *
 * FUNCTION:    UtExpandLineBuffers
 *
 * PARAMETERS:  None. Updates global line buffer pointers.
 *
 * RETURN:      None. Reallocates the global line buffers
 *
 * DESCRIPTION: Called if the current line buffer becomes filled. Reallocates
 *              all global line buffers and updates Gbl_LineBufferSize. NOTE:
 *              Also used for the initial allocation of the buffers, when
 *              all of the buffer pointers are NULL. Initial allocations are
 *              of size ASL_DEFAULT_LINE_BUFFER_SIZE
 *
 *****************************************************************************/

void
UtExpandLineBuffers (
    void)
{
    UINT32                  NewSize;


    /* Attempt to double the size of all line buffers */

    NewSize = Gbl_LineBufferSize * 2;
    if (Gbl_CurrentLineBuffer)
    {
        DbgPrint (ASL_DEBUG_OUTPUT,
            "Increasing line buffer size from %u to %u\n",
            Gbl_LineBufferSize, NewSize);
    }

    UtReallocLineBuffers (&Gbl_CurrentLineBuffer, Gbl_LineBufferSize, NewSize);
    UtReallocLineBuffers (&Gbl_MainTokenBuffer, Gbl_LineBufferSize, NewSize);
    UtReallocLineBuffers (&Gbl_MacroTokenBuffer, Gbl_LineBufferSize, NewSize);
    UtReallocLineBuffers (&Gbl_ExpressionTokenBuffer, Gbl_LineBufferSize, NewSize);

    Gbl_LineBufPtr = Gbl_CurrentLineBuffer;
    Gbl_LineBufferSize = NewSize;
}


/******************************************************************************
 *
 * FUNCTION:    UtReallocLineBuffers
 *
 * PARAMETERS:  Buffer              - Buffer to realloc
 *              OldSize             - Old size of Buffer
 *              NewSize             - New size of Buffer
 *
 * RETURN:      none
 *
 * DESCRIPTION: Reallocate and initialize Buffer
 *
 *****************************************************************************/

void
UtReallocLineBuffers (
    char                    **Buffer,
    UINT32                  OldSize,
    UINT32                  NewSize)
{

    *Buffer = realloc (*Buffer, NewSize);
    if (*Buffer)
    {
        memset (*Buffer + OldSize, 0, NewSize - OldSize);
        return;
    }

    printf ("Could not increase line buffer size from %u to %u\n",
        OldSize, NewSize);

    AslError (ASL_ERROR, ASL_MSG_BUFFER_ALLOCATION, NULL, NULL);
    AslAbort ();
}


/******************************************************************************
 *
 * FUNCTION:    UtFreeLineBuffers
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Free all line buffers
 *
 *****************************************************************************/

void
UtFreeLineBuffers (
    void)
{

    free (Gbl_CurrentLineBuffer);
    free (Gbl_MainTokenBuffer);
    free (Gbl_MacroTokenBuffer);
    free (Gbl_ExpressionTokenBuffer);
}
