/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <infiltratr/filesystem_module_loader.h>
#include <assert.h>
#ifndef FILESYSTEM_MODULE_COMMON_FIXTURE
#error "CMake must provide fixture paths"
#endif
#ifndef FILESYSTEM_MODULE_LEGACY_FIXTURE
#error "CMake must provide fixture paths"
#endif
int main(void)
{
    InfiltratrDynlib library = INFILTRATR_DYNLIB_INIT;
    InfiltratrFilesystemModuleDiscovery out = {0};
    assert(infiltratr_dynlib_open(&library, FILESYSTEM_MODULE_LEGACY_FIXTURE));
    assert(infiltratr_filesystem_module_inspect_loaded(&library, "ofs", &out));
    assert(out.legacy_table && !out.common_api && out.legacy_version == 3u);
    assert(!infiltratr_filesystem_module_inspect_loaded(&library, "ffs", &out));
    assert(!out.common_api && !out.legacy_table && !out.legacy_version);
    infiltratr_dynlib_close(&library);
    assert(infiltratr_dynlib_open(&library, FILESYSTEM_MODULE_COMMON_FIXTURE));
    assert(infiltratr_filesystem_module_inspect_loaded(&library, "fat12", &out));
    assert(out.common_api && !out.legacy_table && !out.legacy_version);
    assert(out.common_api->capabilities == IFS_FS_CAP_PROBE);
    assert(!infiltratr_filesystem_module_inspect_loaded(&library, "fat16", &out));
    assert(!out.common_api);
    infiltratr_dynlib_close(&library);
    return 0;
}
