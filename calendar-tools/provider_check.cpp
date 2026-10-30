// SPDX-License-Identifier: GPL-3.0-or-later
// Native C++20 ABI regression and PGO trainer for Infiltrator calendars.
#include <infiltratr/temporal_calendar.h>
#include <dlfcn.h>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <limits>

using MetadataGetter=const InfiltratrTemporalResourceMetadata*(*)();
using ProviderGetter=const InfiltratrTemporalCalendarProvider*(*)();
[[noreturn]] static void fail(const std::string& s) { throw std::runtime_error(s); }
static long long num(const char* s) { std::size_t pos=0; auto value=std::stoll(s,&pos); if(pos!=std::strlen(s)) fail("Invalid integer"); return value; }
static bool tf(const char* s) { if(std::strcmp(s,"true")==0)return true; if(std::strcmp(s,"false")==0)return false;fail("Invalid boolean");}
static void match(bool yes,const std::string& why) { if(!yes) fail(why); }

int main(int argc,char**argv) {
 try {
  match(argc>=5,"usage: calendar-check MODULE ID NAME train|fields|start|periods|years|calendar_format ...");
  void* handle=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
  if(!handle)fail(dlerror());
  auto metadata_get=reinterpret_cast<MetadataGetter>(dlsym(handle,"infiltratr_temporal_resource_metadata_v1"));
  auto provider_get=reinterpret_cast<ProviderGetter>(dlsym(handle,"infiltratr_temporal_calendar_provider_v1"));
  match(metadata_get && provider_get,"Missing native provider exports");
  auto* meta=metadata_get();auto* p=provider_get();
  match(meta && p && meta->struct_size==sizeof(*meta) && p->struct_size==sizeof(*p)
     && meta->abi_version==INFILTRATR_TEMPORAL_RESOURCE_METADATA_ABI
     && p->abi_version==INFILTRATR_TEMPORAL_CALENDAR_PROVIDER_ABI
     && meta->kind==INFILTRATR_TEMPORAL_RESOURCE_CALENDAR
     && meta->id && meta->name && p->calendar_id
     && std::strcmp(meta->id,argv[2])==0 && std::strcmp(p->calendar_id,argv[2])==0
     && std::strcmp(meta->name,argv[3])==0
     && p->fields_from_jdn && p->format && p->period_start && p->add_periods && p->add_years,"ABI mismatch");
  const std::string op=argv[4];
  if(op=="train") {
    for(std::int64_t i=0;i<2048;++i){
      const std::int64_t jdn=2305448+(i*89)%182621;
      InfiltratrTemporalCalendarFields f{};
      std::int64_t result=0;
      std::array<char,512> buffer{};
      std::size_t length=0;
      (void)p->fields_from_jdn(p,jdn,&f);
      (void)p->period_start(p,jdn,&result);
      (void)p->add_periods(p,jdn,static_cast<int32_t>(i%25-12),&result);
      (void)p->add_years(p,jdn,static_cast<int32_t>(i%9-4),&result);
      for(int part=0;part<6;++part)
        (void)p->format(p,jdn,static_cast<InfiltratrTemporalCalendarDatePart>(part),buffer.data(),buffer.size(),&length);
    }
    std::cout<<argv[2]<<": 20480 native C++ PGO calls\n";
  } else if(op=="fields") {
    match(argc==12,"Wrong fields fixture width");
    InfiltratrTemporalCalendarFields f{12345,12,23,4,true};
    const bool ok=p->fields_from_jdn(p,num(argv[5]),&f);
    match(ok==tf(argv[6]) && f.year==num(argv[7]) && f.month==num(argv[8])
         && f.day==num(argv[9]) && f.auxiliary==num(argv[10])
         && f.special==tf(argv[11]),"Fields mismatch");
  } else if(op=="start" || op=="periods" || op=="years") {
    const bool unary=op=="start";
    match(argc==(unary?8:9),"Wrong navigation fixture width");
    std::int64_t out=1234567890;
    const std::int64_t jd=num(argv[5]);
    const bool ok=unary?p->period_start(p,jd,&out):
       op=="periods"?p->add_periods(p,jd,static_cast<int32_t>(num(argv[6])),&out):
                     p->add_years(p,jd,static_cast<int32_t>(num(argv[6])),&out);
    const int e=unary?6:7;
    match(ok==tf(argv[e]) && out==num(argv[e+1]),"Navigation mismatch");
  } else if(op=="calendar_format") {
    match(argc==11,"Wrong formatting fixture width");
    std::array<char,512> out;out.fill('U');
    std::size_t length=123456789;
    const auto capacity=num(argv[7]);
    match(capacity>=0 && capacity<=512,"Invalid fixture buffer size");
    const bool ok=p->format(p,num(argv[5]),
         static_cast<InfiltratrTemporalCalendarDatePart>(num(argv[6])),
         out.data(),static_cast<std::size_t>(capacity),&length);
    const bool expected=tf(argv[8]);
    match(ok==expected,"Format success mismatch");
    if(ok) {
      match(length==static_cast<std::size_t>(num(argv[10])) &&
            std::string(out.data())==argv[9],"Format content mismatch");
    } else {
      match(length==123456789,"Failed format modified length");
      for(const char c:out)match(c=='U',"Failed format modified buffer");
    }
  } else fail("Unknown fixture operation");
  dlclose(handle);
  return 0;
 }catch(const std::exception& e){std::cerr<<"Calendar verification failed: "<<e.what()<<'\n';return 1;}
}
