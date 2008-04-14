# $OpenLDAP: pkg/ldap/contrib/ldapc++/src/Makefile.am,v 1.10.2.5 2008/04/14 23:02:35 quanah Exp $

###
# Copyright 2000, OpenLDAP Foundation, All Rights Reserved.
# COPYING RESTRICTIONS APPLY, see COPYRIGHT file
##

lib_LTLIBRARIES = libldapcpp.la

libldapcpp_la_SOURCES = LDAPAddRequest.cpp \
			LDAPAsynConnection.cpp \
			LDAPAttribute.cpp \
			LDAPAttributeList.cpp \
			LDAPAttrType.cpp \
			LDAPBindRequest.cpp \
			LDAPCompareRequest.cpp \
			LDAPConnection.cpp \
			LDAPConstraints.cpp \
			LDAPControl.cpp \
			LDAPControlSet.cpp \
			LDAPDeleteRequest.cpp \
			LDAPEntry.cpp \
			LDAPEntryList.cpp \
			LDAPException.cpp \
			LDAPExtRequest.cpp \
			LDAPExtResult.cpp \
			LDAPMessage.cpp \
			LDAPMessageQueue.cpp \
			LDAPModDNRequest.cpp \
			LDAPModification.cpp \
			LDAPModifyRequest.cpp \
			LDAPModList.cpp \
			LDAPObjClass.cpp \
			LDAPRebind.cpp \
			LDAPRebindAuth.cpp \
			LDAPReferenceList.cpp \
			LDAPRequest.cpp \
			LDAPResult.cpp \
			LDAPSaslBindResult.cpp \
			LDAPSchema.cpp \
			LDAPSearchReference.cpp \
			LDAPSearchRequest.cpp \
			LDAPSearchResult.cpp \
			LDAPSearchResults.cpp \
			LDAPUrl.cpp \
			LDAPUrlList.cpp \
			LdifReader.cpp \
			LdifWriter.cpp \
			SaslInteraction.cpp \
			SaslInteractionHandler.cpp \
			StringList.cpp 

include_HEADERS = LDAPAsynConnection.h \
			LDAPAttribute.h \
			LDAPAttributeList.h \
			LDAPAttrType.h \
			LDAPConnection.h \
			LDAPConstraints.h \
			LDAPControl.h \
			LDAPControlSet.h \
			LDAPEntry.h \
			LDAPEntryList.h \
			LDAPException.h \
			LDAPExtResult.h \
			LDAPMessage.h \
			LDAPMessageQueue.h \
			LDAPModification.h \
			LDAPModList.h \
			LDAPObjClass.h \
			LDAPRebind.h \
			LDAPRebindAuth.h \
			LDAPReferenceList.h \
			LDAPResult.h \
			LDAPSaslBindResult.h \
			LDAPSchema.h \
			LDAPSearchReference.h \
			LDAPSearchResult.h \
			LDAPSearchResults.h \
			LDAPUrl.h \
			LDAPUrlList.h \
			LdifReader.h \
			LdifWriter.h \
			SaslInteraction.h \
			SaslInteractionHandler.h \
			StringList.h 

noinst_HEADERS = LDAPAddRequest.h \
                LDAPBindRequest.h \
                LDAPCompareRequest.h \
                LDAPDeleteRequest.h \
                LDAPExtRequest.h \
                LDAPModDNRequest.h \
                LDAPModifyRequest.h \
                LDAPRequest.h \
                LDAPSearchRequest.h 

libldapcpp_la_LIBADD = -lldap -llber
libldapcpp_la_LDFLAGS = -version-info 0:5:0

