# Native Linux font resources

The canonical MB Corpo fonts are shared machine resources. As of Common
1.19.58, the three core roles use the newer version 1.010 binaries:

- Brand regular: `mb_corpo_a_title_cond_regular.ttf`
- UI bold: `mb_corpo_s_title_bold.ttf`
- UI regular: `mb_corpo_s_title_regular.ttf`

Their internal family names are unchanged. These replace three older
version 1.000 binaries with the same glyph outlines and metrics.

`cmake/InfiltratrEnsureNativeFonts.sh` verifies these three assets by
SHA-256. It first reuses an existing valid cache entry, then a verified
InfiltratorOS local seed under `/usr/lib/infiltratoros/font-seed`, and only
if necessary downloads an immutable, pinned file from InfiltratorOS. A
standalone application installer therefore works without the OS package,
while an InfiltratorOS installation never needs another font download.

InfiltratorOS supplies all 23 Mercedes/Daimler faces; the independent
`InfiltratrEnsureExtendedFonts.sh` helper offers the complete set on
machines without InfiltratorOS.

Both helpers install the persistent cache under `/System/Resources/Fonts`
and register it using
`/etc/fonts/conf.d/90-infiltrator-native-fonts.conf`. Application uninstalls
must not delete machine-wide fonts. The core helper never recreates the
retired three filenames.

Consumers using prior Common source revisions may still refer to the old
filenames and hashes. They must update their pinned Common revision and
any copied font bootstrap before removing the old resources.
