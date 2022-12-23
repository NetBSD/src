/* Copyright (C) 2021 Free Software Foundation, Inc.
   Contributed by Oracle.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

/*
 *	javax/xml/parsers/SAXParser.java
 *
 *	Based on JavaTM 2 Platform Standard Ed. 5.0
 */

#ifndef _SAXParser_h
#define _SAXParser_h

class File;
class DefaultHandler;
class SAXException;

class SAXParser
{
public:

  virtual ~SAXParser () { }
  virtual void reset () { }
  virtual void parse (File*, DefaultHandler*) = 0;
  virtual bool isNamespaceAware () = 0;
  virtual bool isValidating () = 0;

protected:

  SAXParser () { }
};

#endif /* _SAXParser_h */
