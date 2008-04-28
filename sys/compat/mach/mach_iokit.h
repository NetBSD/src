/*	$NetBSD: mach_iokit.h,v 1.26 2008/04/28 20:23:44 martin Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
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

#ifndef	_MACH_IOKIT_H_
#define	_MACH_IOKIT_H_

typedef struct mach_io_object *mach_io_object_t;

/* mach_io_service_get_matching_services */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_io_object_t req_io_master;
	mach_msg_size_t req_size;
	char req_string[0];
} mach_io_service_get_matching_services_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_msg_body_t rep_body;
	mach_msg_port_descriptor_t rep_match;
	mach_msg_trailer_t rep_trailer;
} mach_io_service_get_matching_services_reply_t;

/* mach_io_iterator_next */

typedef struct {
	mach_msg_header_t req_msgh;
} mach_io_iterator_next_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_msg_body_t rep_body;
	mach_msg_port_descriptor_t rep_object;
	mach_msg_trailer_t rep_trailer;
} mach_io_iterator_next_reply_t;

/* mach_io_service_open */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_msg_body_t req_body;
	mach_msg_port_descriptor_t req_owningtask;
	mach_ndr_record_t req_ndr;
	int mach_connect_type;
} mach_io_service_open_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_msg_body_t rep_body;
	mach_msg_port_descriptor_t rep_connect;
	mach_msg_trailer_t rep_trailer;
} mach_io_service_open_reply_t;

/* mach_io_connect_method_scalari_scalaro */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	int req_selector;
	mach_msg_type_number_t req_incount;
	int req_in[0];
	mach_msg_type_number_t req_outcount;
} mach_io_connect_method_scalari_scalaro_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_type_number_t rep_outcount;
	int rep_out[16];
	mach_msg_trailer_t rep_trailer;
} mach_io_connect_method_scalari_scalaro_reply_t;

/* io_connect_get_service */

typedef struct {
	mach_msg_header_t req_msgh;
} mach_io_connect_get_service_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_msg_body_t rep_body;
	mach_msg_port_descriptor_t rep_service;
	mach_msg_trailer_t rep_trailer;
} mach_io_connect_get_service_reply_t;

/* io_registry_entry_get_property */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_msg_type_number_t req_property_nameoffset;
	mach_msg_type_number_t req_property_namecount;
	char req_property_name[0];
} mach_io_registry_entry_get_property_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_msg_body_t rep_body;
	mach_msg_ool_descriptor_t rep_properties;
	mach_ndr_record_t rep_ndr;
	mach_msg_type_number_t rep_properties_count;
	mach_msg_trailer_t rep_trailer;
} mach_io_registry_entry_get_property_reply_t;

/* io_registry_entry_create_iterator */

#define MACH_IOKIT_RECURSIVE_ITERATOR	1
#define MACH_IOKIT_PARENT_ITERATOR	2
typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_msg_type_number_t req_planeoffset;
	mach_msg_type_number_t req_planecount;
	char req_plane[0];
	int req_options;
} mach_io_registry_entry_create_iterator_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_msg_body_t rep_body;
	mach_msg_port_descriptor_t rep_iterator;
	mach_msg_trailer_t rep_trailer;
} mach_io_registry_entry_create_iterator_reply_t;

/* io_object_conforms_to */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_msg_type_number_t req_classnameoffset;
	mach_msg_type_number_t req_classnamecount;
	char req_classname[0];
} mach_io_object_conforms_to_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_boolean_t rep_conforms;
	mach_msg_trailer_t rep_trailer;
} mach_io_object_conforms_to_reply_t;

/* io_service_add_interest_notification */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_msg_body_t req_body;
	mach_msg_port_descriptor_t req_wake_port;
	mach_ndr_record_t req_ndr;
	mach_msg_type_number_t req_typeofinterestoffset;
	mach_msg_type_number_t req_typeofinterestcount;
	char req_typeofinterest[0];
	mach_msg_type_number_t req_refcount;
	mach_natural_t req_ref[0];
} mach_io_service_add_interest_notification_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_msg_body_t rep_body;
	mach_msg_port_descriptor_t rep_notification;
	mach_msg_trailer_t rep_trailer;
} mach_io_service_add_interest_notification_reply_t;

/* io_connect_set_notification_port */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_msg_body_t req_body;
	mach_msg_port_descriptor_t req_port;
	mach_ndr_record_t req_ndr;
	int req_notification_type;
	int req_reference;
} mach_io_connect_set_notification_port_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_io_connect_set_notification_port_reply_t;

/* io_registry_get_root_entry */

typedef struct {
	mach_msg_header_t req_msgh;
} mach_io_registry_get_root_entry_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_msg_body_t rep_body;
	mach_msg_port_descriptor_t rep_root;
	mach_msg_trailer_t rep_trailer;
} mach_io_registry_get_root_entry_reply_t;

/* io_registry_entry_get_child_iterator */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_msg_type_number_t req_planeoffset;
	mach_msg_type_number_t req_planecount;
	char req_plane[0];
} mach_io_registry_entry_get_child_iterator_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_msg_body_t rep_body;
	mach_msg_port_descriptor_t rep_iterator;
	mach_msg_trailer_t rep_trailer;
} mach_io_registry_entry_get_child_iterator_reply_t;

/* io_registry_entry_get_name_in_plane */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_msg_type_number_t req_planeoffset;
	mach_msg_type_number_t req_planecount;
	char req_plane[0];
} mach_io_registry_entry_get_name_in_plane_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_type_number_t rep_nameoffset;
	mach_msg_type_number_t rep_namecount;
	char rep_name[128];
	mach_msg_trailer_t rep_trailer;
} mach_io_registry_entry_get_name_in_plane_reply_t;

/* io_object_get_class */

typedef struct {
	mach_msg_header_t req_msgh;
} mach_io_object_get_class_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_type_number_t rep_nameoffset;
	mach_msg_type_number_t rep_namecount;
	char rep_name[128];
	mach_msg_trailer_t rep_trailer;
} mach_io_object_get_class_reply_t;

/* io_registry_entry_get_location_in_plane */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_msg_type_number_t req_nameoffset;
	mach_msg_type_number_t req_namecount;
	char req_plane[0];
} mach_io_registry_entry_get_location_in_plane_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_type_number_t rep_locationoffset;
	mach_msg_type_number_t rep_locationcount;
	char rep_location[128];
	mach_msg_trailer_t rep_trailer;
} mach_io_registry_entry_get_location_in_plane_reply_t;

/* io_registry_entry_get_properties */

typedef struct {
	mach_msg_header_t req_msgh;
} mach_io_registry_entry_get_properties_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_msg_body_t rep_body;
	mach_msg_ool_descriptor_t rep_properties;
	mach_ndr_record_t rep_ndr;
	mach_msg_type_number_t rep_count;
	mach_msg_trailer_t rep_trailer;
} mach_io_registry_entry_get_properties_reply_t;

/* io_registry_entry_get_path */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_msg_type_number_t req_offset;
	mach_msg_type_number_t req_count;
	char req_plane[0];
} mach_io_registry_entry_get_path_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_type_number_t rep_offset;
	mach_msg_type_number_t rep_count;
	char rep_path[512];
	mach_msg_trailer_t rep_trailer;
} mach_io_registry_entry_get_path_reply_t;

/* io_connect_map_memory */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_msg_body_t req_body;
	mach_msg_port_descriptor_t req_task;
	mach_ndr_record_t req_ndr;
	int req_memtype;
	mach_vm_address_t req_addr;
	mach_vm_size_t req_len;
	int req_flags;
} mach_io_connect_map_memory_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_vm_address_t rep_addr;
	mach_vm_size_t rep_len;
	mach_msg_trailer_t rep_trailer;
} mach_io_connect_map_memory_reply_t;

/* io_iterator_reset */

typedef struct {
	mach_msg_header_t req_msgh;
#if 0	/* Is it optional? Darwin don't include them */
	mach_ndr_record_t req_ndr;
	int req_flags;
#endif
} mach_io_iterator_reset_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_io_iterator_reset_reply_t;

/* io_connect_set_properties */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_msg_body_t req_body;
	mach_msg_ool_descriptor_t req_properties;
	mach_ndr_record_t req_ndr;
	mach_msg_type_number_t req_count;
} mach_io_connect_set_properties_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_natural_t rep_result;
	mach_msg_trailer_t rep_trailer;
} mach_io_connect_set_properties_reply_t;

/* io_connect_method_scalari_structo */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	int req_selector;
	mach_msg_type_number_t req_incount;
	int req_in[0];
	mach_msg_type_number_t req_outcount;
} mach_io_connect_method_scalari_structo_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_type_number_t rep_outcount;
	char rep_out[4096];
	mach_msg_trailer_t rep_trailer;
} mach_io_connect_method_scalari_structo_reply_t;

/* io_connect_method_structi_structo */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	int req_selector;
	mach_msg_type_number_t req_incount;
	char req_in[0];
	mach_msg_type_number_t req_outcount;
} mach_io_connect_method_structi_structo_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_type_number_t rep_outcount;
	char rep_out[4096];
	mach_msg_trailer_t rep_trailer;
} mach_io_connect_method_structi_structo_reply_t;

/* io_service_close */

typedef struct {
	mach_msg_header_t req_msgh;
} mach_io_service_close_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_io_service_close_reply_t;

/* io_connect_add_client */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_msg_body_t req_body;
	mach_msg_port_descriptor_t req_connect;
} mach_io_connect_add_client_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_io_connect_add_client_reply_t;

/* io_connect_method_scalari_structi */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	int req_selector;
	mach_msg_type_number_t req_incount;
	int req_in[0];
	mach_msg_type_number_t req_instructcount;
	char req_instruct[0];
} mach_io_connect_method_scalari_structi_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_trailer_t rep_trailer;
} mach_io_connect_method_scalari_structi_reply_t;

/* io_registry_entry_from_path */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_msg_type_number_t req_pathoffset;
	mach_msg_type_number_t req_pathcount;
	char req_path[0];
} mach_io_registry_entry_from_path_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_msg_body_t rep_body;
	mach_msg_port_descriptor_t rep_entry;
	mach_msg_trailer_t rep_trailer;
} mach_io_registry_entry_from_path_reply_t;

/* io_registry_entry_get_parent_iterator */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_msg_type_number_t req_offset;
	mach_msg_type_number_t req_count;
	char req_plane[0];
} mach_io_registry_entry_get_parent_iterator_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_msg_body_t rep_body;
	mach_msg_port_descriptor_t rep_iterator;
	mach_msg_trailer_t rep_trailer;
} mach_io_registry_entry_get_parent_iterator_reply_t;


/* Kernel-private structures */

extern struct mach_iokit_devclass *mach_iokit_devclasses[];

struct mach_iokit_property {
	const char *mip_name;
	const char *mip_value;
};

struct mach_device_iterator {
	int mdi_current;
	struct mach_iokit_devclass *mdi_devices[1];
};

/* Make this dynamic if it ever gets useful */
#define MACH_IOKIT_MAX_PARENTS	8
struct mach_iokit_devclass {
	const char *mid_string;
	struct mach_iokit_devclass *mid_parent[MACH_IOKIT_MAX_PARENTS];
	const char *mid_properties;
	struct mach_iokit_property *mid_properties_array;
	int (*mid_connect_method_scalari_scalaro)(struct mach_trap_args *);
	int (*mid_connect_method_scalari_structo)(struct mach_trap_args *);
	int (*mid_connect_method_structi_structo)(struct mach_trap_args *);
	int (*mid_connect_method_scalari_structi)(struct mach_trap_args *);
	int (*mid_connect_map_memory)(struct mach_trap_args *);
	const char *mid_name;
	struct mach_right *mid_notify;
};

extern struct mach_iokit_devclass mach_ioroot_devclass;

void mach_iokit_cleanup_notify(struct mach_right *);

#endif /* _MACH_IOKIT_H_ */
