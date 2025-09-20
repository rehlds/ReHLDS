cmake_minimum_required(VERSION 3.1)
project(hlds CXX)

option(DEBUG "Build with debug information." OFF)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Avoid -rdynamic -fPIC options
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

set(COMPILE_FLAGS "-m32 -fno-pic -fno-pie -U_FORTIFY_SOURCE -DFORTIFY_SOURCE=0")
set(LINK_FLAGS "-m32 -no-pie -Wl,--no-export-dynamic")

set(COMPILE_FLAGS "${COMPILE_FLAGS} -Wall -fno-exceptions -fno-strict-aliasing -fno-strict-overflow")

if (DEBUG)
	set(COMPILE_FLAGS "${COMPILE_FLAGS} -g3 -O3 -ggdb")
else()
	set(COMPILE_FLAGS "${COMPILE_FLAGS} -g0 -O3 -fno-stack-protector")
endif()

# Check Intel C++ compiler
if ("$ENV{CXX}" MATCHES "icpc")
	set(COMPILE_FLAGS "${COMPILE_FLAGS} -Qoption,cpp,--treat_func_as_string_literal_cpp")
	set(LINK_FLAGS "${LINK_FLAGS} -static-intel -no-intel-extensions")

	if (NOT DEBUG)
		set(COMPILE_FLAGS "${COMPILE_FLAGS} -ipo")
		set(LINK_FLAGS "${COMPILE_FLAGS} -ipo")
	endif()
else()
	# Produce code optimized for the most common IA32/AMD64/EM64T processors.
	# As new processors are deployed in the marketplace, the behavior of this option will change.
	set(COMPILE_FLAGS "${COMPILE_FLAGS} \
		-mtune=generic -msse3\
		-fpermissive -fno-sized-deallocation\
		-Wno-unused-result")

	# Check if not Clang compiler AND GCC >= 8.3
	if (NOT "$ENV{CXX}" MATCHES "clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 8.0)
		set(COMPILE_FLAGS "${COMPILE_FLAGS} -Wno-stringop-truncation -Wno-format-truncation")
	endif()
endif()

# GCC >= 8.3
if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 8.0)
	set(COMPILE_FLAGS "${COMPILE_FLAGS} -fcf-protection=none")
endif()

set(PROJECT_SRC_DIR
	"${PROJECT_SOURCE_DIR}/src"
	"${PROJECT_SOURCE_DIR}/../"
)

set(PROJECT_PUBLIC_DIR
	"${PROJECT_SOURCE_DIR}/../engine"
	"${PROJECT_SOURCE_DIR}/../common"
	"${PROJECT_SOURCE_DIR}/../public"
	"${PROJECT_SOURCE_DIR}/../public/rehlds"
)

set(DEDICATED_SRCS
	"src/dbg.cpp"
	"src/dedicated_exports.cpp"
	"src/public_amalgamation.cpp"
	"src/sys_ded.cpp"
	"src/sys_linux.cpp"
	"src/vgui/vguihelpers.cpp"
)

set(COMMON_SRCS
	"../common/textconsole.cpp"
	"../common/TextConsoleUnix.cpp"
	"../common/SteamAppStartUp.cpp"
	"../common/ObjectList.cpp"
	"../common/commandline.cpp"
	"../common/minidump.cpp"
	"../engine/mem.cpp"
)

if (NOT TARGET appversion)
	add_custom_target(appversion DEPENDS COMMAND "${PROJECT_SOURCE_DIR}/../version/appversion.sh" "${PROJECT_SOURCE_DIR}/../..")
endif()

add_executable(hlds ${appversion.sh} ${DEDICATED_SRCS})
add_dependencies(hlds appversion)

target_include_directories(hlds PRIVATE
	${PROJECT_SRC_DIR}
	${PROJECT_PUBLIC_DIR}
)

target_compile_definitions(hlds PRIVATE
	LAUNCHER_FIXES
	_CONSOLE
	_LINUX
	LINUX
	_GLIBCXX_USE_CXX11_ABI=0
	_stricmp=strcasecmp
	_strnicmp=strncasecmp
	_strdup=strdup
	_vsnprintf=vsnprintf
)

target_sources(hlds PRIVATE
	${COMMON_SRCS}
)

target_link_libraries(hlds PRIVATE
	dl
)

set_target_properties(hlds PROPERTIES
	OUTPUT_NAME hlds_linux
	PREFIX ""
	COMPILE_FLAGS ${COMPILE_FLAGS}
	LINK_FLAGS ${LINK_FLAGS}
	POSITION_INDEPENDENT_CODE OFF
)
