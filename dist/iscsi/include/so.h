
/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By downloading, copying, installing or
 * using the software you agree to this license. If you do not agree to this license, do not download, install,
 * copy or use the software. 
 *
 * Intel License Agreement 
 *
 * Copyright (c) 2002, Intel Corporation
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met: 
 *
 * -Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *  following disclaimer. 
 *
 * -Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 *  following disclaimer in the documentation and/or other materials provided with the distribution. 
 *
 * -The name of Intel Corporation may not be used to endorse or promote products derived from this software
 *  without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE. 
 */


#ifndef _SO_H
#define _SO_H

#ifndef _SCSI_H
#include "scsi.h"
#endif

#ifndef _GENDISK_H
#include <linux/genhd.h>
#endif

typedef struct scsi_osd {
  Scsi_Device *device;
  unsigned capacity;              /* size in blocks */
  unsigned char ready;            /* flag ready for FLOPTICAL */
  unsigned char write_prot;       /* flag write_protect for rmvable dev */
  unsigned char sector_bit_size;  /* sector_size = 2 to the  bit size power */
  unsigned char sector_bit_shift; /* power of 2 sectors per FS block */
  unsigned has_part_table:1;	  /* has partition table */
} Scsi_Osd;

#endif
