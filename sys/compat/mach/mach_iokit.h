/*	$NetBSD: mach_iokit.h,v 1.5 2003/02/09 22:13:46 manu Exp $ */

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
	int req_in[16];
	mach_msg_type_number_t req_outcount;
} mach_io_connect_method_scalari_scalaro_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_ndr_record_t rep_ndr;
	mach_kern_return_t rep_retval;
	mach_msg_type_number_t rep_outcount;
	int rep_out[4096];
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
	mach_msg_type_number_t req_planeoffset;
	mach_msg_type_number_t req_planecount;
	char req_plane[128];
	mach_msg_type_number_t req_property_nameoffser;
	mach_msg_type_number_t req_property_namecount;
	char req_propery_name[128];
	int req_options;
} mach_io_registry_entry_get_property_request_t;

typedef struct {
	mach_msg_header_t rep_msgh;
	mach_msg_body_t rep_body;
	mach_msg_port_descriptor_t rep_properties;
	mach_ndr_record_t rep_ndr;
	mach_msg_type_number_t rep_properties_count;
	mach_msg_trailer_t rep_trailer;
} mach_io_registry_entry_get_property_reply_t;

/* io_registry_entry_create_iterator */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_ndr_record_t req_ndr;
	mach_msg_type_number_t req_planeoffset;
	mach_msg_type_number_t req_planecount;
	char req_plane[128];
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
	char req_classname[128];
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
	char req_typeofinterest[128];
	mach_msg_type_number_t req_refcount;
	mach_natural_t req_ref[8];
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
	char req_plane[128];
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
	char req_plane[128];
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
	char req_plane[128];
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

int mach_io_service_get_matching_services(struct mach_trap_args *);
int mach_io_iterator_next(struct mach_trap_args *);
int mach_io_service_open(struct mach_trap_args *);
int mach_io_connect_method_scalari_scalaro(struct mach_trap_args *);
int mach_io_connect_get_service(struct mach_trap_args *);
int mach_io_registry_entry_get_property(struct mach_trap_args *);
int mach_io_registry_entry_create_iterator(struct mach_trap_args *);
int mach_io_object_conforms_to(struct mach_trap_args *);
int mach_io_service_add_interest_notification(struct mach_trap_args *);
int mach_io_connect_set_notification_port(struct mach_trap_args *);
int mach_io_registry_get_root_entry(struct mach_trap_args *);
int mach_io_registry_entry_get_child_iterator(struct mach_trap_args *);
int mach_io_registry_entry_get_name_in_plane(struct mach_trap_args *);
int mach_io_object_get_class(struct mach_trap_args *);
int mach_io_registry_entry_get_location_in_plane(struct mach_trap_args *);

struct mach_device_iterator {
	struct device *mdi_parent;
	struct device *mdi_current;
};

#endif /* _MACH_IOKIT_H_ */
