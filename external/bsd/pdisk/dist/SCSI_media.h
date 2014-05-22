/*
 * SCSI_media.h -
 *
 * Written by Eryk Vershen
 */

/*
 * Copyright 1997,1998 by Apple Computer, Inc.
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

#ifndef __SCSI_media__
#define __SCSI_media__

#include "media.h"


/*
 * Defines
 */


/*
 * Types
 */


/*
 * Global Constants
 */


/*
 * Global Variables
 */


/*
 * Forward declarations
 */
MEDIA SCSI_FindDevice(long dRefNum);
MEDIA open_old_scsi_as_media(long device);
MEDIA open_scsi_as_media(long bus, long device);
MEDIA_ITERATOR create_scsi_iterator(void);
MEDIA open_linux_scsi_as_media(long index, int is_cdrom);
char *linux_scsi_name(long bus, long id);
char *linux_old_scsi_name(long id);

#endif /* __SCSI_media__ */
