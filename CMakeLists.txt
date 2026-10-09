cmake_minimum_required(VERSION 3.20)

# ---------------------------------------------------------------------------
# macOS SDK/ARCH pinning — must run BEFORE project() because project() is what
# triggers the compiler-ABI check, and on a Mac with a mismatched CLT SDK the
# SDK's .tbd stubs (libSystem.B.tbd, libc++.1.tbd, OpenGL.tbd) carry arch
# tokens (e.g. "arm64e.x1-macos") that the active ld/tapi can't parse:
#     tapi error: malformed file .../libSystem.B.tbd:4:20: unknown architecture
# That breaks even the trivial compiler test. The usual cause is a stale
# CMAKE_OSX_SYSROOT cached from a previous (removed) Command Line Tools install,
# or a freshly-updated CLT whose SDK is newer than the installed ld.
#
# Strategy: pin CMAKE_OSX_SYSROOT to the SDK xcrun actually reports (the one
# matching the active toolchain) and set an explicit arch matching the host.
# Pass -DIW_SYSROOT=<path> to override (e.g. a known-good older SDK).
#
# A stale CMAKE_OSX_SYSROOT cached from a PREVIOUS configure (pointing at a
# since-removed CLT SDK) is the #1 real-world cause of the .tbd malformed-arch
# failure, and "NOT DEFINED CMAKE_OSX_SYSROOT" does NOT catch it — a cached
# value counts as DEFINED. So on a non-cross Apple build we ALWAYS re-pin from
# xcrun unless the user explicitly overrode with -DIW_SYSROOT (cache FORCE
# overwrites the stale entry).
if(APPLE AND NOT CMAKE_CROSSCOMPILING)
  if(DEFINED IW_SYSROOT)
    set(CMAKE_OSX_SYSROOT "${IW_SYSROOT}" CACHE PATH "macOS SDK sysroot (pinned before project())" FORCE)
    message(STATUS "Using -DIW_SYSROOT override: ${IW_SYSROOT}")
  else()
    execute_process(COMMAND xcrun --show-sdk-path
      OUTPUT_VARIABLE iw_sdk_path OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET RESULT_VARIABLE iw_xcrun_rc)
    if(iw_xcrun_rc EQUAL 0)
      if(EXISTS "${iw_sdk_path}/usr/lib/libSystem.tbd")
        set(CMAKE_OSX_SYSROOT "${iw_sdk_path}" CACHE PATH "macOS SDK sysroot (pinned before project())" FORCE)
        message(STATUS "Pinned macOS SDK sysroot: ${iw_sdk_path}")
      else()
        set(CMAKE_OSX_SYSROOT "${iw_sdk_path}" CACHE PATH "macOS SDK sysroot (pinned before project())" FORCE)
        message(WARNING "xcrun SDK path ${iw_sdk_path} lacks usr/lib/libSystem.tbd; using it anyway")
      endif()
    else()
      # xcrun itself failed (CLT half-installed / not on PATH). A cached
      # CMAKE_OSX_SYSROOT — if any — is now untrustworthy; warn loudly instead
      # of silently using a possibly-stale root.
      message(WARNING "xcrun --show-sdk-path failed (rc=${iw_xcrun_rc}); cannot pin the macOS SDK. If the build fails with 'malformed file ...tbd ... unknown architecture', run 'xcode-select --install' or pass -DIW_SYSROOT=<sdk path>.")
    endif()
  endif()
  if(NOT DEFINED CMAKE_OSX_ARCHITECTURES)
    execute_process(COMMAND uname -m OUTPUT_VARIABLE iw_host_arch OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    if(iw_host_arch STREQUAL "arm64")
      set(CMAKE_OSX_ARCHITECTURES arm64 CACHE STRING "macOS build arch" FORCE)
    elseif(iw_host_arch STREQUAL "x86_64")
      set(CMAKE_OSX_ARCHITECTURES x86_64 CACHE STRING "macOS build arch" FORCE)
    endif()
  endif()
endif()

project(DemoEngine VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
option(IW_BUILD_TESTS "Build the engine and wireframe-demo tests" ON)
# Off by default so a fresh toolchain (new Apple clang / MSVC) can't break a
# user's first build over an unrelated new warning. CI sets this ON to keep the
# strictness where the toolchain is controlled.
option(IW_WARNINGS_AS_ERRORS "Treat project warnings as errors" OFF)
function(iw_warnings target)
 if(MSVC)
  target_compile_options(${target} PRIVATE /W4 $<$<BOOL:${IW_WARNINGS_AS_ERRORS}>:/WX>)
 else()
  target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic $<$<BOOL:${IW_WARNINGS_AS_ERRORS}>:-Werror>)
 endif()
endfunction()

# --- Engine core (no GL): geometry, timeline, effects, font, image decode ---
add_library(de_core engine/src/Geometry.cpp engine/src/Timeline.cpp engine/src/TextScroller.cpp engine/src/Image.cpp engine/src/Effects.cpp)
target_include_directories(de_core PUBLIC engine/include thirdparty)
iw_warnings(de_core)

# --- Wireframe demo content (scenes + procedural geometry; no GL) ----------
add_library(wireframe_content demos/wireframe/src/Scene.cpp demos/wireframe/src/AdvancedGeometry.cpp demos/wireframe/src/ShowpieceGeometry.cpp)
target_include_directories(wireframe_content PUBLIC demos/wireframe/include)
target_link_libraries(wireframe_content PUBLIC de_core)
iw_warnings(wireframe_content)

# --- Dependency detection --------------------------------------------------
# Common install prefixes searched by the find_path/find_library fallbacks
# below (Homebrew, MacPorts, /usr/local, and — on Linux — the system lib dir).
set(IW_SEARCH_INC /opt/homebrew/include /opt/local/include /usr/local/include /usr/include)
set(IW_SEARCH_LIB /opt/homebrew/lib /opt/local/lib /usr/local/lib /usr/lib /usr/lib64)

find_package(OpenGL QUIET)
find_package(glfw3 QUIET)
find_package(PkgConfig QUIET)

# GLFW: prefer the imported target; fall back to a hand-rolled imported target
# (carries the found path) when find_package(glfw3) didn't create one; fall back
# to the bare name (linker default search) as a last resort.
if(glfw3_FOUND AND NOT TARGET GLFW::GLFW3)
 find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h HINTS ${IW_SEARCH_INC})
 # Prefer the shared library (glfw): Homebrew also ships a static libglfw3.a, which would need
 # the Cocoa/IOKit/CoreFoundation frameworks (and the Objective-C runtime) added by hand.
 find_library(GLFW_LIBRARY NAMES glfw glfw3 HINTS ${IW_SEARCH_LIB})
 if(GLFW_INCLUDE_DIR AND GLFW_LIBRARY)
  add_library(GlfwFallback UNKNOWN IMPORTED)
  set_target_properties(GlfwFallback PROPERTIES IMPORTED_LOCATION "${GLFW_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${GLFW_INCLUDE_DIR}")
  if(APPLE AND GLFW_LIBRARY MATCHES "\\.a$")
   set_property(TARGET GlfwFallback PROPERTY INTERFACE_LINK_LIBRARIES "-framework Cocoa" "-framework IOKit" "-framework CoreFoundation")
  endif()
 endif()
endif()
if(TARGET GLFW::GLFW3)
 set(IW_GLFW_TARGET GLFW::GLFW3)
elseif(TARGET GlfwFallback)
 set(IW_GLFW_TARGET GlfwFallback)
else()
 # Last resort: the bare "glfw" name (linker default search). This only works
 # if glfw3 sits in a default search dir; on a fresh Mac it usually does not
 # (it's in /opt/homebrew/lib). Warn so a link failure isn't a mystery.
 set(IW_GLFW_TARGET glfw)
 message(WARNING "glfw3 found by find_package but no usable library target/path was resolved; falling back to the bare 'glfw' link name. If linking fails, install GLFW (macOS: brew install glfw; Linux: apt install libglfw3-dev).")
endif()

# SDL2: CMake config, then pkg-config, then raw find_path/find_library
# (Homebrew/CLT ship SDL2 with no CMake config and no pkg-config on a fresh Mac).
find_package(SDL2 CONFIG QUIET)
if(TARGET SDL2::SDL2)
 set(IW_SDL_TARGET SDL2::SDL2)
elseif(PkgConfig_FOUND)
 pkg_check_modules(SDL2 QUIET IMPORTED_TARGET sdl2)
 if(TARGET PkgConfig::SDL2)
  set(IW_SDL_TARGET PkgConfig::SDL2)
 endif()
endif()
if(NOT IW_SDL_TARGET)
 find_path(SDL2_INCLUDE_DIR SDL.h HINTS ${IW_SEARCH_INC})
 find_library(SDL2_LIBRARY NAMES SDL2 HINTS ${IW_SEARCH_LIB})
 if(SDL2_INCLUDE_DIR AND SDL2_LIBRARY)
  add_library(iw_sdl2 UNKNOWN IMPORTED)
  set_target_properties(iw_sdl2 PROPERTIES IMPORTED_LOCATION "${SDL2_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}")
  set(IW_SDL_TARGET iw_sdl2)
 endif()
endif()

# libopenmpt: pkg-config, then raw find_path/find_library.
if(PkgConfig_FOUND)
 pkg_check_modules(OPENMPT QUIET IMPORTED_TARGET libopenmpt)
endif()
if(TARGET PkgConfig::OPENMPT)
 set(IW_OPENMPT_TARGET PkgConfig::OPENMPT)
else()
 find_path(OPENMPT_INCLUDE_DIR libopenmpt/libopenmpt.h HINTS ${IW_SEARCH_INC})
 find_library(OPENMPT_LIBRARY NAMES openmpt HINTS ${IW_SEARCH_LIB})
 if(OPENMPT_INCLUDE_DIR AND OPENMPT_LIBRARY)
  add_library(iw_openmpt UNKNOWN IMPORTED)
  set_target_properties(iw_openmpt PROPERTIES IMPORTED_LOCATION "${OPENMPT_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${OPENMPT_INCLUDE_DIR}")
  set(IW_OPENMPT_TARGET iw_openmpt)
 endif()
endif()


# --- Engine runtime (window, GL renderer, audio, run loop) ------------------
if(OpenGL_FOUND AND glfw3_FOUND)
 if(NOT EXISTS ${CMAKE_SOURCE_DIR}/engine/shaders/wire.vert)
  message(FATAL_ERROR "engine/shaders/ (wire.vert) missing; cannot build the engine")
 endif()
 add_library(de_engine engine/src/Engine.cpp engine/src/Renderer.cpp engine/src/Audio.cpp)
 target_include_directories(de_engine PUBLIC engine/include thirdparty)
 target_link_libraries(de_engine PUBLIC de_core ${IW_GLFW_TARGET})
 # macOS OpenGL: prefer a real dylib over the CLT SDK's -framework OpenGL (whose .tbd stub some
 # ld/tapi versions misparse), and add the include dir so <OpenGL/gl3.h> resolves.
 if(APPLE)
  find_path(IW_OPENGL_INCLUDE_DIR OpenGL/gl3.h HINTS ${IW_SEARCH_INC} ${OPENGL_INCLUDE_DIR})
  find_library(IW_OPENGL_LIB OpenGL HINTS ${IW_SEARCH_LIB})
  if(IW_OPENGL_LIB)
   target_link_libraries(de_engine PUBLIC ${IW_OPENGL_LIB})
  else()
   target_link_libraries(de_engine PUBLIC "-framework OpenGL")
  endif()
  if(IW_OPENGL_INCLUDE_DIR)
   target_include_directories(de_engine PUBLIC "${IW_OPENGL_INCLUDE_DIR}")
  endif()
  target_compile_definitions(de_engine PUBLIC GL_SILENCE_DEPRECATION)
 elseif(WIN32)
  if(TARGET OpenGL::GL)
   target_link_libraries(de_engine PUBLIC OpenGL::GL)
  else()
   target_link_libraries(de_engine PUBLIC opengl32)
   message(WARNING "OpenGL::GL target not found on Windows; falling back to the raw 'opengl32' link name.")
  endif()
 else()
  target_link_libraries(de_engine PUBLIC OpenGL::GL)
 endif()
 if(IW_SDL_TARGET AND IW_OPENMPT_TARGET)
  target_link_libraries(de_engine PUBLIC ${IW_SDL_TARGET} ${IW_OPENMPT_TARGET})
  target_compile_definitions(de_engine PUBLIC IW_HAS_AUDIO SDL_MAIN_HANDLED)
 elseif(NOT IW_SDL_TARGET)
  message(WARNING "SDL2 not found: building with BPM-only timing (macOS: brew install sdl2; Linux: apt install libsdl2-dev)")
 elseif(NOT IW_OPENMPT_TARGET)
  message(WARNING "libopenmpt not found: building with BPM-only timing (macOS: brew install libopenmpt; Linux: apt install libopenmpt-dev)")
 endif()
 iw_warnings(de_engine)

 # Every demo links de_engine and gets the engine shaders copied beside its binary
 # (rm -rf first so a deleted shader does not linger in the build dir).
 function(de_add_demo name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE de_engine)
  iw_warnings(${name})
  add_custom_command(TARGET ${name} POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E rm -rf $<TARGET_FILE_DIR:${name}>/shaders
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/engine/shaders $<TARGET_FILE_DIR:${name}>/shaders)
 endfunction()

 de_add_demo(impossible_wireframe demos/wireframe/src/main.cpp)
 target_link_libraries(impossible_wireframe PRIVATE wireframe_content)
 de_add_demo(hello_demo demos/hello/main.cpp)
 de_add_demo(ripple_demo demos/ripple/main.cpp)
else()
 message(WARNING "OpenGL/glfw3 not found: engine and demos disabled; core libraries and tests remain buildable (macOS: brew install glfw; Linux: apt install libglfw3-dev libgl-dev)")
endif()

# --- Tests -----------------------------------------------------------------
if(IW_BUILD_TESTS)
 enable_testing()
 add_executable(geometry_tests tests/geometry_tests.cpp)
 target_link_libraries(geometry_tests PRIVATE wireframe_content)
 target_compile_definitions(geometry_tests PRIVATE IW_TEST_LOGO_PACK="${CMAKE_SOURCE_DIR}/demos/wireframe/assets/logos")
 iw_warnings(geometry_tests)
 add_test(NAME geometry_tests COMMAND geometry_tests)
endif()
