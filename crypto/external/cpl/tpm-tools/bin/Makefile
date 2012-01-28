# $NetBSD: Makefile,v 1.1 2012/01/28 16:04:12 christos Exp $
.include <bsd.own.mk>
SUBDIR = ${TPM_MGMT} ${CMDS}

TPM_MGMT=\
tpm_changeownerauth \
tpm_clear \
tpm_createek \
tpm_getpubek \
tpm_restrictpubek \
tpm_setactive \
tpm_setenable \
tpm_setclearable \
tpm_setownable \
tpm_setpresence \
tpm_takeownership \
tpm_version \
tpm_selftest \
tpm_nvdefine \
tpm_nvinfo \
tpm_nvread \
tpm_nvrelease \
tpm_nvwrite \
tpm_resetdalock \
tpm_restrictsrk \
tpm_revokeek \
tpm_setoperatorauth

CMDS=\
tpm_sealdata \
tpm_unsealdata

.include <bsd.subdir.mk>
