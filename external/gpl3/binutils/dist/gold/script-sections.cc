// script-sections.cc -- linker script SECTIONS for gold

// Copyright 2008 Free Software Foundation, Inc.
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

#include <cstring>
#include <algorithm>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <fnmatch.h>

#include "parameters.h"
#include "object.h"
#include "layout.h"
#include "output.h"
#include "script-c.h"
#include "script.h"
#include "script-sections.h"

// Support for the SECTIONS clause in linker scripts.

namespace gold
{

// An element in a SECTIONS clause.

class Sections_element
{
 public:
  Sections_element()
  { }

  virtual ~Sections_element()
  { }

  // Record that an output section is relro.
  virtual void
  set_is_relro()
  { }

  // Create any required output sections.  The only real
  // implementation is in Output_section_definition.
  virtual void
  create_sections(Layout*)
  { }

  // Add any symbol being defined to the symbol table.
  virtual void
  add_symbols_to_table(Symbol_table*)
  { }

  // Finalize symbols and check assertions.
  virtual void
  finalize_symbols(Symbol_table*, const Layout*, uint64_t*)
  { }

  // Return the output section name to use for an input file name and
  // section name.  This only real implementation is in
  // Output_section_definition.
  virtual const char*
  output_section_name(const char*, const char*, Output_section***)
  { return NULL; }

  // Return whether to place an orphan output section after this
  // element.
  virtual bool
  place_orphan_here(const Output_section *, bool*, bool*) const
  { return false; }

  // Set section addresses.  This includes applying assignments if the
  // the expression is an absolute value.
  virtual void
  set_section_addresses(Symbol_table*, Layout*, uint64_t*, uint64_t*)
  { }

  // Check a constraint (ONLY_IF_RO, etc.) on an output section.  If
  // this section is constrained, and the input sections do not match,
  // return the constraint, and set *POSD.
  virtual Section_constraint
  check_constraint(Output_section_definition**)
  { return CONSTRAINT_NONE; }

  // See if this is the alternate output section for a constrained
  // output section.  If it is, transfer the Output_section and return
  // true.  Otherwise return false.
  virtual bool
  alternate_constraint(Output_section_definition*, Section_constraint)
  { return false; }

  // Get the list of segments to use for an allocated section when
  // using a PHDRS clause.  If this is an allocated section, return
  // the Output_section, and set *PHDRS_LIST (the first parameter) to
  // the list of PHDRS to which it should be attached.  If the PHDRS
  // were not specified, don't change *PHDRS_LIST.  When not returning
  // NULL, set *ORPHAN (the second parameter) according to whether
  // this is an orphan section--one that is not mentioned in the
  // linker script.
  virtual Output_section*
  allocate_to_segment(String_list**, bool*)
  { return NULL; }

  // Look for an output section by name and return the address, the
  // load address, the alignment, and the size.  This is used when an
  // expression refers to an output section which was not actually
  // created.  This returns true if the section was found, false
  // otherwise.  The only real definition is for
  // Output_section_definition.
  virtual bool
  get_output_section_info(const char*, uint64_t*, uint64_t*, uint64_t*,
                          uint64_t*) const
  { return false; }

  // Return the associated Output_section if there is one.
  virtual Output_section*
  get_output_section() const
  { return NULL; }

  // Print the element for debugging purposes.
  virtual void
  print(FILE* f) const = 0;
};

// An assignment in a SECTIONS clause outside of an output section.

class Sections_element_assignment : public Sections_element
{
 public:
  Sections_element_assignment(const char* name, size_t namelen,
			      Expression* val, bool provide, bool hidden)
    : assignment_(name, namelen, val, provide, hidden)
  { }

  // Add the symbol to the symbol table.
  void
  add_symbols_to_table(Symbol_table* symtab)
  { this->assignment_.add_to_table(symtab); }

  // Finalize the symbol.
  void
  finalize_symbols(Symbol_table* symtab, const Layout* layout,
		   uint64_t* dot_value)
  {
    this->assignment_.finalize_with_dot(symtab, layout, *dot_value, NULL);
  }

  // Set the section address.  There is no section here, but if the
  // value is absolute, we set the symbol.  This permits us to use
  // absolute symbols when setting dot.
  void
  set_section_addresses(Symbol_table* symtab, Layout* layout,
			uint64_t* dot_value, uint64_t*)
  {
    this->assignment_.set_if_absolute(symtab, layout, true, *dot_value);
  }

  // Print for debugging.
  void
  print(FILE* f) const
  {
    fprintf(f, "  ");
    this->assignment_.print(f);
  }

 private:
  Symbol_assignment assignment_;
};

// An assignment to the dot symbol in a SECTIONS clause outside of an
// output section.

class Sections_element_dot_assignment : public Sections_element
{
 public:
  Sections_element_dot_assignment(Expression* val)
    : val_(val)
  { }

  // Finalize the symbol.
  void
  finalize_symbols(Symbol_table* symtab, const Layout* layout,
		   uint64_t* dot_value)
  {
    // We ignore the section of the result because outside of an
    // output section definition the dot symbol is always considered
    // to be absolute.
    Output_section* dummy;
    *dot_value = this->val_->eval_with_dot(symtab, layout, true, *dot_value,
					   NULL, &dummy);
  }

  // Update the dot symbol while setting section addresses.
  void
  set_section_addresses(Symbol_table* symtab, Layout* layout,
			uint64_t* dot_value, uint64_t* load_address)
  {
    Output_section* dummy;
    *dot_value = this->val_->eval_with_dot(symtab, layout, false, *dot_value,
					   NULL, &dummy);
    *load_address = *dot_value;
  }

  // Print for debugging.
  void
  print(FILE* f) const
  {
    fprintf(f, "  . = ");
    this->val_->print(f);
    fprintf(f, "\n");
  }

 private:
  Expression* val_;
};

// An assertion in a SECTIONS clause outside of an output section.

class Sections_element_assertion : public Sections_element
{
 public:
  Sections_element_assertion(Expression* check, const char* message,
			     size_t messagelen)
    : assertion_(check, message, messagelen)
  { }

  // Check the assertion.
  void
  finalize_symbols(Symbol_table* symtab, const Layout* layout, uint64_t*)
  { this->assertion_.check(symtab, layout); }

  // Print for debugging.
  void
  print(FILE* f) const
  {
    fprintf(f, "  ");
    this->assertion_.print(f);
  }

 private:
  Script_assertion assertion_;
};

// An element in an output section in a SECTIONS clause.

class Output_section_element
{
 public:
  // A list of input sections.
  typedef std::list<std::pair<Relobj*, unsigned int> > Input_section_list;

  Output_section_element()
  { }

  virtual ~Output_section_element()
  { }

  // Return whether this element requires an output section to exist.
  virtual bool
  needs_output_section() const
  { return false; }

  // Add any symbol being defined to the symbol table.
  virtual void
  add_symbols_to_table(Symbol_table*)
  { }

  // Finalize symbols and check assertions.
  virtual void
  finalize_symbols(Symbol_table*, const Layout*, uint64_t*, Output_section**)
  { }

  // Return whether this element matches FILE_NAME and SECTION_NAME.
  // The only real implementation is in Output_section_element_input.
  virtual bool
  match_name(const char*, const char*) const
  { return false; }

  // Set section addresses.  This includes applying assignments if the
  // the expression is an absolute value.
  virtual void
  set_section_addresses(Symbol_table*, Layout*, Output_section*, uint64_t,
			uint64_t*, Output_section**, std::string*,
			Input_section_list*)
  { }

  // Print the element for debugging purposes.
  virtual void
  print(FILE* f) const = 0;

 protected:
  // Return a fill string that is LENGTH bytes long, filling it with
  // FILL.
  std::string
  get_fill_string(const std::string* fill, section_size_type length) const;
};

std::string
Output_section_element::get_fill_string(const std::string* fill,
					section_size_type length) const
{
  std::string this_fill;
  this_fill.reserve(length);
  while (this_fill.length() + fill->length() <= length)
    this_fill += *fill;
  if (this_fill.length() < length)
    this_fill.append(*fill, 0, length - this_fill.length());
  return this_fill;
}

// A symbol assignment in an output section.

class Output_section_element_assignment : public Output_section_element
{
 public:
  Output_section_element_assignment(const char* name, size_t namelen,
				    Expression* val, bool provide,
				    bool hidden)
    : assignment_(name, namelen, val, provide, hidden)
  { }

  // Add the symbol to the symbol table.
  void
  add_symbols_to_table(Symbol_table* symtab)
  { this->assignment_.add_to_table(symtab); }

  // Finalize the symbol.
  void
  finalize_symbols(Symbol_table* symtab, const Layout* layout,
		   uint64_t* dot_value, Output_section** dot_section)
  {
    this->assignment_.finalize_with_dot(symtab, layout, *dot_value,
					*dot_section);
  }

  // Set the section address.  There is no section here, but if the
  // value is absolute, we set the symbol.  This permits us to use
  // absolute symbols when setting dot.
  void
  set_section_addresses(Symbol_table* symtab, Layout* layout, Output_section*,
			uint64_t, uint64_t* dot_value, Output_section**,
			std::string*, Input_section_list*)
  {
    this->assignment_.set_if_absolute(symtab, layout, true, *dot_value);
  }

  // Print for debugging.
  void
  print(FILE* f) const
  {
    fprintf(f, "    ");
    this->assignment_.print(f);
  }

 private:
  Symbol_assignment assignment_;
};

// An assignment to the dot symbol in an output section.

class Output_section_element_dot_assignment : public Output_section_element
{
 public:
  Output_section_element_dot_assignment(Expression* val)
    : val_(val)
  { }

  // Finalize the symbol.
  void
  finalize_symbols(Symbol_table* symtab, const Layout* layout,
		   uint64_t* dot_value, Output_section** dot_section)
  {
    *dot_value = this->val_->eval_with_dot(symtab, layout, true, *dot_value,
					   *dot_section, dot_section);
  }

  // Update the dot symbol while setting section addresses.
  void
  set_section_addresses(Symbol_table* symtab, Layout* layout, Output_section*,
			uint64_t, uint64_t* dot_value, Output_section**,
			std::string*, Input_section_list*);

  // Print for debugging.
  void
  print(FILE* f) const
  {
    fprintf(f, "    . = ");
    this->val_->print(f);
    fprintf(f, "\n");
  }

 private:
  Expression* val_;
};

// Update the dot symbol while setting section addresses.

void
Output_section_element_dot_assignment::set_section_addresses(
    Symbol_table* symtab,
    Layout* layout,
    Output_section* output_section,
    uint64_t,
    uint64_t* dot_value,
    Output_section** dot_section,
    std::string* fill,
    Input_section_list*)
{
  uint64_t next_dot = this->val_->eval_with_dot(symtab, layout, false,
						*dot_value, *dot_section,
						dot_section);
  if (next_dot < *dot_value)
    gold_error(_("dot may not move backward"));
  if (next_dot > *dot_value && output_section != NULL)
    {
      section_size_type length = convert_to_section_size_type(next_dot
							      - *dot_value);
      Output_section_data* posd;
      if (fill->empty())
	posd = new Output_data_zero_fill(length, 0);
      else
	{
	  std::string this_fill = this->get_fill_string(fill, length);
	  posd = new Output_data_const(this_fill, 0);
	}
      output_section->add_output_section_data(posd);
    }
  *dot_value = next_dot;
}

// An assertion in an output section.

class Output_section_element_assertion : public Output_section_element
{
 public:
  Output_section_element_assertion(Expression* check, const char* message,
				   size_t messagelen)
    : assertion_(check, message, messagelen)
  { }

  void
  print(FILE* f) const
  {
    fprintf(f, "    ");
    this->assertion_.print(f);
  }

 private:
  Script_assertion assertion_;
};

// We use a special instance of Output_section_data to handle BYTE,
// SHORT, etc.  This permits forward references to symbols in the
// expressions.

class Output_data_expression : public Output_section_data
{
 public:
  Output_data_expression(int size, bool is_signed, Expression* val,
			 const Symbol_table* symtab, const Layout* layout,
			 uint64_t dot_value, Output_section* dot_section)
    : Output_section_data(size, 0),
      is_signed_(is_signed), val_(val), symtab_(symtab),
      layout_(layout), dot_value_(dot_value), dot_section_(dot_section)
  { }

 protected:
  // Write the data to the output file.
  void
  do_write(Output_file*);

  // Write the data to a buffer.
  void
  do_write_to_buffer(unsigned char*);

  // Write to a map file.
  void
  do_print_to_mapfile(Mapfile* mapfile) const
  { mapfile->print_output_data(this, _("** expression")); }

 private:
  template<bool big_endian>
  void
  endian_write_to_buffer(uint64_t, unsigned char*);

  bool is_signed_;
  Expression* val_;
  const Symbol_table* symtab_;
  const Layout* layout_;
  uint64_t dot_value_;
  Output_section* dot_section_;
};

// Write the data element to the output file.

void
Output_data_expression::do_write(Output_file* of)
{
  unsigned char* view = of->get_output_view(this->offset(), this->data_size());
  this->write_to_buffer(view);
  of->write_output_view(this->offset(), this->data_size(), view);
}

// Write the data element to a buffer.

void
Output_data_expression::do_write_to_buffer(unsigned char* buf)
{
  Output_section* dummy;
  uint64_t val = this->val_->eval_with_dot(this->symtab_, this->layout_,
					   true, this->dot_value_,
					   this->dot_section_, &dummy);

  if (parameters->target().is_big_endian())
    this->endian_write_to_buffer<true>(val, buf);
  else
    this->endian_write_to_buffer<false>(val, buf);
}

template<bool big_endian>
void
Output_data_expression::endian_write_to_buffer(uint64_t val,
					       unsigned char* buf)
{
  switch (this->data_size())
    {
    case 1:
      elfcpp::Swap_unaligned<8, big_endian>::writeval(buf, val);
      break;
    case 2:
      elfcpp::Swap_unaligned<16, big_endian>::writeval(buf, val);
      break;
    case 4:
      elfcpp::Swap_unaligned<32, big_endian>::writeval(buf, val);
      break;
    case 8:
      if (parameters->target().get_size() == 32)
	{
	  val &= 0xffffffff;
	  if (this->is_signed_ && (val & 0x80000000) != 0)
	    val |= 0xffffffff00000000LL;
	}
      elfcpp::Swap_unaligned<64, big_endian>::writeval(buf, val);
      break;
    default:
      gold_unreachable();
    }
}

// A data item in an output section.

class Output_section_element_data : public Output_section_element
{
 public:
  Output_section_element_data(int size, bool is_signed, Expression* val)
    : size_(size), is_signed_(is_signed), val_(val)
  { }

  // If there is a data item, then we must create an output section.
  bool
  needs_output_section() const
  { return true; }

  // Finalize symbols--we just need to update dot.
  void
  finalize_symbols(Symbol_table*, const Layout*, uint64_t* dot_value,
		   Output_section**)
  { *dot_value += this->size_; }

  // Store the value in the section.
  void
  set_section_addresses(Symbol_table*, Layout*, Output_section*, uint64_t,
			uint64_t* dot_value, Output_section**, std::string*,
			Input_section_list*);

  // Print for debugging.
  void
  print(FILE*) const;

 private:
  // The size in bytes.
  int size_;
  // Whether the value is signed.
  bool is_signed_;
  // The value.
  Expression* val_;
};

// Store the value in the section.

void
Output_section_element_data::set_section_addresses(
    Symbol_table* symtab,
    Layout* layout,
    Output_section* os,
    uint64_t,
    uint64_t* dot_value,
    Output_section** dot_section,
    std::string*,
    Input_section_list*)
{
  gold_assert(os != NULL);
  os->add_output_section_data(new Output_data_expression(this->size_,
							 this->is_signed_,
							 this->val_,
							 symtab,
							 layout,
							 *dot_value,
							 *dot_section));
  *dot_value += this->size_;
}

// Print for debugging.

void
Output_section_element_data::print(FILE* f) const
{
  const char* s;
  switch (this->size_)
    {
    case 1:
      s = "BYTE";
      break;
    case 2:
      s = "SHORT";
      break;
    case 4:
      s = "LONG";
      break;
    case 8:
      if (this->is_signed_)
	s = "SQUAD";
      else
	s = "QUAD";
      break;
    default:
      gold_unreachable();
    }
  fprintf(f, "    %s(", s);
  this->val_->print(f);
  fprintf(f, ")\n");
}

// A fill value setting in an output section.

class Output_section_element_fill : public Output_section_element
{
 public:
  Output_section_element_fill(Expression* val)
    : val_(val)
  { }

  // Update the fill value while setting section addresses.
  void
  set_section_addresses(Symbol_table* symtab, Layout* layout, Output_section*,
			uint64_t, uint64_t* dot_value,
			Output_section** dot_section,
			std::string* fill, Input_section_list*)
  {
    Output_section* fill_section;
    uint64_t fill_val = this->val_->eval_with_dot(symtab, layout, false,
						  *dot_value, *dot_section,
						  &fill_section);
    if (fill_section != NULL)
      gold_warning(_("fill value is not absolute"));
    // FIXME: The GNU linker supports fill values of arbitrary length.
    unsigned char fill_buff[4];
    elfcpp::Swap_unaligned<32, true>::writeval(fill_buff, fill_val);
    fill->assign(reinterpret_cast<char*>(fill_buff), 4);
  }

  // Print for debugging.
  void
  print(FILE* f) const
  {
    fprintf(f, "    FILL(");
    this->val_->print(f);
    fprintf(f, ")\n");
  }

 private:
  // The new fill value.
  Expression* val_;
};

// Return whether STRING contains a wildcard character.  This is used
// to speed up matching.

static inline bool
is_wildcard_string(const std::string& s)
{
  return strpbrk(s.c_str(), "?*[") != NULL;
}

// An input section specification in an output section

class Output_section_element_input : public Output_section_element
{
 public:
  Output_section_element_input(const Input_section_spec* spec, bool keep);

  // Finalize symbols--just update the value of the dot symbol.
  void
  finalize_symbols(Symbol_table*, const Layout*, uint64_t* dot_value,
		   Output_section** dot_section)
  {
    *dot_value = this->final_dot_value_;
    *dot_section = this->final_dot_section_;
  }

  // See whether we match FILE_NAME and SECTION_NAME as an input
  // section.
  bool
  match_name(const char* file_name, const char* section_name) const;

  // Set the section address.
  void
  set_section_addresses(Symbol_table* symtab, Layout* layout, Output_section*,
			uint64_t subalign, uint64_t* dot_value,
			Output_section**, std::string* fill,
			Input_section_list*);

  // Print for debugging.
  void
  print(FILE* f) const;

 private:
  // An input section pattern.
  struct Input_section_pattern
  {
    std::string pattern;
    bool pattern_is_wildcard;
    Sort_wildcard sort;

    Input_section_pattern(const char* patterna, size_t patternlena,
			  Sort_wildcard sorta)
      : pattern(patterna, patternlena),
	pattern_is_wildcard(is_wildcard_string(this->pattern)),
	sort(sorta)
    { }
  };

  typedef std::vector<Input_section_pattern> Input_section_patterns;

  // Filename_exclusions is a pair of filename pattern and a bool
  // indicating whether the filename is a wildcard.
  typedef std::vector<std::pair<std::string, bool> > Filename_exclusions;

  // Return whether STRING matches PATTERN, where IS_WILDCARD_PATTERN
  // indicates whether this is a wildcard pattern.
  static inline bool
  match(const char* string, const char* pattern, bool is_wildcard_pattern)
  {
    return (is_wildcard_pattern
	    ? fnmatch(pattern, string, 0) == 0
	    : strcmp(string, pattern) == 0);
  }

  // See if we match a file name.
  bool
  match_file_name(const char* file_name) const;

  // The file name pattern.  If this is the empty string, we match all
  // files.
  std::string filename_pattern_;
  // Whether the file name pattern is a wildcard.
  bool filename_is_wildcard_;
  // How the file names should be sorted.  This may only be
  // SORT_WILDCARD_NONE or SORT_WILDCARD_BY_NAME.
  Sort_wildcard filename_sort_;
  // The list of file names to exclude.
  Filename_exclusions filename_exclusions_;
  // The list of input section patterns.
  Input_section_patterns input_section_patterns_;
  // Whether to keep this section when garbage collecting.
  bool keep_;
  // The value of dot after including all matching sections.
  uint64_t final_dot_value_;
  // The section where dot is defined after including all matching
  // sections.
  Output_section* final_dot_section_;
};

// Construct Output_section_element_input.  The parser records strings
// as pointers into a copy of the script file, which will go away when
// parsing is complete.  We make sure they are in std::string objects.

Output_section_element_input::Output_section_element_input(
    const Input_section_spec* spec,
    bool keep)
  : filename_pattern_(),
    filename_is_wildcard_(false),
    filename_sort_(spec->file.sort),
    filename_exclusions_(),
    input_section_patterns_(),
    keep_(keep),
    final_dot_value_(0),
    final_dot_section_(NULL)
{
  // The filename pattern "*" is common, and matches all files.  Turn
  // it into the empty string.
  if (spec->file.name.length != 1 || spec->file.name.value[0] != '*')
    this->filename_pattern_.assign(spec->file.name.value,
				   spec->file.name.length);
  this->filename_is_wildcard_ = is_wildcard_string(this->filename_pattern_);

  if (spec->input_sections.exclude != NULL)
    {
      for (String_list::const_iterator p =
	     spec->input_sections.exclude->begin();
	   p != spec->input_sections.exclude->end();
	   ++p)
	{
	  bool is_wildcard = is_wildcard_string(*p);
	  this->filename_exclusions_.push_back(std::make_pair(*p,
							      is_wildcard));
	}
    }

  if (spec->input_sections.sections != NULL)
    {
      Input_section_patterns& isp(this->input_section_patterns_);
      for (String_sort_list::const_iterator p =
	     spec->input_sections.sections->begin();
	   p != spec->input_sections.sections->end();
	   ++p)
	isp.push_back(Input_section_pattern(p->name.value, p->name.length,
					    p->sort));
    }
}

// See whether we match FILE_NAME.

bool
Output_section_element_input::match_file_name(const char* file_name) const
{
  if (!this->filename_pattern_.empty())
    {
      // If we were called with no filename, we refuse to match a
      // pattern which requires a file name.
      if (file_name == NULL)
	return false;

      if (!match(file_name, this->filename_pattern_.c_str(),
		 this->filename_is_wildcard_))
	return false;
    }

  if (file_name != NULL)
    {
      // Now we have to see whether FILE_NAME matches one of the
      // exclusion patterns, if any.
      for (Filename_exclusions::const_iterator p =
	     this->filename_exclusions_.begin();
	   p != this->filename_exclusions_.end();
	   ++p)
	{
	  if (match(file_name, p->first.c_str(), p->second))
	    return false;
	}
    }

  return true;
}

// See whether we match FILE_NAME and SECTION_NAME.

bool
Output_section_element_input::match_name(const char* file_name,
					 const char* section_name) const
{
  if (!this->match_file_name(file_name))
    return false;

  // If there are no section name patterns, then we match.
  if (this->input_section_patterns_.empty())
    return true;

  // See whether we match the section name patterns.
  for (Input_section_patterns::const_iterator p =
	 this->input_section_patterns_.begin();
       p != this->input_section_patterns_.end();
       ++p)
    {
      if (match(section_name, p->pattern.c_str(), p->pattern_is_wildcard))
	return true;
    }

  // We didn't match any section names, so we didn't match.
  return false;
}

// Information we use to sort the input sections.

struct Input_section_info
{
  Relobj* relobj;
  unsigned int shndx;
  std::string section_name;
  uint64_t size;
  uint64_t addralign;
};

// A class to sort the input sections.

class Input_section_sorter
{
 public:
  Input_section_sorter(Sort_wildcard filename_sort, Sort_wildcard section_sort)
    : filename_sort_(filename_sort), section_sort_(section_sort)
  { }

  bool
  operator()(const Input_section_info&, const Input_section_info&) const;

 private:
  Sort_wildcard filename_sort_;
  Sort_wildcard section_sort_;
};

bool
Input_section_sorter::operator()(const Input_section_info& isi1,
				 const Input_section_info& isi2) const
{
  if (this->section_sort_ == SORT_WILDCARD_BY_NAME
      || this->section_sort_ == SORT_WILDCARD_BY_NAME_BY_ALIGNMENT
      || (this->section_sort_ == SORT_WILDCARD_BY_ALIGNMENT_BY_NAME
	  && isi1.addralign == isi2.addralign))
    {
      if (isi1.section_name != isi2.section_name)
	return isi1.section_name < isi2.section_name;
    }
  if (this->section_sort_ == SORT_WILDCARD_BY_ALIGNMENT
      || this->section_sort_ == SORT_WILDCARD_BY_NAME_BY_ALIGNMENT
      || this->section_sort_ == SORT_WILDCARD_BY_ALIGNMENT_BY_NAME)
    {
      if (isi1.addralign != isi2.addralign)
	return isi1.addralign < isi2.addralign;
    }
  if (this->filename_sort_ == SORT_WILDCARD_BY_NAME)
    {
      if (isi1.relobj->name() != isi2.relobj->name())
	return isi1.relobj->name() < isi2.relobj->name();
    }

  // Otherwise we leave them in the same order.
  return false;
}

// Set the section address.  Look in INPUT_SECTIONS for sections which
// match this spec, sort them as specified, and add them to the output
// section.

void
Output_section_element_input::set_section_addresses(
    Symbol_table*,
    Layout*,
    Output_section* output_section,
    uint64_t subalign,
    uint64_t* dot_value,
    Output_section** dot_section,
    std::string* fill,
    Input_section_list* input_sections)
{
  // We build a list of sections which match each
  // Input_section_pattern.

  typedef std::vector<std::vector<Input_section_info> > Matching_sections;
  size_t input_pattern_count = this->input_section_patterns_.size();
  if (input_pattern_count == 0)
    input_pattern_count = 1;
  Matching_sections matching_sections(input_pattern_count);

  // Look through the list of sections for this output section.  Add
  // each one which matches to one of the elements of
  // MATCHING_SECTIONS.

  Input_section_list::iterator p = input_sections->begin();
  while (p != input_sections->end())
    {
      // Calling section_name and section_addralign is not very
      // efficient.
      Input_section_info isi;
      isi.relobj = p->first;
      isi.shndx = p->second;

      // Lock the object so that we can get information about the
      // section.  This is OK since we know we are single-threaded
      // here.
      {
	const Task* task = reinterpret_cast<const Task*>(-1);
	Task_lock_obj<Object> tl(task, p->first);

	isi.section_name = p->first->section_name(p->second);
	isi.size = p->first->section_size(p->second);
	isi.addralign = p->first->section_addralign(p->second);
      }

      if (!this->match_file_name(isi.relobj->name().c_str()))
	++p;
      else if (this->input_section_patterns_.empty())
	{
	  matching_sections[0].push_back(isi);
	  p = input_sections->erase(p);
	}
      else
	{
	  size_t i;
	  for (i = 0; i < input_pattern_count; ++i)
	    {
	      const Input_section_pattern&
		isp(this->input_section_patterns_[i]);
	      if (match(isi.section_name.c_str(), isp.pattern.c_str(),
			isp.pattern_is_wildcard))
		break;
	    }

	  if (i >= this->input_section_patterns_.size())
	    ++p;
	  else
	    {
	      matching_sections[i].push_back(isi);
	      p = input_sections->erase(p);
	    }
	}
    }

  // Look through MATCHING_SECTIONS.  Sort each one as specified,
  // using a stable sort so that we get the default order when
  // sections are otherwise equal.  Add each input section to the
  // output section.

  for (size_t i = 0; i < input_pattern_count; ++i)
    {
      if (matching_sections[i].empty())
	continue;

      gold_assert(output_section != NULL);

      const Input_section_pattern& isp(this->input_section_patterns_[i]);
      if (isp.sort != SORT_WILDCARD_NONE
	  || this->filename_sort_ != SORT_WILDCARD_NONE)
	std::stable_sort(matching_sections[i].begin(),
			 matching_sections[i].end(),
			 Input_section_sorter(this->filename_sort_,
					      isp.sort));

      for (std::vector<Input_section_info>::const_iterator p =
	     matching_sections[i].begin();
	   p != matching_sections[i].end();
	   ++p)
	{
	  uint64_t this_subalign = p->addralign;
	  if (this_subalign < subalign)
	    this_subalign = subalign;

	  uint64_t address = align_address(*dot_value, this_subalign);

	  if (address > *dot_value && !fill->empty())
	    {
	      section_size_type length =
		convert_to_section_size_type(address - *dot_value);
	      std::string this_fill = this->get_fill_string(fill, length);
	      Output_section_data* posd = new Output_data_const(this_fill, 0);
	      output_section->add_output_section_data(posd);
	    }

	  output_section->add_input_section_for_script(p->relobj,
						       p->shndx,
						       p->size,
						       this_subalign);

	  *dot_value = address + p->size;
	}
    }

  this->final_dot_value_ = *dot_value;
  this->final_dot_section_ = *dot_section;
}

// Print for debugging.

void
Output_section_element_input::print(FILE* f) const
{
  fprintf(f, "    ");

  if (this->keep_)
    fprintf(f, "KEEP(");

  if (!this->filename_pattern_.empty())
    {
      bool need_close_paren = false;
      switch (this->filename_sort_)
	{
	case SORT_WILDCARD_NONE:
	  break;
	case SORT_WILDCARD_BY_NAME:
	  fprintf(f, "SORT_BY_NAME(");
	  need_close_paren = true;
	  break;
	default:
	  gold_unreachable();
	}

      fprintf(f, "%s", this->filename_pattern_.c_str());

      if (need_close_paren)
	fprintf(f, ")");
    }

  if (!this->input_section_patterns_.empty()
      || !this->filename_exclusions_.empty())
    {
      fprintf(f, "(");

      bool need_space = false;
      if (!this->filename_exclusions_.empty())
	{
	  fprintf(f, "EXCLUDE_FILE(");
	  bool need_comma = false;
	  for (Filename_exclusions::const_iterator p =
		 this->filename_exclusions_.begin();
	       p != this->filename_exclusions_.end();
	       ++p)
	    {
	      if (need_comma)
		fprintf(f, ", ");
	      fprintf(f, "%s", p->first.c_str());
	      need_comma = true;
	    }
	  fprintf(f, ")");
	  need_space = true;
	}

      for (Input_section_patterns::const_iterator p =
	     this->input_section_patterns_.begin();
	   p != this->input_section_patterns_.end();
	   ++p)
	{
	  if (need_space)
	    fprintf(f, " ");

	  int close_parens = 0;
	  switch (p->sort)
	    {
	    case SORT_WILDCARD_NONE:
	      break;
	    case SORT_WILDCARD_BY_NAME:
	      fprintf(f, "SORT_BY_NAME(");
	      close_parens = 1;
	      break;
	    case SORT_WILDCARD_BY_ALIGNMENT:
	      fprintf(f, "SORT_BY_ALIGNMENT(");
	      close_parens = 1;
	      break;
	    case SORT_WILDCARD_BY_NAME_BY_ALIGNMENT:
	      fprintf(f, "SORT_BY_NAME(SORT_BY_ALIGNMENT(");
	      close_parens = 2;
	      break;
	    case SORT_WILDCARD_BY_ALIGNMENT_BY_NAME:
	      fprintf(f, "SORT_BY_ALIGNMENT(SORT_BY_NAME(");
	      close_parens = 2;
	      break;
	    default:
	      gold_unreachable();
	    }

	  fprintf(f, "%s", p->pattern.c_str());

	  for (int i = 0; i < close_parens; ++i)
	    fprintf(f, ")");

	  need_space = true;
	}

      fprintf(f, ")");
    }

  if (this->keep_)
    fprintf(f, ")");

  fprintf(f, "\n");
}

// An output section.

class Output_section_definition : public Sections_element
{
 public:
  typedef Output_section_element::Input_section_list Input_section_list;

  Output_section_definition(const char* name, size_t namelen,
			    const Parser_output_section_header* header);

  // Finish the output section with the information in the trailer.
  void
  finish(const Parser_output_section_trailer* trailer);

  // Add a symbol to be defined.
  void
  add_symbol_assignment(const char* name, size_t length, Expression* value,
			bool provide, bool hidden);

  // Add an assignment to the special dot symbol.
  void
  add_dot_assignment(Expression* value);

  // Add an assertion.
  void
  add_assertion(Expression* check, const char* message, size_t messagelen);

  // Add a data item to the current output section.
  void
  add_data(int size, bool is_signed, Expression* val);

  // Add a setting for the fill value.
  void
  add_fill(Expression* val);

  // Add an input section specification.
  void
  add_input_section(const Input_section_spec* spec, bool keep);

  // Record that the output section is relro.
  void
  set_is_relro()
  { this->is_relro_ = true; }

  // Create any required output sections.
  void
  create_sections(Layout*);

  // Add any symbols being defined to the symbol table.
  void
  add_symbols_to_table(Symbol_table* symtab);

  // Finalize symbols and check assertions.
  void
  finalize_symbols(Symbol_table*, const Layout*, uint64_t*);

  // Return the output section name to use for an input file name and
  // section name.
  const char*
  output_section_name(const char* file_name, const char* section_name,
		      Output_section***);

  // Return whether to place an orphan section after this one.
  bool
  place_orphan_here(const Output_section *os, bool* exact, bool*) const;

  // Set the section address.
  void
  set_section_addresses(Symbol_table* symtab, Layout* layout,
			uint64_t* dot_value, uint64_t* load_address);

  // Check a constraint (ONLY_IF_RO, etc.) on an output section.  If
  // this section is constrained, and the input sections do not match,
  // return the constraint, and set *POSD.
  Section_constraint
  check_constraint(Output_section_definition** posd);

  // See if this is the alternate output section for a constrained
  // output section.  If it is, transfer the Output_section and return
  // true.  Otherwise return false.
  bool
  alternate_constraint(Output_section_definition*, Section_constraint);

  // Get the list of segments to use for an allocated section when
  // using a PHDRS clause.
  Output_section*
  allocate_to_segment(String_list** phdrs_list, bool* orphan);

  // Look for an output section by name and return the address, the
  // load address, the alignment, and the size.  This is used when an
  // expression refers to an output section which was not actually
  // created.  This returns true if the section was found, false
  // otherwise.
  bool
  get_output_section_info(const char*, uint64_t*, uint64_t*, uint64_t*,
                          uint64_t*) const;

  // Return the associated Output_section if there is one.
  Output_section*
  get_output_section() const
  { return this->output_section_; }

  // Print the contents to the FILE.  This is for debugging.
  void
  print(FILE*) const;

 private:
  typedef std::vector<Output_section_element*> Output_section_elements;

  // The output section name.
  std::string name_;
  // The address.  This may be NULL.
  Expression* address_;
  // The load address.  This may be NULL.
  Expression* load_address_;
  // The alignment.  This may be NULL.
  Expression* align_;
  // The input section alignment.  This may be NULL.
  Expression* subalign_;
  // The constraint, if any.
  Section_constraint constraint_;
  // The fill value.  This may be NULL.
  Expression* fill_;
  // The list of segments this section should go into.  This may be
  // NULL.
  String_list* phdrs_;
  // The list of elements defining the section.
  Output_section_elements elements_;
  // The Output_section created for this definition.  This will be
  // NULL if none was created.
  Output_section* output_section_;
  // The address after it has been evaluated.
  uint64_t evaluated_address_;
  // The load address after it has been evaluated.
  uint64_t evaluated_load_address_;
  // The alignment after it has been evaluated.
  uint64_t evaluated_addralign_;
  // The output section is relro.
  bool is_relro_;
};

// Constructor.

Output_section_definition::Output_section_definition(
    const char* name,
    size_t namelen,
    const Parser_output_section_header* header)
  : name_(name, namelen),
    address_(header->address),
    load_address_(header->load_address),
    align_(header->align),
    subalign_(header->subalign),
    constraint_(header->constraint),
    fill_(NULL),
    phdrs_(NULL),
    elements_(),
    output_section_(NULL),
    evaluated_address_(0),
    evaluated_load_address_(0),
    evaluated_addralign_(0),
    is_relro_(false)
{
}

// Finish an output section.

void
Output_section_definition::finish(const Parser_output_section_trailer* trailer)
{
  this->fill_ = trailer->fill;
  this->phdrs_ = trailer->phdrs;
}

// Add a symbol to be defined.

void
Output_section_definition::add_symbol_assignment(const char* name,
						 size_t length,
						 Expression* value,
						 bool provide,
						 bool hidden)
{
  Output_section_element* p = new Output_section_element_assignment(name,
								    length,
								    value,
								    provide,
								    hidden);
  this->elements_.push_back(p);
}

// Add an assignment to the special dot symbol.

void
Output_section_definition::add_dot_assignment(Expression* value)
{
  Output_section_element* p = new Output_section_element_dot_assignment(value);
  this->elements_.push_back(p);
}

// Add an assertion.

void
Output_section_definition::add_assertion(Expression* check,
					 const char* message,
					 size_t messagelen)
{
  Output_section_element* p = new Output_section_element_assertion(check,
								   message,
								   messagelen);
  this->elements_.push_back(p);
}

// Add a data item to the current output section.

void
Output_section_definition::add_data(int size, bool is_signed, Expression* val)
{
  Output_section_element* p = new Output_section_element_data(size, is_signed,
							      val);
  this->elements_.push_back(p);
}

// Add a setting for the fill value.

void
Output_section_definition::add_fill(Expression* val)
{
  Output_section_element* p = new Output_section_element_fill(val);
  this->elements_.push_back(p);
}

// Add an input section specification.

void
Output_section_definition::add_input_section(const Input_section_spec* spec,
					     bool keep)
{
  Output_section_element* p = new Output_section_element_input(spec, keep);
  this->elements_.push_back(p);
}

// Create any required output sections.  We need an output section if
// there is a data statement here.

void
Output_section_definition::create_sections(Layout* layout)
{
  if (this->output_section_ != NULL)
    return;
  for (Output_section_elements::const_iterator p = this->elements_.begin();
       p != this->elements_.end();
       ++p)
    {
      if ((*p)->needs_output_section())
	{
	  const char* name = this->name_.c_str();
	  this->output_section_ = layout->make_output_section_for_script(name);
	  return;
	}
    }
}

// Add any symbols being defined to the symbol table.

void
Output_section_definition::add_symbols_to_table(Symbol_table* symtab)
{
  for (Output_section_elements::iterator p = this->elements_.begin();
       p != this->elements_.end();
       ++p)
    (*p)->add_symbols_to_table(symtab);
}

// Finalize symbols and check assertions.

void
Output_section_definition::finalize_symbols(Symbol_table* symtab,
					    const Layout* layout,
					    uint64_t* dot_value)
{
  if (this->output_section_ != NULL)
    *dot_value = this->output_section_->address();
  else
    {
      uint64_t address = *dot_value;
      if (this->address_ != NULL)
	{
	  Output_section* dummy;
	  address = this->address_->eval_with_dot(symtab, layout, true,
						  *dot_value, NULL,
						  &dummy);
	}
      if (this->align_ != NULL)
	{
	  Output_section* dummy;
	  uint64_t align = this->align_->eval_with_dot(symtab, layout, true,
						       *dot_value,
						       NULL,
						       &dummy);
	  address = align_address(address, align);
	}
      *dot_value = address;
    }

  Output_section* dot_section = this->output_section_;
  for (Output_section_elements::iterator p = this->elements_.begin();
       p != this->elements_.end();
       ++p)
    (*p)->finalize_symbols(symtab, layout, dot_value, &dot_section);
}

// Return the output section name to use for an input section name.

const char*
Output_section_definition::output_section_name(const char* file_name,
					       const char* section_name,
					       Output_section*** slot)
{
  // Ask each element whether it matches NAME.
  for (Output_section_elements::const_iterator p = this->elements_.begin();
       p != this->elements_.end();
       ++p)
    {
      if ((*p)->match_name(file_name, section_name))
	{
	  // We found a match for NAME, which means that it should go
	  // into this output section.
	  *slot = &this->output_section_;
	  return this->name_.c_str();
	}
    }

  // We don't know about this section name.
  return NULL;
}

// Return whether to place an orphan output section after this
// section.

bool
Output_section_definition::place_orphan_here(const Output_section *os,
					     bool* exact,
					     bool* is_relro) const
{
  *is_relro = this->is_relro_;

  // Check for the simple case first.
  if (this->output_section_ != NULL
      && this->output_section_->type() == os->type()
      && this->output_section_->flags() == os->flags())
    {
      *exact = true;
      return true;
    }

  // Otherwise use some heuristics.

  if ((os->flags() & elfcpp::SHF_ALLOC) == 0)
    return false;

  if (os->type() == elfcpp::SHT_NOBITS)
    {
      if (this->name_ == ".bss")
	{
	  *exact = true;
	  return true;
	}
      if (this->output_section_ != NULL
	  && this->output_section_->type() == elfcpp::SHT_NOBITS)
	return true;
    }
  else if (os->type() == elfcpp::SHT_NOTE)
    {
      if (this->output_section_ != NULL
	  && this->output_section_->type() == elfcpp::SHT_NOTE)
	{
	  *exact = true;
	  return true;
	}
      if (this->name_.compare(0, 5, ".note") == 0)
	{
	  *exact = true;
	  return true;
	}
      if (this->name_ == ".interp")
	return true;
      if (this->output_section_ != NULL
	  && this->output_section_->type() == elfcpp::SHT_PROGBITS
	  && (this->output_section_->flags() & elfcpp::SHF_WRITE) == 0)
	return true;
    }
  else if (os->type() == elfcpp::SHT_REL || os->type() == elfcpp::SHT_RELA)
    {
      if (this->name_.compare(0, 4, ".rel") == 0)
	{
	  *exact = true;
	  return true;
	}
      if (this->output_section_ != NULL
	  && (this->output_section_->type() == elfcpp::SHT_REL
	      || this->output_section_->type() == elfcpp::SHT_RELA))
	{
	  *exact = true;
	  return true;
	}
      if (this->output_section_ != NULL
	  && this->output_section_->type() == elfcpp::SHT_PROGBITS
	  && (this->output_section_->flags() & elfcpp::SHF_WRITE) == 0)
	return true;
    }
  else if (os->type() == elfcpp::SHT_PROGBITS
	   && (os->flags() & elfcpp::SHF_WRITE) != 0)
    {
      if (this->name_ == ".data")
	{
	  *exact = true;
	  return true;
	}
      if (this->output_section_ != NULL
	  && this->output_section_->type() == elfcpp::SHT_PROGBITS
	  && (this->output_section_->flags() & elfcpp::SHF_WRITE) != 0)
	return true;
    }
  else if (os->type() == elfcpp::SHT_PROGBITS
	   && (os->flags() & elfcpp::SHF_EXECINSTR) != 0)
    {
      if (this->name_ == ".text")
	{
	  *exact = true;
	  return true;
	}
      if (this->output_section_ != NULL
	  && this->output_section_->type() == elfcpp::SHT_PROGBITS
	  && (this->output_section_->flags() & elfcpp::SHF_EXECINSTR) != 0)
	return true;
    }
  else if (os->type() == elfcpp::SHT_PROGBITS
	   || (os->type() != elfcpp::SHT_PROGBITS
	       && (os->flags() & elfcpp::SHF_WRITE) == 0))
    {
      if (this->name_ == ".rodata")
	{
	  *exact = true;
	  return true;
	}
      if (this->output_section_ != NULL
	  && this->output_section_->type() == elfcpp::SHT_PROGBITS
	  && (this->output_section_->flags() & elfcpp::SHF_WRITE) == 0)
	return true;
    }

  return false;
}

// Set the section address.  Note that the OUTPUT_SECTION_ field will
// be NULL if no input sections were mapped to this output section.
// We still have to adjust dot and process symbol assignments.

void
Output_section_definition::set_section_addresses(Symbol_table* symtab,
						 Layout* layout,
						 uint64_t* dot_value,
                                                 uint64_t* load_address)
{
  uint64_t address;
  if (this->address_ == NULL)
    address = *dot_value;
  else
    {
      Output_section* dummy;
      address = this->address_->eval_with_dot(symtab, layout, true,
					      *dot_value, NULL, &dummy);
    }

  uint64_t align;
  if (this->align_ == NULL)
    {
      if (this->output_section_ == NULL)
	align = 0;
      else
	align = this->output_section_->addralign();
    }
  else
    {
      Output_section* align_section;
      align = this->align_->eval_with_dot(symtab, layout, true, *dot_value,
					  NULL, &align_section);
      if (align_section != NULL)
	gold_warning(_("alignment of section %s is not absolute"),
		     this->name_.c_str());
      if (this->output_section_ != NULL)
	this->output_section_->set_addralign(align);
    }

  address = align_address(address, align);

  uint64_t start_address = address;

  *dot_value = address;

  // The address of non-SHF_ALLOC sections is forced to zero,
  // regardless of what the linker script wants.
  if (this->output_section_ != NULL
      && (this->output_section_->flags() & elfcpp::SHF_ALLOC) != 0)
    this->output_section_->set_address(address);

  this->evaluated_address_ = address;
  this->evaluated_addralign_ = align;

  if (this->load_address_ == NULL)
    this->evaluated_load_address_ = address;
  else
    {
      Output_section* dummy;
      uint64_t load_address =
	this->load_address_->eval_with_dot(symtab, layout, true, *dot_value,
					   this->output_section_, &dummy);
      if (this->output_section_ != NULL)
        this->output_section_->set_load_address(load_address);
      this->evaluated_load_address_ = load_address;
    }

  uint64_t subalign;
  if (this->subalign_ == NULL)
    subalign = 0;
  else
    {
      Output_section* subalign_section;
      subalign = this->subalign_->eval_with_dot(symtab, layout, true,
						*dot_value, NULL,
						&subalign_section);
      if (subalign_section != NULL)
	gold_warning(_("subalign of section %s is not absolute"),
		     this->name_.c_str());
    }

  std::string fill;
  if (this->fill_ != NULL)
    {
      // FIXME: The GNU linker supports fill values of arbitrary
      // length.
      Output_section* fill_section;
      uint64_t fill_val = this->fill_->eval_with_dot(symtab, layout, true,
						     *dot_value,
						     NULL,
						     &fill_section);
      if (fill_section != NULL)
	gold_warning(_("fill of section %s is not absolute"),
		     this->name_.c_str());
      unsigned char fill_buff[4];
      elfcpp::Swap_unaligned<32, true>::writeval(fill_buff, fill_val);
      fill.assign(reinterpret_cast<char*>(fill_buff), 4);
    }

  Input_section_list input_sections;
  if (this->output_section_ != NULL)
    {
      // Get the list of input sections attached to this output
      // section.  This will leave the output section with only
      // Output_section_data entries.
      address += this->output_section_->get_input_sections(address,
							   fill,
							   &input_sections);
      *dot_value = address;
    }

  Output_section* dot_section = this->output_section_;
  for (Output_section_elements::iterator p = this->elements_.begin();
       p != this->elements_.end();
       ++p)
    (*p)->set_section_addresses(symtab, layout, this->output_section_,
				subalign, dot_value, &dot_section, &fill,
				&input_sections);

  gold_assert(input_sections.empty());

  if (this->load_address_ == NULL || this->output_section_ == NULL)
    *load_address = *dot_value;
  else
    *load_address = (this->output_section_->load_address()
                     + (*dot_value - start_address));

  if (this->output_section_ != NULL)
    {
      if (this->is_relro_)
	this->output_section_->set_is_relro();
      else
	this->output_section_->clear_is_relro();
    }
}

// Check a constraint (ONLY_IF_RO, etc.) on an output section.  If
// this section is constrained, and the input sections do not match,
// return the constraint, and set *POSD.

Section_constraint
Output_section_definition::check_constraint(Output_section_definition** posd)
{
  switch (this->constraint_)
    {
    case CONSTRAINT_NONE:
      return CONSTRAINT_NONE;

    case CONSTRAINT_ONLY_IF_RO:
      if (this->output_section_ != NULL
	  && (this->output_section_->flags() & elfcpp::SHF_WRITE) != 0)
	{
	  *posd = this;
	  return CONSTRAINT_ONLY_IF_RO;
	}
      return CONSTRAINT_NONE;

    case CONSTRAINT_ONLY_IF_RW:
      if (this->output_section_ != NULL
	  && (this->output_section_->flags() & elfcpp::SHF_WRITE) == 0)
	{
	  *posd = this;
	  return CONSTRAINT_ONLY_IF_RW;
	}
      return CONSTRAINT_NONE;

    case CONSTRAINT_SPECIAL:
      if (this->output_section_ != NULL)
	gold_error(_("SPECIAL constraints are not implemented"));
      return CONSTRAINT_NONE;

    default:
      gold_unreachable();
    }
}

// See if this is the alternate output section for a constrained
// output section.  If it is, transfer the Output_section and return
// true.  Otherwise return false.

bool
Output_section_definition::alternate_constraint(
    Output_section_definition* posd,
    Section_constraint constraint)
{
  if (this->name_ != posd->name_)
    return false;

  switch (constraint)
    {
    case CONSTRAINT_ONLY_IF_RO:
      if (this->constraint_ != CONSTRAINT_ONLY_IF_RW)
	return false;
      break;

    case CONSTRAINT_ONLY_IF_RW:
      if (this->constraint_ != CONSTRAINT_ONLY_IF_RO)
	return false;
      break;

    default:
      gold_unreachable();
    }

  // We have found the alternate constraint.  We just need to move
  // over the Output_section.  When constraints are used properly,
  // THIS should not have an output_section pointer, as all the input
  // sections should have matched the other definition.

  if (this->output_section_ != NULL)
    gold_error(_("mismatched definition for constrained sections"));

  this->output_section_ = posd->output_section_;
  posd->output_section_ = NULL;

  if (this->is_relro_)
    this->output_section_->set_is_relro();
  else
    this->output_section_->clear_is_relro();

  return true;
}

// Get the list of segments to use for an allocated section when using
// a PHDRS clause.

Output_section*
Output_section_definition::allocate_to_segment(String_list** phdrs_list,
					       bool* orphan)
{
  if (this->output_section_ == NULL)
    return NULL;
  if ((this->output_section_->flags() & elfcpp::SHF_ALLOC) == 0)
    return NULL;
  *orphan = false;
  if (this->phdrs_ != NULL)
    *phdrs_list = this->phdrs_;
  return this->output_section_;
}

// Look for an output section by name and return the address, the load
// address, the alignment, and the size.  This is used when an
// expression refers to an output section which was not actually
// created.  This returns true if the section was found, false
// otherwise.

bool
Output_section_definition::get_output_section_info(const char* name,
                                                   uint64_t* address,
                                                   uint64_t* load_address,
                                                   uint64_t* addralign,
                                                   uint64_t* size) const
{
  if (this->name_ != name)
    return false;

  if (this->output_section_ != NULL)
    {
      *address = this->output_section_->address();
      if (this->output_section_->has_load_address())
        *load_address = this->output_section_->load_address();
      else
        *load_address = *address;
      *addralign = this->output_section_->addralign();
      *size = this->output_section_->current_data_size();
    }
  else
    {
      *address = this->evaluated_address_;
      *load_address = this->evaluated_load_address_;
      *addralign = this->evaluated_addralign_;
      *size = 0;
    }

  return true;
}

// Print for debugging.

void
Output_section_definition::print(FILE* f) const
{
  fprintf(f, "  %s ", this->name_.c_str());

  if (this->address_ != NULL)
    {
      this->address_->print(f);
      fprintf(f, " ");
    }

  fprintf(f, ": ");

  if (this->load_address_ != NULL)
    {
      fprintf(f, "AT(");
      this->load_address_->print(f);
      fprintf(f, ") ");
    }

  if (this->align_ != NULL)
    {
      fprintf(f, "ALIGN(");
      this->align_->print(f);
      fprintf(f, ") ");
    }

  if (this->subalign_ != NULL)
    {
      fprintf(f, "SUBALIGN(");
      this->subalign_->print(f);
      fprintf(f, ") ");
    }

  fprintf(f, "{\n");

  for (Output_section_elements::const_iterator p = this->elements_.begin();
       p != this->elements_.end();
       ++p)
    (*p)->print(f);

  fprintf(f, "  }");

  if (this->fill_ != NULL)
    {
      fprintf(f, " = ");
      this->fill_->print(f);
    }

  if (this->phdrs_ != NULL)
    {
      for (String_list::const_iterator p = this->phdrs_->begin();
	   p != this->phdrs_->end();
	   ++p)
	fprintf(f, " :%s", p->c_str());
    }

  fprintf(f, "\n");
}

// An output section created to hold orphaned input sections.  These
// do not actually appear in linker scripts.  However, for convenience
// when setting the output section addresses, we put a marker to these
// sections in the appropriate place in the list of SECTIONS elements.

class Orphan_output_section : public Sections_element
{
 public:
  Orphan_output_section(Output_section* os)
    : os_(os)
  { }

  // Return whether to place an orphan section after this one.
  bool
  place_orphan_here(const Output_section *os, bool* exact, bool*) const;

  // Set section addresses.
  void
  set_section_addresses(Symbol_table*, Layout*, uint64_t*, uint64_t*);

  // Get the list of segments to use for an allocated section when
  // using a PHDRS clause.
  Output_section*
  allocate_to_segment(String_list**, bool*);

  // Return the associated Output_section.
  Output_section*
  get_output_section() const
  { return this->os_; }

  // Print for debugging.
  void
  print(FILE* f) const
  {
    fprintf(f, "  marker for orphaned output section %s\n",
	    this->os_->name());
  }

 private:
  Output_section* os_;
};

// Whether to place another orphan section after this one.

bool
Orphan_output_section::place_orphan_here(const Output_section* os,
					 bool* exact,
					 bool* is_relro) const
{
  if (this->os_->type() == os->type()
      && this->os_->flags() == os->flags())
    {
      *exact = true;
      *is_relro = this->os_->is_relro();
      return true;
    }
  return false;
}

// Set section addresses.

void
Orphan_output_section::set_section_addresses(Symbol_table*, Layout*,
					     uint64_t* dot_value,
                                             uint64_t* load_address)
{
  typedef std::list<std::pair<Relobj*, unsigned int> > Input_section_list;

  bool have_load_address = *load_address != *dot_value;

  uint64_t address = *dot_value;
  address = align_address(address, this->os_->addralign());

  if ((this->os_->flags() & elfcpp::SHF_ALLOC) != 0)
    {
      this->os_->set_address(address);
      if (have_load_address)
        this->os_->set_load_address(align_address(*load_address,
                                                  this->os_->addralign()));
    }

  Input_section_list input_sections;
  address += this->os_->get_input_sections(address, "", &input_sections);

  for (Input_section_list::iterator p = input_sections.begin();
       p != input_sections.end();
       ++p)
    {
      uint64_t addralign;
      uint64_t size;

      // We know what are single-threaded, so it is OK to lock the
      // object.
      {
	const Task* task = reinterpret_cast<const Task*>(-1);
	Task_lock_obj<Object> tl(task, p->first);
	addralign = p->first->section_addralign(p->second);
	size = p->first->section_size(p->second);
      }

      address = align_address(address, addralign);
      this->os_->add_input_section_for_script(p->first, p->second, size,
                                              addralign);
      address += size;
    }

  if (!have_load_address)
    *load_address = address;
  else
    *load_address += address - *dot_value;

  *dot_value = address;
}

// Get the list of segments to use for an allocated section when using
// a PHDRS clause.  If this is an allocated section, return the
// Output_section.  We don't change the list of segments.

Output_section*
Orphan_output_section::allocate_to_segment(String_list**, bool* orphan)
{
  if ((this->os_->flags() & elfcpp::SHF_ALLOC) == 0)
    return NULL;
  *orphan = true;
  return this->os_;
}

// Class Phdrs_element.  A program header from a PHDRS clause.

class Phdrs_element
{
 public:
  Phdrs_element(const char* name, size_t namelen, unsigned int type,
		bool includes_filehdr, bool includes_phdrs,
		bool is_flags_valid, unsigned int flags,
		Expression* load_address)
    : name_(name, namelen), type_(type), includes_filehdr_(includes_filehdr),
      includes_phdrs_(includes_phdrs), is_flags_valid_(is_flags_valid),
      flags_(flags), load_address_(load_address), load_address_value_(0),
      segment_(NULL)
  { }

  // Return the name of this segment.
  const std::string&
  name() const
  { return this->name_; }

  // Return the type of the segment.
  unsigned int
  type() const
  { return this->type_; }

  // Whether to include the file header.
  bool
  includes_filehdr() const
  { return this->includes_filehdr_; }

  // Whether to include the program headers.
  bool
  includes_phdrs() const
  { return this->includes_phdrs_; }

  // Return whether there is a load address.
  bool
  has_load_address() const
  { return this->load_address_ != NULL; }

  // Evaluate the load address expression if there is one.
  void
  eval_load_address(Symbol_table* symtab, Layout* layout)
  {
    if (this->load_address_ != NULL)
      this->load_address_value_ = this->load_address_->eval(symtab, layout,
							    true);
  }

  // Return the load address.
  uint64_t
  load_address() const
  {
    gold_assert(this->load_address_ != NULL);
    return this->load_address_value_;
  }

  // Create the segment.
  Output_segment*
  create_segment(Layout* layout)
  {
    this->segment_ = layout->make_output_segment(this->type_, this->flags_);
    return this->segment_;
  }

  // Return the segment.
  Output_segment*
  segment()
  { return this->segment_; }

  // Set the segment flags if appropriate.
  void
  set_flags_if_valid()
  {
    if (this->is_flags_valid_)
      this->segment_->set_flags(this->flags_);
  }

  // Print for debugging.
  void
  print(FILE*) const;

 private:
  // The name used in the script.
  std::string name_;
  // The type of the segment (PT_LOAD, etc.).
  unsigned int type_;
  // Whether this segment includes the file header.
  bool includes_filehdr_;
  // Whether this segment includes the section headers.
  bool includes_phdrs_;
  // Whether the flags were explicitly specified.
  bool is_flags_valid_;
  // The flags for this segment (PF_R, etc.) if specified.
  unsigned int flags_;
  // The expression for the load address for this segment.  This may
  // be NULL.
  Expression* load_address_;
  // The actual load address from evaluating the expression.
  uint64_t load_address_value_;
  // The segment itself.
  Output_segment* segment_;
};

// Print for debugging.

void
Phdrs_element::print(FILE* f) const
{
  fprintf(f, "  %s 0x%x", this->name_.c_str(), this->type_);
  if (this->includes_filehdr_)
    fprintf(f, " FILEHDR");
  if (this->includes_phdrs_)
    fprintf(f, " PHDRS");
  if (this->is_flags_valid_)
    fprintf(f, " FLAGS(%u)", this->flags_);
  if (this->load_address_ != NULL)
    {
      fprintf(f, " AT(");
      this->load_address_->print(f);
      fprintf(f, ")");
    }
  fprintf(f, ";\n");
}

// Class Script_sections.

Script_sections::Script_sections()
  : saw_sections_clause_(false),
    in_sections_clause_(false),
    sections_elements_(NULL),
    output_section_(NULL),
    phdrs_elements_(NULL),
    data_segment_align_index_(-1U),
    saw_relro_end_(false)
{
}

// Start a SECTIONS clause.

void
Script_sections::start_sections()
{
  gold_assert(!this->in_sections_clause_ && this->output_section_ == NULL);
  this->saw_sections_clause_ = true;
  this->in_sections_clause_ = true;
  if (this->sections_elements_ == NULL)
    this->sections_elements_ = new Sections_elements;
}

// Finish a SECTIONS clause.

void
Script_sections::finish_sections()
{
  gold_assert(this->in_sections_clause_ && this->output_section_ == NULL);
  this->in_sections_clause_ = false;
}

// Add a symbol to be defined.

void
Script_sections::add_symbol_assignment(const char* name, size_t length,
				       Expression* val, bool provide,
				       bool hidden)
{
  if (this->output_section_ != NULL)
    this->output_section_->add_symbol_assignment(name, length, val,
						 provide, hidden);
  else
    {
      Sections_element* p = new Sections_element_assignment(name, length,
							    val, provide,
							    hidden);
      this->sections_elements_->push_back(p);
    }
}

// Add an assignment to the special dot symbol.

void
Script_sections::add_dot_assignment(Expression* val)
{
  if (this->output_section_ != NULL)
    this->output_section_->add_dot_assignment(val);
  else
    {
      Sections_element* p = new Sections_element_dot_assignment(val);
      this->sections_elements_->push_back(p);
    }
}

// Add an assertion.

void
Script_sections::add_assertion(Expression* check, const char* message,
			       size_t messagelen)
{
  if (this->output_section_ != NULL)
    this->output_section_->add_assertion(check, message, messagelen);
  else
    {
      Sections_element* p = new Sections_element_assertion(check, message,
							   messagelen);
      this->sections_elements_->push_back(p);
    }
}

// Start processing entries for an output section.

void
Script_sections::start_output_section(
    const char* name,
    size_t namelen,
    const Parser_output_section_header *header)
{
  Output_section_definition* posd = new Output_section_definition(name,
								  namelen,
								  header);
  this->sections_elements_->push_back(posd);
  gold_assert(this->output_section_ == NULL);
  this->output_section_ = posd;
}

// Stop processing entries for an output section.

void
Script_sections::finish_output_section(
    const Parser_output_section_trailer* trailer)
{
  gold_assert(this->output_section_ != NULL);
  this->output_section_->finish(trailer);
  this->output_section_ = NULL;
}

// Add a data item to the current output section.

void
Script_sections::add_data(int size, bool is_signed, Expression* val)
{
  gold_assert(this->output_section_ != NULL);
  this->output_section_->add_data(size, is_signed, val);
}

// Add a fill value setting to the current output section.

void
Script_sections::add_fill(Expression* val)
{
  gold_assert(this->output_section_ != NULL);
  this->output_section_->add_fill(val);
}

// Add an input section specification to the current output section.

void
Script_sections::add_input_section(const Input_section_spec* spec, bool keep)
{
  gold_assert(this->output_section_ != NULL);
  this->output_section_->add_input_section(spec, keep);
}

// This is called when we see DATA_SEGMENT_ALIGN.  It means that any
// subsequent output sections may be relro.

void
Script_sections::data_segment_align()
{
  if (this->data_segment_align_index_ != -1U)
    gold_error(_("DATA_SEGMENT_ALIGN may only appear once in a linker script"));
  this->data_segment_align_index_ = this->sections_elements_->size();
}

// This is called when we see DATA_SEGMENT_RELRO_END.  It means that
// any output sections seen since DATA_SEGMENT_ALIGN are relro.

void
Script_sections::data_segment_relro_end()
{
  if (this->saw_relro_end_)
    gold_error(_("DATA_SEGMENT_RELRO_END may only appear once "
		 "in a linker script"));
  this->saw_relro_end_ = true;

  if (this->data_segment_align_index_ == -1U)
    gold_error(_("DATA_SEGMENT_RELRO_END must follow DATA_SEGMENT_ALIGN"));
  else
    {
      for (size_t i = this->data_segment_align_index_;
	   i < this->sections_elements_->size();
	   ++i)
	(*this->sections_elements_)[i]->set_is_relro();
    }
}

// Create any required sections.

void
Script_sections::create_sections(Layout* layout)
{
  if (!this->saw_sections_clause_)
    return;
  for (Sections_elements::iterator p = this->sections_elements_->begin();
       p != this->sections_elements_->end();
       ++p)
    (*p)->create_sections(layout);
}

// Add any symbols we are defining to the symbol table.

void
Script_sections::add_symbols_to_table(Symbol_table* symtab)
{
  if (!this->saw_sections_clause_)
    return;
  for (Sections_elements::iterator p = this->sections_elements_->begin();
       p != this->sections_elements_->end();
       ++p)
    (*p)->add_symbols_to_table(symtab);
}

// Finalize symbols and check assertions.

void
Script_sections::finalize_symbols(Symbol_table* symtab, const Layout* layout)
{
  if (!this->saw_sections_clause_)
    return;
  uint64_t dot_value = 0;
  for (Sections_elements::iterator p = this->sections_elements_->begin();
       p != this->sections_elements_->end();
       ++p)
    (*p)->finalize_symbols(symtab, layout, &dot_value);
}

// Return the name of the output section to use for an input file name
// and section name.

const char*
Script_sections::output_section_name(const char* file_name,
				     const char* section_name,
				     Output_section*** output_section_slot)
{
  for (Sections_elements::const_iterator p = this->sections_elements_->begin();
       p != this->sections_elements_->end();
       ++p)
    {
      const char* ret = (*p)->output_section_name(file_name, section_name,
						  output_section_slot);

      if (ret != NULL)
	{
	  // The special name /DISCARD/ means that the input section
	  // should be discarded.
	  if (strcmp(ret, "/DISCARD/") == 0)
	    {
	      *output_section_slot = NULL;
	      return NULL;
	    }
	  return ret;
	}
    }

  // If we couldn't find a mapping for the name, the output section
  // gets the name of the input section.

  *output_section_slot = NULL;

  return section_name;
}

// Place a marker for an orphan output section into the SECTIONS
// clause.

void
Script_sections::place_orphan(Output_section* os)
{
  // Look for an output section definition which matches the output
  // section.  Put a marker after that section.
  bool is_relro = false;
  Sections_elements::iterator place = this->sections_elements_->end();
  for (Sections_elements::iterator p = this->sections_elements_->begin();
       p != this->sections_elements_->end();
       ++p)
    {
      bool exact = false;
      bool is_relro_here;
      if ((*p)->place_orphan_here(os, &exact, &is_relro_here))
	{
	  place = p;
	  is_relro = is_relro_here;
	  if (exact)
	    break;
	}
    }

  // The insert function puts the new element before the iterator.
  if (place != this->sections_elements_->end())
    ++place;

  this->sections_elements_->insert(place, new Orphan_output_section(os));

  if (is_relro)
    os->set_is_relro();
  else
    os->clear_is_relro();
}

// Set the addresses of all the output sections.  Walk through all the
// elements, tracking the dot symbol.  Apply assignments which set
// absolute symbol values, in case they are used when setting dot.
// Fill in data statement values.  As we find output sections, set the
// address, set the address of all associated input sections, and
// update dot.  Return the segment which should hold the file header
// and segment headers, if any.

Output_segment*
Script_sections::set_section_addresses(Symbol_table* symtab, Layout* layout)
{
  gold_assert(this->saw_sections_clause_);

  // Implement ONLY_IF_RO/ONLY_IF_RW constraints.  These are a pain
  // for our representation.
  for (Sections_elements::iterator p = this->sections_elements_->begin();
       p != this->sections_elements_->end();
       ++p)
    {
      Output_section_definition* posd;
      Section_constraint failed_constraint = (*p)->check_constraint(&posd);
      if (failed_constraint != CONSTRAINT_NONE)
	{
	  Sections_elements::iterator q;
	  for (q = this->sections_elements_->begin();
	       q != this->sections_elements_->end();
	       ++q)
	    {
	      if (q != p)
		{
		  if ((*q)->alternate_constraint(posd, failed_constraint))
		    break;
		}
	    }

	  if (q == this->sections_elements_->end())
	    gold_error(_("no matching section constraint"));
	}
    }

  // Force the alignment of the first TLS section to be the maximum
  // alignment of all TLS sections.
  Output_section* first_tls = NULL;
  uint64_t tls_align = 0;
  for (Sections_elements::const_iterator p = this->sections_elements_->begin();
       p != this->sections_elements_->end();
       ++p)
    {
      Output_section *os = (*p)->get_output_section();
      if (os != NULL && (os->flags() & elfcpp::SHF_TLS) != 0)
	{
	  if (first_tls == NULL)
	    first_tls = os;
	  if (os->addralign() > tls_align)
	    tls_align = os->addralign();
	}
    }
  if (first_tls != NULL)
    first_tls->set_addralign(tls_align);

  // For a relocatable link, we implicitly set dot to zero.
  uint64_t dot_value = 0;
  uint64_t load_address = 0;
  for (Sections_elements::iterator p = this->sections_elements_->begin();
       p != this->sections_elements_->end();
       ++p)
    (*p)->set_section_addresses(symtab, layout, &dot_value, &load_address);

  if (this->phdrs_elements_ != NULL)
    {
      for (Phdrs_elements::iterator p = this->phdrs_elements_->begin();
	   p != this->phdrs_elements_->end();
	   ++p)
	(*p)->eval_load_address(symtab, layout);
    }

  return this->create_segments(layout);
}

// Sort the sections in order to put them into segments.

class Sort_output_sections
{
 public:
  bool
  operator()(const Output_section* os1, const Output_section* os2) const;
};

bool
Sort_output_sections::operator()(const Output_section* os1,
				 const Output_section* os2) const
{
  // Sort first by the load address.
  uint64_t lma1 = (os1->has_load_address()
		   ? os1->load_address()
		   : os1->address());
  uint64_t lma2 = (os2->has_load_address()
		   ? os2->load_address()
		   : os2->address());
  if (lma1 != lma2)
    return lma1 < lma2;

  // Then sort by the virtual address.
  if (os1->address() != os2->address())
    return os1->address() < os2->address();

  // Sort TLS sections to the end.
  bool tls1 = (os1->flags() & elfcpp::SHF_TLS) != 0;
  bool tls2 = (os2->flags() & elfcpp::SHF_TLS) != 0;
  if (tls1 != tls2)
    return tls2;

  // Sort PROGBITS before NOBITS.
  if (os1->type() == elfcpp::SHT_PROGBITS && os2->type() == elfcpp::SHT_NOBITS)
    return true;
  if (os1->type() == elfcpp::SHT_NOBITS && os2->type() == elfcpp::SHT_PROGBITS)
    return false;

  // Otherwise we don't care.
  return false;
}

// Return whether OS is a BSS section.  This is a SHT_NOBITS section.
// We treat a section with the SHF_TLS flag set as taking up space
// even if it is SHT_NOBITS (this is true of .tbss), as we allocate
// space for them in the file.

bool
Script_sections::is_bss_section(const Output_section* os)
{
  return (os->type() == elfcpp::SHT_NOBITS
	  && (os->flags() & elfcpp::SHF_TLS) == 0);
}

// Return the size taken by the file header and the program headers.

size_t
Script_sections::total_header_size(Layout* layout) const
{
  size_t segment_count = layout->segment_count();
  size_t file_header_size;
  size_t segment_headers_size;
  if (parameters->target().get_size() == 32)
    {
      file_header_size = elfcpp::Elf_sizes<32>::ehdr_size;
      segment_headers_size = segment_count * elfcpp::Elf_sizes<32>::phdr_size;
    }
  else if (parameters->target().get_size() == 64)
    {
      file_header_size = elfcpp::Elf_sizes<64>::ehdr_size;
      segment_headers_size = segment_count * elfcpp::Elf_sizes<64>::phdr_size;
    }
  else
    gold_unreachable();

  return file_header_size + segment_headers_size;
}

// Return the amount we have to subtract from the LMA to accomodate
// headers of the given size.  The complication is that the file
// header have to be at the start of a page, as otherwise it will not
// be at the start of the file.

uint64_t
Script_sections::header_size_adjustment(uint64_t lma,
					size_t sizeof_headers) const
{
  const uint64_t abi_pagesize = parameters->target().abi_pagesize();
  uint64_t hdr_lma = lma - sizeof_headers;
  hdr_lma &= ~(abi_pagesize - 1);
  return lma - hdr_lma;
}

// Create the PT_LOAD segments when using a SECTIONS clause.  Returns
// the segment which should hold the file header and segment headers,
// if any.

Output_segment*
Script_sections::create_segments(Layout* layout)
{
  gold_assert(this->saw_sections_clause_);

  if (parameters->options().relocatable())
    return NULL;

  if (this->saw_phdrs_clause())
    return create_segments_from_phdrs_clause(layout);

  Layout::Section_list sections;
  layout->get_allocated_sections(&sections);

  // Sort the sections by address.
  std::stable_sort(sections.begin(), sections.end(), Sort_output_sections());

  this->create_note_and_tls_segments(layout, &sections);

  // Walk through the sections adding them to PT_LOAD segments.
  const uint64_t abi_pagesize = parameters->target().abi_pagesize();
  Output_segment* first_seg = NULL;
  Output_segment* current_seg = NULL;
  bool is_current_seg_readonly = true;
  Layout::Section_list::iterator plast = sections.end();
  uint64_t last_vma = 0;
  uint64_t last_lma = 0;
  uint64_t last_size = 0;
  for (Layout::Section_list::iterator p = sections.begin();
       p != sections.end();
       ++p)
    {
      const uint64_t vma = (*p)->address();
      const uint64_t lma = ((*p)->has_load_address()
			    ? (*p)->load_address()
			    : vma);
      const uint64_t size = (*p)->current_data_size();

      bool need_new_segment;
      if (current_seg == NULL)
	need_new_segment = true;
      else if (lma - vma != last_lma - last_vma)
	{
	  // This section has a different LMA relationship than the
	  // last one; we need a new segment.
	  need_new_segment = true;
	}
      else if (align_address(last_lma + last_size, abi_pagesize)
	       < align_address(lma, abi_pagesize))
	{
	  // Putting this section in the segment would require
	  // skipping a page.
	  need_new_segment = true;
	}
      else if (is_bss_section(*plast) && !is_bss_section(*p))
	{
	  // A non-BSS section can not follow a BSS section in the
	  // same segment.
	  need_new_segment = true;
	}
      else if (is_current_seg_readonly
	       && ((*p)->flags() & elfcpp::SHF_WRITE) != 0
	       && !parameters->options().omagic())
	{
	  // Don't put a writable section in the same segment as a
	  // non-writable section.
	  need_new_segment = true;
	}
      else
	{
	  // Otherwise, reuse the existing segment.
	  need_new_segment = false;
	}

      elfcpp::Elf_Word seg_flags =
	Layout::section_flags_to_segment((*p)->flags());

      if (need_new_segment)
	{
	  current_seg = layout->make_output_segment(elfcpp::PT_LOAD,
						    seg_flags);
	  current_seg->set_addresses(vma, lma);
	  if (first_seg == NULL)
	    first_seg = current_seg;
	  is_current_seg_readonly = true;
	}

      current_seg->add_output_section(*p, seg_flags);

      if (((*p)->flags() & elfcpp::SHF_WRITE) != 0)
	is_current_seg_readonly = false;

      plast = p;
      last_vma = vma;
      last_lma = lma;
      last_size = size;
    }

  // An ELF program should work even if the program headers are not in
  // a PT_LOAD segment.  However, it appears that the Linux kernel
  // does not set the AT_PHDR auxiliary entry in that case.  It sets
  // the load address to p_vaddr - p_offset of the first PT_LOAD
  // segment.  It then sets AT_PHDR to the load address plus the
  // offset to the program headers, e_phoff in the file header.  This
  // fails when the program headers appear in the file before the
  // first PT_LOAD segment.  Therefore, we always create a PT_LOAD
  // segment to hold the file header and the program headers.  This is
  // effectively what the GNU linker does, and it is slightly more
  // efficient in any case.  We try to use the first PT_LOAD segment
  // if we can, otherwise we make a new one.

  if (first_seg == NULL)
    return NULL;

  size_t sizeof_headers = this->total_header_size(layout);

  uint64_t vma = first_seg->vaddr();
  uint64_t lma = first_seg->paddr();

  uint64_t subtract = this->header_size_adjustment(lma, sizeof_headers);

  if ((lma & (abi_pagesize - 1)) >= sizeof_headers)
    {
      first_seg->set_addresses(vma - subtract, lma - subtract);
      return first_seg;
    }

  // If there is no room to squeeze in the headers, then punt.  The
  // resulting executable probably won't run on GNU/Linux, but we
  // trust that the user knows what they are doing.
  if (lma < subtract || vma < subtract)
    return NULL;

  Output_segment* load_seg = layout->make_output_segment(elfcpp::PT_LOAD,
							 elfcpp::PF_R);
  load_seg->set_addresses(vma - subtract, lma - subtract);

  return load_seg;
}

// Create a PT_NOTE segment for each SHT_NOTE section and a PT_TLS
// segment if there are any SHT_TLS sections.

void
Script_sections::create_note_and_tls_segments(
    Layout* layout,
    const Layout::Section_list* sections)
{
  gold_assert(!this->saw_phdrs_clause());

  bool saw_tls = false;
  for (Layout::Section_list::const_iterator p = sections->begin();
       p != sections->end();
       ++p)
    {
      if ((*p)->type() == elfcpp::SHT_NOTE)
	{
	  elfcpp::Elf_Word seg_flags =
	    Layout::section_flags_to_segment((*p)->flags());
	  Output_segment* oseg = layout->make_output_segment(elfcpp::PT_NOTE,
							     seg_flags);
	  oseg->add_output_section(*p, seg_flags);

	  // Incorporate any subsequent SHT_NOTE sections, in the
	  // hopes that the script is sensible.
	  Layout::Section_list::const_iterator pnext = p + 1;
	  while (pnext != sections->end()
		 && (*pnext)->type() == elfcpp::SHT_NOTE)
	    {
	      seg_flags = Layout::section_flags_to_segment((*pnext)->flags());
	      oseg->add_output_section(*pnext, seg_flags);
	      p = pnext;
	      ++pnext;
	    }
	}

      if (((*p)->flags() & elfcpp::SHF_TLS) != 0)
	{
	  if (saw_tls)
	    gold_error(_("TLS sections are not adjacent"));

	  elfcpp::Elf_Word seg_flags =
	    Layout::section_flags_to_segment((*p)->flags());
	  Output_segment* oseg = layout->make_output_segment(elfcpp::PT_TLS,
							     seg_flags);
	  oseg->add_output_section(*p, seg_flags);

	  Layout::Section_list::const_iterator pnext = p + 1;
	  while (pnext != sections->end()
		 && ((*pnext)->flags() & elfcpp::SHF_TLS) != 0)
	    {
	      seg_flags = Layout::section_flags_to_segment((*pnext)->flags());
	      oseg->add_output_section(*pnext, seg_flags);
	      p = pnext;
	      ++pnext;
	    }

	  saw_tls = true;
	}
    }
}

// Add a program header.  The PHDRS clause is syntactically distinct
// from the SECTIONS clause, but we implement it with the SECTIONS
// support becauase PHDRS is useless if there is no SECTIONS clause.

void
Script_sections::add_phdr(const char* name, size_t namelen, unsigned int type,
			  bool includes_filehdr, bool includes_phdrs,
			  bool is_flags_valid, unsigned int flags,
			  Expression* load_address)
{
  if (this->phdrs_elements_ == NULL)
    this->phdrs_elements_ = new Phdrs_elements();
  this->phdrs_elements_->push_back(new Phdrs_element(name, namelen, type,
						     includes_filehdr,
						     includes_phdrs,
						     is_flags_valid, flags,
						     load_address));
}

// Return the number of segments we expect to create based on the
// SECTIONS clause.  This is used to implement SIZEOF_HEADERS.

size_t
Script_sections::expected_segment_count(const Layout* layout) const
{
  if (this->saw_phdrs_clause())
    return this->phdrs_elements_->size();

  Layout::Section_list sections;
  layout->get_allocated_sections(&sections);

  // We assume that we will need two PT_LOAD segments.
  size_t ret = 2;

  bool saw_note = false;
  bool saw_tls = false;
  for (Layout::Section_list::const_iterator p = sections.begin();
       p != sections.end();
       ++p)
    {
      if ((*p)->type() == elfcpp::SHT_NOTE)
	{
	  // Assume that all note sections will fit into a single
	  // PT_NOTE segment.
	  if (!saw_note)
	    {
	      ++ret;
	      saw_note = true;
	    }
	}
      else if (((*p)->flags() & elfcpp::SHF_TLS) != 0)
	{
	  // There can only be one PT_TLS segment.
	  if (!saw_tls)
	    {
	      ++ret;
	      saw_tls = true;
	    }
	}
    }

  return ret;
}

// Create the segments from a PHDRS clause.  Return the segment which
// should hold the file header and program headers, if any.

Output_segment*
Script_sections::create_segments_from_phdrs_clause(Layout* layout)
{
  this->attach_sections_using_phdrs_clause(layout);
  return this->set_phdrs_clause_addresses(layout);
}

// Create the segments from the PHDRS clause, and put the output
// sections in them.

void
Script_sections::attach_sections_using_phdrs_clause(Layout* layout)
{
  typedef std::map<std::string, Output_segment*> Name_to_segment;
  Name_to_segment name_to_segment;
  for (Phdrs_elements::const_iterator p = this->phdrs_elements_->begin();
       p != this->phdrs_elements_->end();
       ++p)
    name_to_segment[(*p)->name()] = (*p)->create_segment(layout);

  // Walk through the output sections and attach them to segments.
  // Output sections in the script which do not list segments are
  // attached to the same set of segments as the immediately preceding
  // output section.
  String_list* phdr_names = NULL;
  for (Sections_elements::const_iterator p = this->sections_elements_->begin();
       p != this->sections_elements_->end();
       ++p)
    {
      bool orphan;
      Output_section* os = (*p)->allocate_to_segment(&phdr_names, &orphan);
      if (os == NULL)
	continue;

      if (phdr_names == NULL)
	{
	  gold_error(_("allocated section not in any segment"));
	  continue;
	}

      // If this is an orphan section--one that was not explicitly
      // mentioned in the linker script--then it should not inherit
      // any segment type other than PT_LOAD.  Otherwise, e.g., the
      // PT_INTERP segment will pick up following orphan sections,
      // which does not make sense.  If this is not an orphan section,
      // we trust the linker script.
      if (orphan)
	{
	  String_list::iterator q = phdr_names->begin();
	  while (q != phdr_names->end())
	    {
	      Name_to_segment::const_iterator r = name_to_segment.find(*q);
	      // We give errors about unknown segments below.
	      if (r == name_to_segment.end()
		  || r->second->type() == elfcpp::PT_LOAD)
		++q;
	      else
		q = phdr_names->erase(q);
	    }
	}

      bool in_load_segment = false;
      for (String_list::const_iterator q = phdr_names->begin();
	   q != phdr_names->end();
	   ++q)
	{
	  Name_to_segment::const_iterator r = name_to_segment.find(*q);
	  if (r == name_to_segment.end())
	    gold_error(_("no segment %s"), q->c_str());
	  else
	    {
	      elfcpp::Elf_Word seg_flags =
		Layout::section_flags_to_segment(os->flags());
	      r->second->add_output_section(os, seg_flags);

	      if (r->second->type() == elfcpp::PT_LOAD)
		{
		  if (in_load_segment)
		    gold_error(_("section in two PT_LOAD segments"));
		  in_load_segment = true;
		}
	    }
	}

      if (!in_load_segment)
	gold_error(_("allocated section not in any PT_LOAD segment"));
    }
}

// Set the addresses for segments created from a PHDRS clause.  Return
// the segment which should hold the file header and program headers,
// if any.

Output_segment*
Script_sections::set_phdrs_clause_addresses(Layout* layout)
{
  Output_segment* load_seg = NULL;
  for (Phdrs_elements::const_iterator p = this->phdrs_elements_->begin();
       p != this->phdrs_elements_->end();
       ++p)
    {
      // Note that we have to set the flags after adding the output
      // sections to the segment, as adding an output segment can
      // change the flags.
      (*p)->set_flags_if_valid();

      Output_segment* oseg = (*p)->segment();

      if (oseg->type() != elfcpp::PT_LOAD)
	{
	  // The addresses of non-PT_LOAD segments are set from the
	  // PT_LOAD segments.
	  if ((*p)->has_load_address())
	    gold_error(_("may only specify load address for PT_LOAD segment"));
	  continue;
	}

      // The output sections should have addresses from the SECTIONS
      // clause.  The addresses don't have to be in order, so find the
      // one with the lowest load address.  Use that to set the
      // address of the segment.

      Output_section* osec = oseg->section_with_lowest_load_address();
      if (osec == NULL)
	{
	  oseg->set_addresses(0, 0);
	  continue;
	}

      uint64_t vma = osec->address();
      uint64_t lma = osec->has_load_address() ? osec->load_address() : vma;

      // Override the load address of the section with the load
      // address specified for the segment.
      if ((*p)->has_load_address())
	{
	  if (osec->has_load_address())
	    gold_warning(_("PHDRS load address overrides "
			   "section %s load address"),
			 osec->name());

	  lma = (*p)->load_address();
	}

      bool headers = (*p)->includes_filehdr() && (*p)->includes_phdrs();
      if (!headers && ((*p)->includes_filehdr() || (*p)->includes_phdrs()))
	{
	  // We could support this if we wanted to.
	  gold_error(_("using only one of FILEHDR and PHDRS is "
		       "not currently supported"));
	}
      if (headers)
	{
	  size_t sizeof_headers = this->total_header_size(layout);
	  uint64_t subtract = this->header_size_adjustment(lma,
							   sizeof_headers);
	  if (lma >= subtract && vma >= subtract)
	    {
	      lma -= subtract;
	      vma -= subtract;
	    }
	  else
	    {
	      gold_error(_("sections loaded on first page without room "
			   "for file and program headers "
			   "are not supported"));
	    }

	  if (load_seg != NULL)
	    gold_error(_("using FILEHDR and PHDRS on more than one "
			 "PT_LOAD segment is not currently supported"));
	  load_seg = oseg;
	}

      oseg->set_addresses(vma, lma);
    }

  return load_seg;
}

// Add the file header and segment headers to non-load segments
// specified in the PHDRS clause.

void
Script_sections::put_headers_in_phdrs(Output_data* file_header,
				      Output_data* segment_headers)
{
  gold_assert(this->saw_phdrs_clause());
  for (Phdrs_elements::iterator p = this->phdrs_elements_->begin();
       p != this->phdrs_elements_->end();
       ++p)
    {
      if ((*p)->type() != elfcpp::PT_LOAD)
	{
	  if ((*p)->includes_phdrs())
	    (*p)->segment()->add_initial_output_data(segment_headers);
	  if ((*p)->includes_filehdr())
	    (*p)->segment()->add_initial_output_data(file_header);
	}
    }
}

// Look for an output section by name and return the address, the load
// address, the alignment, and the size.  This is used when an
// expression refers to an output section which was not actually
// created.  This returns true if the section was found, false
// otherwise.

bool
Script_sections::get_output_section_info(const char* name, uint64_t* address,
                                         uint64_t* load_address,
                                         uint64_t* addralign,
                                         uint64_t* size) const
{
  if (!this->saw_sections_clause_)
    return false;
  for (Sections_elements::const_iterator p = this->sections_elements_->begin();
       p != this->sections_elements_->end();
       ++p)
    if ((*p)->get_output_section_info(name, address, load_address, addralign,
                                      size))
      return true;
  return false;
}

// Print the SECTIONS clause to F for debugging.

void
Script_sections::print(FILE* f) const
{
  if (!this->saw_sections_clause_)
    return;

  fprintf(f, "SECTIONS {\n");

  for (Sections_elements::const_iterator p = this->sections_elements_->begin();
       p != this->sections_elements_->end();
       ++p)
    (*p)->print(f);

  fprintf(f, "}\n");

  if (this->phdrs_elements_ != NULL)
    {
      fprintf(f, "PHDRS {\n");
      for (Phdrs_elements::const_iterator p = this->phdrs_elements_->begin();
	   p != this->phdrs_elements_->end();
	   ++p)
	(*p)->print(f);
      fprintf(f, "}\n");
    }
}

} // End namespace gold.
