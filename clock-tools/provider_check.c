// SPDX-License-Identifier: GPL-3.0-or-later
// Native regression fixture executor and real-ABI PGO trainer.
#include <infiltratr/temporal_clock_resource.h>
#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef const InfiltratrTemporalResourceMetadata *(*MetadataGetter)(void);
typedef const InfiltratrTemporalClockProvider *(*ProviderGetter)(void);
static void fail(const char *message) { fprintf(stderr, "clock validation failed: %s\n", message); exit(1); }
static int64_t signed_number(const char *s) { errno=0; char *e=NULL; intmax_t x=strtoimax(s,&e,10); if(errno==ERANGE && s[0] != '-') { errno=0; unsigned long long raw=strtoull(s,&e,10); if(errno || !e || *e) fail("invalid 64-bit fixture"); return (int64_t)(uint64_t)raw; } if(errno || !e || *e || x<INT64_MIN || x>INT64_MAX) fail("invalid signed integer fixture"); return (int64_t)x; }
static double float_number(const char *s) { errno=0; char *e=NULL; double x=strtod(s,&e); if(errno || !e || *e) fail("invalid coordinate fixture"); return x; }
static bool boolean(const char *s) { if(!strcmp(s,"true")) return true; if(!strcmp(s,"false")) return false; fail("invalid bool"); return false; }
static size_t unsigned_size(const char *s) { errno=0; char *e=NULL; unsigned long long n=strtoull(s,&e,10); if(errno || !e || *e || n>SIZE_MAX) fail("invalid size"); return (size_t)n; }
static uint32_t u32_number(const char *s) { uint64_t x=unsigned_size(s); if(x>UINT32_MAX) fail("invalid u32"); return (uint32_t)x; }
int main(int argc, char **argv)
{
    if (argc<5) fail("usage: provider_check LIB ID NAME train|clock_format|tick [fixture arguments]");
    void *handle=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
    if(!handle){fprintf(stderr,"dlopen: %s\n",dlerror());return 1;}
    void *meta_symbol=dlsym(handle,"infiltratr_temporal_resource_metadata_v1"); MetadataGetter get_meta=NULL; memcpy(&get_meta,&meta_symbol,sizeof(get_meta));
    void *provider_symbol=dlsym(handle,"infiltratr_temporal_clock_provider_v1"); ProviderGetter get_provider=NULL; memcpy(&get_provider,&provider_symbol,sizeof(get_provider));
    if(!get_meta || !get_provider) fail("missing stable ABI exports");
    const InfiltratrTemporalResourceMetadata *m=get_meta();
    const InfiltratrTemporalClockProvider *p=get_provider();
    if(!m || !p || m->struct_size!=sizeof(*m) || p->struct_size!=sizeof(*p) ||
       m->abi_version!=INFILTRATR_TEMPORAL_RESOURCE_METADATA_ABI ||
       p->abi_version!=INFILTRATR_TEMPORAL_CLOCK_PROVIDER_ABI ||
       m->kind!=INFILTRATR_TEMPORAL_RESOURCE_CLOCK ||
       !m->id || !m->name || !p->clock_id || strcmp(m->id,argv[2]) ||
       strcmp(p->clock_id,argv[2]) || strcmp(m->name,argv[3]) ||
       !p->format || !p->next_tick) fail("ABI metadata mismatch");
    const char *op=argv[4];
    if(!strcmp(op,"train")) {
        char buf[512];size_t len=0;uint32_t delay=0;
        const int32_t offsets[]={-43200,0,19800,36000,50400};
        const double lats[]={-66.0,-37.8,0.0,31.8,51.5,69.0};
        const double longs[]={-179.9,-74.0,0.0,35.2,144.9,179.9};
        const char *locales[]={"en_US","ar","ja_JP"};
        for(int64_t i=0;i<2048;++i){
            int64_t instant=(-INT64_C(2208988800)+i*INT64_C(2304007))*INT64_C(1000000)+i*INT64_C(123457);
            int32_t offset=offsets[i%5];double lat=lats[i%6], lon=longs[i%6];
            (void)p->format(p,locales[i%3],instant,offset,i%2!=0,i%3!=0,lat,lon,buf,sizeof(buf),&len);
            (void)p->next_tick(p,instant,offset,i%2!=0,lat,lon,&delay);
        }
        printf("%s: PGO trained 4096 native ABI calls\n",argv[2]);
    } else if(!strcmp(op,"clock_format")) {
        if(argc!=16) fail("clock format fixture has incorrect argument count");
        char out[512];memset(out,'U',sizeof(out));size_t len=123456789;
        const char *locale=!strcmp(argv[11],"__NULL__")?NULL:argv[11];
        size_t capacity=unsigned_size(argv[12]);if(capacity>sizeof(out)) fail("fixture exceeds output capacity");
        bool actual=p->format(p,locale,signed_number(argv[5]),(int32_t)signed_number(argv[6]),
            boolean(argv[7]),boolean(argv[8]),float_number(argv[9]),float_number(argv[10]),
            out,capacity,&len);
        bool expected=boolean(argv[13]);
        if(actual!=expected) fail("clock format success mismatch");
        if(actual){
            if(len!=unsigned_size(argv[15]) || strcmp(out,argv[14])) fail("clock format output mismatch");
        } else {
            if(len!=123456789) fail("failure mutated length");
            for(size_t i=0;i<sizeof(out);++i) if(out[i]!='U') fail("failure mutated output buffer");
            // Expected failed-output hash corresponds to 512 unchanged 'U' bytes.
        }
    } else if(!strcmp(op,"tick")) {
        if(argc!=12) fail("tick fixture has incorrect argument count");
        uint32_t out=123456789;
        bool actual=p->next_tick(p,signed_number(argv[5]),(int32_t)signed_number(argv[6]),
          boolean(argv[7]),float_number(argv[8]),float_number(argv[9]),&out);
        if(actual!=boolean(argv[10]) || out!=u32_number(argv[11])) fail("tick mismatch");
    } else fail("unknown operation");
    dlclose(handle);
    return 0;
}
