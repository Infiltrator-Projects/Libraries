/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <infiltratr/filesystem_module_loader.h>
#include <assert.h>
#ifndef FILESYSTEM_MODULE_V2_GOOD_FIXTURE
#error "Missing valid V2 fixture"
#endif
#ifndef FILESYSTEM_MODULE_V2_BAD_FIXTURE
#error "Missing invalid V2 fixture"
#endif
int main(void) {
    InfiltratrDynlib library=INFILTRATR_DYNLIB_INIT;
    const IfsFsModuleApiV2 *api=NULL;
    assert(infiltratr_dynlib_open(&library,FILESYSTEM_MODULE_V2_GOOD_FIXTURE));
    assert(infiltratr_filesystem_module_inspect_loaded_v2(&library,"fat12",&api));
    assert(api && ifs_fs_api_v2_valid(api,"fat12"));
    assert(!infiltratr_filesystem_module_inspect_loaded_v2(&library,"ntfs",&api));
    assert(!api);
    infiltratr_dynlib_close(&library);

    assert(infiltratr_dynlib_open(&library,FILESYSTEM_MODULE_V2_BAD_FIXTURE));
    assert(!infiltratr_filesystem_module_inspect_loaded_v2(&library,"fat12",&api));
    assert(!api);
    infiltratr_dynlib_close(&library);
    return 0;
}
