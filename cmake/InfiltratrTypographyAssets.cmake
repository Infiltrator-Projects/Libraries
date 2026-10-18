# SPDX-License-Identifier: GPL-3.0-or-later
# Common owns the canonical typography contract and immutable binary hashes.
# Files are physical resources provided by InfiltratorOS, not by Common.
# The independent three-face bootstrap also works on systems without the OS.
set(INFILTRATR_MB_CORPO_SOURCE_COMMIT
    "66c17fe9ea3ea9678b4028ffea1862a37d488f79")
set(INFILTRATR_MB_CORPO_SOURCE_REPOSITORY "Infiltrator-Projects/InfiltratorOS")
set(INFILTRATR_MB_CORPO_SOURCE_PATH "assets/fonts")
set(INFILTRATR_MB_CORPO_BRAND_REGULAR_FILE "mb_corpo_a_title_cond_regular.ttf")
set(INFILTRATR_MB_CORPO_UI_BOLD_FILE "mb_corpo_s_title_bold.ttf")
set(INFILTRATR_MB_CORPO_UI_REGULAR_FILE "mb_corpo_s_title_regular.ttf")
set(INFILTRATR_MB_CORPO_BRAND_REGULAR_SHA256 "db395c845eeec3a2d24dc7879e0c69029368fb4c7390a531369e0c8157ce3e85")
set(INFILTRATR_MB_CORPO_UI_BOLD_SHA256 "b046af5f32cd166643630f4a70876ec2a13403c01f7cd64958d1d43616dc7e9d")
set(INFILTRATR_MB_CORPO_UI_REGULAR_SHA256 "d452c95fd1017bfee6ad5c7f9d5378a4b1b77a3e03911f85ed3ce0893f69f9c1")

set(INFILTRATR_MB_CORPO_NATIVE_FONT_DIR "/System/Resources/Fonts")
set(INFILTRATR_MB_CORPO_FONTCONFIG_FILE
    "/etc/fonts/conf.d/90-infiltrator-native-fonts.conf")
set(INFILTRATR_MB_CORPO_ENSURE_SCRIPT
    "${CMAKE_CURRENT_LIST_DIR}/InfiltratrEnsureNativeFonts.sh")
