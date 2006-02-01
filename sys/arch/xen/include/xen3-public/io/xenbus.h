/*****************************************************************************
 * xenbus.h
 *
 * Xenbus protocol details.
 *
 * Copyright (C) 2005 XenSource Ltd.
 * 
 * This file may be distributed separately from the Linux kernel, or
 * incorporated into other software packages, subject to the following
 * license:
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
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

#ifndef _XEN_XENBUS_H
#define _XEN_XENBUS_H


/* The state of either end of the Xenbus, i.e. the current communication
   status of initialisation across the bus.  States here imply nothing about
   the state of the connection between the driver and the kernel's device
   layers.  */
typedef enum
{
  XenbusStateUnknown      = 0,
  XenbusStateInitialising = 1,
  XenbusStateInitWait     = 2,  /* Finished early initialisation, but waiting
                                   for information from the peer or hotplug
				   scripts. */
  XenbusStateInitialised  = 3,  /* Initialised and waiting for a connection
				   from the peer. */
  XenbusStateConnected    = 4,
  XenbusStateClosing      = 5,  /* The device is being closed due to an error
				   or an unplug event. */
  XenbusStateClosed       = 6

} XenbusState;


#endif /* _XEN_XENBUS_H */

/*
 * Local variables:
 *  c-file-style: "linux"
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
