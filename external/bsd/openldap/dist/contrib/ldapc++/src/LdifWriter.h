/*	$NetBSD: LdifWriter.h,v 1.1.1.6 2019/08/08 13:31:08 christos Exp $	*/

// $OpenLDAP$
/*
 * Copyright 2008-2019 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef LDIF_WRITER_H
#define LDIF_WRITER_H

#include <LDAPEntry.h>
#include <iosfwd>
#include <list>

class LdifWriter
{
    public:
        LdifWriter( std::ostream& output, int version = 0 );
        void writeRecord(const LDAPEntry& le);
        void writeIncludeRecord(const std::string& target);

    private:
        void breakline( const std::string &line, std::ostream &out );

        std::ostream& m_ldifstream;
        int m_version;
        bool m_addSeparator;

};

#endif /* LDIF_WRITER_H */

