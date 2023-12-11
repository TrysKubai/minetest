#
# Cmake & Compiler Config
#
cmake_minimum_required(VERSION 3.13)

# Set policies up to 3.23 as vcpkg recommends at least 3.22.4 for it's tool chain
if(${CMAKE_VERSION} VERSION_LESS 3.23)
	message(WARNING "CMake minimum version 3.23 is recomended. Project might not build correctly with older builds.") 
	cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
	cmake_policy(VERSION 3.23)
endif()

# Tell CMake where to find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(GCC_MINIMUM_VERSION "7.5")
set(CLANG_MINIMUM_VERSION "7.0.1")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "${GCC_MINIMUM_VERSION}")
		message(FATAL_ERROR "Insufficient gcc version, found ${CMAKE_CXX_COMPILER_VERSION}. "
			"Version ${GCC_MINIMUM_VERSION} or higher is required.")
	endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "(Apple)?Clang")
	if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "${CLANG_MINIMUM_VERSION}")
		message(FATAL_ERROR "Insufficient clang version, found ${CMAKE_CXX_COMPILER_VERSION}. "
			"Version ${CLANG_MINIMUM_VERSION} or higher is required.")
	endif()
endif()

#
# Project Setup
#

project(t3c_game_runtime)
set(PROJECT_NAME_CAPITALIZED "Three Cubes Game")

# You should not need to edit these manually, use util/bump_version.sh
set(VERSION_MAJOR 0)
set(VERSION_MINOR 3)
set(VERSION_PATCH 0)

#
#	Configuration Parameters
#
set(VERSION_EXTRA "" CACHE STRING "Stuff to append to version string")

set(BUILD_CLIENT TRUE CACHE BOOL "Build client")
set(BUILD_SERVER FALSE CACHE BOOL "Build server")
set(BUILD_UNITTESTS TRUE CACHE BOOL "Build unittests")
set(BUILD_BENCHMARKS FALSE CACHE BOOL "Build benchmarks")

set(WARN_ALL TRUE CACHE BOOL "Enable -Wall for Release build")

if(NOT CMAKE_BUILD_TYPE)
	# Default to release
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type: Debug or Release" FORCE)
endif()

#TODO Update checker needs to be removed
set(ENABLE_UPDATE_CHECKER FALSE CACHE BOOL
	"Whether to enable update checks by default")

set(RESOURCES_DIR "${CMAKE_SOURCE_DIR}/resources" CACHE STRING "Base dir of non-compiled resource files")
set(OVERRIDE_OUTPUT_DIR "" CACHE STRING "Override build artifacts placement directory")
set(OVERRIDE_INSTALL_DIR "" CACHE STRING "Override installation file placement location")

set(IRRLICHTMT_BUILD_DIR "" CACHE PATH "Path to IrrlichtMt build directory.")

set(INSTALL_DEVTEST FALSE CACHE BOOL "Install Development Test")

#
#	Static & Derivative Parameters
#
if("${OVERRIDE_OUTPUT_DIR}" STREQUAL "")
	set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/builds/${CMAKE_BUILD_TYPE}")
else()
	set(OUTPUT_DIR OVERRIDE_OUTPUT_DIR)
endif()

if("${OVERRIDE_INSTALL_DIR}" STREQUAL "")
	set(INSTALL_DIR "${CMAKE_BINARY_DIR}/out/${CMAKE_BUILD_TYPE}")
else()
	set(INSTALL_DIR OVERRIDE_INSTALL_DIR)
endif()


set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
if(VERSION_EXTRA)
	set(VERSION_STRING "${VERSION_STRING}-${VERSION_EXTRA}")
endif()

if (CMAKE_BUILD_TYPE STREQUAL Debug)
	# Append "-debug" to version string
	set(VERSION_STRING "${VERSION_STRING}-debug")
endif()

if("${OUTPUT_DIR}" STREQUAL "")
	set(OUTPUT_DIR "${CMAKE_BINARY_DIR}")
endif()

set(SHARE_DIR "${OUTPUT_DIR}")
set(BIN_DIR "${SHARE_DIR}/bin")
set(DOC_DIR "${SHARE_DIR}/doc")
set(EXAMPLE_CONF_DIR "${SHARE_DIR}")
set(LOCALE_DIR "${SHARE_DIR}/locale")

if(APPLE)
	# Ignoring and keeping this default for now

	# set(BUNDLE_NAME ${PROJECT_NAME}.app)
	# set(BUNDLE_PATH "${BUNDLE_NAME}")
	# set(BINDIR ${BUNDLE_NAME}/Contents/MacOS)
	# set(SHARE_DIR ${BUNDLE_NAME}/Contents/Resources)
	# set(DOCDIR "${SHARE_DIR}/${PROJECT_NAME}")
	# set(EXAMPLE_CONF_DIR ${DOCDIR})
	# set(LOCALEDIR "${SHARE_DIR}/locale")
elseif(UNIX) # Linux, BSD etc
	set(MANDIR "${SHARE_DIR}/unix/man")
	set(XDG_APPS_DIR "${SHARE_DIR}/unix/applications")
	set(APPDATADIR "${SHARE_DIR}/unix/metainfo")
	set(ICONDIR "${SHARE_DIR}/unix/icons")
endif()


#
#	Resolving IrrlichtMt
#

# 1. If ${IRRLICHTMT_BUILD_DIR} is set looks in that dir;
# 2. Else if ${CMAKE_CURRENT_SOURCE_DIR}/lib/irrlichtmt exists looks for it there;
# 3. Else uses find_package to try resolving for a global package
if(NOT "${IRRLICHTMT_BUILD_DIR}" STREQUAL "")
	find_package(IrrlichtMt QUIET
		PATHS "${IRRLICHTMT_BUILD_DIR}"
		NO_DEFAULT_PATH
	)
	if(NOT TARGET IrrlichtMt::IrrlichtMt)
		# find_package() searches certain subdirectories. ${PATH}/cmake is not
		# the only one, but it is the one where IrrlichtMt is supposed to export
		# IrrlichtMtConfig.cmake
		message(FATAL_ERROR "Could not find IrrlichtMtConfig.cmake in ${IRRLICHTMT_BUILD_DIR}/cmake.")
	endif()
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/irrlichtmt")
	message(STATUS "Using user-provided IrrlichtMt at subdirectory 'lib/irrlichtmt'")
	if(BUILD_CLIENT)
		# tell IrrlichtMt to create a static library
		set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared library" FORCE)
		add_subdirectory(lib/irrlichtmt EXCLUDE_FROM_ALL)
		unset(BUILD_SHARED_LIBS CACHE)

		if(NOT TARGET IrrlichtMt)
			message(FATAL_ERROR "IrrlichtMt project is missing a CMake target?!")
		endif()
	else()
		add_library(IrrlichtMt::IrrlichtMt INTERFACE IMPORTED)
		set_target_properties(IrrlichtMt::IrrlichtMt PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/lib/irrlichtmt/include")
	endif()
else()
	find_package(IrrlichtMt QUIET)
	if(NOT TARGET IrrlichtMt::IrrlichtMt)
		string(CONCAT explanation_msg
			"The Minetest team has forked Irrlicht to make their own customizations. "
			"It can be found here: https://github.com/minetest/irrlicht\n"
			"For example use: git clone --depth=1 https://github.com/minetest/irrlicht lib/irrlichtmt\n")
		if(BUILD_CLIENT)
			message(FATAL_ERROR "IrrlichtMt is required to build the client, but it was not found.\n${explanation_msg}")
		endif()

		include(MinetestFindIrrlichtHeaders)
		if(NOT IRRLICHT_INCLUDE_DIR)
			message(FATAL_ERROR "IrrlichtMt headers are required to build the server, but none found.\n${explanation_msg}")
		endif()
		message(STATUS "Found IrrlichtMt headers: ${IRRLICHT_INCLUDE_DIR}")
		add_library(IrrlichtMt::IrrlichtMt INTERFACE IMPORTED)
		# Note that we can't use target_include_directories() since that doesn't work for IMPORTED targets before CMake 3.11
		set_target_properties(IrrlichtMt::IrrlichtMt PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${IRRLICHT_INCLUDE_DIR}")
	endif()
endif()

# 
#	Lib dependencies
#

find_package(GMP REQUIRED)
find_package(Json REQUIRED)
find_package(Lua REQUIRED)


#
#	Build
#
message(STATUS "*** Will build version ${VERSION_STRING} ***")

# Copy resource files into build dir

add_custom_target(copy_resource_files_to_share_dir ALL
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${RESOURCES_DIR}
		${SHARE_DIR}
)

#
#	Install
#

install(DIRECTORY "${RESOURCES_DIR}/builtin" DESTINATION "${INSTALL_DIR}")
install(FILES "${RESOURCES_DIR}/mods/mods_here.txt" DESTINATION "${INSTALL_DIR}/mods")
install(FILES "${RESOURCES_DIR}/textures/texture_packs_here.txt" DESTINATION "${INSTALL_DIR}/textures")
install(DIRECTORY "${RESOURCES_DIR}/games/minetest_game" DESTINATION "${INSTALL_DIR}/games/"
	COMPONENT "SUBGAME_MINETEST_GAME" OPTIONAL PATTERN ".git*" EXCLUDE )
install(DIRECTORY "${RESOURCES_DIR}/games/devtest" DESTINATION "${INSTALL_DIR}/games/"
		PATTERN ".git*" EXCLUDE )

if(BUILD_CLIENT)
	install(DIRECTORY "${RESOURCES_DIR}/client/shaders" DESTINATION "${INSTALL_DIR}/client")
	install(DIRECTORY "${RESOURCES_DIR}/textures/base/pack" DESTINATION "${INSTALL_DIR}/textures/base")
	install(DIRECTORY "${RESOURCES_DIR}/clientmods" DESTINATION "${INSTALL_DIR}")
	install(DIRECTORY "${RESOURCES_DIR}/client/serverlist" DESTINATION "${INSTALL_DIR}/client")
endif()

if(UNIX AND NOT APPLE)
	install(FILES "${RESOURCES_DIR}/doc/minetest.6" "${RESOURCES_DIR}/doc/minetestserver.6" DESTINATION "${INSTALL_DIR}/unix/man6")
	install(FILES "${RESOURCES_DIR}/misc/net.minetest.minetest.desktop" DESTINATION "${INSTALL_DIR}/unix/applications")
	install(FILES "${RESOURCES_DIR}/misc/net.minetest.minetest.appdata.xml" DESTINATION "${INSTALL_DIR}/unix/metainfo")
	install(FILES "${RESOURCES_DIR}/misc/minetest.svg" DESTINATION "${INSTALL_DIR}/unix/icons/hicolor/scalable/apps")
	install(FILES "${RESOURCES_DIR}/misc/minetest-xorg-icon-128.png"
		DESTINATION "${ICONDIR}/hicolor/128x128/apps"
		RENAME "minetest.png")
endif()

if(APPLE)
	install(FILES "${RESOURCES_DIR}/misc/minetest-icon.icns" DESTINATION "${INSTALL_DIR}")
	install(FILES "${RESOURCES_DIR}/misc/Info.plist" DESTINATION "${BUNDLE_PATH}/Contents")
endif()

# Library pack

if(BUILD_BENCHMARKS)
	add_subdirectory(lib/catch2)
endif()

# Subdirectories
# Be sure to add all relevant definitions above this
# add_subdirectory(external)
add_subdirectory(src)

#
#	CPack
#

#TODO CPack might have limited use now. We should no longer generate an installer
# as installation will be handled by platform - all we need is a archive file with
# all the needed files for execution.


set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Three Cubes Game runtime installer")
set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
# set(CPACK_PACKAGE_VENDOR "celeron55")
# set(CPACK_PACKAGE_CONTACT "Perttu Ahola <celeron55@gmail.com>")

include(CPackComponent)

cpack_add_component(Docs
	DISPLAY_NAME "Documentation"
	DESCRIPTION "Documentation about Minetest and Minetest modding"
)

if(WIN32)
	# Include all dynamically linked runtime libraries such as MSVCRxxx.dll
	include(InstallRequiredSystemLibraries)

	if(RUN_IN_PLACE)
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${VERSION_STRING}-win64")
		else()
			set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${VERSION_STRING}-win32")
		endif()

		set(CPACK_GENERATOR ZIP)

	else()
		set(CPACK_GENERATOR WIX)
		set(CPACK_PACKAGE_NAME "${PROJECT_NAME_CAPITALIZED}")
		set(CPACK_PACKAGE_INSTALL_DIRECTORY ".")
		set(CPACK_PACKAGE_EXECUTABLES ${PROJECT_NAME} "${PROJECT_NAME_CAPITALIZED}")
		set(CPACK_CREATE_DESKTOP_LINKS ${PROJECT_NAME})
		set(CPACK_PACKAGING_INSTALL_PREFIX "/${PROJECT_NAME_CAPITALIZED}")

		set(CPACK_WIX_PRODUCT_ICON "${RESOURCES_DIR}/misc/minetest-icon.ico")
		# Supported languages can be found at
		# http://wixtoolset.org/documentation/manual/v3/wixui/wixui_localization.html
		#set(CPACK_WIX_CULTURES "ar-SA,bg-BG,ca-ES,hr-HR,cs-CZ,da-DK,nl-NL,en-US,et-EE,fi-FI,fr-FR,de-DE")
		set(CPACK_WIX_UI_BANNER "${RESOURCES_DIR}/misc/CPACK_WIX_UI_BANNER.BMP")
		set(CPACK_WIX_UI_DIALOG "${RESOURCES_DIR}/misc/CPACK_WIX_UI_DIALOG.BMP")

		set(CPACK_RESOURCE_FILE_LICENSE "${RESOURCES_DIR}/doc/lgpl-2.1.txt")

		# The correct way would be to include both x32 and x64 into one installer
		# and install the appropriate one.
		# CMake does not support that, so there are two separate GUID's
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(CPACK_WIX_UPGRADE_GUID "745A0FB3-5552-44CA-A587-A91C397CCC56")
		else()
			set(CPACK_WIX_UPGRADE_GUID "814A2E2D-2779-4BBD-9ACD-FC3BD51FBBA2")
		endif()
	endif()
elseif(APPLE)
	set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
	set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${VERSION_STRING}-osx")
	set(CPACK_GENERATOR ZIP)
else()
	set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${VERSION_STRING}-linux")
	set(CPACK_GENERATOR TGZ)
	set(CPACK_SOURCE_GENERATOR TGZ)
endif()

include(CPack)



# Add a target to generate API documentation with Doxygen
if(BUILD_DOCUMENTATION)
	find_package(Doxygen)
	if(DOXYGEN_FOUND)
		configure_file(${RESOURCES_DIR}/doc/Doxyfile.in
				${OUTPUT_DIR}/doc/Doxyfile @ONLY)
		add_custom_target(doc
			${DOXYGEN_EXECUTABLE} ${OUTPUT_DIR}/doc/Doxyfile
			WORKING_DIRECTORY ${OUTPUT_DIR}/doc
			COMMENT "Generating API documentation with Doxygen" VERBATIM
		)
	endif()
endif()
