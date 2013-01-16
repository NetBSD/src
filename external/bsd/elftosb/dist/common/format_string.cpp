/*
 * File:	format_string.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "format_string.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdexcept>
#include <string.h>
#include <stdlib.h>
//! Size of the temporary buffer to hold the formatted output string.
#define WIN32_FMT_BUF_LEN (512)

/*!
 * \brief Simple template class to free a pointer.
 */
template <typename T>
class free_ptr
{
public:
	//! \brief Constructor.
	free_ptr(T ptr)
	:	m_p(ptr)
	{
	}
	
	//! \brief Destructor.
	~free_ptr()
	{
		if (m_p)
		{
			free(m_p);
		}
	}

protected:
	T m_p;	//!< The value to free.
};

//! The purpose of this function to provide a convenient way of generating formatted
//! STL strings inline. This is especially useful when throwing exceptions that take
//! a std::string for a message. The length of the formatted output string is limited
//! only by memory. Memory temporarily allocated for the output string is disposed of
//! before returning.
//!
//! Example usage:
//! \code
//!		throw std::runtime_error(format_string("error on line %d", line));
//! \endcode
//!
//! \param fmt Format string using printf-style format markers.
//! \return An STL string object of the formatted output.
std::string format_string(const char * fmt, ...)
{
	char * buf = 0;
	va_list vargs;
	va_start(vargs, fmt);
	int result = -1;
#if WIN32
    buf = (char *)malloc(WIN32_FMT_BUF_LEN);
    if (buf)
    {
        result = _vsnprintf(buf, WIN32_FMT_BUF_LEN, fmt, vargs);
    }
#else // WIN32
	result = vasprintf(&buf, fmt, vargs);
#endif // WIN32
	va_end(vargs);
	if (result != -1 && buf)
	{
		free_ptr<char *> freebuf(buf);
		return std::string(buf);
	}
	return "";
}

