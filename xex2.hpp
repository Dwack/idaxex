#pragma once
#include <cstdint>
#include <vector>
#include "xex_optheaders.hpp"
#include "pe_structs.hpp"

// XEX header magic values
#define MAGIC_XEX1 0x58455831  // 'XEX1'
#define MAGIC_XEX2 0x58455832  // 'XEX2'
#define MAGIC_XEX2D 0x5845582D // 'XEX-'
#define MAGIC_XEX25 0x58455825 // 'XEX%'
#define MAGIC_XEX3F 0x5845583F // 'XEX?'

// Basefile magic values
#define MAGIC_XUIZ 0x5A495558  // 'ZIUX'

// Function pointer types, these let us support both IDA's IO functions & regular C's IO
typedef size_t(*read_fn)(void* buffer, size_t element_size, size_t element_count, void* file);
typedef int(*seek_fn)(void* file, long long offset, int origin);
typedef long long(*tell_fn)(void* file);
typedef int(*dbgmsg_fn)(const char* format, ...);
int stdio_msg(const char* format, ...); // xex2.cpp

struct XEXFunction
{
  uint32_t ThunkAddr = 0;
  uint32_t FuncAddr = 0;
};

class XEXFile
{
  // IO function pointers
  read_fn read;
  seek_fn seek;
  tell_fn tell;
  dbgmsg_fn dbgmsg;

  uint32_t data_length_; // length of file data (filesize - headersize)

  xex::XexHeader xex_header_;
  std::map<uint32_t, uint32_t> directory_entries_;

  int key_index_;
  uint8_t session_key_[0x10];

  std::vector<uint8_t> xex_headers_;
  std::vector<uint8_t> pe_data_;

  // Values from SecurityInfo
  xe::be<uint32_t> image_size_;
  uint8_t image_key_[0x10];
  xe::be<xex::GameRegion> game_regions_;
  xex::ImageFlags image_flags_;
  xex::AllowedMediaTypes media_types_;
  xe::be<uint32_t> base_address_;
  xe::be<uint32_t> export_table_va_;

  // Values of various optional headers
  xe::be<uint32_t> entry_point_;
  xex_opt::XexFileDataDescriptor* data_descriptor_;
  xex_opt::XexPrivileges privileges_;
  xex_opt::XexPrivileges32 privileges32_;
  xex_opt::XexExecutionId* execution_id_;
  xex_opt::XexVitalStats* vital_stats_;
  xex_opt::XexTlsData* tls_data_;

  // Imports & Exports
  std::vector<xex_opt::XexImageLibraryVersion> libraries_;
  std::map<std::string, std::map<uint32_t, XEXFunction>> imports_;
  std::map<uint32_t, XEXFunction> exports_;
  std::map<std::string, xex_opt::XexImportTable> import_tables_;
  std::string exports_libname_ = "";

  // Sections from XEX headers
  std::vector<IMAGE_SECTION_HEADER> xex_sections_;

  // Sections from PE headers (includes XEX sections above)
  std::vector<IMAGE_SECTION_HEADER> sections_;

  bool read_imports(void* file);
  bool read_exports(void* file);

  bool read_secinfo(void* file);

  bool read_basefile(void* file, int key_index);
  bool read_basefile_raw(void* file, bool encrypted);
  bool read_basefile_uncompressed(void* file, bool encrypted);
  bool read_basefile_compressed(void* file, bool encrypted);

  bool pe_load(const uint8_t* data);
  bool pe_load_imports(const uint8_t* data);
  bool pe_load_exports(const uint8_t* data);
  uint32_t pe_rva_to_offset(uint32_t rva);

public:
  static bool VerifyBaseFileHeader(const uint8_t* data);

  XEXFile() { 
#if fread != dont_use_fread
    read = (read_fn)fread; seek = (seek_fn)_fseeki64; tell = (tell_fn)_ftelli64; dbgmsg = stdio_msg;
#endif
  }
  void use_ida_io();

  bool Read(void* file);

  const xex::XexHeader& header() { return xex_header_; }
  uint32_t base_address() { return base_address_; }
  uint32_t image_size() { return image_size_; }
  uint32_t entry_point() { return entry_point_; }

  const uint8_t* xex_headers() { return xex_headers_.data(); }
  const uint8_t* pe_data() { return pe_data_.data(); }

  const std::vector<IMAGE_SECTION_HEADER>& sections() { return sections_; }
  const std::vector<IMAGE_SECTION_HEADER>& xex_sections() { return xex_sections_; }

  const std::map<std::string, std::map<uint32_t, XEXFunction>>& imports() { return imports_; }
  const std::map<uint32_t, XEXFunction>& exports() { return exports_; }

  std::map<std::string, xex_opt::XexImportTable>& import_tables() { return import_tables_; }
  const std::string& exports_libname() { return exports_libname_; }

  uint32_t opt_header(uint32_t id);
  void* opt_header_ptr(uint32_t id);
};