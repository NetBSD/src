#	$NetBSD: Makefile,v 1.21 2020/06/06 21:25:59 thorpej Exp $

.include <bsd.own.mk>

WARNS?=5
USE_SHLIBDIR=	yes

PROPLIBDIR=${NETBSDSRCDIR}/common/lib/libprop

.include "${PROPLIBDIR}/Makefile.inc"

CPPFLAGS+=-D_LIBPROP
CPPFLAGS+=-D_REENTRANT -I${NETBSDSRCDIR}/lib/libc/include

LINTFLAGS+=-w
LIB=	prop

MAN=	prop_array.3 prop_bool.3 prop_data.3 prop_dictionary.3 \
	prop_ingest.3 prop_number.3 prop_object.3 prop_string.3 proplib.3

MAN+=	prop_copyin_ioctl.9
MLINKS+= prop_copyin_ioctl.9 prop_array_copyin_ioctl.9
MLINKS+= prop_copyin_ioctl.9 prop_array_copyout_ioctl.9
MLINKS+= prop_copyin_ioctl.9 prop_dictionary_copyin_ioctl.9
MLINKS+= prop_copyin_ioctl.9 prop_dictionary_copyout_ioctl.9

MAN+=	prop_send_ioctl.3
MLINKS+= prop_send_ioctl.3 prop_array_send_ioctl.3
MLINKS+= prop_send_ioctl.3 prop_array_recv_ioctl.3
MLINKS+= prop_send_ioctl.3 prop_dictionary_send_ioctl.3
MLINKS+= prop_send_ioctl.3 prop_dictionary_recv_ioctl.3
MLINKS+= prop_send_ioctl.3 prop_dictionary_sendrecv_ioctl.3

MAN+=	prop_send_syscall.3
MLINKS+= prop_send_syscall.3 prop_array_send_syscall.3
MLINKS+= prop_send_syscall.3 prop_array_recv_syscall.3
MLINKS+= prop_send_syscall.3 prop_dictionary_send_syscall.3
MLINKS+= prop_send_syscall.3 prop_dictionary_recv_syscall.3

MAN+=	prop_dictionary_util.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_get_bool.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_get_schar.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_get_short.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_get_int.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_get_long.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_get_longlong.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_get_intptr.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_get_int8.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_get_int16.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_get_int32.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_get_int64.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_get_uchar.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_get_ushort.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_get_uint.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_get_ulong.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_get_ulonglong.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_get_uintptr.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_get_uint8.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_get_uint16.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_get_uint32.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_get_uint64.3

MLINKS+= prop_dictionary_util.3 prop_dictionary_set_bool.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_set_schar.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_set_short.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_set_int.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_set_long.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_set_longlong.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_set_intptr.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_set_int8.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_set_int16.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_set_int32.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_set_int64.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_set_uchar.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_set_ushort.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_set_uint.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_set_ulong.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_set_ulonglong.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_set_uintptr.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_set_uint8.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_set_uint16.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_set_uint32.3
MLINKS+= prop_dictionary_util.3 prop_dictionary_set_uint64.3

MLINKS+= prop_array.3 prop_array_add.3
MLINKS+= prop_array.3 prop_array_capacity.3
MLINKS+= prop_array.3 prop_array_copy.3
MLINKS+= prop_array.3 prop_array_copy_mutable.3
MLINKS+= prop_array.3 prop_array_count.3
MLINKS+= prop_array.3 prop_array_create.3
MLINKS+= prop_array.3 prop_array_create_with_capacity.3
MLINKS+= prop_array.3 prop_array_ensure_capacity.3
MLINKS+= prop_array.3 prop_array_equals.3
MLINKS+= prop_array.3 prop_array_externalize.3
MLINKS+= prop_array.3 prop_array_externalize_to_file.3
MLINKS+= prop_array.3 prop_array_externalize_to_pref.3
MLINKS+= prop_array.3 prop_array_get.3
MLINKS+= prop_array.3 prop_array_internalize.3
MLINKS+= prop_array.3 prop_array_internalize_from_file.3
MLINKS+= prop_array.3 prop_array_internalize_from_pref.3
MLINKS+= prop_array.3 prop_array_iterator.3
MLINKS+= prop_array.3 prop_array_make_immutable.3
MLINKS+= prop_array.3 prop_array_mutable.3
MLINKS+= prop_array.3 prop_array_remove.3
MLINKS+= prop_array.3 prop_array_set.3

MAN+=	prop_array_util.3
MLINKS+= prop_array_util.3 prop_array_get_bool.3

MLINKS+= prop_array_util.3 prop_array_add_bool.3
MLINKS+= prop_array_util.3 prop_array_add_schar.3
MLINKS+= prop_array_util.3 prop_array_add_short.3
MLINKS+= prop_array_util.3 prop_array_add_int.3
MLINKS+= prop_array_util.3 prop_array_add_long.3
MLINKS+= prop_array_util.3 prop_array_add_longlong.3
MLINKS+= prop_array_util.3 prop_array_add_intptr.3
MLINKS+= prop_array_util.3 prop_array_add_int8.3
MLINKS+= prop_array_util.3 prop_array_add_int16.3
MLINKS+= prop_array_util.3 prop_array_add_int32.3
MLINKS+= prop_array_util.3 prop_array_add_int64.3
MLINKS+= prop_array_util.3 prop_array_add_uchar.3
MLINKS+= prop_array_util.3 prop_array_add_ushort.3
MLINKS+= prop_array_util.3 prop_array_add_uint.3
MLINKS+= prop_array_util.3 prop_array_add_ulong.3
MLINKS+= prop_array_util.3 prop_array_add_ulonglong.3
MLINKS+= prop_array_util.3 prop_array_add_uintptr.3
MLINKS+= prop_array_util.3 prop_array_add_uint8.3
MLINKS+= prop_array_util.3 prop_array_add_uint16.3
MLINKS+= prop_array_util.3 prop_array_add_uint32.3
MLINKS+= prop_array_util.3 prop_array_add_uint64.3
MLINKS+= prop_array_util.3 prop_array_add_data.3
MLINKS+= prop_array_util.3 prop_array_add_data_nocopy.3
MLINKS+= prop_array_util.3 prop_array_add_string.3
MLINKS+= prop_array_util.3 prop_array_add_string_nocopy.3
MLINKS+= prop_array_util.3 prop_array_add_and_rel.3

MLINKS+= prop_array_util.3 prop_array_get_schar.3
MLINKS+= prop_array_util.3 prop_array_get_short.3
MLINKS+= prop_array_util.3 prop_array_get_int.3
MLINKS+= prop_array_util.3 prop_array_get_long.3
MLINKS+= prop_array_util.3 prop_array_get_longlong.3
MLINKS+= prop_array_util.3 prop_array_get_intptr.3
MLINKS+= prop_array_util.3 prop_array_get_int8.3
MLINKS+= prop_array_util.3 prop_array_get_int16.3
MLINKS+= prop_array_util.3 prop_array_get_int32.3
MLINKS+= prop_array_util.3 prop_array_get_int64.3
MLINKS+= prop_array_util.3 prop_array_get_uchar.3
MLINKS+= prop_array_util.3 prop_array_get_ushort.3
MLINKS+= prop_array_util.3 prop_array_get_uint.3
MLINKS+= prop_array_util.3 prop_array_get_ulong.3
MLINKS+= prop_array_util.3 prop_array_get_ulonglong.3
MLINKS+= prop_array_util.3 prop_array_get_uintptr.3
MLINKS+= prop_array_util.3 prop_array_get_uint8.3
MLINKS+= prop_array_util.3 prop_array_get_uint16.3
MLINKS+= prop_array_util.3 prop_array_get_uint32.3
MLINKS+= prop_array_util.3 prop_array_get_uint64.3
MLINKS+= prop_array_util.3 prop_array_get_string.3

MLINKS+= prop_array_util.3 prop_array_set_bool.3
MLINKS+= prop_array_util.3 prop_array_set_schar.3
MLINKS+= prop_array_util.3 prop_array_set_short.3
MLINKS+= prop_array_util.3 prop_array_set_int.3
MLINKS+= prop_array_util.3 prop_array_set_long.3
MLINKS+= prop_array_util.3 prop_array_set_longlong.3
MLINKS+= prop_array_util.3 prop_array_set_intptr.3
MLINKS+= prop_array_util.3 prop_array_set_int8.3
MLINKS+= prop_array_util.3 prop_array_set_int16.3
MLINKS+= prop_array_util.3 prop_array_set_int32.3
MLINKS+= prop_array_util.3 prop_array_set_int64.3
MLINKS+= prop_array_util.3 prop_array_set_uchar.3
MLINKS+= prop_array_util.3 prop_array_set_ushort.3
MLINKS+= prop_array_util.3 prop_array_set_uint.3
MLINKS+= prop_array_util.3 prop_array_set_ulong.3
MLINKS+= prop_array_util.3 prop_array_set_ulonglong.3
MLINKS+= prop_array_util.3 prop_array_set_uintptr.3
MLINKS+= prop_array_util.3 prop_array_set_uint8.3
MLINKS+= prop_array_util.3 prop_array_set_uint16.3
MLINKS+= prop_array_util.3 prop_array_set_uint32.3
MLINKS+= prop_array_util.3 prop_array_set_uint64.3
MLINKS+= prop_array_util.3 prop_array_set_data.3
MLINKS+= prop_array_util.3 prop_array_set_data_nocopy.3
MLINKS+= prop_array_util.3 prop_array_set_string.3
MLINKS+= prop_array_util.3 prop_array_set_string_nocopy.3
MLINKS+= prop_array_util.3 prop_array_set_and_rel.3

MLINKS+= prop_bool.3 prop_bool_copy.3
MLINKS+= prop_bool.3 prop_bool_create.3
MLINKS+= prop_bool.3 prop_bool_true.3
MLINKS+= prop_bool.3 prop_bool_value.3

MLINKS+= prop_data.3 prop_data_copy.3
MLINKS+= prop_data.3 prop_data_copy_value.3
MLINKS+= prop_data.3 prop_data_create_copy.3
MLINKS+= prop_data.3 prop_data_create_nocopy.3
MLINKS+= prop_data.3 prop_data_equals.3
MLINKS+= prop_data.3 prop_data_equals_data.3
MLINKS+= prop_data.3 prop_data_size.3
MLINKS+= prop_data.3 prop_data_value.3

MLINKS+= prop_dictionary.3 prop_dictionary_all_keys.3
MLINKS+= prop_dictionary.3 prop_dictionary_capacity.3
MLINKS+= prop_dictionary.3 prop_dictionary_copy.3
MLINKS+= prop_dictionary.3 prop_dictionary_copy_mutable.3
MLINKS+= prop_dictionary.3 prop_dictionary_count.3
MLINKS+= prop_dictionary.3 prop_dictionary_create.3
MLINKS+= prop_dictionary.3 prop_dictionary_create_with_capacity.3
MLINKS+= prop_dictionary.3 prop_dictionary_ensure_capacity.3
MLINKS+= prop_dictionary.3 prop_dictionary_equals.3
MLINKS+= prop_dictionary.3 prop_dictionary_externalize.3
MLINKS+= prop_dictionary.3 prop_dictionary_externalize_to_file.3
MLINKS+= prop_dictionary.3 prop_dictionary_externalize_to_pref.3
MLINKS+= prop_dictionary.3 prop_dictionary_get.3
MLINKS+= prop_dictionary.3 prop_dictionary_get_keysym.3
MLINKS+= prop_dictionary.3 prop_dictionary_internalize.3
MLINKS+= prop_dictionary.3 prop_dictionary_internalize_from_file.3
MLINKS+= prop_dictionary.3 prop_dictionary_internalize_from_pref.3
MLINKS+= prop_dictionary.3 prop_dictionary_iterator.3
MLINKS+= prop_dictionary.3 prop_dictionary_keysym_equals.3
MLINKS+= prop_dictionary.3 prop_dictionary_make_immutable.3
MLINKS+= prop_dictionary.3 prop_dictionary_mutable.3
MLINKS+= prop_dictionary.3 prop_dictionary_remove.3
MLINKS+= prop_dictionary.3 prop_dictionary_remove_keysym.3
MLINKS+= prop_dictionary.3 prop_dictionary_set.3
MLINKS+= prop_dictionary.3 prop_dictionary_set_keysym.3

MLINKS+= prop_ingest.3 prop_ingest_context_alloc.3
MLINKS+= prop_ingest.3 prop_ingest_context_error.3
MLINKS+= prop_ingest.3 prop_ingest_context_free.3
MLINKS+= prop_ingest.3 prop_ingest_context_key.3
MLINKS+= prop_ingest.3 prop_ingest_context_private.3
MLINKS+= prop_ingest.3 prop_ingest_context_type.3
MLINKS+= prop_ingest.3 prop_dictionary_ingest.3

MLINKS+= prop_number.3 prop_number_copy.3
MLINKS+= prop_number.3 prop_number_create_signed.3
MLINKS+= prop_number.3 prop_number_create_unsigned.3
MLINKS+= prop_number.3 prop_number_equals.3
MLINKS+= prop_number.3 prop_number_equals_signed.3
MLINKS+= prop_number.3 prop_number_equals_unsigned.3
MLINKS+= prop_number.3 prop_number_signed_value.3
MLINKS+= prop_number.3 prop_number_size.3
MLINKS+= prop_number.3 prop_number_unsigned.3
MLINKS+= prop_number.3 prop_number_unsigned_value.3

MLINKS+= prop_number.3 prop_number_schar_value.3
MLINKS+= prop_number.3 prop_number_short_value.3
MLINKS+= prop_number.3 prop_number_int_value.3
MLINKS+= prop_number.3 prop_number_long_value.3
MLINKS+= prop_number.3 prop_number_longlong_value.3
MLINKS+= prop_number.3 prop_number_intptr_value.3
MLINKS+= prop_number.3 prop_number_int8_value.3
MLINKS+= prop_number.3 prop_number_int16_value.3
MLINKS+= prop_number.3 prop_number_int32_value.3
MLINKS+= prop_number.3 prop_number_int64_value.3

MLINKS+= prop_number.3 prop_number_uchar_value.3
MLINKS+= prop_number.3 prop_number_ushort_value.3
MLINKS+= prop_number.3 prop_number_uint_value.3
MLINKS+= prop_number.3 prop_number_ulong_value.3
MLINKS+= prop_number.3 prop_number_ulonglong_value.3
MLINKS+= prop_number.3 prop_number_uintptr_value.3
MLINKS+= prop_number.3 prop_number_uint8_value.3
MLINKS+= prop_number.3 prop_number_uint16_value.3
MLINKS+= prop_number.3 prop_number_uint32_value.3
MLINKS+= prop_number.3 prop_number_uint64_value.3

MLINKS+= prop_object.3 prop_object_equals.3
MLINKS+= prop_object.3 prop_object_iterator_next.3
MLINKS+= prop_object.3 prop_object_iterator_release.3
MLINKS+= prop_object.3 prop_object_iterator_reset.3
MLINKS+= prop_object.3 prop_object_release.3
MLINKS+= prop_object.3 prop_object_retain.3
MLINKS+= prop_object.3 prop_object_type.3

MLINKS+= prop_string.3 prop_string_compare.3
MLINKS+= prop_string.3 prop_string_compare_string.3
MLINKS+= prop_string.3 prop_string_copy.3
MLINKS+= prop_string.3 prop_string_create_copy.3
MLINKS+= prop_string.3 prop_string_create_format.3
MLINKS+= prop_string.3 prop_string_create_nocopy.3
MLINKS+= prop_string.3 prop_string_equals.3
MLINKS+= prop_string.3 prop_string_equals_string.3
MLINKS+= prop_string.3 prop_string_size.3
MLINKS+= prop_string.3 prop_string_value.3

.include <bsd.lib.mk>
