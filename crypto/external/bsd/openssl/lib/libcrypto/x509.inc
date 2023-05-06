#	$NetBSD: x509.inc,v 1.4 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/x509

X509_SRCS += \
by_dir.c \
by_file.c \
by_store.c \
pcy_cache.c \
pcy_data.c \
pcy_lib.c \
pcy_map.c \
pcy_node.c \
pcy_tree.c \
t_crl.c \
t_req.c \
t_x509.c \
v3_addr.c \
v3_admis.c \
v3_akeya.c \
v3_akid.c \
v3_asid.c \
v3_bcons.c \
v3_bitst.c \
v3_conf.c \
v3_cpols.c \
v3_crld.c \
v3_enum.c \
v3_extku.c \
v3_genn.c \
v3_ia5.c \
v3_info.c \
v3_int.c \
v3_ist.c \
v3_lib.c \
v3_ncons.c \
v3_pci.c \
v3_pcia.c \
v3_pcons.c \
v3_pku.c \
v3_pmaps.c \
v3_prn.c \
v3_purp.c \
v3_san.c \
v3_skid.c \
v3_sxnet.c \
v3_tlsf.c \
v3_utf8.c \
v3_utl.c \
v3err.c \
x509_att.c \
x509_cmp.c \
x509_d2.c \
x509_def.c \
x509_err.c \
x509_ext.c \
x509_lu.c \
x509_meth.c \
x509_obj.c \
x509_r2x.c \
x509_req.c \
x509_set.c \
x509_trust.c \
x509_txt.c \
x509_v3.c \
x509_vfy.c \
x509_vpm.c \
x509cset.c \
x509name.c \
x509rset.c \
x509spki.c \
x509type.c \
x_all.c \
x_attrib.c \
x_crl.c \
x_exten.c \
x_name.c \
x_pubkey.c \
x_req.c \
x_x509.c \
x_x509a.c \

SRCS += ${X509_SRCS}

.for cryptosrc in ${X509_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/x509 ${X509CPPFLAGS}
.endfor
