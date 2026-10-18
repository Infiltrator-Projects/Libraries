# Extended native typography

The full set of 23 MB Corpo/Daimler faces is owned physically and indexed by [InfiltratorOS's verified manifest](https://github.com/Infiltrator-Projects/InfiltratorOS/blob/main/assets/fonts/SHA256SUMS). The [product font role map](https://github.com/Infiltrator-Projects/InfiltratorOS/blob/main/docs/FONT-ROLES.md) identifies exact TTF internal family names and considered uses across InfiltratorOS, System Monitor, Calculator and Files.

`cmake/InfiltratrEnsureExtendedFonts.sh` is an optional installer-side, POSIX shell provisioning helper. It is additive to the new three-face version 1.010 bootstrap. Older pinned Common consumers retain their own independent legacy contract until upgraded. It pins an immutable InfiltratorOS font source revision, contains the SHA-256 inventory of that revision and installs only missing or mismatched files into persistent `/System/Resources/Fonts`. A verified local OS font seed is preferred, otherwise missing files are downloaded from that pinned revision and checked before atomic publication. Fonts and Fontconfig registration are not owned by an individual package; uninstalling one app must never delete them.

The richer Text, Title, Condensed, Screen and Daimler families are *available*, not mandatory on every screen. Product-owned presentation layers choose semantic typesetting for their own tasks; Common continues to own general Day/Night colours, metrics and a three-face minimum referencing the newer Title binaries. The helper supports `INFILTRATR_NATIVE_ROOT` for safe staging tests and `INFILTRATR_EXTENDED_FONT_SOURCE_DIR` for verified offline payloads.

Font integrity checks do not establish redistribution rights. The font binaries remain outside this Libraries repository.
