/*	$NetBSD: mach_iokit.c,v 1.9 2003/03/09 18:33:28 manu Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_compat_darwin.h"
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mach_iokit.c,v 1.9 2003/03/09 18:33:28 manu Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/signal.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_map.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_port.h>
#include <compat/mach/mach_errno.h>
#include <compat/mach/mach_iokit.h>

#ifdef COMPAT_DARWIN
#include <compat/darwin/darwin_iokit.h>
#endif

struct mach_iokit_devclass *mach_iokit_devclasses[] = {
#ifdef COMPAT_DARWIN
	DARWIN_IOKIT_DEVCLASSES
#endif
	NULL,
};

int
mach_io_service_get_matching_services(args)
	struct mach_trap_args *args;
{
	mach_io_service_get_matching_services_request_t *req = args->smsg;
	mach_io_service_get_matching_services_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 
	struct lwp *l = args->l;
	struct mach_port *mp;
	struct mach_right *mr;
	struct mach_iokit_devclass *mid; 
	int i;

	mp = mach_port_get();
	mp->mp_flags |= MACH_MP_INKERNEL;
	mr = mach_right_get(mp, l, MACH_PORT_TYPE_SEND, 0);
	
	i = 0;
	while ((mid = mach_iokit_devclasses[i++]) != NULL) {
		if (memcmp(req->req_string, mid->mid_string, 
		    strlen(mid->mid_string)) == 0) {
			mp->mp_datatype = MACH_MP_IOKIT_DEVCLASS;
			mp->mp_data = mid;
			break;
		}
	}

	rep->rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE) |
	    MACH_MSGH_BITS_COMPLEX;
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_body.msgh_descriptor_count = 1;
	rep->rep_match.name = (mach_port_t)mr->mr_name;
	rep->rep_match.disposition = 0x11; /* XXX */
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int
mach_io_iterator_next(args)
	struct mach_trap_args *args;
{
	mach_io_iterator_next_request_t *req = args->smsg;
	mach_io_iterator_next_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 
	struct lwp *l = args->l;
	struct mach_port *mp;
	struct mach_right *mr;
	struct device *dev;
	struct mach_device_iterator *mdi;
	mach_port_t mn;

	mn = req->req_msgh.msgh_remote_port;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_iokit_error(args, MACH_IOKIT_EPERM);

	switch (mr->mr_port->mp_datatype) {
	case MACH_MP_IOKIT_DEVCLASS:
		/* Do not come here again */
		mr->mr_port->mp_datatype = MACH_MP_NONE;

		mp = mach_port_get();
		mp->mp_flags |= MACH_MP_INKERNEL;
		mp->mp_datatype = MACH_MP_IOKIT_DEVCLASS;
		mp->mp_data = mr->mr_port->mp_data;

		break;

	case MACH_MP_DEVICE_ITERATOR:
		mdi = mr->mr_port->mp_data;

		/* XXX No lock for the device list? */
		/* Check that mdi->mdi_parent still exists, else give up */
		TAILQ_FOREACH(dev, &alldevs, dv_list)
			if (dev == mdi->mdi_parent)
				break;
		if (dev == NULL)
			return mach_iokit_error(args, MACH_IOKIT_ENODEV);

		/* Check that mdi->mdi_current still exists, else reset it */
		TAILQ_FOREACH(dev, &alldevs, dv_list)
			if (dev == mdi->mdi_current)
				break;
		if (dev == NULL)
			mdi->mdi_current = TAILQ_FIRST(&alldevs);

		/* And now, find the next child of mdi->mdi_parrent. */
		do {
			if (dev->dv_parent == mdi->mdi_parent) {
				mdi->mdi_current = dev;
				break;
			}
		} while ((dev = TAILQ_NEXT(dev, dv_list)) != NULL);

		if (dev == NULL) 
			return mach_iokit_error(args, MACH_IOKIT_ENODEV);

		mp = mach_port_get();
		mp->mp_flags |= MACH_MP_INKERNEL;
		mp->mp_datatype = MACH_MP_DEVICE;
		mp->mp_data = mdi->mdi_current;
		mdi->mdi_current = TAILQ_NEXT(mdi->mdi_current, dv_list);
		break;
		
	default:
		return mach_iokit_error(args, MACH_IOKIT_EINVAL);
		break;
	}

	mr = mach_right_get(mp, l, MACH_PORT_TYPE_SEND, 0);
	
	rep->rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE) |
	    MACH_MSGH_BITS_COMPLEX;
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_body.msgh_descriptor_count = 1;
	rep->rep_object.name = (mach_port_t)mr->mr_name;
	rep->rep_object.disposition = 0x11; /* XXX */
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int
mach_io_service_open(args)
	struct mach_trap_args *args;
{
	mach_io_service_open_request_t *req = args->smsg;
	mach_io_service_open_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 
	struct lwp *l = args->l;
	struct mach_port *mp;
	struct mach_right *mr;
	mach_port_t mn;

	mn = req->req_msgh.msgh_remote_port;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_iokit_error(args, MACH_IOKIT_EPERM);

	mp = mach_port_get();
	mp->mp_flags |= MACH_MP_INKERNEL;
	if (mr->mr_port->mp_datatype == MACH_MP_IOKIT_DEVCLASS) {
		mp->mp_datatype = MACH_MP_IOKIT_DEVCLASS;
		mp->mp_data = mr->mr_port->mp_data;
	}
	mr = mach_right_get(mp, l, MACH_PORT_TYPE_SEND, 0);
	
	rep->rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE) |
	    MACH_MSGH_BITS_COMPLEX;
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_body.msgh_descriptor_count = 1;
	rep->rep_connect.name = (mach_port_t)mr->mr_name;
	rep->rep_connect.disposition = 0x11; /* XXX */
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int
mach_io_connect_method_scalari_scalaro(args)
	struct mach_trap_args *args;
{
	mach_io_connect_method_scalari_scalaro_request_t *req = args->smsg;
	mach_io_connect_method_scalari_scalaro_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 
	struct lwp *l = args->l;
	mach_port_t mn;
	struct mach_right *mr;
	struct mach_iokit_devclass *mid;

	mn = req->req_msgh.msgh_remote_port;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_iokit_error(args, MACH_IOKIT_EPERM);
	
	if (mr->mr_port->mp_datatype == MACH_MP_IOKIT_DEVCLASS) {
		mid = mr->mr_port->mp_data;
		if (mid->mid_connect_method_scalari_scalaro == NULL)
			printf("no connect_method_scalari_scalaro method "
			    "for darwin_iokit_class %s\n", mid->mid_name);
		else
			return (mid->mid_connect_method_scalari_scalaro)(args);
	}

	rep->rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_outcount = 0;
	rep->rep_out[rep->rep_outcount + 1] = 8; /* XXX Trailer */

	*msglen = sizeof(*rep) - ((4096 + rep->rep_outcount) * sizeof(int));
	return 0;
}

int
mach_io_connect_get_service(args)
	struct mach_trap_args *args;
{
	mach_io_connect_get_service_request_t *req = args->smsg;
	mach_io_connect_get_service_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 
	struct lwp *l = args->l;
	struct mach_port *mp;
	struct mach_right *mr;
	mach_port_t mn;

	mn = req->req_msgh.msgh_remote_port;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_iokit_error(args, MACH_IOKIT_EPERM);

	mp = mach_port_get();
	mp->mp_flags |= MACH_MP_INKERNEL;
	if (mr->mr_port->mp_datatype == MACH_MP_IOKIT_DEVCLASS) {
		mp->mp_datatype = MACH_MP_IOKIT_DEVCLASS;
		mp->mp_data = mr->mr_port->mp_data;
	}
	mr = mach_right_get(mp, l, MACH_PORT_TYPE_SEND, 0);

	/* 
	 * XXX Bump the refcount to workaround an emulation bug
	 * that causes Windowserver to release the port too early.
	 */
	mr->mr_refcount++;

	rep->rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE) |
	    MACH_MSGH_BITS_COMPLEX;
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_body.msgh_descriptor_count = 1;
	rep->rep_service.name = (mach_port_t)mr->mr_name;
	rep->rep_service.disposition = 0x11; /* XXX */
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int
mach_io_registry_entry_get_property(args)
	struct mach_trap_args *args;
{
	mach_io_registry_entry_get_property_request_t *req = args->smsg;
	mach_io_registry_entry_get_property_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 
	struct lwp *l = args->l;
	struct mach_port *mp;
	struct mach_right *mr;
	mach_port_t mn;
	struct mach_iokit_devclass *mid;

	mn = req->req_msgh.msgh_remote_port;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_iokit_error(args, MACH_IOKIT_EPERM);
	
	if (mr->mr_port->mp_datatype == MACH_MP_IOKIT_DEVCLASS) {
		mid = mr->mr_port->mp_data;
		if (mid->mid_registry_entry_get_property == NULL)
			printf("no registry_entry_get_property method "
			    "for darwin_iokit_class %s\n", mid->mid_name);
		else
			return (mid->mid_registry_entry_get_property)(args);
	}

	mp = mach_port_get();
	mp->mp_flags |= MACH_MP_INKERNEL;
	mr = mach_right_get(mp, l, MACH_PORT_TYPE_SEND, 0);

	rep->rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE) |
	    MACH_MSGH_BITS_COMPLEX;
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_body.msgh_descriptor_count = 1;
	rep->rep_properties.name = (mach_port_t)mr->mr_name;
	rep->rep_properties.disposition = 0x11; /* XXX */
	rep->rep_properties_count = 1; /* XXX */
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int
mach_io_registry_entry_create_iterator(args)
	struct mach_trap_args *args;
{
	mach_io_registry_entry_create_iterator_request_t *req = args->smsg;
	mach_io_registry_entry_create_iterator_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 
	struct lwp *l = args->l;
	struct mach_port *mp;
	struct mach_right *mr;

	mp = mach_port_get();
	mp->mp_flags |= MACH_MP_INKERNEL;
	mr = mach_right_get(mp, l, MACH_PORT_TYPE_SEND, 0);

	rep->rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE) |
	    MACH_MSGH_BITS_COMPLEX;
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_body.msgh_descriptor_count = 1;
	rep->rep_iterator.name = (mach_port_t)mr->mr_name;
	rep->rep_iterator.disposition = 0x11; /* XXX */
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int
mach_io_object_conforms_to(args)
	struct mach_trap_args *args;
{
	mach_io_object_conforms_to_request_t *req = args->smsg;
	mach_io_object_conforms_to_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 

	rep->rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_conforms = 1; /* XXX */
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int
mach_io_service_add_interest_notification(args)
	struct mach_trap_args *args;
{
	mach_io_service_add_interest_notification_request_t *req = args->smsg;
	mach_io_service_add_interest_notification_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 
	struct lwp *l = args->l;
	struct mach_port *mp;
	struct mach_right *mr;

	mp = mach_port_get();
	mp->mp_flags |= MACH_MP_INKERNEL;
	mr = mach_right_get(mp, l, MACH_PORT_TYPE_SEND, 0);

	rep->rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE) |
	    MACH_MSGH_BITS_COMPLEX;
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_body.msgh_descriptor_count = 1;
	rep->rep_notification.name = (mach_port_t)mr->mr_name;
	rep->rep_notification.disposition = 0x11; /* XXX */
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int
mach_io_connect_set_notification_port(args)
	struct mach_trap_args *args;
{
	mach_io_connect_set_notification_port_request_t *req = args->smsg;
	mach_io_connect_set_notification_port_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 

	rep->rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int
mach_io_registry_get_root_entry(args)
	struct mach_trap_args *args;
{
	mach_io_registry_get_root_entry_request_t *req = args->smsg;
	mach_io_registry_get_root_entry_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 
	struct lwp *l = args->l;
	struct mach_port *mp;
	struct mach_right *mr;

	mp = mach_port_get();
	mp->mp_flags |= MACH_MP_INKERNEL;
	mp->mp_datatype = MACH_MP_DEVICE;
	mp->mp_data = TAILQ_FIRST(&alldevs);

	mr = mach_right_get(mp, l, MACH_PORT_TYPE_SEND, 0);

	rep->rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE) |
	    MACH_MSGH_BITS_COMPLEX;
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_body.msgh_descriptor_count = 1;
	rep->rep_root.name = (mach_port_t)mr->mr_name;
	rep->rep_root.disposition = 0x11; /* XXX */
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int
mach_io_registry_entry_get_child_iterator(args)
	struct mach_trap_args *args;
{
	mach_io_registry_entry_get_child_iterator_request_t *req = args->smsg;
	mach_io_registry_entry_get_child_iterator_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 
	struct lwp *l = args->l;
	struct mach_port *mp;
	struct mach_right *mr;
	mach_port_t mn;
	struct mach_device_iterator *mdi;

	mn = req->req_msgh.msgh_remote_port;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_iokit_error(args, MACH_IOKIT_EPERM);
	if (mr->mr_port->mp_datatype != MACH_MP_DEVICE)
		return mach_iokit_error(args, MACH_IOKIT_EINVAL);

	mp = mach_port_get();
	mp->mp_flags |= (MACH_MP_INKERNEL | MACH_MP_DATA_ALLOCATED);
	mp->mp_datatype = MACH_MP_DEVICE_ITERATOR;

	mdi = malloc(sizeof(*mdi), M_EMULDATA, M_WAITOK);
	mdi->mdi_parent = mr->mr_port->mp_data;
	mdi->mdi_current = TAILQ_FIRST(&alldevs);
	mp->mp_data = mdi;

	mr = mach_right_get(mp, l, MACH_PORT_TYPE_SEND, 0);

	rep->rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE) |
	    MACH_MSGH_BITS_COMPLEX;
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_body.msgh_descriptor_count = 1;
	rep->rep_iterator.name = (mach_port_t)mr->mr_name;
	rep->rep_iterator.disposition = 0x11; /* XXX */
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int
mach_io_registry_entry_get_name_in_plane(args)
	struct mach_trap_args *args;
{
	mach_io_registry_entry_get_name_in_plane_request_t *req = args->smsg;
	mach_io_registry_entry_get_name_in_plane_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 
	struct lwp *l = args->l;
	struct mach_right *mr;
	mach_port_t mn;
	struct device *dev;

	mn = req->req_msgh.msgh_remote_port;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_iokit_error(args, MACH_IOKIT_EPERM);
	if (mr->mr_port->mp_datatype != MACH_MP_DEVICE)
		return mach_iokit_error(args, MACH_IOKIT_EINVAL);
	dev = mr->mr_port->mp_data;

	rep->rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	/* XXX Just return a dummy name for now */ 
	rep->rep_namecount = sizeof(dev->dv_xname);
	memcpy(&rep->rep_name, dev->dv_xname, sizeof(dev->dv_xname));
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int
mach_io_object_get_class(args)
	struct mach_trap_args *args;
{
	mach_io_object_get_class_request_t *req = args->smsg;
	mach_io_object_get_class_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 
	char classname[] = "unknownClass";

	rep->rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	/* XXX Just return a dummy name for now */ 
	rep->rep_namecount = sizeof(classname);
	memcpy(&rep->rep_name, classname, sizeof(classname));
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int
mach_io_registry_entry_get_location_in_plane(args)
	struct mach_trap_args *args;
{
	mach_io_registry_entry_get_location_in_plane_request_t *req = 
	    args->smsg;
	mach_io_registry_entry_get_location_in_plane_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 
	char location[] = "";

	rep->rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	/* XXX Just return a dummy name for now */ 
	rep->rep_locationcount = sizeof(location);
	memcpy(&rep->rep_location, location, sizeof(location));
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int
mach_io_registry_entry_get_properties(args)
	struct mach_trap_args *args;
{
	mach_io_registry_entry_get_properties_request_t *req = args->smsg;
	mach_io_registry_entry_get_properties_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 
	struct lwp *l = args->l;
	int error;
	vaddr_t va;
	size_t size;
	mach_port_t mn;
	struct mach_right *mr;
	struct mach_iokit_devclass *mid;

	mn = req->req_msgh.msgh_remote_port;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_iokit_error(args, MACH_IOKIT_EPERM);
	
	if (mr->mr_port->mp_datatype != MACH_MP_IOKIT_DEVCLASS) 
		return mach_iokit_error(args, MACH_IOKIT_EINVAL);

	mid = mr->mr_port->mp_data;
	if (mid->mid_properties == NULL)
		return mach_iokit_error(args, MACH_IOKIT_EINVAL);
	size = round_page(strlen(mid->mid_properties));

	va = vm_map_min(&l->l_proc->p_vmspace->vm_map);
	if ((error = uvm_map(&l->l_proc->p_vmspace->vm_map, &va, size,
	    NULL, UVM_UNKNOWN_OFFSET, 0, UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_ALL,
	    UVM_INH_COPY, UVM_ADV_NORMAL, UVM_FLAG_COPYONW))) != 0)
		return mach_msg_error(args, error);

#ifdef DEBUG_MACH
	printf("pid %d.%d: copyout iokit properties at %p\n",
		    l->l_proc->p_pid, l->l_lid, (void *)va);
#endif
	if ((error = copyout(mid->mid_properties, (void *)va, size)) != 0) {
#ifdef DEBUG_MACH
		printf("pid %d.%d: copyout iokit properties failed\n",
		    l->l_proc->p_pid, l->l_lid);
#endif
	}

	rep->rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE) |
	    MACH_MSGH_BITS_COMPLEX;
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_body.msgh_descriptor_count = 1;
	rep->rep_properties.address = (void *)va;
	rep->rep_properties.size = size;
	rep->rep_properties.deallocate = 0; /* XXX */
	rep->rep_properties.copy = 2; /* XXX */
	rep->rep_properties.pad1 = 1; /* XXX */
	rep->rep_properties.type = 1; /* XXX */
	rep->rep_count = size;
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

