/*
 * Copyright 2005 Adam Jackson.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * ADAM JACKSON BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* derived from tdfx_drv.h */

#ifndef __IMAGINE_DRV_H__
#define __IMAGINE_DRV_H__

#define DRIVER_AUTHOR       "Adam Jackson"
#define DRIVER_NAME         "imagine"
#define DRIVER_DESC         "#9 Imagine128 and Ticket 2 Ride"
#define DRIVER_DATE         "20050328"
#define DRIVER_MAJOR        0
#define DRIVER_MINOR        0
#define DRIVER_PATCHLEVEL   1

enum imagine_family {
    IMAGINE_128,
    IMAGINE_128_2,
    IMAGINE_T2R,
    IMAGINE_REV4
};

#endif /* __IMAGINE_DRV_H__ */
