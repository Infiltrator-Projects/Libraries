/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <infiltratr/filesystem_module_loader.h>
#include <assert.h>
#ifndef FILESYSTEM_MODULE_V2_GOOD_FIXTURE
#error "Missing V2 good fixture"
#endif
#ifndef FILESYSTEM_MODULE_V2_BAD_FIXTURE
#error "Missing V2 bad fixture"
#endif
#ifndef FILESYSTEM_MODULE_COMMON_FIXTURE
#error "Missing V1 fallback fixture"
#endif
#ifndef FILESYSTEM_MODULE_LEGACY_FIXTURE
#error "Missing legacy fallback fixture"
#endif
int main(void) {
    InfiltratrDynlib so=INFILTRATR_DYNLIB_INIT;
    InfiltratrFilesystemModuleDiscoveryV2 out={0};
    assert(infiltratr_dynlib_open(&so,FILESYSTEM_MODULE_V2_GOOD_FIXTURE));
    assert(infiltratr_filesystem_module_inspect_loaded_v2(&so,"fat12",&out));
    assert(out.v2 && out.v1 && out.v1==out.v2->base_v1);
    assert(out.v2->abi_version==2u && !out.legacy_table);
    assert(!infiltratr_filesystem_module_inspect_loaded_v2(&so,"fat16",&out));
    assert(!out.v2 && !out.v1 && !out.legacy_table);
    infiltratr_dynlib_close(&so);

    assert(infiltratr_dynlib_open(&so,FILESYSTEM_MODULE_V2_BAD_FIXTURE));
    /* The fixture also exports a fully valid V1: never downgrade. */
    assert(!infiltratr_filesystem_module_inspect_loaded_v2(&so,"fat12",&out));
    assert(!out.v2 && !out.v1 && !out.legacy_table);
    infiltratr_dynlib_close(&so);

    assert(infiltratr_dynlib_open(&so,FILESYSTEM_MODULE_COMMON_FIXTURE));
    assert(infiltratr_filesystem_module_inspect_loaded_v2(&so,"fat12",&out));
    assert(!out.v2 && out.v1 && !out.legacy_table);
    infiltratr_dynlib_close(&so);

    assert(infiltratr_dynlib_open(&so,FILESYSTEM_MODULE_LEGACY_FIXTURE));
    assert(infiltratr_filesystem_module_inspect_loaded_v2(&so,"ofs",&out));
    assert(!out.v2 && !out.v1 && out.legacy_table && out.legacy_version==3u);
    infiltratr_dynlib_close(&so);
    return 0;
}
