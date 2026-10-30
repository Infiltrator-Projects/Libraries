// SPDX-License-Identifier: GPL-3.0-or-later
// Convert validated GNU gettext catalogues into a self-contained C translation table.
// C++17 is used for safe byte-buffer ownership; the resulting provider retains its C ABI.
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>
using std::string;
struct Translation {string lang,key,value;};
static std::vector<unsigned char> read_bytes(const string& filename) {
  std::ifstream f(filename,std::ios::binary);
  if(!f)throw std::runtime_error("Cannot open translation catalogue: "+filename);
  return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
}
static uint32_t u32(const std::vector<unsigned char>& d,size_t at,bool big) {
  if(at>d.size() || d.size()-at<4)throw std::runtime_error("Truncated MO header");
  if(big)return (uint32_t(d[at])<<24)|(uint32_t(d[at+1])<<16)|(uint32_t(d[at+2])<<8)|d[at+3];
  return uint32_t(d[at])|(uint32_t(d[at+1])<<8)|(uint32_t(d[at+2])<<16)|(uint32_t(d[at+3])<<24);
}
static string slice(const std::vector<unsigned char>& data,uint32_t start,uint32_t length){
  if(start>data.size() || length>data.size()-start)throw std::runtime_error("Invalid MO entry offset");
  return string(reinterpret_cast<const char*>(data.data()+start),length);
}
static void load_mo(const string& file,const string& lang,std::vector<Translation>& result) {
  auto d=read_bytes(file);
  if(d.size()<28)throw std::runtime_error("Truncated MO file");
  bool big=false;
  if(u32(d,0,false)==0x950412de)big=false;
  else if(u32(d,0,true)==0x950412de)big=true;
  else throw std::runtime_error("Invalid GNU MO magic");
  const uint32_t count=u32(d,8,big),ids=u32(d,12,big),texts=u32(d,16,big);
  if(count>100000 || ids>d.size() || texts>d.size() ||
    uint64_t(ids)+uint64_t(count)*8>d.size() ||
    uint64_t(texts)+uint64_t(count)*8>d.size())throw std::runtime_error("Invalid GNU MO index");
  for(uint32_t i=0;i<count;++i) {
    auto key=slice(d,u32(d,ids+8*i+4,big),u32(d,ids+8*i,big));
    auto value=slice(d,u32(d,texts+8*i+4,big),u32(d,texts+8*i,big));
    if(key.empty()||value.empty()||key.find('\0')!=string::npos || value.find('\0')!=string::npos || key==value)continue;
    result.push_back({lang,std::move(key),std::move(value)});
  }
}
static string c_string(const string& in){
  static const char hex[]="01234567";
  string result="\"";
  for(unsigned char c:in) {
    if(c=='\\')result+="\\\\";
    else if(c=='"')result+="\\\"";
    else if(c=='\n')result+="\\n";
    else if(c=='\r')result+="\\r";
    else if(c=='\t')result+="\\t";
    else if(c>=32 && c<=126)result+=char(c);
    else {result+='\\';result+=hex[(c>>6)&7];result+=hex[(c>>3)&7];result+=hex[c&7];}
  }
  return result+"\"";
}
int main(int argc,char** argv) {
  if(argc!=4){std::cerr<<"usage: embed-translations LINGUAS MO_DIR OUTPUT_C\n";return 2;}
  try {
    std::ifstream languages(argv[1]);if(!languages)throw std::runtime_error("No LINGUAS file");
    std::vector<Translation> translations;
    string lang;
    while(std::getline(languages,lang)){
      const auto first=lang.find_first_not_of(" \t");
      if(first==string::npos||lang[first]=='#')continue;
      const auto last=lang.find_last_not_of(" \t\r");
      lang=lang.substr(first,last-first+1);
      if(lang.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_0123456789@-")!=string::npos)
        throw std::runtime_error("Unsafe locale name");
      load_mo(string(argv[2])+"/"+lang+".mo",lang,translations);
    }
    std::sort(translations.begin(),translations.end(),[](const auto& a,const auto& b) {
      if(a.lang!=b.lang)return a.lang<b.lang;
      return a.key<b.key;
    });
    std::ofstream out(argv[3]);if(!out)throw std::runtime_error("Cannot write generated source");
    out<<"// Generated from validated gettext catalogues by native C++17 utility.\n"
          "#include <glib.h>\n#include <stdlib.h>\n#include <string.h>\n"
          "typedef struct {const char *lang,*key,*value;} Translation;\n"
          "static const Translation translations[] = {\n";
    for(const auto& t:translations)out<<"  {"<<c_string(t.lang)<<","<<c_string(t.key)<<","<<c_string(t.value)<<"},\n";
    out<<"  {NULL,NULL,NULL}\n};\n"
          "const gchar *infiltratr_embedded_dgettext(const gchar *domain, const gchar *msgid) {\n"
          "  if (!msgid || !domain || strcmp(domain,GETTEXT_PACKAGE)) return msgid;\n"
          "  const char *env=getenv(\"LANGUAGE\");\n"
          "  if (!env || !*env) env=getenv(\"LC_ALL\");\n"
          "  if (!env || !*env) env=getenv(\"LC_MESSAGES\");\n"
          "  if (!env || !*env) env=getenv(\"LANG\");\n"
          "  if (!env) return msgid;\n"
          "  char lang[64]; size_t n=0;\n"
          "  while (env[n] && env[n]!=':' && env[n]!='.' && env[n]!='@' && n<sizeof(lang)-1) {\n"
          "    lang[n]=env[n]; ++n;\n"
          "  }\n"
          "  lang[n]='\\0';\n"
          "  for (size_t i=0;translations[i].lang;i++)\n"
          "    if (!strcmp(translations[i].key,msgid) &&\n"
          "        (!strcmp(translations[i].lang,lang) ||\n"
          "         (strlen(lang)==2 && !strncmp(translations[i].lang,lang,2))))\n"
          "      return translations[i].value;\n"
          "  return msgid;\n}\n";
    if(!out)throw std::runtime_error("Error writing translation table");
    std::cout<<"Embedded "<<translations.size()<<" translated strings from provider MO catalogues\n";
    return 0;
  }catch(const std::exception& e){std::cerr<<"Native C++ translation embedding failed: "<<e.what()<<"\n";return 1;}
}
