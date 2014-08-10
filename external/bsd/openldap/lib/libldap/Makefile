#	$NetBSD: Makefile,v 1.3.22.1 2014/08/10 07:09:53 tls Exp $

USE_FORT?=	yes

CWARNFLAGS.clang=	-Wno-format-extra-args

.include "../libldap/Makefile.libldap"

LIB=		ldap

.PATH:		${LDAP_SRCDIR}/man

MAN=		ldap.3 ldap_abandon.3 ldap_add.3 ldap_bind.3 ldap_compare.3 \
		ldap_delete.3 ldap_dup.3 ldap_error.3 ldap_first_attribute.3 \
		ldap_first_entry.3 ldap_first_message.3 \
		ldap_first_reference.3 ldap_get_dn.3 ldap_get_values.3 \
		ldap_modify.3 ldap_modrdn.3 ldap_open.3 \
		ldap_parse_reference.3 ldap_parse_result.3 ldap_result.3 \
		ldap_schema.3 ldap_search.3 ldap_sort.3 ldap_url.3

MLINKS+=	ldap_abandon.3 ldap_abandon_ext.3

MLINKS+=	ldap_add.3 ldap_add_s.3 \
		ldap_add.3 ldap_add_ext.3 \
		ldap_add.3 ldap_add_ext_s.3

MLINKS+=	ldap_bind.3 ldap_bind_s.3 \
		ldap_bind.3 ldap_simple_bind.3 \
		ldap_bind.3 ldap_simple_bind_s.3 \
		ldap_bind.3 ldap_sasl_bind.3 \
		ldap_bind.3 ldap_sasl_bind_s.3 \
		ldap_bind.3 ldap_kerberos_bind_s.3 \
		ldap_bind.3 ldap_kerberos_bind1.3 \
		ldap_bind.3 ldap_kerberos_bind1_s.3 \
		ldap_bind.3 ldap_kerberos_bind2.3 \
		ldap_bind.3 ldap_kerberos_bind2_s.3 \
		ldap_bind.3 ldap_unbind.3 \
		ldap_bind.3 ldap_unbind_ext.3 \
		ldap_bind.3 ldap_unbind_s.3 \
		ldap_bind.3 ldap_unbind_ext_s.3

MLINKS+=	ldap_compare.3 ldap_compare_s.3 \
		ldap_compare.3 ldap_compare_ext.3 \
		ldap_compare.3 ldap_compare_ext_s.3

MLINKS+=	ldap_delete.3 ldap_delete_s.3 \
		ldap_delete.3 ldap_delete_ext.3 \
		ldap_delete.3 ldap_delete_ext_s.3

MLINKS+=	ldap_error.3 ldap_perror.3 \
		ldap_error.3 ld_errno.3 \
		ldap_error.3 ldap_result2error.3 \
		ldap_error.3 ldap_errlist.3 \
		ldap_error.3 ldap_err2string.3

MLINKS+=	ldap_first_attribute.3 ldap_next_attribute.3

MLINKS+=	ldap_first_entry.3 ldap_next_entry.3 \
		ldap_first_entry.3 ldap_count_entries.3

MLINKS+=	ldap_first_message.3 ldap_next_message.3 \
		ldap_first_message.3 ldap_count_messages.3

MLINKS+=	ldap_first_reference.3 ldap_next_reference.3 \
		ldap_first_reference.3 ldap_count_references.3

MLINKS+=	ldap_get_dn.3 ldap_explode_dn.3 \
		ldap_get_dn.3 ldap_explode_rdn.3 \
		ldap_get_dn.3 ldap_dn2ufn.3 \
		ldap_get_dn.3 ldap_str2dn.3 \
		ldap_get_dn.3 ldap_dn2str.3 \
		ldap_get_dn.3 ldap_dn2dcedn.3 \
		ldap_get_dn.3 ldap_dcedn2dn.3 \
		ldap_get_dn.3 ldap_dn2ad_canonical.3

MLINKS+=	ldap_get_values.3 ldap_get_values_len.3 \
		ldap_get_values.3 ldap_value_free.3 \
		ldap_get_values.3 ldap_value_free_len.3 \
		ldap_get_values.3 ldap_count_values.3 \
		ldap_get_values.3 ldap_count_values_len.3

MLINKS+=	ldap_modify.3 ldap_modify_s.3 \
		ldap_modify.3 ldap_modify_ext.3 \
		ldap_modify.3 ldap_modify_ext_s.3 \
		ldap_modify.3 ldap_mods_free.3

MLINKS+=	ldap_modrdn.3 ldap_modrdn_s.3 \
		ldap_modrdn.3 ldap_modrdn2.3 \
		ldap_modrdn.3 ldap_modrdn2_s.3

MLINKS+=	ldap_open.3 ldap_init.3

MLINKS+=	ldap_parse_result.3 ldap_parse_sasl_bind_result.3 \
		ldap_parse_result.3 ldap_parse_extended_result.3

MLINKS+=	ldap_result.3 ldap_msgfree.3 \
		ldap_result.3 ldap_msgtype.3 \
		ldap_result.3 ldap_msgid.3

MLINKS+=	ldap_schema.3 ldap_str2syntax.3 \
		ldap_schema.3 ldap_syntax2str.3 \
		ldap_schema.3 ldap_syntax2name.3 \
		ldap_schema.3 ldap_syntax_free.3 \
		ldap_schema.3 ldap_str2matchingrule.3 \
		ldap_schema.3 ldap_matchingrule2str.3 \
		ldap_schema.3 ldap_matchingrule2name.3 \
		ldap_schema.3 ldap_matchingrule_free.3 \
		ldap_schema.3 ldap_str2attributetype.3 \
		ldap_schema.3 ldap_attributetype2str.3 \
		ldap_schema.3 ldap_attributetype2name.3 \
		ldap_schema.3 ldap_attributetype_free.3 \
		ldap_schema.3 ldap_str2objectclass.3 \
		ldap_schema.3 ldap_objectclass2str.3 \
		ldap_schema.3 ldap_objectclass2name.3 \
		ldap_schema.3 ldap_objectclass_free.3 \
		ldap_schema.3 ldap_scherr2str.3

MLINKS+=	ldap_search.3 ldap_search_s.3 \
		ldap_search.3 ldap_search_st.3 \
		ldap_search.3 ldap_search_ext.3 \
		ldap_search.3 ldap_search_ext_s.3

MLINKS+=	ldap_sort.3 ldap_sort_entries.3 \
		ldap_sort.3 ldap_sort_values.3 \
		ldap_sort.3 ldap_sort_strcasecmp.3

MLINKS+=	ldap_url.3 ldap_is_ldap_url.3 \
		ldap_url.3 ldap_url_parse.3 \
		ldap_url.3 ldap_free_urldesc.3


MAN+=		ldap.conf.5 ldif.5

.include <bsd.lib.mk>
