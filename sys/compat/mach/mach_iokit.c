/*	$NetBSD: mach_iokit.c,v 1.22 2003/09/11 23:16:18 manu Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mach_iokit.c,v 1.22 2003/09/11 23:16:18 manu Exp $");

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
		mr->mr_port->mp_datatype = MACH_MP_IOKIT_DEVCLASS_DONE;

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

		/* And now, find the next child of mdi->mdi_parent. */
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

	return mach_iokit_error(args, MACH_IOKIT_ENODEV);
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
	struct lwp *l = args->l;
	size_t *msglen = args->rsize; 
	mach_port_t mnn, mn;
	struct mach_right *mrn;
	struct mach_right *mr;
	struct mach_iokit_devclass *mid;

#ifdef DEBUG_DARWIN
	printf("mach_io_connect_set_notification_port\n");
#endif
	mnn = req->req_port.name;
	if ((mrn = mach_right_check(mnn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_msg_error(args, EINVAL);
		
	mn = req->req_msgh.msgh_remote_port;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_msg_error(args, EINVAL);

	if (mr->mr_port->mp_datatype != MACH_MP_IOKIT_DEVCLASS)
		return mach_msg_error(args, EINVAL);

#ifdef DEBUG_DARWIN
	printf("notification on right %p, name %x\n", mrn, mrn->mr_name);
#endif
	mid = (struct mach_iokit_devclass *)mr->mr_port->mp_data;
	mid->mid_notify = mrn;

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
	size = strlen(mid->mid_properties) + 1; /* Include trailing zero */

	va = vm_map_min(&l->l_proc->p_vmspace->vm_map);
	if ((error = uvm_map(&l->l_proc->p_vmspace->vm_map, &va, 
	    round_page(size), NULL, UVM_UNKNOWN_OFFSET, 0, 
	    UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_ALL,
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

int
mach_io_registry_entry_get_property(args)
	struct mach_trap_args *args;
{
	mach_io_registry_entry_get_property_request_t *req = args->smsg;
	mach_io_registry_entry_get_property_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 
	struct lwp *l = args->l;
	int error;
	vaddr_t va;
	size_t size;
	size_t len;
	mach_port_t mn;
	struct mach_right *mr;
	struct mach_iokit_devclass *mid;
	struct mach_iokit_property *mip;

#ifdef DEBUG_MACH
	/* 
	 * We do not handle non zero offset and multiple names,
	 * but it seems that Darwin binaries just jold random values
	 * in theses fields. We have yet to see a real use of 
	 * non null offset / multiple names.
	 */
	if (req->req_property_nameoffset != 0) {
		printf("pid %d.%d: mach_io_registry_entry_get_property "
		    "offset = %d (ignoring)\n", l->l_proc->p_pid, l->l_lid,
		    req->req_property_nameoffset);
	}
	if (req->req_property_namecount != 1) {
		printf("pid %d.%d: mach_io_registry_entry_get_property "
		    "count = %d (ignoring)\n", l->l_proc->p_pid, l->l_lid, 
		    req->req_property_namecount);
	}
#endif

	/* Find the port */
	mn = req->req_msgh.msgh_remote_port;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_iokit_error(args, MACH_IOKIT_EPERM);
	
	/* Find the devclass information */
	if (mr->mr_port->mp_datatype != MACH_MP_IOKIT_DEVCLASS) 
		return mach_iokit_error(args, MACH_IOKIT_EINVAL);

	mid = mr->mr_port->mp_data;
	if (mid->mid_properties_array == NULL)
		return mach_iokit_error(args, MACH_IOKIT_EINVAL);

	/* Lookup the property name */
	for (mip = mid->mid_properties_array; mip->mip_name; mip++) {
		len = strlen(mip->mip_name);
		if (memcmp(mip->mip_name, req->req_propery_name, len) == 0)
			break;
	}
	if (mip->mip_value == NULL)
		return mach_iokit_error(args, MACH_IOKIT_ENOENT);

	/* And copyout its associated value */
	va = vm_map_min(&l->l_proc->p_vmspace->vm_map);
	size = strlen(mip->mip_value) + 1; /* Include trailing zero */

	if ((error = uvm_map(&l->l_proc->p_vmspace->vm_map, &va, 
	    round_page(size), NULL, UVM_UNKNOWN_OFFSET, 0, 
	    UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_ALL,
	    UVM_INH_COPY, UVM_ADV_NORMAL, UVM_FLAG_COPYONW))) != 0)
		return mach_msg_error(args, error);

#ifdef DEBUG_MACH
	printf("pid %d.%d: copyout iokit property at %p\n",
		    l->l_proc->p_pid, l->l_lid, (void *)va);
#endif
	if ((error = copyout(mip->mip_value, (void *)va, size)) != 0) {
#ifdef DEBUG_MACH
		printf("pid %d.%d: copyout iokit property failed\n",
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
	rep->rep_properties.pad1 = 0; /* XXX */
	rep->rep_properties.type = 0; /* XXX */
	rep->rep_properties_count = size;
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int
mach_io_registry_entry_get_path(args)
	struct mach_trap_args *args;
{
	mach_io_registry_entry_get_path_request_t *req = args->smsg;
	mach_io_registry_entry_get_path_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 
	char location[] = ":/GossamerPE/pci@80000000/AppleGracklePCI/"
	    "ATY,264LT-G@11/.Display_Video_ATI_mach64-01018002/"
	    "display0/AppleBacklightDisplay";
	char *cp;
	size_t len, plen;

	/* XXX Just return a dummy name for now */ 
	len = req->req_count + strlen(location) - 1; 

	/* Sanity check for len */
	if (len > 512)
		return mach_iokit_error(args, MACH_IOKIT_EINVAL);
	plen = (len & ~0x3UL) + 4;	/* Round to an int */

	rep->rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep->rep_msgh.msgh_size = sizeof(*rep) + 
	    (plen - 512) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_retval = 0;
	rep->rep_count = len;

	cp = &rep->rep_path[0];
	memcpy(cp, &req->req_plane, req->req_count);
	cp += (req->req_count - 1);	/* overwrite trailing \0 */
	memcpy(cp, location, strlen(location));
	cp += strlen(location);
	*cp = '\0';

	rep->rep_path[plen + 7] = 8;	/* Trailer */

	*msglen = sizeof(*rep) + (plen - 512);
	return 0;
}

int
mach_io_connect_map_memory(args)
	struct mach_trap_args *args;
{
	mach_io_connect_map_memory_request_t *req = args->smsg;
	struct lwp *l = args->l;
	mach_port_t mn;
	struct mach_right *mr;
	struct mach_iokit_devclass *mid;

	mn = req->req_msgh.msgh_remote_port;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_iokit_error(args, MACH_IOKIT_EPERM);
	
	if (mr->mr_port->mp_datatype == MACH_MP_IOKIT_DEVCLASS) {
		mid = mr->mr_port->mp_data;
		if (mid->mid_connect_map_memory == NULL)
			printf("no connect_map_memory method "
			    "for darwin_iokit_class %s\n", mid->mid_name);
		else
			return (mid->mid_connect_map_memory)(args);
	}

	return mach_iokit_error(args, MACH_IOKIT_ENODEV);
}

int
mach_io_iterator_reset(args)
	struct mach_trap_args *args;
{
	mach_io_iterator_reset_request_t *req = args->smsg;
	mach_io_iterator_reset_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 
	struct lwp *l = args->l;
	mach_port_t mn;
	struct mach_right *mr;
	struct mach_device_iterator *mdi;

	mn = req->req_msgh.msgh_remote_port;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_iokit_error(args, MACH_IOKIT_EPERM);
	
	switch(mr->mr_port->mp_datatype) {
	case MACH_MP_DEVICE_ITERATOR:
		mdi = mr->mr_port->mp_data;
		mdi->mdi_parent = mr->mr_port->mp_data;
		mdi->mdi_current = TAILQ_FIRST(&alldevs);
		break;
	
	case MACH_MP_IOKIT_DEVCLASS_DONE:
		mr->mr_port->mp_datatype = MACH_MP_IOKIT_DEVCLASS;
		break;

	case MACH_MP_IOKIT_DEVCLASS:
	default:
		printf("mach_io_iterator_reset: unknown type\n");
	}

	rep->rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_retval = 0;
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);

	return 0;
}

int
mach_io_connect_method_scalari_structo(args)
	struct mach_trap_args *args;
{
	mach_io_connect_method_scalari_structo_request_t *req = args->smsg;
	struct lwp *l = args->l;
	mach_port_t mn;
	struct mach_right *mr;
	struct mach_iokit_devclass *mid;

	mn = req->req_msgh.msgh_remote_port;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_iokit_error(args, MACH_IOKIT_EPERM);
	
	if (mr->mr_port->mp_datatype == MACH_MP_IOKIT_DEVCLASS) {
		mid = mr->mr_port->mp_data;
		if (mid->mid_connect_method_scalari_structo == NULL)
			printf("no connect_method_scalari_structo method "
			    "for darwin_iokit_class %s\n", mid->mid_name);
		else
			return (mid->mid_connect_method_scalari_structo)(args);
	}

	return mach_iokit_error(args, MACH_IOKIT_ENODEV);
}

int
mach_io_connect_method_structi_structo(args)
	struct mach_trap_args *args;
{
	mach_io_connect_method_structi_structo_request_t *req = args->smsg;
	struct lwp *l = args->l;
	mach_port_t mn;
	struct mach_right *mr;
	struct mach_iokit_devclass *mid;

	mn = req->req_msgh.msgh_remote_port;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_iokit_error(args, MACH_IOKIT_EPERM);
	
	if (mr->mr_port->mp_datatype == MACH_MP_IOKIT_DEVCLASS) {
		mid = mr->mr_port->mp_data;
		if (mid->mid_connect_method_structi_structo == NULL)
			printf("no connect_method_structi_structo method "
			    "for darwin_iokit_class %s\n", mid->mid_name);
		else
			return (mid->mid_connect_method_structi_structo)(args);
	}

	return mach_iokit_error(args, MACH_IOKIT_ENODEV);
}

int
mach_io_connect_set_properties(args)
	struct mach_trap_args *args;
{
	mach_io_connect_set_properties_request_t *req = args->smsg;
	mach_io_connect_set_properties_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 
	struct lwp *l = args->l;

	printf("pid %d.%d: mach_io_connect_set_properties\n",
	    l->l_proc->p_pid, l->l_lid);
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
mach_io_service_close(args)
	struct mach_trap_args *args;
{
	mach_io_service_close_request_t *req = args->smsg;
	mach_io_service_close_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 

	rep->rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_retval = 0;
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int
mach_io_connect_add_client(args)
	struct mach_trap_args *args;
{
	mach_io_connect_add_client_request_t *req = args->smsg;
	mach_io_connect_add_client_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 

	rep->rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_retval = 0;
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int
mach_io_connect_method_scalari_structi(args)
	struct mach_trap_args *args;
{
	mach_io_connect_method_scalari_structi_request_t *req = args->smsg;
	struct lwp *l = args->l;
	mach_port_t mn;
	struct mach_right *mr;
	struct mach_iokit_devclass *mid;

	mn = req->req_msgh.msgh_remote_port;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_iokit_error(args, MACH_IOKIT_EPERM);
	
	if (mr->mr_port->mp_datatype == MACH_MP_IOKIT_DEVCLASS) {
		mid = mr->mr_port->mp_data;
		if (mid->mid_connect_method_scalari_structi == NULL)
			printf("no connect_method_scalari_structi method "
			    "for darwin_iokit_class %s\n", mid->mid_name);
		else
			return (mid->mid_connect_method_scalari_structi)(args);
	}

	return mach_iokit_error(args, MACH_IOKIT_ENODEV);
}

int
mach_io_registry_entry_from_path(args)
	struct mach_trap_args *args;
{
	mach_io_registry_entry_from_path_request_t *req = args->smsg;
	mach_io_registry_entry_from_path_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 
	struct lwp *l = args->l;
	struct mach_port *mp;
	struct mach_right *mr;
	struct mach_iokit_devclass *mid; 
	int i, len;

#ifdef DEBUG_MACH
	printf("mach_io_registry_entry_from_path: path = %s\n", req->req_path);
#endif

	mp = mach_port_get();
	mp->mp_flags |= MACH_MP_INKERNEL;
	mr = mach_right_get(mp, l, MACH_PORT_TYPE_SEND, 0);

	if (sizeof(*req) - 512 + req->req_pathcount > args->ssize) {
#ifdef DEBUG_MACH
		printf("pathcount too big, truncating\n");
#endif
		req->req_pathcount = args->ssize - (sizeof(*req) - 512);
	}

	i = 0;
	while ((mid = mach_iokit_devclasses[i++]) != NULL) {
		len = strlen(mid->mid_name);
		/* XXX sanity check req_pathcount */
#ifdef DEBUG_MACH
		printf("trying \"%s\" vs \"%s\"\n", 
		    &req->req_path[req->req_pathcount - 1 - len], 
		    mid->mid_name);
#endif
		if (memcmp(&req->req_path[req->req_pathcount - 1 - len], 
		    mid->mid_name, len) == 0) {
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
	rep->rep_entry.name = (mach_port_t)mr->mr_name;
	rep->rep_entry.disposition = 0x11; /* XXX */
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int
mach_io_registry_entry_get_parent_iterator(args)
	struct mach_trap_args *args;
{
	mach_io_registry_entry_get_parent_iterator_request_t *req = args->smsg;
	mach_io_registry_entry_get_parent_iterator_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 
	struct lwp *l = args->l;
	struct mach_port *mp;
	struct mach_right *mr;

#ifdef DEBUG_MACH
	printf("mach_io_registry_entry_get_parent_iterator: plane = %s\n", 
	    req->req_plane);
#endif

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

void 
mach_iokit_cleanup_notify(mr)
	struct mach_right *mr;
{
	int i;
	struct mach_iokit_devclass *mid; 
	
	i = 0;
	while ((mid = mach_iokit_devclasses[i++]) != NULL) 
		if (mid->mid_notify == mr)
			mid->mid_notify = NULL;

	return;
}
