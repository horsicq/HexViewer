cmake_minimum_required(VERSION 3.15)
project(HexViewer VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
    set(CMAKE_C_FLAGS_RELEASE "/O1 /GL /Gy /DNDEBUG" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_RELEASE "/O1 /GL /Gy /DNDEBUG" CACHE STRING "" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG /OPT:REF /OPT:ICF /INCREMENTAL:NO" CACHE STRING "" FORCE)
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "/LTCG /OPT:REF /OPT:ICF /INCREMENTAL:NO" CACHE STRING "" FORCE)
    set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "/LTCG" CACHE STRING "" FORCE)
endif()

set(CAPSTONE_DIR "")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Capstone/src")
    set(CAPSTONE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Capstone/src")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Capstone")
    set(CAPSTONE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Capstone")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/capstone")
    set(CAPSTONE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/capstone")
endif()

if(CAPSTONE_DIR)
    message(STATUS "Building Capstone from source in ${CAPSTONE_DIR}")
    if(MSVC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
    set(CAPSTONE_BUILD_STATIC ON CACHE BOOL "Build static library")
    set(CAPSTONE_BUILD_SHARED OFF CACHE BOOL "Build shared library")
    set(CAPSTONE_BUILD_TESTS OFF CACHE BOOL "Build tests")
    set(CAPSTONE_BUILD_CSTOOL OFF CACHE BOOL "Build cstool")
    set(CAPSTONE_ARCHITECTURE_DEFAULT ON CACHE BOOL "Enable all architectures")

    add_subdirectory(${CAPSTONE_DIR} ${CMAKE_BINARY_DIR}/capstone)

    set(CAPSTONE_FOUND TRUE)
    set(CAPSTONE_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/XCapstone/3rdparty/Capstone/src/include")
    set(CAPSTONE_LIBRARIES capstone)
else()
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(CAPSTONE capstone)
    endif()
    if(NOT CAPSTONE_FOUND)
        find_path(CAPSTONE_INCLUDE_DIR capstone/capstone.h PATHS /usr/include /usr/local/include)
        find_library(CAPSTONE_LIBRARY NAMES capstone PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
        if(CAPSTONE_INCLUDE_DIR AND CAPSTONE_LIBRARY)
            set(CAPSTONE_FOUND TRUE)
            set(CAPSTONE_INCLUDE_DIRS ${CAPSTONE_INCLUDE_DIR})
            set(CAPSTONE_LIBRARIES ${CAPSTONE_LIBRARY})
        endif()
    endif()
    if(NOT CAPSTONE_FOUND)
        message(FATAL_ERROR "Capstone library not found! Place source in ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Capstone/src or install libcapstone-dev")
    endif()
endif()

set(SOURCES
    HexViewer/src/HexViewer.cpp
    HexViewer/src/core/render.cpp
    HexViewer/src/core/HexData.cpp
    HexViewer/src/ui/menu.cpp
    HexViewer/src/ui/options.cpp
    HexViewer/src/ui/searchdialog.cpp
    HexViewer/src/ui/about.cpp
    HexViewer/src/ui/contextmenu.cpp
    HexViewer/src/system/update.cpp
)

set(HEADERS
    HexViewer/include/core/HexData.h
    HexViewer/include/core/render.h
    HexViewer/include/core/imageloader.h
    HexViewer/include/ui/menu.h
    HexViewer/include/ui/options.h
    HexViewer/include/ui/searchdialog.h
    HexViewer/include/ui/about.h
    HexViewer/include/ui/contextmenu.h
    HexViewer/include/ui/darkmode.h
    HexViewer/include/system/update.h
    HexViewer/include/system/language.h
    HexViewer/include/system/resource.h
)


if(WIN32)
    set(RESOURCE_FILES HexViewer/resources/ui/HexViewer.rc)
    add_executable(HexViewer ${SOURCES} ${HEADERS} ${RESOURCE_FILES})

elseif(APPLE)
    add_executable(HexViewer ${SOURCES} ${HEADERS})

else()
    find_program(CMAKE_OBJCOPY objcopy REQUIRED)

    set(RESOURCE_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/HexViewer/resources/images/about.png)
    set(RESOURCE_OUTPUT ${CMAKE_BINARY_DIR}/about.o)

    add_custom_command(
        OUTPUT ${RESOURCE_OUTPUT}
        COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/HexViewer/resources/images && 
			${CMAKE_OBJCOPY} -I binary -O elf64-x86-64 -B i386
			--redefine-sym _binary_about_png_start=_binary_about_png_start
			--redefine-sym _binary_about_png_end=_binary_about_png_end
			--redefine-sym _binary_about_png_size=_binary_about_png_size
			about.png ${RESOURCE_OUTPUT}
        DEPENDS ${RESOURCE_INPUT}
        COMMENT "Embedding about.png into object file"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/HexViewer/resources
    )

    add_executable(HexViewer ${SOURCES} ${HEADERS} ${RESOURCE_OUTPUT})
endif()

if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/HexViewerShell")
    message(STATUS "Found HexViewerShell directory - building DLL")
    set(HEXVIEWERSHELL_SOURCE HexViewerShell/src/dllmain.cpp)

    add_library(HexViewerShell SHARED ${HEXVIEWERSHELL_SOURCE})

    target_compile_definitions(HexViewerShell PRIVATE UNICODE _UNICODE)

    if(MSVC)
        set_target_properties(HexViewerShell PROPERTIES
            LINK_FLAGS "/MACHINE:X64"
        )
    endif()

    target_link_libraries(HexViewerShell
        user32
        shell32
        shlwapi
        runtimeobject
    )

    set_target_properties(HexViewerShell PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    )
    
    install(TARGETS HexViewerShell RUNTIME DESTINATION bin)
else()
    message(STATUS "HexViewerShell directory not found - skipping DLL build")
endif()

if(WIN32)
    set_target_properties(HexViewer PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if(WIN32)
    target_compile_definitions(HexViewer PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN)
    target_link_libraries(HexViewer PRIVATE ${CAPSTONE_LIBRARIES} gdi32 user32 shell32 comdlg32)
elseif(APPLE)
    target_compile_definitions(HexViewer PRIVATE __APPLE__)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(COREGRAPHICS_FRAMEWORK CoreGraphics)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
    target_link_libraries(HexViewer PRIVATE ${CAPSTONE_LIBRARIES}
        ${COCOA_FRAMEWORK} ${COREGRAPHICS_FRAMEWORK} ${COREFOUNDATION_FRAMEWORK})
else()
    find_package(X11 REQUIRED)
    target_compile_definitions(HexViewer PRIVATE __linux__)
    target_link_libraries(HexViewer PRIVATE ${CAPSTONE_LIBRARIES} ${X11_LIBRARIES} pthread dl)
    target_include_directories(HexViewer PRIVATE ${X11_INCLUDE_DIR})
endif()

target_include_directories(HexViewer PRIVATE
    ${CAPSTONE_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/HexViewer/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/HexViewer/include/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/HexViewer/include/system
	${CMAKE_CURRENT_SOURCE_DIR}/HexViewer/resources/images
    ${CMAKE_CURRENT_SOURCE_DIR}/HexViewer/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/HexViewer/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/HexViewer/src/system
)

if(MSVC)
    target_compile_options(HexViewer PRIVATE /W3)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(HexViewer PRIVATE /O1 /GL /Gy)
        target_link_options(HexViewer PRIVATE /LTCG /OPT:REF /OPT:ICF)
        set_property(TARGET HexViewer PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
    else()
        set_property(TARGET HexViewer PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
    endif()
else()
    target_compile_options(HexViewer PRIVATE -Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(HexViewer PRIVATE -Os -s)
    endif()
endif()

if(MSVC AND CMAKE_BUILD_TYPE STREQUAL "Release")
    string(REPLACE "/Zi" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/DEBUG" "" CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
endif()

set_target_properties(HexViewer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/HexViewerExten")
    message(STATUS "Found HexViewerExten directory (Windows only)")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/HexViewerExten/CMakeLists.txt")
        add_subdirectory(HexViewerExten)
    else()
        file(GLOB_RECURSE HEXVIEWER_EXTERN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/HexViewerExten/src/*.[ch]pp" "${CMAKE_CURRENT_SOURCE_DIR}/HexViewerExten/src/*.[ch]")
        file(GLOB_RECURSE HEXVIEWER_EXTERN_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/HexViewerExten/include/*.[ch]pp" "${CMAKE_CURRENT_SOURCE_DIR}/HexViewerExten/include/*.[ch]")
        if(HEXVIEWER_EXTERN_SOURCES)
            add_executable(HexViewerExten ${HEXVIEWER_EXTERN_SOURCES} ${HEXVIEWER_EXTERN_HEADERS})
           
            target_compile_definitions(HexViewerExten PRIVATE
                UNICODE _UNICODE WIN32_LEAN_AND_MEAN
                DIESHELLEXTENSION_EXPORTS _WINDOWS _USRDLL
            )
           
            target_link_libraries(HexViewerExten PRIVATE ${CAPSTONE_LIBRARIES} gdi32 user32 shell32 comdlg32)
            target_include_directories(HexViewerExten PRIVATE
                ${CAPSTONE_INCLUDE_DIRS}
                ${CMAKE_CURRENT_SOURCE_DIR}/HexViewerExten/include
                ${CMAKE_CURRENT_SOURCE_DIR}/HexViewerExten/src
            )
            if(MSVC)
                target_compile_options(HexViewerExten PRIVATE /W3)
                if(CMAKE_BUILD_TYPE STREQUAL "Release")
                    target_compile_options(HexViewerExten PRIVATE /O1 /GL /Gy)
                                        target_link_options(HexViewerExten PRIVATE
                        /LTCG
                        /OPT:REF
                        /OPT:ICF
                    )
                    set_property(TARGET HexViewerExten PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
                else()
                    set_property(TARGET HexViewerExten PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
                endif()
            endif()
            set_target_properties(HexViewerExten PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
                ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
                LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            )
            install(TARGETS HexViewerExten RUNTIME DESTINATION bin)
        endif()
    endif()
else()
    message(STATUS "HexViewerExten directory not found - skipping build")
endif()

install(TARGETS HexViewer RUNTIME DESTINATION bin)

set(CPACK_PACKAGE_NAME "HexViewer")
set(CPACK_PACKAGE_VENDOR "HexViewer")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cross-platform hex editor with disassembly")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "HexViewer")

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
else()
    set(CPACK_GENERATOR "TGZ;DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "DevX-Cipher")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libx11-6")
endif()

include(CPack)
