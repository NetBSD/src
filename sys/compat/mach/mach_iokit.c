/*	$NetBSD: mach_iokit.c,v 1.33.6.1 2006/04/22 11:38:16 simonb Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mach_iokit.c,v 1.33.6.1 2006/04/22 11:38:16 simonb Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/signal.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/ktrace.h>
#include <sys/device.h>
#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_port.h>
#include <compat/mach/mach_errno.h>
#include <compat/mach/mach_iokit.h>
#include <compat/mach/mach_services.h>

#ifdef COMPAT_DARWIN
#include <compat/darwin/darwin_iokit.h>
#endif

struct mach_iokit_devclass mach_ioroot_devclass = {
	"(unknwon)",
	{ NULL },
	"<dict ID=\"0\"></dict>",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"Root",
	NULL,
};

struct mach_iokit_devclass *mach_iokit_devclasses[] = {
	&mach_ioroot_devclass,
#ifdef COMPAT_DARWIN
	DARWIN_IOKIT_DEVCLASSES
#endif
	NULL,
};


static int mach_fill_child_iterator(struct mach_device_iterator *, int, int,
    struct mach_iokit_devclass *);
static int mach_fill_parent_iterator(struct mach_device_iterator *, int, int,
    struct mach_iokit_devclass *);

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
	struct mach_device_iterator *mdi;
	size_t size;
	int end_offset;
	int i;

	/* Sanity check req_size */
	end_offset = req->req_size;
	if (MACH_REQMSG_OVERFLOW(args, req->req_string[end_offset]))
		return mach_msg_error(args, EINVAL);

	mp = mach_port_get();
	mp->mp_flags |= MACH_MP_INKERNEL;
	mr = mach_right_get(mp, l, MACH_PORT_TYPE_SEND, 0);

	mp->mp_data = NULL;
	i = 0;
	while ((mid = mach_iokit_devclasses[i++]) != NULL) {
		if (memcmp(req->req_string, mid->mid_string,
		    req->req_size) == 0) {
			mp->mp_datatype = MACH_MP_DEVICE_ITERATOR;

			size = sizeof(*mdi)
			    + sizeof(struct mach_device_iterator *);
			mdi = malloc(size, M_EMULDATA, M_WAITOK);
			mdi->mdi_devices[0] = mid;
			mdi->mdi_devices[1] = NULL;
			mdi->mdi_current = 0;

			mp->mp_data = mdi;
			break;
		}
	}

	if (mp->mp_data == NULL)
		return mach_iokit_error(args, MACH_IOKIT_ENOENT);

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	mach_add_port_desc(rep, mr->mr_name);
	mach_set_trailer(rep, *msglen);

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
	struct mach_device_iterator *mdi;
	mach_port_t mn;

	mn = req->req_msgh.msgh_remote_port;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_iokit_error(args, MACH_IOKIT_EPERM);

	if (mr->mr_port->mp_datatype != MACH_MP_DEVICE_ITERATOR)
		return mach_iokit_error(args, MACH_IOKIT_EINVAL);

	mdi = mr->mr_port->mp_data;

	/* Is there something coming next? */
	if (mdi->mdi_devices[mdi->mdi_current] == NULL)
		return mach_iokit_error(args, MACH_IOKIT_EINVAL);

	mp = mach_port_get();
	mp->mp_flags |= MACH_MP_INKERNEL;
	mp->mp_datatype = MACH_MP_IOKIT_DEVCLASS;
	mp->mp_data = mdi->mdi_devices[mdi->mdi_current++];

	mr = mach_right_get(mp, l, MACH_PORT_TYPE_SEND, 0);

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	mach_add_port_desc(rep, mr->mr_name);
	mach_set_trailer(rep, *msglen);

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

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	mach_add_port_desc(rep, mr->mr_name);
	mach_set_trailer(rep, *msglen);

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
	int end_offset, outcount;

	/*
	 * Sanity check req_incount
	 * the +1 gives us the last field of the message, req_outcount
	 */
	end_offset = req->req_incount +
		     (sizeof(req->req_outcount) / sizeof(req->req_in[0]));
	if (MACH_REQMSG_OVERFLOW(args, req->req_in[end_offset]))
		return mach_msg_error(args, EINVAL);

	/* Sanity check req->req_outcount */
	outcount = req->req_in[req->req_incount];
	if (outcount > 16)
		return mach_msg_error(args, EINVAL);

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

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	mach_add_port_desc(rep, mr->mr_name);
	mach_set_trailer(rep, *msglen);

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
	mach_port_t mn;
	struct mach_right *mr;
	struct mach_iokit_devclass *mid;
	struct mach_device_iterator *mdi;
	struct mach_iokit_devclass **midp;
	int maxdev, index;
	size_t size;
	int end_offset;

	/*
	 * req_planeoffset is not used.
	 * Sanity check req_planecount
	 */
	end_offset = req->req_planecount +
		     (sizeof(req->req_options) / sizeof(req->req_plane[0]));
	if (MACH_REQMSG_OVERFLOW(args, req->req_plane[end_offset]))
		return mach_msg_error(args, EINVAL);

	mn = req->req_msgh.msgh_remote_port;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_iokit_error(args, MACH_IOKIT_EPERM);
	if (mr->mr_port->mp_datatype != MACH_MP_IOKIT_DEVCLASS)
		return mach_iokit_error(args, MACH_IOKIT_EINVAL);
	mid = (struct mach_iokit_devclass *)mr->mr_port->mp_data;

	mp = mach_port_get();
	mp->mp_flags |= (MACH_MP_INKERNEL | MACH_MP_DATA_ALLOCATED);
	mp->mp_datatype = MACH_MP_DEVICE_ITERATOR;

	maxdev = sizeof(mach_iokit_devclasses);
	size = sizeof(*mdi) + (maxdev * sizeof(struct mach_iokit_devclass *));
	mdi = malloc(size, M_EMULDATA, M_WAITOK);
	mp->mp_data = mdi;

	if (req->req_options & MACH_IOKIT_PARENT_ITERATOR)
		index = mach_fill_parent_iterator(mdi, maxdev, 0, mid);
	else
		index = mach_fill_child_iterator(mdi, maxdev, 0, mid);

	/* XXX This is untested */
	if (req->req_options & MACH_IOKIT_RECURSIVE_ITERATOR) {
		for (midp = mdi->mdi_devices; *midp != NULL; midp++) {
			if (req->req_options & MACH_IOKIT_PARENT_ITERATOR)
				index = mach_fill_parent_iterator(mdi,
				    maxdev, index, *midp);
			else
				index = mach_fill_child_iterator(mdi,
				    maxdev, index, *midp);
		}
	}

	mdi->mdi_current = 0;

	mr = mach_right_get(mp, l, MACH_PORT_TYPE_SEND, 0);

#ifdef DEBUG_MACH
	printf("io_registry_entry_create_iterator\n");
#endif

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	mach_add_port_desc(rep, mr->mr_name);
	mach_set_trailer(rep, *msglen);

	return 0;
}

int
mach_io_object_conforms_to(args)
	struct mach_trap_args *args;
{
	mach_io_object_conforms_to_request_t *req = args->smsg;
	mach_io_object_conforms_to_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	int end_offset;

	/*
	 * req_classnameoffset is not used.
	 * Sanity check req_classnamecount.
	 */
	end_offset = req->req_classnamecount;
	if (MACH_REQMSG_OVERFLOW(args, req->req_classname[end_offset]))
		return mach_msg_error(args, EINVAL);

#ifdef DEBUG_DARWIN
	uprintf("Unimplemented mach_io_object_conforms_to\n");
#endif

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_conforms = 1; /* XXX */

	mach_set_trailer(rep, *msglen);

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
	int end_offset, refcount_offset;
	int item_size, refitem_size, refcount;

	/*
	 * req_typeofinterestoffset is not used.
	 * Sanity checks: first check refcount is not
	 * outside the message. NB: it is word aligned
	 */
	refcount_offset = (req->req_typeofinterestcount & ~0x3UL) + 4;
	if (MACH_REQMSG_OVERFLOW(args,
	    req->req_typeofinterest[refcount_offset]))
		return mach_msg_error(args, EINVAL);
	refcount = req->req_typeofinterest[refcount_offset];

	/*
	 * Sanity check typeofinterestcount and refcount
	 */
	item_size = sizeof(req->req_typeofinterest[0]);
	refitem_size = sizeof(req->req_ref[0]);
	end_offset = req->req_typeofinterestcount +
		     (sizeof(refcount) / item_size) +
		     (refcount * refitem_size / item_size);
	if (MACH_REQMSG_OVERFLOW(args, req->req_typeofinterest[end_offset]))
		return mach_msg_error(args, EINVAL);

	mp = mach_port_get();
	mp->mp_flags |= MACH_MP_INKERNEL;
	mr = mach_right_get(mp, l, MACH_PORT_TYPE_SEND, 0);

#ifdef DEBUG_DARWIN
	uprintf("Unimplemented mach_io_service_add_interest_notification\n");
#endif
	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	mach_add_port_desc(rep, mr->mr_name);
	mach_set_trailer(rep, *msglen);

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

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_retval = 0;

	mach_set_trailer(rep, *msglen);

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
	mp->mp_datatype = MACH_MP_IOKIT_DEVCLASS;
	mp->mp_data = &mach_ioroot_devclass;

	mr = mach_right_get(mp, l, MACH_PORT_TYPE_SEND, 0);

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	mach_add_port_desc(rep, mr->mr_name);
	mach_set_trailer(rep, *msglen);

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
	struct mach_iokit_devclass *mid;
	struct mach_device_iterator *mdi;
	int maxdev;
	size_t size;
	int end_offset;

	/*
	 * req_planeoffset is not used.
	 * Sanity check req_planecount.
	 */
	end_offset = req->req_planecount;
	if (MACH_REQMSG_OVERFLOW(args, req->req_plane[end_offset]))
		return mach_msg_error(args, EINVAL);

	mn = req->req_msgh.msgh_remote_port;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_iokit_error(args, MACH_IOKIT_EPERM);
	if (mr->mr_port->mp_datatype != MACH_MP_IOKIT_DEVCLASS)
		return mach_iokit_error(args, MACH_IOKIT_EINVAL);
	mid = (struct mach_iokit_devclass *)mr->mr_port->mp_data;

	mp = mach_port_get();
	mp->mp_flags |= (MACH_MP_INKERNEL | MACH_MP_DATA_ALLOCATED);
	mp->mp_datatype = MACH_MP_DEVICE_ITERATOR;

	maxdev = sizeof(mach_iokit_devclasses);
	size = sizeof(*mdi) + (maxdev * sizeof(struct mach_iokit_devclass *));
	mdi = malloc(size, M_EMULDATA, M_WAITOK);
	mp->mp_data = mdi;

	(void)mach_fill_child_iterator(mdi, maxdev, 0, mid);
	mdi->mdi_current = 0;

	mr = mach_right_get(mp, l, MACH_PORT_TYPE_SEND, 0);

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	mach_add_port_desc(rep, mr->mr_name);
	mach_set_trailer(rep, *msglen);

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
	struct mach_iokit_devclass *mid;
	int end_offset;

	/*
	 * req_planeoffset is not used.
	 * Sanity check req_planecount.
	 */
	end_offset = req->req_planecount;
	if (MACH_REQMSG_OVERFLOW(args, req->req_plane[end_offset]))
		return mach_msg_error(args, EINVAL);

	mn = req->req_msgh.msgh_remote_port;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_iokit_error(args, MACH_IOKIT_EPERM);
	if (mr->mr_port->mp_datatype != MACH_MP_IOKIT_DEVCLASS)
		return mach_iokit_error(args, MACH_IOKIT_EINVAL);
	mid = mr->mr_port->mp_data;

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_namecount = strlen(mid->mid_name);
	if (rep->rep_namecount >= 128)
		rep->rep_namecount = 128;
	memcpy(&rep->rep_name, mid->mid_name, rep->rep_namecount);

	mach_set_trailer(rep, *msglen);

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

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	/* XXX Just return a dummy name for now */
	rep->rep_namecount = strlen(classname);
	if (rep->rep_namecount >= 128)
		rep->rep_namecount = 128;
	memcpy(&rep->rep_name, classname, rep->rep_namecount);

	mach_set_trailer(rep, *msglen);

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
	int end_offset;

	/*
	 * req_nameoffset is not used.
	 * Sanity check req_namecount.
	 */
	end_offset = req->req_namecount;
	if (MACH_REQMSG_OVERFLOW(args, req->req_plane[end_offset]))
		return mach_msg_error(args, EINVAL);

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	/* XXX Just return a dummy name for now */
	rep->rep_locationcount = sizeof(location);
	memcpy(&rep->rep_location, location, sizeof(location));

	mach_set_trailer(rep, *msglen);

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
	void *uaddr;
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

	if ((error = mach_ool_copyout(l,
	    mid->mid_properties, &uaddr, size, MACH_OOL_TRACE)) != 0) {
#ifdef DEBUG_MACH
		printf("pid %d.%d: copyout iokit properties failed\n",
		    l->l_proc->p_pid, l->l_lid);
#endif
	}

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	mach_add_ool_desc(rep, uaddr, size);

	rep->rep_count = size;

	mach_set_trailer(rep, *msglen);

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
	void *uaddr;
	size_t size;
	mach_port_t mn;
	struct mach_right *mr;
	struct mach_iokit_devclass *mid;
	struct mach_iokit_property *mip;
	int end_offset;

	/*
	 * req_property_nameoffset is not used.
	 * Sanity check req_property_namecount.
	 */
	end_offset = req->req_property_namecount;
	if (MACH_REQMSG_OVERFLOW(args, req->req_property_name[end_offset]))
		return mach_iokit_error(args, MACH_IOKIT_EINVAL);

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
	for (mip = mid->mid_properties_array; mip->mip_name; mip++)
		if (memcmp(mip->mip_name, req->req_property_name,
		    req->req_property_namecount) == 0)
			break;

	if (mip->mip_value == NULL)
		return mach_iokit_error(args, MACH_IOKIT_ENOENT);
	size = strlen(mip->mip_value) + 1; /* Include trailing zero */

	/* And copyout its associated value */
	if ((error = mach_ool_copyout(l,
	    mip->mip_value, &uaddr, size, MACH_OOL_TRACE)) != 0) {
#ifdef DEBUG_MACH
		printf("pid %d.%d: copyout iokit property failed\n",
		    l->l_proc->p_pid, l->l_lid);
#endif
	}

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	mach_add_ool_desc(rep, uaddr, size);

	rep->rep_properties_count = size;

	mach_set_trailer(rep, *msglen);

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
	int end_offset;

	/*
	 * req_offset is not used.
	 * Sanity check req_count.
	 */
	end_offset = req->req_count;
	if (MACH_REQMSG_OVERFLOW(args, req->req_plane[end_offset]))
		return mach_iokit_error(args, MACH_IOKIT_EINVAL);

	/* XXX Just return a dummy name for now */
	len = req->req_count + strlen(location) - 1;

	/* Sanity check for len */
	if (len > 512)
		return mach_iokit_error(args, MACH_IOKIT_EINVAL);
	plen = (len & ~0x3UL) + 4;	/* Round to an int */

	*msglen = sizeof(*rep) + (plen - 512);
	mach_set_header(rep, req, *msglen);

	rep->rep_retval = 0;
	rep->rep_count = len;

	cp = &rep->rep_path[0];
	memcpy(cp, &req->req_plane, req->req_count);
	cp += (req->req_count - 1);	/* overwrite trailing \0 */
	memcpy(cp, location, strlen(location));
	cp += strlen(location);
	*cp = '\0';

	mach_set_trailer(rep, *msglen);

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

	if (mr->mr_port->mp_datatype != MACH_MP_DEVICE_ITERATOR)
		return mach_iokit_error(args, MACH_IOKIT_EINVAL);

	mdi = mr->mr_port->mp_data;
	mdi->mdi_current = 0;

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_retval = 0;

	mach_set_trailer(rep, *msglen);

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
	int end_offset;

	/* Sanity check req_incount */
	end_offset = req->req_incount +
		     (sizeof(req->req_outcount) / sizeof(req->req_in[0]));
	if (MACH_REQMSG_OVERFLOW(args, req->req_in[end_offset]))
		return mach_msg_error(args, EINVAL);

	/* Sanity check req_outcount */
	if (req->req_outcount > 4096)
		return mach_msg_error(args, EINVAL);

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
	int end_offset;
	int outcount;

	/* Sanity check req_incount */
	end_offset = req->req_incount +
		     (sizeof(req->req_outcount) / sizeof(req->req_in[0]));
	if (MACH_REQMSG_OVERFLOW(args, req->req_in[end_offset]))
		return mach_msg_error(args, EINVAL);

	/* Sanity check outcount. It is word aligned */
	outcount = req->req_in[(req->req_incount & ~0x3UL) + 4];
	if (outcount > 4096)
		return mach_msg_error(args, EINVAL);

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

#ifdef DEBUG_MACH
	uprintf("Unimplemented mach_io_connect_set_properties\n");
#endif

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	mach_set_trailer(rep, *msglen);

	return 0;
}

int
mach_io_service_close(args)
	struct mach_trap_args *args;
{
	mach_io_service_close_request_t *req = args->smsg;
	mach_io_service_close_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;

#ifdef DEBUG_MACH
	uprintf("Unimplemented mach_io_service_close\n");
#endif

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_retval = 0;

	mach_set_trailer(rep, *msglen);

	return 0;
}

int
mach_io_connect_add_client(args)
	struct mach_trap_args *args;
{
	mach_io_connect_add_client_request_t *req = args->smsg;
	mach_io_connect_add_client_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;

#ifdef DEBUG_MACH
	uprintf("Unimplemented mach_io_connect_add_client\n");
#endif

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_retval = 0;

	mach_set_trailer(rep, *msglen);

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
	int end_offset;
	int scalar_size, struct_size, instructcount;

	/* Sanity check req_incount and get instructcount */
	if (MACH_REQMSG_OVERFLOW(args, req->req_in[req->req_incount]))
		return mach_msg_error(args, EINVAL);
	instructcount = req->req_in[req->req_incount];

	/* Sanity check instructcount */
	scalar_size = sizeof(req->req_in[0]);
	struct_size = sizeof(req->req_instruct[0]);
	end_offset = req->req_incount +
		     (sizeof(instructcount) / scalar_size) +
		     (instructcount * struct_size / scalar_size);
	if (MACH_REQMSG_OVERFLOW(args, req->req_in[end_offset]))
		return mach_msg_error(args, EINVAL);

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
	int end_offset;

	/*
	 * req_pathoffset is not used.
	 * Sanity check req_pathcount.
	 */
	end_offset = req->req_pathcount;
	if (MACH_REQMSG_OVERFLOW(args, req->req_path[end_offset]))
		return mach_msg_error(args, EINVAL);

#ifdef DEBUG_MACH
	printf("mach_io_registry_entry_from_path: path = %s\n", req->req_path);
#endif

	mp = mach_port_get();
	mp->mp_flags |= MACH_MP_INKERNEL;
	mr = mach_right_get(mp, l, MACH_PORT_TYPE_SEND, 0);

	i = 0;
	while ((mid = mach_iokit_devclasses[i++]) != NULL) {
		len = strlen(mid->mid_name);
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

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	mach_add_port_desc(rep, mr->mr_name);
	mach_set_trailer(rep, *msglen);

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
	struct mach_iokit_devclass *mid;
	struct mach_device_iterator *mdi;
	mach_port_t mn;
	int maxdev;
	size_t size;
	int end_offset;

	/*
	 * req_offset is unused
	 * Sanity check req_count
	 */
	end_offset = req->req_count;
	if (MACH_REQMSG_OVERFLOW(args, req->req_plane[end_offset]))
		return mach_msg_error(args, EINVAL);

#ifdef DEBUG_MACH
	printf("mach_io_registry_entry_get_parent_iterator: plane = %s\n",
	    req->req_plane);
#endif

	mn = req->req_msgh.msgh_remote_port;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_iokit_error(args, MACH_IOKIT_EPERM);

	if (mr->mr_port->mp_datatype != MACH_MP_IOKIT_DEVCLASS)
		return mach_iokit_error(args, MACH_IOKIT_EINVAL);
	mid = mr->mr_port->mp_data;

	mp = mach_port_get();
	mp->mp_flags |= (MACH_MP_INKERNEL | MACH_MP_DATA_ALLOCATED);
	mp->mp_datatype = MACH_MP_DEVICE_ITERATOR;

	maxdev = sizeof(mach_iokit_devclasses);
	size = sizeof(*mdi) + (maxdev * sizeof(struct mach_iokit_devclass *));
	mdi = malloc(size, M_EMULDATA, M_WAITOK);
	mp->mp_data = mdi;

	(void)mach_fill_parent_iterator(mdi, maxdev, 0, mid);
	mdi->mdi_current = 0;

	mr = mach_right_get(mp, l, MACH_PORT_TYPE_SEND, 0);

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	mach_add_port_desc(rep, mr->mr_name);
	mach_set_trailer(rep, *msglen);

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

static int
mach_fill_child_iterator(mdi, size, index, mid)
	struct mach_device_iterator *mdi;
	int size;
	int index;
	struct mach_iokit_devclass *mid;
{
	struct mach_iokit_devclass **midp;
	struct mach_iokit_devclass **midq;

	for (midp = mach_iokit_devclasses; *midp != NULL; midp++) {
		for (midq = (*midp)->mid_parent; *midq != NULL; midq++) {
			if (*midq == mid) {
				mdi->mdi_devices[index++] = *midp;
				break;
			}
		}
#ifdef DIAGNOSTIC
		if (index >= size) {
			printf("mach_device_iterator overflow\n");
			break;
		}
#endif
	}
	mdi->mdi_devices[index] = NULL;

	return index;
}

static int
mach_fill_parent_iterator(mdi, size, index, mid)
	struct mach_device_iterator *mdi;
	int size;
	int index;
	struct mach_iokit_devclass *mid;
{
	struct mach_iokit_devclass **midp;

	for (midp = mid->mid_parent; *midp != NULL; midp++) {
		mdi->mdi_devices[index++] = *midp;
#ifdef DIAGNOSTIC
		if (index >= size) {
			printf("mach_device_iterator overflow\n");
			break;
		}
#endif
	}
	mdi->mdi_devices[index] = NULL;

	return index;
}
