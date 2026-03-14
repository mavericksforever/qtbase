# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Need an explicit call at the top level. This is the absolute minimum version
# needed to configure the project with any combination of enabled features.
# The call to qt_build_repo_begin() will upgrade policies further.
#
# The absolute minimum version for building Qt is 3.16 because for metatype.json generation, we
# depend on being able to find the location of json files created by AUTOMOC/moc.
cmake_minimum_required(VERSION 3.16)

# Get the repo version and CMake policy details
include(.cmake.conf)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/QtBaseHelpers.cmake)

qt_internal_check_if_path_has_symlinks("${CMAKE_BINARY_DIR}")

# This needs to be called before the first project() call.
qt_internal_qtbase_run_autodetect()

# This call will load any provided cmake toolchain file.
project(QtBase
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt Base Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C ASM
)

set(QT_BUILD_EXTRA_IDE_FILE_PATTERNS bin/* libexec/*)

# macOS < 10.13 lacks aligned allocation in libc++.
# -fno-aligned-allocation prevents the compiler from emitting calls to aligned
# new/delete. A polyfill in src/corelib/global/qaligned_alloc_mavericks.cpp
# provides fallback symbols for any remaining references.
if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS "10.13")
    add_compile_options(-fno-aligned-allocation -faligned-new)
endif()
# Disable ObjC runtime optimizations that require macOS 10.14.4+/10.15+
# (objc_alloc_init, objc_opt_class, objc_opt_isKindOfClass, etc.)
if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS "10.14")
    add_compile_options(-fno-objc-msgsend-selector-stubs)
endif()

qt_internal_qtbase_build_repo()
