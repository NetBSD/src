/* $NetBSD: e820.h,v 1.2.14.3 2008/01/09 01:50:12 matt Exp $ */

/*
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

#ifndef __XEN_PUBLIC_HVM_E820_H__
#define __XEN_PUBLIC_HVM_E820_H__

/* PC BIOS standard E820 types. */
#define E820_RAM          1
#define E820_RESERVED     2
#define E820_ACPI         3
#define E820_NVS          4

/* E820 location in HVM virtual address space. */
#define E820_MAP_PAGE        0x00090000
#define E820_MAP_NR_OFFSET   0x000001E8
#define E820_MAP_OFFSET      0x000002D0

struct e820entry {
    uint64_t addr;
    uint64_t size;
    uint32_t type;
} __packed;

#define HVM_BELOW_4G_RAM_END        0xF0000000

#define HVM_BELOW_4G_MMIO_START     HVM_BELOW_4G_RAM_END
#define HVM_BELOW_4G_MMIO_LENGTH    ((1ULL << 32) - HVM_BELOW_4G_MMIO_START)

#endif /* __XEN_PUBLIC_HVM_E820_H__ */
