// parameters.cc -- general parameters for a link using gold

// Copyright 2006, 2007, 2008 Free Software Foundation, Inc.
// Written by Ian Lance Taylor <iant@google.com>.

// This file is part of gold.

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
// MA 02110-1301, USA.

#include "gold.h"

#include "debug.h"
#include "options.h"
#include "target.h"
#include "target-select.h"

namespace gold
{

void
Parameters::set_errors(Errors* errors)
{
  gold_assert(this->errors_ == NULL);
  this->errors_ = errors;
}

void
Parameters::set_options(const General_options* options)
{
  gold_assert(!this->options_valid());
  this->options_ = options;
  // For speed, we convert the options() debug var from a string to an
  // enum (from debug.h).
  this->debug_ = debug_string_to_enum(this->options().debug());
  // If --verbose is set, it acts as "--debug=files".
  if (options->verbose())
    this->debug_ |= DEBUG_FILES;
}

void
Parameters::set_doing_static_link(bool doing_static_link)
{
  gold_assert(!this->doing_static_link_valid_);
  this->doing_static_link_ = doing_static_link;
  this->doing_static_link_valid_ = true;
}

void
Parameters::set_target(const Target* target)
{
  if (!this->target_valid())
    this->target_ = target;
  else
    gold_assert(target == this->target_);
}

// The x86_64 kernel build converts a binary file to an object file
// using -r --format binary --oformat elf32-i386 foo.o.  In order to
// support that for gold we support determining the default target
// choice from the output format.  We recognize names that the GNU
// linker uses.

const Target&
Parameters::default_target() const
{
  gold_assert(this->options_valid());
  if (this->options().user_set_oformat())
    {
      const Target* target
          = select_target_by_name(this->options().oformat());
      if (target != NULL)
	return *target;

      gold_error(_("unrecognized output format %s"),
                 this->options().oformat());
    }

  // The GOLD_DEFAULT_xx macros are defined by the configure script.
  const Target* target = select_target(elfcpp::GOLD_DEFAULT_MACHINE,
                                       GOLD_DEFAULT_SIZE,
                                       GOLD_DEFAULT_BIG_ENDIAN,
                                       0, 0);
  gold_assert(target != NULL);
  return *target;
}

Parameters::Target_size_endianness
Parameters::size_and_endianness() const
{
  if (this->target().get_size() == 32)
    {
      if (!this->target().is_big_endian())
	{
#ifdef HAVE_TARGET_32_LITTLE
	  return TARGET_32_LITTLE;
#else
	  gold_unreachable();
#endif
	}
      else
	{
#ifdef HAVE_TARGET_32_BIG
	  return TARGET_32_BIG;
#else
	  gold_unreachable();
#endif
	}
    }
  else if (parameters->target().get_size() == 64)
    {
      if (!parameters->target().is_big_endian())
	{
#ifdef HAVE_TARGET_64_LITTLE
	  return TARGET_64_LITTLE;
#else
	  gold_unreachable();
#endif
	}
      else
	{
#ifdef HAVE_TARGET_64_BIG
	  return TARGET_64_BIG;
#else
	  gold_unreachable();
#endif
	}
    }
  else
    gold_unreachable();
}


// Our local version of the variable, which is not const.

static Parameters static_parameters;

// The global variable.

const Parameters* parameters = &static_parameters;

void
set_parameters_errors(Errors* errors)
{ static_parameters.set_errors(errors); }

void
set_parameters_options(const General_options* options)
{ static_parameters.set_options(options); }

void
set_parameters_target(const Target* target)
{ static_parameters.set_target(target); }

void
set_parameters_doing_static_link(bool doing_static_link)
{ static_parameters.set_doing_static_link(doing_static_link); }

} // End namespace gold.
