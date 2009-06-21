/*
 *
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _INTEL_CHIPSET_H
#define _INTEL_CHIPSET_H

#define IS_I830(dev) ((dev)->pci_device == 0x3577)
#define IS_845G(dev) ((dev)->pci_device == 0x2562)
#define IS_I85X(dev) ((dev)->pci_device == 0x3582)
#define IS_I855(dev) ((dev)->pci_device == 0x3582)
#define IS_I865G(dev) ((dev)->pci_device == 0x2572)

#define IS_I915G(dev) ((dev)->pci_device == 0x2582 || (dev)->pci_device == 0x258a)
#define IS_I915GM(dev) ((dev)->pci_device == 0x2592)
#define IS_I945G(dev) ((dev)->pci_device == 0x2772)
#define IS_I945GM(dev) ((dev)->pci_device == 0x27A2 ||\
                        (dev)->pci_device == 0x27AE)
#define IS_I965G(dev) ((dev)->pci_device == 0x2972 || \
                       (dev)->pci_device == 0x2982 || \
                       (dev)->pci_device == 0x2992 || \
                       (dev)->pci_device == 0x29A2 || \
                       (dev)->pci_device == 0x2A02 || \
                       (dev)->pci_device == 0x2A12 || \
                       (dev)->pci_device == 0x2A42 || \
                       (dev)->pci_device == 0x2E02 || \
                       (dev)->pci_device == 0x2E12 || \
                       (dev)->pci_device == 0x2E22)

#define IS_I965GM(dev) ((dev)->pci_device == 0x2A02)

#define IS_GM45(dev) ((dev)->pci_device == 0x2A42)

#define IS_G4X(dev) ((dev)->pci_device == 0x2E02 || \
                     (dev)->pci_device == 0x2E12 || \
                     (dev)->pci_device == 0x2E22)

#define IS_G33(dev)    ((dev)->pci_device == 0x29C2 ||  \
                        (dev)->pci_device == 0x29B2 ||  \
                        (dev)->pci_device == 0x29D2)

#define IS_I9XX(dev) (IS_I915G(dev) || IS_I915GM(dev) || IS_I945G(dev) || \
                      IS_I945GM(dev) || IS_I965G(dev) || IS_G33(dev))

#define IS_MOBILE(dev) (IS_I830(dev) || IS_I85X(dev) || IS_I915GM(dev) || \
                        IS_I945GM(dev) || IS_I965GM(dev) || IS_GM45(dev))

#endif /* _INTEL_CHIPSET_H */
