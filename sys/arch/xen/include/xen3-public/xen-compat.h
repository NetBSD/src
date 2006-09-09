/******************************************************************************
 * xen-compat.h
 * 
 * Guest OS interface to Xen.  Compatibility layer.
 * 
 * Copyright (c) 2006, Christian Limpach
 */

#ifndef __XEN_PUBLIC_XEN_COMPAT_H__
#define __XEN_PUBLIC_XEN_COMPAT_H__

#define __XEN_LATEST_INTERFACE_VERSION__ 0x00030101

#if defined(__XEN__)
/* Xen is built with matching headers and implements the latest interface. */
#define __XEN_INTERFACE_VERSION__ __XEN_LATEST_INTERFACE_VERSION__
#elif !defined(__XEN_INTERFACE_VERSION__)
/* Guests which do not specify a version get the legacy interface. */
#define __XEN_INTERFACE_VERSION__ 0x00000000
#endif

#if __XEN_INTERFACE_VERSION__ > __XEN_LATEST_INTERFACE_VERSION__
#error "These header files do not support the requested interface version."
#endif

#if __XEN_INTERFACE_VERSION__ < 0x00030101
#undef __HYPERVISOR_sched_op
#define __HYPERVISOR_sched_op __HYPERVISOR_sched_op_compat
#endif

#endif /* __XEN_PUBLIC_XEN_COMPAT_H__ */
