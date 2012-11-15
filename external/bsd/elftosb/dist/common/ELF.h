/*
 * File:	ELF.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_ELF_h_)
#define _ELF_h_

//! \name ELF types
//! Types used in ELF file structures.
//@{
typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef /*off_t*/ uint32_t Elf32_Off;
typedef int32_t Elf32_Sword;
typedef uint32_t Elf32_Word;
//@}

// All ELF structures are byte aligned. Any alignment padding is explicit.
#pragma pack(1)

//! \name File header
//@{

/*!
 * Constants for the various fields of Elf32_Ehdr.e_ident.
 */
enum {
	EI_MAG0 = 0,
	EI_MAG1,
	EI_MAG2,
	EI_MAG3,
	EI_CLASS,
	EI_DATA,
	EI_VERSION,
	EI_PAD,
	EI_NIDENT = 16,
	
	// Magic number.
	ELFMAG0 = 0x7f,
	ELFMAG1 = 'E',
	ELFMAG2 = 'L',
	ELFMAG3 = 'F',
	
	// EI_CLASS
	ELFCLASSNONE = 0,
	ELFCLASS32 = 1,
	ELFCLASS64 = 2,
	
	// EI_DATA
	ELFDATANONE = 0,
	ELFDATA2LSB = 1,
	ELFDATA2MSB = 2
};

/*!
 * \brief ELF file header.
 */
struct Elf32_Ehdr
{
	unsigned char e_ident[EI_NIDENT];	//!< Magic number identifying the file format.
	Elf32_Half e_type;		//!< Identifies the object file format.
	Elf32_Half e_machine;	//!< Specified the architecture for the object file.
	Elf32_Word e_version;	//!< Object file version.
	Elf32_Addr e_entry;		//!< Virtual address of the entry point, or 0.
	Elf32_Off e_phoff;		//!< Program header table offset in bytes, or 0 if no program header table.
	Elf32_Off e_shoff;		//!< Section header table offset in bytes, or 0 if no section header table.
	Elf32_Word e_flags;		//!< Processor-specific flags associated with the file.
	Elf32_Half e_ehsize;	//!< The ELF header's size in bytes.
	Elf32_Half e_phentsize;	//!< Size in bytes of one entry in the program header table.
	Elf32_Half e_phnum;		//!< Number of entries in the program header table.
	Elf32_Half e_shentsize;	//!< Size in bytes of an entry in the section header table.
	Elf32_Half e_shnum;		//!< Number of entries in the section header table.
	Elf32_Half e_shstrndx;	//!< Section header table index of the section name string table.
};

/*!
 * Constants for #Elf32_Ehdr.e_type.
 */
enum {
	ET_NONE,	//!< No file type.
	ET_REL,		//!< Relocatable file.
	ET_EXEC,	//!< Executable file.
	ET_DYN,		//!< Shared object file.
	ET_CORE,	//!< Core file.
	ET_LOPROC,	//!< Low bound of processor-specific file types.
	ET_HIPROC	//!< High bound of processor-specific file types.
};	

/*!
 * ARM-specific #Elf32_Ehdr.e_flags
 */
enum {
	EF_ARM_HASENTRY = 0x02,			//!< #Elf32_Ehdr.e_entry contains a program-loader entry point.
	EF_ARM_SYMSARESORTED = 0x04,	//!< Each subsection of the symbol table is sorted by symbol value.
	EF_ARM_DYNSYMSUSESEGIDX = 0x08,	//!< Symbols in dynamic symbol tables that are defined in sections included in program segment n have #Elf32_Sym.st_shndx = n + 1.
	EF_ARM_MAPSYMSFIRST = 0x10,		//!< Mapping symbols precede other local symbols in the symbol table.
	EF_ARM_EABIMASK = 0xff000000,	//!< This masks an 8-bit version number, the version of the ARM EABI to which this ELF file conforms. The current EABI version is #ARM_EABI_VERSION.
	
	ARM_EABI_VERSION = 0x02000000	//!< Current ARM EABI version.
};

//@}

//! \name Sections
//@{

/*!
 * \brief ELF section header.
 *
 * An object file's section header table lets one locate all the file's
 * sections. The section header table is an array of #Elf32_Shdr structures.
 * A section header table index is a subscript into this array. The ELF
 * header's #Elf32_Ehdr::e_shoff member gives the byte offset from the beginning of
 * the file to the section header table; #Elf32_Ehdr::e_shnum tells how many entries
 * the section header table contains; #Elf32_Ehdr::e_shentsize gives the size in bytes
 * of each entry.
 *
 * Some section header table indexes are reserved. An object file will not
 * have sections for these special indexes:
 *  - #SHN_UNDEF
 *  - #SHN_LORESERVE
 *  - #SHN_LOPROC
 *  - #SHN_HIPROC
 *  - #SHN_ABS
 *  - #SHN_COMMON
 *  - #SHN_HIRESERVE
 */
struct Elf32_Shdr
{
	Elf32_Word sh_name;		//!< The section's name. Index into the section header string table section.
	Elf32_Word sh_type;		//!< Section type, describing the contents and semantics.
	Elf32_Word sh_flags;	//!< Section flags describing various attributes.
	Elf32_Addr sh_addr;		//!< The address at which the section will appear in the memory image, or 0.
	Elf32_Off sh_offset;	//!< Offset from beginning of the file to the first byte in the section.
	Elf32_Word sh_size;		//!< The section's size in bytes.
	Elf32_Word sh_link;		//!< Section header table link index. Interpretation depends on section type.
	Elf32_Word sh_info;		//!< Extra information about the section. Depends on section type.
	Elf32_Word sh_addralign;	//!< Address alignment constraint. Values are 0 and positive powers of 2.
	Elf32_Word sh_entsize;	//!< Size in bytes of section entries, or 0 if the section does not have fixed-size entries.
};

/*!
 * Special section indexes.
 */
enum {
	SHN_UNDEF = 0,
	SHN_LORESERVE = 0xff00,
	SHN_LOPROC = 0xff00,
	SHN_HIPROC = 0xff1f,
	SHN_ABS = 0xfff1,		//!< The symbol has an absolute value that will not change because of relocation.
	SHN_COMMON = 0xfff2,	//!< The symbol labels a common block that has not yet been allocated.
	SHN_HIRESERVE = 0xffff
};

/*!
 * Section type constants.
 */
enum {
	SHT_NULL = 0,
	SHT_PROGBITS = 1,
	SHT_SYMTAB = 2,
	SHT_STRTAB = 3,
	SHT_RELA = 4,
	SHT_HASH = 5,
	SHT_DYNAMIC = 6,
	SHT_NOTE = 7,
	SHT_NOBITS = 8,
	SHT_REL = 9,
	SHT_SHLIB = 10,
	SHT_DYNSYM = 11
};

/*!
 * Section flag constants.
 */
enum {
	SHF_WRITE = 0x1,	//!< Section is writable.
	SHF_ALLOC = 0x2,	//!< Allocate section.
	SHF_EXECINSTR = 0x4	//!< Section contains executable instructions.
};

/*!
 * ARM-specific section flag constants
 */
enum {
	SHF_ENTRYSECT = 0x10000000,	//!< The section contains an entry point.
	SHF_COMDEF = 0x80000000		//!< The section may be multiply defined in the input to a link step.
};

#define BSS_SECTION_NAME ".bss"
#define DATA_SECTION_NAME ".data"
#define TEXT_SECTION_NAME ".text"
#define SHSTRTAB_SECTION_NAME ".shstrtab"
#define STRTAB_SECTION_NAME ".strtab"
#define SYMTAB_SECTION_NAME ".symtab"

//@}

//! \name Segments
//@{

/*!
 * \brief ELF program header.
 *
 * An executable or shared object file's program header table is an array of
 * structures, each describing a segment or other information the system needs
 * to prepare the program for execution. An object file segment contains one
 * or more sections. Program headers are meaningful only for executable and
 * shared object files. A file specifies its own program header size with the
 * ELF header's #Elf32_Ehdr::e_phentsize and #Elf32_Ehdr::e_phnum members.
 */
struct Elf32_Phdr
{
	Elf32_Word p_type;		//!< What type of segment this header describes.
	Elf32_Off p_offset;		//!< Offset in bytes from start of file to the first byte of the segment.
	Elf32_Addr p_vaddr;		//!< Virtual address at which the segment will reside in memory.
	Elf32_Addr p_paddr;		//!< Physical address, for systems where this is relevant.
	Elf32_Word p_filesz;	//!< Number of bytes of file data the segment consumes. May be zero.
	Elf32_Word p_memsz;		//!< Size in bytes of the segment in memory. May be zero.
	Elf32_Word p_flags;		//!< Flags relevant to the segment.
	Elf32_Word p_align;		//!< Alignment constraint for segment addresses. Possible values are 0 and positive powers of 2.
};

/*!
 * Segment type constants.
 */
enum {
	PT_NULL = 0,
	PT_LOAD = 1,
	PT_DYNAMIC = 2,
	PT_INTERP = 3,
	PT_NOTE = 4,
	PT_SHLIB = 5,
	PT_PHDR = 6
};

/*!
 * Program header flag constants.
 */
enum {
	PF_X = 0x1,	//!< Segment is executable.
	PF_W = 0x2,	//!< Segment is writable.
	PF_R = 0x4	//!< Segment is readable.
};

//@}

//! \name Symbol table
//@{

enum {
	STN_UNDEF = 0	//!< Undefined symbol index.
};

/*!
 * \brief ELF symbol table entry.
 *
 * An object file's symbol table holds information needed to locate and
 * relocate a program's symbolic definitions and references. A symbol
 * table index is a subscript into this array. Index 0 both designates
 * the first entry in the table and serves as the undefined symbol index.
 */
struct Elf32_Sym
{
	Elf32_Word st_name;		//!< Index into file's string table.
	Elf32_Addr st_value;	//!< Value associated with the symbol. Depends on context.
	Elf32_Word st_size;		//!< Size associated with symbol. 0 if the symbol has no size or an unknown size.
	unsigned char st_info;	//!< Specified the symbol's type and binding attributes.
	unsigned char st_other;	//!< Currently 0 (reserved).
	Elf32_Half st_shndx;	//!< Section header table index for this symbol.
};

//! \name st_info macros
//! Macros for manipulating the st_info field of Elf32_Sym struct.
//@{
#define ELF32_ST_BIND(i) ((i) >> 4)			//!< Get binding attributes.
#define ELF32_ST_TYPE(i) ((i) & 0x0f)		//!< Get symbol type.
#define ELF32_ST_INFO(b, t) (((b) << 4) + ((t) & 0x0f))	//!< Construct st_info value from binding attributes and symbol type.
//@}

/*!
 * \brief Symbol binding attributes.
 *
 * These constants are mask values.
 */
enum {
	STB_LOCAL = 0,	//!< Local symbol not visible outside the object file.
	STB_GLOBAL = 1,	//!< Symbol is visible to all object files being linked together.
	STB_WEAK = 2,	//!< Like global symbols, but with lower precedence.
	
	// Processor-specific semantics.
	STB_LOPROC = 13,
	STB_HIPROC = 15
};

/*!
 * \brief Symbol types.
 */
enum {
	STT_NOTYPE = 0,		//!< The symbol's type is not specified.
	STT_OBJECT = 1,		//!< The symbol is associated with a data object, such as a variable or array.
	STT_FUNC = 2,		//!< The symbol is associated with a function or other executable code.
	STT_SECTION = 3,	//!< The synmbol is associated with a section. Primarily used for relocation.
	STT_FILE = 4,		//!< A file symbol has STB_LOCAL binding, its section index is SHN_ABS, and it precedes the other STB_LOCAL symbols for the file, if it is present.
	
	STT_LOPROC = 13,	//!< Low bound of processor-specific symbol types.
	STT_HIPROC = 15		//!< High bound of processor-specific symbol types.
};

/*!
 * GHS-specific constants
 */
enum {
	STO_THUMB = 1	//!< This flag is set on #Elf32_Sym.st_other if the symbol is Thumb mode code.
};

#define ARM_SEQUENCE_MAPSYM "$a"
#define DATA_SEQUENCE_MAPSYM "$d"
#define THUMB_SEQUENCE_MAPSYM "$t"

#define THUMB_BL_TAGSYM "$b"
#define FN_PTR_CONST_TAGSYM "$f"
#define INDIRECT_FN_CALL_TAGSYM "$p"
#define MAPPING_SYMBOL_COUNT_TAGSYM "$m"

//@}

#pragma pack()

#endif // _ELF_h_
