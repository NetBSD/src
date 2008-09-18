// $OpenLDAP: pkg/ldap/contrib/ldapc++/src/LDAPReferenceList.cpp,v 1.2.10.1 2008/04/14 23:09:26 quanah Exp $
/*
 * Copyright 2000, OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#include "LDAPReferenceList.h"
#include "LDAPSearchReference.h"

LDAPReferenceList::LDAPReferenceList(){
}

LDAPReferenceList::LDAPReferenceList(const LDAPReferenceList& e){
    m_refs = e.m_refs;
}

LDAPReferenceList::~LDAPReferenceList(){
}

size_t LDAPReferenceList::size() const{
    return m_refs.size();
}

bool LDAPReferenceList::empty() const{
    return m_refs.empty();
}

LDAPReferenceList::const_iterator LDAPReferenceList::begin() const{
    return m_refs.begin();
}

LDAPReferenceList::const_iterator LDAPReferenceList::end() const{
    return m_refs.end();
}

void LDAPReferenceList::addReference(const LDAPSearchReference& e){
    m_refs.push_back(e);
}

