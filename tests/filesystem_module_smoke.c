/* SPDX-License-Identifier: GPL-3.0-or-later
 * Header-only, freestanding ABI compatibility checks.
 */
#include <infiltratr/filesystem_module.h>
#include <assert.h>
#include <string.h>
typedef struct {
    uint32_t abi_version, structure_bytes;
    const char *format_id, *name;
    uint64_t native_capabilities; /* deliberately NOT universal */
} MockLegacy;
static int mock_probe(const IfsFsHostIoV1 *io, uint32_t *variant)
{
    (void)io; if (variant) *variant = 0; return IFS_FS_OK;
}
static int mock_mount(const IfsFsHostIoV1 *io, void **vol,
                      IfsFsVolumeInfoV1 *info)
{
    (void)io; (void)info; *vol = (void *)1; return IFS_FS_OK;
}
static int mock_unmount(void *vol) { return vol ? IFS_FS_OK : IFS_FS_INVALID; }
static int mock_stat(void *vol, const char *path, IfsFsEntryV1 *out)
{
    (void)vol; (void)path; (void)out; return IFS_FS_NOT_FOUND;
}
static int mock_read(void *vol, const char *p, uint64_t at, void *b, uint32_t n)
{
    (void)vol; (void)p; (void)at; (void)b; (void)n; return IFS_FS_OK;
}
int main(void)
{
    static const char *ids[] = {
        "fat12", "fat16", "fat32", "ofs", "ffs", "exfat",
        "ext2", "minix", "bfs"
    };
    for (unsigned i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i) {
        MockLegacy legacy = {1u, sizeof(MockLegacy), ids[i], "Legacy", 1u};
        assert(ifs_fs_legacy_prefix_valid(&legacy, 1u, ids[i]));
        assert(!ifs_fs_legacy_prefix_valid(&legacy, 2u, ids[i]));
        assert(!ifs_fs_legacy_prefix_valid(&legacy, 1u, "other"));
        legacy.structure_bytes = sizeof(IfsFsLegacyPrefix) - 1u;
        assert(!ifs_fs_legacy_prefix_valid(&legacy, 1u, ids[i]));
    }
    assert(!ifs_fs_format_id_valid("FAT16"));
    assert(!ifs_fs_format_id_valid("../fat12"));
    assert(!ifs_fs_format_id_valid(""));
    assert(!ifs_fs_format_id_valid(NULL));
    assert(ifs_fs_format_id_equal("ofs", "ofs"));
    assert(!ifs_fs_format_id_equal("ofs", "ffs"));
    {
        IfsFsModuleApiV1 api = {0};
        api.abi_version = 1;
        api.structure_bytes = sizeof(api);
        api.format_id = "ofs";
        api.capabilities = IFS_FS_CAP_PROBE | IFS_FS_CAP_MOUNT |
                           IFS_FS_CAP_READ;
        api.probe = mock_probe;
        api.mount = mock_mount;
        api.unmount = mock_unmount;
        api.stat = mock_stat;
        api.read = mock_read;
        assert(ifs_fs_common_api_valid(&api, "ofs"));
        assert(!ifs_fs_common_api_valid(&api, "ffs"));
        api.capabilities |= IFS_FS_CAP_WRITE; /* no callback: reject */
        assert(!ifs_fs_common_api_valid(&api, "ofs"));
        api.capabilities &= ~IFS_FS_CAP_WRITE;
        api.capabilities |= IFS_FS_CAP_DEFRAG_MOVE; /* not defined yet */
        assert(!ifs_fs_common_api_valid(&api, "ofs"));
        api.capabilities &= ~IFS_FS_CAP_DEFRAG_MOVE;
        api.abi_version = 2;
        assert(!ifs_fs_common_api_valid(&api, "ofs"));
    }
    return 0;
}
