// $OpenLDAP: pkg/ldap/contrib/ldapc++/src/LDAPEntryList.cpp,v 1.2.10.1 2008/04/14 23:09:26 quanah Exp $
/*
 * Copyright 2000, OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#include "LDAPEntryList.h"
#include "LDAPEntry.h"

LDAPEntryList::LDAPEntryList(){
}

LDAPEntryList::LDAPEntryList(const LDAPEntryList& e){
    m_entries = e.m_entries;
}

LDAPEntryList::~LDAPEntryList(){
}

size_t LDAPEntryList::size() const{
    return m_entries.size();
}

bool LDAPEntryList::empty() const{
    return m_entries.empty();
}

LDAPEntryList::const_iterator LDAPEntryList::begin() const{
    return m_entries.begin();
}

LDAPEntryList::const_iterator LDAPEntryList::end() const{
    return m_entries.end();
}

void LDAPEntryList::addEntry(const LDAPEntry& e){
    m_entries.push_back(e);
}

