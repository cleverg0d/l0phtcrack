# BundleLinuxQt.cmake — run as cmake -P script at POST_BUILD time on Linux.
# Bundles Qt5 libs, ICU, xcb, and Qt platform plugins into dist/lib/ and dist/platforms/.
# Usage: cmake -DLC7_BINARY=<path/to/lc7> -DDIST_DIR=<dist/> -P BundleLinuxQt.cmake

cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED LC7_BINARY OR NOT DEFINED DIST_DIR)
    message(FATAL_ERROR "BundleLinuxQt.cmake: LC7_BINARY and DIST_DIR must be set")
endif()

# -----------------------------------------------------------------------
# Helper: run ldd on a binary, collect all lines that match a pattern.
# Returns list of resolved library paths.
# -----------------------------------------------------------------------
function(ldd_collect binary pattern out_var)
    execute_process(
        COMMAND ldd "${binary}"
        OUTPUT_VARIABLE ldd_out
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(result "")
    string(REPLACE "\n" ";" ldd_lines "${ldd_out}")
    foreach(line ${ldd_lines})
        if(line MATCHES "${pattern}")
            # Lines look like:  libFoo.so.5 => /path/to/libFoo.so.5 (0xaddr)
            if(line MATCHES "=> ([^ ]+) ")
                set(lib_path "${CMAKE_MATCH_1}")
                if(EXISTS "${lib_path}" AND NOT lib_path MATCHES "^not$")
                    list(APPEND result "${lib_path}")
                endif()
            endif()
        endif()
    endforeach()
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

# -----------------------------------------------------------------------
# Bundle Qt5 shared libs (resolve symlinks so we get the real file)
# -----------------------------------------------------------------------
set(LIB_DIR "${DIST_DIR}/lib")
file(MAKE_DIRECTORY "${LIB_DIR}")

ldd_collect("${LC7_BINARY}" "libQt5" qt5_libs)
foreach(lib ${qt5_libs})
    get_filename_component(libname "${lib}" NAME)
    set(dest "${LIB_DIR}/${libname}")
    if(NOT EXISTS "${dest}")
        file(COPY "${lib}" DESTINATION "${LIB_DIR}")
        message(STATUS "BundleLinuxQt: bundled ${libname}")
    endif()
endforeach()

# -----------------------------------------------------------------------
# Bundle ICU (required by Qt5Core)
# -----------------------------------------------------------------------
ldd_collect("${LC7_BINARY}" "libicu" icu_libs)
foreach(lib ${icu_libs})
    get_filename_component(libname "${lib}" NAME)
    set(dest "${LIB_DIR}/${libname}")
    if(NOT EXISTS "${dest}")
        file(COPY "${lib}" DESTINATION "${LIB_DIR}")
        message(STATUS "BundleLinuxQt: bundled ICU ${libname}")
    endif()
endforeach()

# -----------------------------------------------------------------------
# Bundle Qt xcb platform plugin + offscreen + minimal
# -----------------------------------------------------------------------
set(PLATFORMS_DIR "${DIST_DIR}/platforms")
file(MAKE_DIRECTORY "${PLATFORMS_DIR}")

# Find Qt plugins dir via qmake or known paths
set(qt_plugin_candidates "")

# Try Qt5Core location — plugins are usually in ../plugins relative to lib
foreach(qt5lib ${qt5_libs})
    if(qt5lib MATCHES "libQt5Core")
        get_filename_component(qt_lib_dir "${qt5lib}" DIRECTORY)
        list(APPEND qt_plugin_candidates "${qt_lib_dir}/../plugins/platforms")
        list(APPEND qt_plugin_candidates "${qt_lib_dir}/qt5/plugins/platforms")
        list(APPEND qt_plugin_candidates "/usr/lib/x86_64-linux-gnu/qt5/plugins/platforms")
        list(APPEND qt_plugin_candidates "/usr/lib/aarch64-linux-gnu/qt5/plugins/platforms")
        list(APPEND qt_plugin_candidates "/usr/lib/qt5/plugins/platforms")
        list(APPEND qt_plugin_candidates "/usr/lib64/qt5/plugins/platforms")
    endif()
endforeach()

foreach(pdir ${qt_plugin_candidates})
    get_filename_component(pdir_abs "${pdir}" ABSOLUTE)
    if(EXISTS "${pdir_abs}")
        foreach(plugin libqxcb.so libqoffscreen.so libqminimal.so)
            if(EXISTS "${pdir_abs}/${plugin}" AND NOT EXISTS "${PLATFORMS_DIR}/${plugin}")
                file(COPY "${pdir_abs}/${plugin}" DESTINATION "${PLATFORMS_DIR}")
                message(STATUS "BundleLinuxQt: bundled platform plugin ${plugin}")
            endif()
        endforeach()
        break()  # found the dir, stop
    endif()
endforeach()

# -----------------------------------------------------------------------
# NOTE: We intentionally do NOT bundle libxcb*, libX11*, libXrender, libSM,
# libICE, libXau, libXdmcp, libGL*, libEGL* or any other X11/GL stack libs.
#
# Reason: the X11 stack (libxcb, libX11, libXcursor, etc.) is always present
# on any system that has a working DISPLAY. More critically, bundling libxcb
# causes a fatal "double xcb" ABI conflict: system libXcursor (loaded by Qt
# for cursor theming) links against the SYSTEM libxcb internally, but
# LD_LIBRARY_PATH causes it to find OUR bundled libxcb instead. Two different
# libxcb versions in the same process → SIGSEGV in xcb_connect().
#
# libxcb ABI is extremely stable (unchanged since xcb 1.x). Our libqxcb.so
# built on Ubuntu 24.04 works correctly with any system libxcb.so.1.
#
# libGL / Mesa: we provide swrast_dri.so + LIBGL_ALWAYS_SOFTWARE=1 so the
# system GL stack is not needed for rendering; Mesa software rasteriser is
# self-contained. libGL.so.1 from the system is loaded but only used as a
# passthrough dispatcher — its ABI is also rock-solid.
# -----------------------------------------------------------------------
message(STATUS "BundleLinuxQt: X11/xcb/GL libs intentionally NOT bundled (use system)")

# -----------------------------------------------------------------------
# Generate qt.conf next to the lc7 binary.
# This overrides the plugin search path that is baked into libQt5Core.so.5
# at compile time (e.g. /usr/lib/x86_64-linux-gnu/qt5/plugins).
# Without qt.conf, Qt finds and tries to load SYSTEM platform plugins that
# are ABI-incompatible with our bundled Qt Core → SIGABRT / segfault on
# any machine whose system Qt differs from the build host.
# With qt.conf, Qt searches ONLY dist/ and dist/platforms/ for plugins.
# -----------------------------------------------------------------------
get_filename_component(lc7_dir "${LC7_BINARY}" DIRECTORY)
file(WRITE "${lc7_dir}/qt.conf"
"[Paths]\nPrefix = .\nPlugins = .\nLibraries = lib\n")
message(STATUS "BundleLinuxQt: wrote qt.conf → ${lc7_dir}/qt.conf")

message(STATUS "BundleLinuxQt: done — libs in ${LIB_DIR}, platform plugins in ${PLATFORMS_DIR}")
