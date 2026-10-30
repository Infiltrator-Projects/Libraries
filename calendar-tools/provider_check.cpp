// SPDX-License-Identifier: GPL-3.0-or-later
// Provider ABI regression runner: native C++17, never shipped to the target machine.
#include <infiltratr/temporal_calendar.h>
#include <dlfcn.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <inttypes.h>
#include <limits>
#include <stdexcept>
#include <string>
#include <array>
using std::string;
[[noreturn]] static void fail(const char* what) { std::fprintf(stderr,"calendar native check: %s\n",what);std::exit(1); }
static int64_t as_i64(const char* value){
  errno=0; char* end=nullptr; auto n=::strtoimax(value,&end,10);
  if(errno || !end || *end || n<std::numeric_limits<int64_t>::min() || n>std::numeric_limits<int64_t>::max()) fail("invalid fixture int64");
  return static_cast<int64_t>(n);
}
static int32_t as_i32(const char* value){ const auto n=as_i64(value);if(n<INT32_MIN||n>INT32_MAX) fail("invalid int32");return static_cast<int32_t>(n); }
static size_t as_size(const char* value){auto n=as_i64(value);if(n<0||static_cast<uint64_t>(n)>SIZE_MAX) fail("invalid size");return static_cast<size_t>(n);}
static bool as_bool(const char* s){if(!std::strcmp(s,"true"))return true;if(!std::strcmp(s,"false"))return false;fail("invalid boolean");}
int main(int argc,char** argv) {
  if(argc<5)fail("usage: provider_check LIBRARY ID NAME OP [args expected]");
  void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
  if(!lib){std::fprintf(stderr,"dlopen: %s\n",dlerror());return 1;}
  using MetadataGetter=const InfiltratrTemporalResourceMetadata* (*)();
  using ProviderGetter=const InfiltratrTemporalCalendarProvider* (*)();
  void* sym1=dlsym(lib,"infiltratr_temporal_resource_metadata_v1");
  void* sym2=dlsym(lib,"infiltratr_temporal_calendar_provider_v1");
  MetadataGetter getmeta=nullptr; ProviderGetter getprovider=nullptr;
  static_assert(sizeof(sym1)==sizeof(getmeta) && sizeof(sym2)==sizeof(getprovider));
  std::memcpy(&getmeta,&sym1,sizeof(getmeta));std::memcpy(&getprovider,&sym2,sizeof(getprovider));
  if(!getmeta||!getprovider)fail("required provider symbols missing");
  auto* meta=getmeta();auto* provider=getprovider();
  if(!meta||!provider||meta->struct_size!=sizeof(*meta)||provider->struct_size!=sizeof(*provider)||
    meta->abi_version!=INFILTRATR_TEMPORAL_RESOURCE_METADATA_ABI||
    provider->abi_version!=INFILTRATR_TEMPORAL_CALENDAR_PROVIDER_ABI||
    meta->kind!=INFILTRATR_TEMPORAL_RESOURCE_CALENDAR||
    !meta->id||!meta->name||!provider->calendar_id||
    std::strcmp(meta->id,argv[2])||std::strcmp(provider->calendar_id,argv[2])||
    std::strcmp(meta->name,argv[3])||!provider->fields_from_jdn||
    !provider->format||!provider->period_start||!provider->add_periods||!provider->add_years) fail("ABI mismatch");
  const string op=argv[4];
  if(op=="train"){
    for(int64_t i=0;i<2048;++i){
      int64_t jd=2305448+(i*89)%182621;
      InfiltratrTemporalCalendarFields fields{};
      int64_t result=0;size_t len=0;std::array<char,512> b{};
      provider->fields_from_jdn(provider,jd,&fields);
      provider->period_start(provider,jd,&result);
      provider->add_periods(provider,jd,static_cast<int32_t>(i%25-12),&result);
      provider->add_years(provider,jd,static_cast<int32_t>(i%9-4),&result);
      for(int k=1;k<=5;++k)provider->format(provider,jd,static_cast<InfiltratrTemporalCalendarDatePart>(k),b.data(),b.size(),&len);
    }
    std::printf("%s: trained 18432 ABI calls\n",argv[2]);
  } else if(op=="fields"){
    if(argc!=12)fail("incorrect fields fixture arity");
    InfiltratrTemporalCalendarFields fields{12345,12,23,4,true};
    bool ok=provider->fields_from_jdn(provider,as_i64(argv[5]),&fields);
    if(ok!=as_bool(argv[6])||fields.year!=as_i64(argv[7])||
      fields.month!=as_i32(argv[8])||fields.day!=as_i32(argv[9])||
      fields.auxiliary!=as_i32(argv[10])||fields.special!=as_bool(argv[11]))fail("fields fixture mismatch");
  } else if(op=="start"||op=="periods"||op=="years"){
    const bool has_amount=(op!="start");
    if(argc!=(has_amount?9:8))fail("incorrect navigation fixture arity");
    int64_t result=1234567890;bool ok=false;
    if(op=="start")ok=provider->period_start(provider,as_i64(argv[5]),&result);
    else if(op=="periods")ok=provider->add_periods(provider,as_i64(argv[5]),as_i32(argv[6]),&result);
    else ok=provider->add_years(provider,as_i64(argv[5]),as_i32(argv[6]),&result);
    const int expected=has_amount?7:6;
    if(ok!=as_bool(argv[expected])||result!=as_i64(argv[expected+1]))fail("navigation fixture mismatch");
  } else if(op=="calendar_format"){
    if(argc!=11)fail("incorrect format fixture arity");
    std::array<char,512> buffer;buffer.fill('U');size_t length=123456789;
    const auto capacity=as_size(argv[7]);if(capacity>buffer.size())fail("invalid capacity");
    bool ok=provider->format(provider,as_i64(argv[5]),static_cast<InfiltratrTemporalCalendarDatePart>(as_i32(argv[6])),
                             buffer.data(),capacity,&length);
    if(ok!=as_bool(argv[8]))fail("format outcome mismatch");
    if(ok) {
      if(length!=as_size(argv[10])||string(buffer.data())!=argv[9])fail("format text mismatch");
    } else {
      if(length!=123456789)fail("failed format changed length");
      for(char c:buffer)if(c!='U')fail("failed format changed output");
    }
  } else fail("unknown provider operation");
  dlclose(lib);return 0;
}
