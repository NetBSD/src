/* $NetBSD: ioreq.h,v 1.1.1.2.4.2 2012/04/17 00:07:09 yamt Exp $ */
/*
 * ioreq.h: I/O request definitions for device models
 * Copyright (c) 2004, Intel Corporation.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef _IOREQ_H_
#define _IOREQ_H_

#define IOREQ_READ      1
#define IOREQ_WRITE     0

#define STATE_IOREQ_NONE        0
#define STATE_IOREQ_READY       1
#define STATE_IOREQ_INPROCESS   2
#define STATE_IORESP_READY      3

#define IOREQ_TYPE_PIO          0 /* pio */
#define IOREQ_TYPE_COPY         1 /* mmio ops */
#define IOREQ_TYPE_TIMEOFFSET   7
#define IOREQ_TYPE_INVALIDATE   8 /* mapcache */

/*
 * VMExit dispatcher should cooperate with instruction decoder to
 * prepare this structure and notify service OS and DM by sending
 * virq
 */
struct ioreq {
    uint64_t addr;          /* physical address */
    uint64_t data;          /* data (or paddr of data) */
    uint32_t count;         /* for rep prefixes */
    uint32_t size;          /* size in bytes */
    uint32_t vp_eport;      /* evtchn for notifications to/from device model */
    uint16_t _pad0;
    uint8_t state:4;
    uint8_t data_is_ptr:1;  /* if 1, data above is the guest paddr 
                             * of the real data to use. */
    uint8_t dir:1;          /* 1=read, 0=write */
    uint8_t df:1;
    uint8_t _pad1:1;
    uint8_t type;           /* I/O type */
};
typedef struct ioreq ioreq_t;

struct shared_iopage {
    struct ioreq vcpu_ioreq[1];
};
typedef struct shared_iopage shared_iopage_t;

struct buf_ioreq {
    uint8_t  type;   /* I/O type                    */
    uint8_t  pad:1;
    uint8_t  dir:1;  /* 1=read, 0=write             */
    uint8_t  size:2; /* 0=>1, 1=>2, 2=>4, 3=>8. If 8, use two buf_ioreqs */
    uint32_t addr:20;/* physical address            */
    uint32_t data;   /* data                        */
};
typedef struct buf_ioreq buf_ioreq_t;

#define IOREQ_BUFFER_SLOT_NUM     511 /* 8 bytes each, plus 2 4-byte indexes */
struct buffered_iopage {
    unsigned int read_pointer;
    unsigned int write_pointer;
    buf_ioreq_t buf_ioreq[IOREQ_BUFFER_SLOT_NUM];
}; /* NB. Size of this structure must be no greater than one page. */
typedef struct buffered_iopage buffered_iopage_t;

#if defined(__ia64__)
struct pio_buffer {
    uint32_t page_offset;
    uint32_t pointer;
    uint32_t data_end;
    uint32_t buf_size;
    void *opaque;
};

#define PIO_BUFFER_IDE_PRIMARY   0 /* I/O port = 0x1F0 */
#define PIO_BUFFER_IDE_SECONDARY 1 /* I/O port = 0x170 */
#define PIO_BUFFER_ENTRY_NUM     2
struct buffered_piopage {
    struct pio_buffer pio[PIO_BUFFER_ENTRY_NUM];
    uint8_t buffer[1];
};
#endif /* defined(__ia64__) */

/*
 * ACPI Control/Event register locations. Location is controlled by a 
 * version number in HVM_PARAM_ACPI_IOPORTS_LOCATION.
 */

/* Version 0 (default): Traditional Xen locations. */
#define ACPI_PM1A_EVT_BLK_ADDRESS_V0 0x1f40
#define ACPI_PM1A_CNT_BLK_ADDRESS_V0 (ACPI_PM1A_EVT_BLK_ADDRESS_V0 + 0x04)
#define ACPI_PM_TMR_BLK_ADDRESS_V0   (ACPI_PM1A_EVT_BLK_ADDRESS_V0 + 0x08)
#define ACPI_GPE0_BLK_ADDRESS_V0     (ACPI_PM_TMR_BLK_ADDRESS_V0 + 0x20)
#define ACPI_GPE0_BLK_LEN_V0         0x08

/* Version 1: Locations preferred by modern Qemu. */
#define ACPI_PM1A_EVT_BLK_ADDRESS_V1 0xb000
#define ACPI_PM1A_CNT_BLK_ADDRESS_V1 (ACPI_PM1A_EVT_BLK_ADDRESS_V1 + 0x04)
#define ACPI_PM_TMR_BLK_ADDRESS_V1   (ACPI_PM1A_EVT_BLK_ADDRESS_V1 + 0x08)
#define ACPI_GPE0_BLK_ADDRESS_V1     0xafe0
#define ACPI_GPE0_BLK_LEN_V1         0x04

/* Compatibility definitions for the default location (version 0). */
#define ACPI_PM1A_EVT_BLK_ADDRESS    ACPI_PM1A_EVT_BLK_ADDRESS_V0
#define ACPI_PM1A_CNT_BLK_ADDRESS    ACPI_PM1A_CNT_BLK_ADDRESS_V0
#define ACPI_PM_TMR_BLK_ADDRESS      ACPI_PM_TMR_BLK_ADDRESS_V0
#define ACPI_GPE0_BLK_ADDRESS        ACPI_GPE0_BLK_ADDRESS_V0
#define ACPI_GPE0_BLK_LEN            ACPI_GPE0_BLK_LEN_V0


#endif /* _IOREQ_H_ */

/*
 * Local variables:
 * mode: C
 * c-set-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
