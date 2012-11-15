/*
 * File:	OptionDictionary.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "OptionDictionary.h"

using namespace elftosb;

//! Deletes all of the option values that have been assigned locally.
//!
OptionDictionary::~OptionDictionary()
{
	option_map_t::iterator it = m_options.begin();
	for (; it != m_options.end(); ++it)
	{
		if (it->second.m_value)
		{
			delete it->second.m_value;
		}
	}
}

//! If a parent context has been set and the option does not exist in
//! this instance, then the parent is asked if it contains the option.
//!
//! \param name The name of the option to query.
//! \retval true The option is present in this instance or one of the parent.
//! \retval false No option with that name is in the dictionary, or any parent
bool OptionDictionary::hasOption(const std::string & name) const
{
	bool hasIt = (m_options.find(name) != m_options.end());
	if (!hasIt && m_parent)
	{
		return m_parent->hasOption(name);
	}
	return hasIt;
}

//! If this object does not contain an option with the name of \a name,
//! then the parent is asked for the value (if a parent has been set).
//!
//! \param name The name of the option.
//! \return The value for the option named \a name.
//! \retval NULL No option is in the table with that name. An option may also
//!		explicitly be set to a NULL value. The only way to tell the difference
//!		is to use the hasOption() method.
const Value * OptionDictionary::getOption(const std::string & name) const
{
	option_map_t::const_iterator it = m_options.find(name);
	if (it == m_options.end())
	{
		if (m_parent)
		{
			return m_parent->getOption(name);
		}
		else
		{
			return NULL;
		}
	}
	
	return it->second.m_value;
}

//! If the option was not already present in the table, it is added.
//! Otherwise the old value is replaced. The option is always set locally;
//! parent objects are never modified.
//!
//! If the option has been locked with a call to lockOption() before trying
//! to set its value, the setOption() is effectively ignored. To tell if
//! an option is locked, use the isOptionLocked() method.
//!
//! \warning If the option already had a value, that previous value is deleted.
//!		This means that it cannot currently be in use by another piece of code.
//!		See the note in getOption().
//!
//! \param name The option's name.
//! \param value New value for the option.
void OptionDictionary::setOption(const std::string & name, Value * value)
{
	option_map_t::iterator it = m_options.find(name);
	OptionValue newValue;

	// delete the option value instance before replacing it
	if (it != m_options.end())
	{
		// Cannot modify value if locked.
		if (it->second.m_isLocked)
		{
			return;
		}

		if (it->second.m_value)
		{
			delete it->second.m_value;
		}

		// save previous locked value
		newValue.m_isLocked = it->second.m_isLocked;
	}
	
	// set new option value
	newValue.m_value = value;
	m_options[name] = newValue;
}

//! \param name The name of the option to remove.
//!
void OptionDictionary::deleteOption(const std::string & name)
{
	if (m_options.find(name) != m_options.end())
	{
		if (m_options[name].m_value)
		{
			delete m_options[name].m_value;
		}
		m_options.erase(name);
	}
}

//! \param name Name of the option to query.
//!
//! \return True if the option is locked, false if unlocked or not present.
//!
bool OptionDictionary::isOptionLocked(const std::string & name) const
{
	option_map_t::const_iterator it = m_options.find(name);
	if (it != m_options.end())
	{
		return it->second.m_isLocked;
	}

	return false;
}

//! \param name Name of the option to lock.
//!
void OptionDictionary::lockOption(const std::string & name)
{
	if (!hasOption(name))
	{
		m_options[name].m_value = 0;
	}

	m_options[name].m_isLocked = true;
}

//! \param name Name of the option to unlock.
//!
void OptionDictionary::unlockOption(const std::string & name)
{
	if (!hasOption(name))
	{
		m_options[name].m_value = 0;
	}

	m_options[name].m_isLocked = false;
}


//! Simply calls getOption().
//!
const Value * OptionDictionary::operator [] (const std::string & name) const
{
	return getOption(name);
}

