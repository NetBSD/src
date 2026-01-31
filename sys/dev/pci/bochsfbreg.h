/*
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Bochs DISPI interface register definitions
 * See:
 *      https://wiki.osdev.org/Bochs_VBE_Extensions
 *      https://www.qemu.org/docs/master/specs/standard-vga.html
 */

#ifndef BOCHSFBREG_H
#define BOCHSFBREG_H

/* Standard VGA I/O ports */
#define VGA_IO_START              0x3C0
#define VGA_IO_SIZE               0x20

/* VGA registers we access */
#define VGA_CRTC_INDEX            0x3D4
#define VGA_CRTC_DATA             0x3D5

/* Bochs VBE DISPI interface I/O ports */
#define VBE_DISPI_IOPORT_INDEX    0x01CE
#if defined(__i386__) || defined(__x86_64__)
#define VBE_DISPI_IOPORT_DATA     0x01CF
#else
#define VBE_DISPI_IOPORT_DATA     0x01D0
#endif

/* Bochs VBE DISPI interface MMIO bar and offset */
#define BOCHSFB_MMIO_BAR          0x14
#define BOCHSFB_MMIO_EDID_OFFSET  0x000
#define BOCHSFB_MMIO_EDID_SIZE    0x400
#define BOCHSFB_MMIO_VGA_OFFSET   0x400
#define BOCHSFB_MMIO_DISPI_OFFSET 0x500

/* VBE DISPI interface indices */
#define VBE_DISPI_INDEX_ID        0x0
#define VBE_DISPI_INDEX_XRES      0x1
#define VBE_DISPI_INDEX_YRES      0x2
#define VBE_DISPI_INDEX_BPP       0x3
#define VBE_DISPI_INDEX_ENABLE    0x4
#define VBE_DISPI_INDEX_BANK      0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH 0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT 0x7
#define VBE_DISPI_INDEX_X_OFFSET  0x8
#define VBE_DISPI_INDEX_Y_OFFSET  0x9
#define VBE_DISPI_INDEX_VIDEO_MEMORY_64K 0xa

/* VBE DISPI interface ID values */
#define VBE_DISPI_ID0             0xB0C0  /* Magic value for first 12 bits */
#define VBE_DISPI_ID1             0xB0C1
#define VBE_DISPI_ID2             0xB0C2
#define VBE_DISPI_ID3             0xB0C3
#define VBE_DISPI_ID4             0xB0C4
#define VBE_DISPI_ID5             0xB0C5

/* VBE DISPI interface enable values */
#define VBE_DISPI_ENABLED         0x01
#define VBE_DISPI_GETCAPS         0x02
#define VBE_DISPI_8BIT_DAC        0x20
#define VBE_DISPI_LFB_ENABLED     0x40
#define VBE_DISPI_NOCLEARMEM      0x80

#endif /* BOCHSFBREG_H */
