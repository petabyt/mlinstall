# cmake -DCMAKE_TOOLCHAIN_FILE=/home/daniel/dotfiles/mingw.cmake -G Ninja -B build && cmake --build build
cmake_minimum_required(VERSION 3.14)
project(mlinstall)
enable_language(C ASM)

set(FILES
	src/main.c
	src/drive.c
	src/model.c
	src/ptp.c
	src/data.S
	src/libui.c
	src/ui.c
)

option(PTP_DEFAULT_LOGGING "" OFF)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/libpict ${CMAKE_CURRENT_BINARY_DIR}/libpict)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/rim ${CMAKE_CURRENT_BINARY_DIR}/rim)

if(UNIX)
	list(APPEND FILES src/drive-unix.c)
	set(LIBS Rim:Rim)
elseif(WIN32)
	list(APPEND FILES src/drive-win.c assets/win.rc)
	set(LIBS LibUI::LibUI)
	list(APPEND LIBS -luser32 -lkernel32 -lgdi32 -lcomctl32 -luxtheme -lmsimg32 -lcomdlg32 -ld2d1 -ldwrite -lole32 -loleaut32 -loleacc)
	list(APPEND LIBS -lstdc++ -lgcc -static -lpthread -lssp -lurlmon -luuid)
	list(APPEND LIBS -Wl,-subsystem,windows) # Disable cmd.exe window on open
elseif(APPLE)
	
endif()

add_executable(mlinstall ${FILES})
target_link_libraries(mlinstall libpict ${LIBS})
target_include_directories(mlinstall PRIVATE src)
target_compile_options(mlinstall PRIVATE -Wshadow -Wcast-qual -Wpedantic -Werror=incompatible-pointer-types -O2)
set_target_properties(mlinstall PROPERTIES LINK_FLAGS_RELEASE "-s -static")

# TODO: UPDATE_INFORMATION="gh-releases-zsync|petabyt|mlinstall|latest|mlinstall-x86_64.AppImage"
if(MLINSTALL_CREATE_APPIMAGE)
    function(create_appimage)
        cmake_parse_arguments(PARSE_ARGV 0 ARG "" "TARGET;ICON;DESKTOP" "")

        install(TARGETS ${ARG_TARGET} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/AppDir/usr/bin)

        find_program(LINUXDEPLOY linuxdeploy)
        if(NOT LINUXDEPLOY)
            message(FATAL_ERROR "linuxdeploy not found")
        endif()
        find_program(APPIMAGETOOL appimagetool)
        if(NOT APPIMAGETOOL)
            message(FATAL_ERROR "appimagetool not found")
        endif()

        # Strip binary included in linuxdeploy is broken, disable stripping
        add_custom_target(${ARG_TARGET}_AppImage ALL
            COMMAND rm -rf AppDir
            COMMAND NO_STRIP=true linuxdeploy --appdir=${CMAKE_CURRENT_BINARY_DIR}/AppDir --executable=$<TARGET_FILE:${ARG_TARGET}> -d ${ARG_DESKTOP} -i ${ARG_ICON} --output appimage
            COMMAND appimagetool ${CMAKE_CURRENT_BINARY_DIR}/AppDir
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS ${ARG_TARGET}
        )
    endfunction()

    create_appimage(
        TARGET mlinstall
        ICON ${CMAKE_CURRENT_SOURCE_DIR}/assets/mlinstall.png
        DESKTOP ${CMAKE_CURRENT_SOURCE_DIR}/assets/mlinstall.desktop
    )
endif()

# bash assets/appify.sh -i assets/mlinstall.png -s mac.out -n mlinstall
# mkdir mlinstall.app/Contents/Frameworks
# cp /opt/local/lib/libusb-1.0.0.dylib mlinstall.app/Contents/Frameworks
# cp /usr/local/lib/libui.dylib mlinstall.app/Contents/Frameworks
# install_name_tool -change /usr/local/opt/libusb/lib/libusb-1.0.0.dylib "@executable_path/../Frameworks/libusb-1.0.0.dylib" ./mlinstall.app/Contents/MacOS/mlinstall
# install_name_tool -change libui.dylib "@executable_path/../Frameworks/libui.dylib" ./mlinstall.app/Contents/MacOS/mlinstall
if(MLINSTALL_CREATE_MAC_APP)
    function(create_mac_app)
        cmake_parse_arguments(PARSE_ARGV 0 ARG "" "TARGET;APP_NAME;PLIST_PATH;DYLIBS" "")
        if(DYLIBS)
            message(FATAL_ERROR "TODO: use install_name_tool on dylibs")
        endif()
        file(MAKE_DIRECTORY
            "${CMAKE_CURRENT_BINARY_DIR}/${ARG_APP_NAME}.app/Contents/MacOS"
            "${CMAKE_CURRENT_BINARY_DIR}/${ARG_APP_NAME}.app/Contents/Resources"
            "${CMAKE_CURRENT_BINARY_DIR}/${ARG_APP_NAME}.app/Contents/Frameworks"
        )
        add_custom_target(${ARG_APP_NAME}_AppBundle ALL
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${ARG_TARGET}> "${CMAKE_CURRENT_BINARY_DIR}/${ARG_APP_NAME}.app/Contents/MacOS/${ARG_APP_NAME}"
            COMMAND chmod +x "${CMAKE_CURRENT_BINARY_DIR}/${ARG_APP_NAME}.app/Contents/MacOS/${ARG_APP_NAME}"
            COMMAND ${CMAKE_COMMAND} -E copy ${ARG_PLIST_PATH} "${CMAKE_CURRENT_BINARY_DIR}/${ARG_APP_NAME}.app/Contents/Info.plist"
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS ${ARG_TARGET}
        )
        add_custom_target(${ARG_APP_NAME}_Package ALL
            COMMAND tar -czf "${CMAKE_CURRENT_BINARY_DIR}/${ARG_APP_NAME}.tar.gz" -C "${CMAKE_CURRENT_BINARY_DIR}" "${ARG_APP_NAME}.app"
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS ${ARG_APP_NAME}_AppBundle
        )
    endfunction()

    create_mac_app(
        TARGET mlinstall
        APP_NAME mlinstall
        ICON ${CMAKE_CURRENT_SOURCE_DIR}/assets/mlinstall.png
        PLIST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/assets/Info.plist
    )
endif()
