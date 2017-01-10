//===- MachOYAML.h - Mach-O YAMLIO implementation ---------------*- C++ -*-===//
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
/// of Mach-O.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_OBJECTYAML_MACHOYAML_H
#define LLVM_OBJECTYAML_MACHOYAML_H

#include "llvm/ObjectYAML/YAML.h"
#include "llvm/ObjectYAML/DWARFYAML.h"
#include "llvm/Support/MachO.h"

namespace llvm {
namespace MachOYAML {

struct Section {
  char sectname[16];
  char segname[16];
  llvm::yaml::Hex64 addr;
  uint64_t size;
  llvm::yaml::Hex32 offset;
  uint32_t align;
  llvm::yaml::Hex32 reloff;
  uint32_t nreloc;
  llvm::yaml::Hex32 flags;
  llvm::yaml::Hex32 reserved1;
  llvm::yaml::Hex32 reserved2;
  llvm::yaml::Hex32 reserved3;
};

struct FileHeader {
  llvm::yaml::Hex32 magic;
  llvm::yaml::Hex32 cputype;
  llvm::yaml::Hex32 cpusubtype;
  llvm::yaml::Hex32 filetype;
  uint32_t ncmds;
  uint32_t sizeofcmds;
  llvm::yaml::Hex32 flags;
  llvm::yaml::Hex32 reserved;
};

struct LoadCommand {
  virtual ~LoadCommand();
  llvm::MachO::macho_load_command Data;
  std::vector<Section> Sections;
  std::vector<llvm::yaml::Hex8> PayloadBytes;
  std::string PayloadString;
  uint64_t ZeroPadBytes;
};

struct NListEntry {
  uint32_t n_strx;
  llvm::yaml::Hex8 n_type;
  uint8_t n_sect;
  uint16_t n_desc;
  uint64_t n_value;
};
struct RebaseOpcode {
  MachO::RebaseOpcode Opcode;
  uint8_t Imm;
  std::vector<yaml::Hex64> ExtraData;
};

struct BindOpcode {
  MachO::BindOpcode Opcode;
  uint8_t Imm;
  std::vector<yaml::Hex64> ULEBExtraData;
  std::vector<int64_t> SLEBExtraData;
  StringRef Symbol;
};

struct ExportEntry {
  ExportEntry()
      : TerminalSize(0), NodeOffset(0), Name(), Flags(0), Address(0), Other(0),
        ImportName(), Children() {}
  uint64_t TerminalSize;
  uint64_t NodeOffset;
  std::string Name;
  llvm::yaml::Hex64 Flags;
  llvm::yaml::Hex64 Address;
  llvm::yaml::Hex64 Other;
  std::string ImportName;
  std::vector<MachOYAML::ExportEntry> Children;
};

struct LinkEditData {
  std::vector<MachOYAML::RebaseOpcode> RebaseOpcodes;
  std::vector<MachOYAML::BindOpcode> BindOpcodes;
  std::vector<MachOYAML::BindOpcode> WeakBindOpcodes;
  std::vector<MachOYAML::BindOpcode> LazyBindOpcodes;
  MachOYAML::ExportEntry ExportTrie;
  std::vector<NListEntry> NameList;
  std::vector<StringRef> StringTable;

  bool isEmpty() const;
};

struct Object {
  bool IsLittleEndian;
  FileHeader Header;
  std::vector<LoadCommand> LoadCommands;
  std::vector<Section> Sections;
  LinkEditData LinkEdit;
  DWARFYAML::Data DWARF;
};

struct FatHeader {
  llvm::yaml::Hex32 magic;
  uint32_t nfat_arch;
};

struct FatArch {
  llvm::yaml::Hex32 cputype;
  llvm::yaml::Hex32 cpusubtype;
  llvm::yaml::Hex64 offset;
  uint64_t size;
  uint32_t align;
  llvm::yaml::Hex32 reserved;
};

struct UniversalBinary {
  FatHeader Header;
  std::vector<FatArch> FatArchs;
  std::vector<Object> Slices;
};

} // namespace llvm::MachOYAML
} // namespace llvm

LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::MachOYAML::LoadCommand)
LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::MachOYAML::Section)
LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::yaml::Hex64)
LLVM_YAML_IS_SEQUENCE_VECTOR(int64_t)
LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::MachOYAML::RebaseOpcode)
LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::MachOYAML::BindOpcode)
LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::MachOYAML::ExportEntry)
LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::MachOYAML::NListEntry)
LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::MachOYAML::Object)
LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::MachOYAML::FatArch)

namespace llvm {
namespace yaml {

template <> struct MappingTraits<MachOYAML::FileHeader> {
  static void mapping(IO &IO, MachOYAML::FileHeader &FileHeader);
};

template <> struct MappingTraits<MachOYAML::Object> {
  static void mapping(IO &IO, MachOYAML::Object &Object);
};

template <> struct MappingTraits<MachOYAML::FatHeader> {
  static void mapping(IO &IO, MachOYAML::FatHeader &FatHeader);
};

template <> struct MappingTraits<MachOYAML::FatArch> {
  static void mapping(IO &IO, MachOYAML::FatArch &FatArch);
};

template <> struct MappingTraits<MachOYAML::UniversalBinary> {
  static void mapping(IO &IO, MachOYAML::UniversalBinary &UniversalBinary);
};

template <> struct MappingTraits<MachOYAML::LoadCommand> {
  static void mapping(IO &IO, MachOYAML::LoadCommand &LoadCommand);
};

template <> struct MappingTraits<MachOYAML::LinkEditData> {
  static void mapping(IO &IO, MachOYAML::LinkEditData &LinkEditData);
};

template <> struct MappingTraits<MachOYAML::RebaseOpcode> {
  static void mapping(IO &IO, MachOYAML::RebaseOpcode &RebaseOpcode);
};

template <> struct MappingTraits<MachOYAML::BindOpcode> {
  static void mapping(IO &IO, MachOYAML::BindOpcode &BindOpcode);
};

template <> struct MappingTraits<MachOYAML::ExportEntry> {
  static void mapping(IO &IO, MachOYAML::ExportEntry &ExportEntry);
};

template <> struct MappingTraits<MachOYAML::Section> {
  static void mapping(IO &IO, MachOYAML::Section &Section);
};

template <> struct MappingTraits<MachOYAML::NListEntry> {
  static void mapping(IO &IO, MachOYAML::NListEntry &NListEntry);
};

#define HANDLE_LOAD_COMMAND(LCName, LCValue, LCStruct)                         \
  io.enumCase(value, #LCName, MachO::LCName);

template <> struct ScalarEnumerationTraits<MachO::LoadCommandType> {
  static void enumeration(IO &io, MachO::LoadCommandType &value) {
#include "llvm/Support/MachO.def"
    io.enumFallback<Hex32>(value);
  }
};

#define ENUM_CASE(Enum) io.enumCase(value, #Enum, MachO::Enum);

template <> struct ScalarEnumerationTraits<MachO::RebaseOpcode> {
  static void enumeration(IO &io, MachO::RebaseOpcode &value) {
    ENUM_CASE(REBASE_OPCODE_DONE)
    ENUM_CASE(REBASE_OPCODE_SET_TYPE_IMM)
    ENUM_CASE(REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB)
    ENUM_CASE(REBASE_OPCODE_ADD_ADDR_ULEB)
    ENUM_CASE(REBASE_OPCODE_ADD_ADDR_IMM_SCALED)
    ENUM_CASE(REBASE_OPCODE_DO_REBASE_IMM_TIMES)
    ENUM_CASE(REBASE_OPCODE_DO_REBASE_ULEB_TIMES)
    ENUM_CASE(REBASE_OPCODE_DO_REBASE_ADD_ADDR_ULEB)
    ENUM_CASE(REBASE_OPCODE_DO_REBASE_ULEB_TIMES_SKIPPING_ULEB)
    io.enumFallback<Hex8>(value);
  }
};

template <> struct ScalarEnumerationTraits<MachO::BindOpcode> {
  static void enumeration(IO &io, MachO::BindOpcode &value) {
    ENUM_CASE(BIND_OPCODE_DONE)
    ENUM_CASE(BIND_OPCODE_SET_DYLIB_ORDINAL_IMM)
    ENUM_CASE(BIND_OPCODE_SET_DYLIB_ORDINAL_ULEB)
    ENUM_CASE(BIND_OPCODE_SET_DYLIB_SPECIAL_IMM)
    ENUM_CASE(BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM)
    ENUM_CASE(BIND_OPCODE_SET_TYPE_IMM)
    ENUM_CASE(BIND_OPCODE_SET_ADDEND_SLEB)
    ENUM_CASE(BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB)
    ENUM_CASE(BIND_OPCODE_ADD_ADDR_ULEB)
    ENUM_CASE(BIND_OPCODE_DO_BIND)
    ENUM_CASE(BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB)
    ENUM_CASE(BIND_OPCODE_DO_BIND_ADD_ADDR_IMM_SCALED)
    ENUM_CASE(BIND_OPCODE_DO_BIND_ULEB_TIMES_SKIPPING_ULEB)
    io.enumFallback<Hex8>(value);
  }
};

// This trait is used for 16-byte chars in Mach structures used for strings
typedef char char_16[16];

template <> struct ScalarTraits<char_16> {
  static void output(const char_16 &Val, void *, llvm::raw_ostream &Out);

  static StringRef input(StringRef Scalar, void *, char_16 &Val);
  static bool mustQuote(StringRef S);
};

// This trait is used for UUIDs. It reads and writes them matching otool's
// formatting style.
typedef uint8_t uuid_t[16];

template <> struct ScalarTraits<uuid_t> {
  static void output(const uuid_t &Val, void *, llvm::raw_ostream &Out);

  static StringRef input(StringRef Scalar, void *, uuid_t &Val);
  static bool mustQuote(StringRef S);
};

// Load Command struct mapping traits

#define LOAD_COMMAND_STRUCT(LCStruct)                                          \
  template <> struct MappingTraits<MachO::LCStruct> {                          \
    static void mapping(IO &IO, MachO::LCStruct &LoadCommand);                 \
  };

#include "llvm/Support/MachO.def"

// Extra structures used by load commands
template <> struct MappingTraits<MachO::dylib> {
  static void mapping(IO &IO, MachO::dylib &LoadCommand);
};

template <> struct MappingTraits<MachO::fvmlib> {
  static void mapping(IO &IO, MachO::fvmlib &LoadCommand);
};

template <> struct MappingTraits<MachO::section> {
  static void mapping(IO &IO, MachO::section &LoadCommand);
};

template <> struct MappingTraits<MachO::section_64> {
  static void mapping(IO &IO, MachO::section_64 &LoadCommand);
};

} // namespace llvm::yaml

} // namespace llvm

#endif
