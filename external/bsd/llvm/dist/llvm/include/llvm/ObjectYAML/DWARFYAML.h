//===- DWARFYAML.h - DWARF YAMLIO implementation ----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief This file declares classes for handling the YAML representation
/// of DWARF Debug Info.
///
//===----------------------------------------------------------------------===//


#ifndef LLVM_OBJECTYAML_DWARFYAML_H
#define LLVM_OBJECTYAML_DWARFYAML_H

#include "llvm/ObjectYAML/YAML.h"
#include "llvm/Support/Dwarf.h"

namespace llvm {
namespace DWARFYAML {

struct AttributeAbbrev {
  llvm::dwarf::Attribute Attribute;
  llvm::dwarf::Form Form;
};

struct Abbrev {
  llvm::yaml::Hex32 Code;
  llvm::dwarf::Tag Tag;
  llvm::dwarf::Constants Children;
  std::vector<AttributeAbbrev> Attributes;
};

struct ARangeDescriptor {
  llvm::yaml::Hex64 Address;
  uint64_t Length;
};

struct ARange {
  uint32_t Length;
  uint16_t Version;
  uint32_t CuOffset;
  uint8_t AddrSize;
  uint8_t SegSize;
  std::vector<ARangeDescriptor> Descriptors;
};

struct PubEntry {
  llvm::yaml::Hex32 DieOffset;
  llvm::yaml::Hex8 Descriptor;
  StringRef Name;
};

struct PubSection {
  PubSection() : IsGNUStyle(false) {}

  uint32_t Length;
  uint16_t Version;
  uint32_t UnitOffset;
  uint32_t UnitSize;
  bool IsGNUStyle;
  std::vector<PubEntry> Entries;
};

struct FormValue {
  llvm::yaml::Hex64 Value;
  StringRef CStr;
  std::vector<llvm::yaml::Hex8> BlockData;
};

struct Entry {
  llvm::yaml::Hex32 AbbrCode;
  std::vector<FormValue> Values;
};

struct Unit {
  uint32_t Length;
  uint16_t Version;
  uint32_t AbbrOffset;
  uint8_t AddrSize;
  std::vector<Entry> Entries;
};

struct Data {
  bool IsLittleEndian;
  std::vector<Abbrev> AbbrevDecls;
  std::vector<StringRef> DebugStrings;
  std::vector<ARange> ARanges;
  PubSection PubNames;
  PubSection PubTypes;

  PubSection GNUPubNames;
  PubSection GNUPubTypes;
  
  std::vector<Unit> CompileUnits;

  bool isEmpty() const;
};

} // namespace llvm::DWARFYAML
} // namespace llvm

LLVM_YAML_IS_SEQUENCE_VECTOR(uint8_t)
LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::StringRef)
LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::yaml::Hex8)
LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::DWARFYAML::AttributeAbbrev)
LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::DWARFYAML::Abbrev)
LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::DWARFYAML::ARangeDescriptor)
LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::DWARFYAML::ARange)
LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::DWARFYAML::PubEntry)
LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::DWARFYAML::Unit)
LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::DWARFYAML::FormValue)
LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::DWARFYAML::Entry)

namespace llvm {
namespace yaml {

template <> struct MappingTraits<DWARFYAML::Data> {
  static void mapping(IO &IO, DWARFYAML::Data &DWARF);
};

template <> struct MappingTraits<DWARFYAML::Abbrev> {
  static void mapping(IO &IO, DWARFYAML::Abbrev &Abbrev);
};

template <> struct MappingTraits<DWARFYAML::AttributeAbbrev> {
  static void mapping(IO &IO, DWARFYAML::AttributeAbbrev &AttAbbrev);
};

template <> struct MappingTraits<DWARFYAML::ARangeDescriptor> {
  static void mapping(IO &IO, DWARFYAML::ARangeDescriptor &Descriptor);
};

template <> struct MappingTraits<DWARFYAML::ARange> {
  static void mapping(IO &IO, DWARFYAML::ARange &Range);
};

template <> struct MappingTraits<DWARFYAML::PubEntry> {
  static void mapping(IO &IO, DWARFYAML::PubEntry &Entry);
};

template <> struct MappingTraits<DWARFYAML::PubSection> {
  static void mapping(IO &IO, DWARFYAML::PubSection &Section);
};

template <> struct MappingTraits<DWARFYAML::Unit> {
  static void mapping(IO &IO, DWARFYAML::Unit &Unit);
};

template <> struct MappingTraits<DWARFYAML::Entry> {
  static void mapping(IO &IO, DWARFYAML::Entry &Entry);
};

template <> struct MappingTraits<DWARFYAML::FormValue> {
  static void mapping(IO &IO, DWARFYAML::FormValue &FormValue);
};

#define HANDLE_DW_TAG(unused, name)                                            \
  io.enumCase(value, "DW_TAG_" #name, dwarf::DW_TAG_##name);

template <> struct ScalarEnumerationTraits<dwarf::Tag> {
  static void enumeration(IO &io, dwarf::Tag &value) {
#include "llvm/Support/Dwarf.def"
    io.enumFallback<Hex16>(value);
  }
};

#define HANDLE_DW_AT(unused, name)                                             \
  io.enumCase(value, "DW_AT_" #name, dwarf::DW_AT_##name);

template <> struct ScalarEnumerationTraits<dwarf::Attribute> {
  static void enumeration(IO &io, dwarf::Attribute &value) {
#include "llvm/Support/Dwarf.def"
    io.enumFallback<Hex16>(value);
  }
};

#define HANDLE_DW_FORM(unused, name)                                           \
  io.enumCase(value, "DW_FORM_" #name, dwarf::DW_FORM_##name);

template <> struct ScalarEnumerationTraits<dwarf::Form> {
  static void enumeration(IO &io, dwarf::Form &value) {
#include "llvm/Support/Dwarf.def"
    io.enumFallback<Hex16>(value);
  }
};

template <> struct ScalarEnumerationTraits<dwarf::Constants> {
  static void enumeration(IO &io, dwarf::Constants &value) {
    io.enumCase(value, "DW_CHILDREN_no", dwarf::DW_CHILDREN_no);
    io.enumCase(value, "DW_CHILDREN_yes", dwarf::DW_CHILDREN_yes);
    io.enumFallback<Hex16>(value);
  }
};

} // namespace llvm::yaml
} // namespace llvm

#endif
