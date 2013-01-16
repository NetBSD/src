/*
 * File:	OptionDictionary.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_OptionDictionary_h_)
#define _OptionDictionary_h_

#include "OptionContext.h"
#include <map>

namespace elftosb
{

/*!
 * \brief Concrete implementation of OptionContext.
 *
 * This context subclass supports having a parent context. If an option is not
 * found in the receiving instance, the request is passed to the parent.
 * The hasOption() and getOption() methods will ask up the parent chain
 * if the requested option does not exist in the receiving instance.
 * But the setOption() and deleteOption() methods only operate locally,
 * on the instance on which they were called. This allows a caller to
 * locally override an option value without affecting any of the parent
 * contexts.
 */
class OptionDictionary : public OptionContext
{
public:
	//! \brief Default constructor.
	OptionDictionary() : m_parent(0) {}
	
	//! \brief Constructor taking a parent context.
	OptionDictionary(OptionContext * parent) : m_parent(parent) {}
	
	//! \brief Destructor.
	~OptionDictionary();
	
	//! \name Parents
	//@{
	//! \brief Returns the current parent context.
	//! \return The current parent context instance.
	//! \retval NULL No parent has been set.
	inline OptionContext * getParent() const { return m_parent; }
	
	//! \brief Change the parent context.
	//! \param newParent The parent context object. May be NULL, in which case
	//!		the object will no longer have a parent context.
	inline void setParent(OptionContext * newParent) { m_parent = newParent; }
	//@}
	
	//! \name Options
	//@{
	//! \brief Detemine whether the named option is present in the table.
	virtual bool hasOption(const std::string & name) const;
	
	//! \brief Returns the option's value.
	virtual const Value * getOption(const std::string & name) const;
	
	//! \brief Adds or changes an option's value.
	virtual void setOption(const std::string & name, Value * value);
	
	//! \brief Removes an option from the table.
	virtual void deleteOption(const std::string & name);
	//@}

	//! \name Locking
	//@{
	//! \brief Returns true if the specified option is locked from further changes.
	bool isOptionLocked(const std::string & name) const;

	//! \brief Prevent further modifications of an option's value.
	void lockOption(const std::string & name);

	//! \brief Allow an option to be changed.
	void unlockOption(const std::string & name);
	//@}
	
	//! \name Operators
	//@{
	//! \brief Indexing operator; returns the value for the option \a name.
	const Value * operator [] (const std::string & name) const;
	//@}
	
protected:
	OptionContext * m_parent;	//!< Our parent context.

	/*!
	 * \brief Information about one option's value.
	 */
	struct OptionValue
	{
		Value * m_value;	//!< The object for this option's value.
		bool m_isLocked;	//!< True if this value is locked from further changes.

		//! \brief Constructor.
		OptionValue() : m_value(0), m_isLocked(false) {}
	};
	
	typedef std::map<std::string, OptionValue> option_map_t;	//!< Map from option name to value.
	option_map_t m_options;	//!< The option dictionary.
};

}; // namespace elftosb

#endif // _OptionDictionary_h_
