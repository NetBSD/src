/*
 * File:	ElftosbLexer.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#include "ElftosbLexer.h"
#include <algorithm>
#include "HexValues.h"

using namespace elftosb;

ElftosbLexer::ElftosbLexer(istream & inputStream)
:	yyFlexLexer(&inputStream), m_line(1), m_blob(0), m_blobFirstLine(0)
{
}

void ElftosbLexer::getSymbolValue(YYSTYPE * value)
{
	if (!value)
	{
		return;
	}
	*value = m_symbolValue;
}

void ElftosbLexer::addSourceName(std::string * ident)
{
	m_sources.push_back(*ident);
}

bool ElftosbLexer::isSourceName(std::string * ident)
{
	string_vector_t::iterator it = find(m_sources.begin(), m_sources.end(), *ident);
	return it != m_sources.end();
}

void ElftosbLexer::LexerError(const char * msg)
{
	throw elftosb::lexical_error(msg);
}

//! Reads the \a in string and writes to the \a out string. These strings can be the same
//! string since the read head is always in front of the write head.
//!
//! \param[in] in Input string containing C-style escape sequences.
//! \param[out] out Output string. All escape sequences in the input string have been converted
//!		to the actual characters. May point to the same string as \a in.
//! \return The length of the resulting \a out string. This length is necessary because
//!		the string may have contained escape sequences that inserted null characters.
int ElftosbLexer::processStringEscapes(const char * in, char * out)
{
	int count = 0;
	while (*in)
	{
		switch (*in)
		{
			case '\\':
			{
				// start of an escape sequence
				char c = *++in;
				switch (c)
				{
					case 0:	// end of the string, bail
						break;
					case 'x':
					{
						// start of a hex char escape sequence
						
						// read high and low nibbles, checking for end of string
						char hi = *++in;
						if (hi == 0) break;
						char lo = *++in;
						if (lo == 0) break;
						
						if (isHexDigit(hi) && isHexDigit(lo))
						{
							if (hi >= '0' && hi <= '9')
								c = (hi - '0') << 4;
							else if (hi >= 'A' && hi <= 'F')
								c = (hi - 'A' + 10) << 4;
							else if (hi >= 'a' && hi <= 'f')
								c = (hi - 'a' + 10) << 4;
							
							if (lo >= '0' && lo <= '9')
								c |= lo - '0';
							else if (lo >= 'A' && lo <= 'F')
								c |= lo - 'A' + 10;
							else if (lo >= 'a' && lo <= 'f')
								c |= lo - 'a' + 10;
								
							*out++ = c;
							count++;
						}
						else
						{
							// not hex digits, the \x must have wanted an 'x' char
							*out++ = 'x';
							*out++ = hi;
							*out++ = lo;
							count += 3;
						}
						break;
					}
					case 'n':
						*out++ = '\n';
						count++;
						break;
					case 't':
						*out++ = '\t';
						count++;
						break;
					case 'r':
						*out++ = '\r';
						count++;
						break;
					case 'b':
						*out++ = '\b';
						count++;
						break;
					case 'f':
						*out++ = '\f';
						count++;
						break;
					case '0':
						*out++ = '\0';
						count++;
						break;
					default:
						*out++ = c;
						count++;
						break;
				}
				break;
			}
			
			default:
				// copy all other chars directly
				*out++ = *in++;
				count++;
		}
	}
	
	// place terminating null char on output
	*out = 0;
	return count;
}


