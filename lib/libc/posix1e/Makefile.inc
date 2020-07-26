# $FreeBSD: head/lib/libc/posix1e/Makefile.inc 338745 2018-09-18 01:40:37Z brd $

.PATH: ${.CURDIR}/posix1e ${NETBSDSRCDIR}/sys/kern

CPPFLAGS+=-D_ACL_PRIVATE

SRCS+=	acl_branding.c			\
	acl_calc_mask.c			\
	acl_copy.c			\
	acl_delete.c			\
	acl_delete_entry.c		\
	acl_entry.c			\
	acl_flag.c			\
	acl_free.c			\
	acl_from_text.c			\
	acl_from_text_nfs4.c            \
	acl_get.c			\
	acl_id_to_name.c		\
	acl_init.c			\
	acl_perm.c			\
	acl_set.c			\
	acl_strip.c			\
	acl_support.c			\
	acl_support_nfs4.c              \
	acl_to_text.c			\
	acl_to_text_nfs4.c              \
	acl_valid.c			\
	subr_acl_nfs4.c

MAN+=	acl.3				\
	acl_add_flag_np.3		\
	acl_add_perm.3			\
	acl_calc_mask.3			\
	acl_clear_flags_np.3		\
	acl_clear_perms.3		\
	acl_copy_entry.3		\
	acl_create_entry.3		\
	acl_delete.3			\
	acl_delete_entry.3		\
	acl_delete_flag_np.3		\
	acl_delete_perm.3		\
	acl_dup.3			\
	acl_free.3			\
	acl_from_text.3			\
	acl_get.3			\
	acl_get_brand_np.3		\
	acl_get_entry.3			\
	acl_get_entry_type_np.3		\
	acl_get_flagset_np.3		\
	acl_get_flag_np.3		\
	acl_get_permset.3		\
	acl_get_perm_np.3		\
	acl_get_qualifier.3		\
	acl_get_tag_type.3		\
	acl_init.3			\
	acl_is_trivial_np.3		\
	acl_set.3			\
	acl_set_entry_type_np.3		\
	acl_set_flagset_np.3		\
	acl_set_permset.3		\
	acl_set_qualifier.3		\
	acl_set_tag_type.3		\
	acl_strip_np.3			\
	acl_to_text.3			\
	acl_valid.3			\
	posix1e.3

MLINKS+=acl_create_entry.3 acl_create_entry_np.3 \
	acl_delete.3 acl_delete_def_file.3	\
	acl_delete.3 acl_delete_file_np.3	\
	acl_delete.3 acl_delete_fd_np.3		\
	acl_delete_entry.3 acl_delete_entry_np.3 \
	acl_get.3 acl_get_file.3		\
	acl_get.3 acl_get_fd.3			\
	acl_get.3 acl_get_fd_np.3		\
	acl_get.3 acl_get_link_np.3		\
	acl_set.3 acl_set_file.3		\
	acl_set.3 acl_set_fd.3			\
	acl_set.3 acl_set_fd_np.3		\
	acl_set.3 acl_set_link_np.3		\
	acl_to_text.3 acl_to_text_np.3		\
	acl_valid.3 acl_valid_file_np.3		\
	acl_valid.3 acl_valid_fd_np.3		\


#	extattr.3 extattr_namespace_to_string.3	\
#	extattr.3 extattr_string_to_namespace.3
