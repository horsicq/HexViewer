cmake_minimum_required(VERSION 3.15)

if (MSVC)
    project(HexViewer LANGUAGES CXX RC)
else()
    project(HexViewer LANGUAGES CXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(
        /nologo /W3 /O1 /GL /GS- /GR-
        /EHs- /EHc- /EHa- /RTC- /sdl- /Zl
        /D_CRT_SECURE_NO_WARNINGS /D_HAS_EXCEPTIONS=0
    )
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/ui
    ${CMAKE_SOURCE_DIR}/include/system
    ${CMAKE_SOURCE_DIR}/resources/ui
)

set(SOURCES
    src/hexviewer.cpp
    src/core/hexdata.cpp
    src/core/render.cpp
    src/core/panelcontent.cpp
    src/ui/menu.cpp
    src/ui/options.cpp
    src/ui/searchdialog.cpp
    src/ui/contextmenu.cpp
    src/ui/about.cpp
    src/system/update.cpp
    src/ui/pluginmanager.cpp
    src/core/pluginexecutor.cpp
)

if (WIN32)
    list(APPEND SOURCES resources/ui/hexviewer.rc)

    set_property(
        SOURCE resources/ui/hexviewer.rc
        PROPERTY COMPILE_OPTIONS "/I${CMAKE_CURRENT_SOURCE_DIR}/resources/images"
    )
endif()

if (UNIX AND NOT APPLE)
    find_program(OBJCOPY objcopy REQUIRED)

    set(ABOUT_PNG about.png)
    set(ABOUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources/images)
    set(ABOUT_OBJ ${CMAKE_BINARY_DIR}/about_png.o)

    add_custom_command(
        OUTPUT ${ABOUT_OBJ}
        COMMAND ${CMAKE_COMMAND} -E chdir ${ABOUT_DIR}
                ${OBJCOPY}
                    --input-target=binary
                    --output-target=elf64-x86-64
                    --binary-architecture=i386
                    ${ABOUT_PNG} ${ABOUT_OBJ}
        DEPENDS ${ABOUT_DIR}/${ABOUT_PNG}
        COMMENT "Embedding about.png into object file with clean symbol names"
    )

    list(APPEND SOURCES ${ABOUT_OBJ})
endif()


add_executable(HexViewer ${SOURCES})

if(MSVC)
    set_source_files_properties(
        src/system/update.cpp
        src/hexviewer.cpp
        src/core/render.cpp
        src/ui/about.cpp
        PROPERTIES COMPILE_OPTIONS "/GL-"
    )
endif()

if (WIN32)
    target_link_options(HexViewer PRIVATE
        /SUBSYSTEM:WINDOWS
        /ENTRY:entry
        /NODEFAULTLIB
        /OPT:REF
        /OPT:ICF
        /LTCG
        /MANIFEST:NO
    )
    target_link_libraries(HexViewer PRIVATE
        dwmapi.lib gdiplus.lib wininet.lib msimg32.lib
    )
elseif(APPLE)
    target_link_libraries(HexViewer PRIVATE
        "-framework Cocoa"
        "-framework AppKit"
        "-framework CoreFoundation"
    )
else()  # Linux
    target_link_libraries(HexViewer PRIVATE X11 pthread dl)
endif()