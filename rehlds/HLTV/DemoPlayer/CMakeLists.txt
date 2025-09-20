cmake_minimum_required(VERSION 3.1)
project(demoplayer CXX)

option(DEBUG "Build with debug information." OFF)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Avoid -fPIC option
set(CMAKE_SHARED_LIBRARY_CXX_FLAGS "")

set(COMPILE_FLAGS "-m32 -U_FORTIFY_SOURCE -DFORTIFY_SOURCE=0")
set(LINK_FLAGS "-m32")

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
		set(LINK_FLAGS "${LINK_FLAGS} -ipo")
	endif()
else()
	# Produce code optimized for the most common IA32/AMD64/EM64T processors.
	# As new processors are deployed in the marketplace, the behavior of this option will change.
	set(COMPILE_FLAGS "${COMPILE_FLAGS} \
		-mtune=generic -msse3 -flto\
		-fpermissive -fno-sized-deallocation\
		-Wno-unused-result -Wno-unknown-pragmas\
		-Wno-sign-compare -Wno-write-strings -Wno-strict-aliasing")
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
	"${PROJECT_SOURCE_DIR}/../.."
	"${PROJECT_SOURCE_DIR}/../../engine"
	"${PROJECT_SOURCE_DIR}/../../common"
	"${PROJECT_SOURCE_DIR}/../../pm_shared"
	"${PROJECT_SOURCE_DIR}/../../public"
	"${PROJECT_SOURCE_DIR}/../../public/rehlds"
)

set(DEMOPLAYER_SRCS
	"src/DemoPlayer.cpp"
	"src/public_amalgamation.cpp"
)

set(COMMON_SRCS
	"../../common/BaseSystemModule.cpp"
	"../../common/ObjectDictionary.cpp"
	"../../common/ObjectList.cpp"
	"../../common/TokenLine.cpp"
	"../../HLTV/common/BitBuffer.cpp"
	"../../HLTV/common/byteorder.cpp"
	"../../HLTV/common/common.cpp"
	"../../HLTV/common/DirectorCmd.cpp"
	"../../HLTV/common/mathlib.cpp"
	"../../engine/mem.cpp"
)

add_library(demoplayer SHARED ${DEMOPLAYER_SRCS})

target_include_directories(demoplayer PRIVATE
	${PROJECT_SRC_DIR}
	${PROJECT_PUBLIC_DIR}
)

target_compile_definitions(demoplayer PRIVATE
	HLTV
	HLTV_FIXES
	_LINUX
	LINUX
	_GLIBCXX_USE_CXX11_ABI=0
	_stricmp=strcasecmp
	_strnicmp=strncasecmp
	_strdup=strdup
	_vsnprintf=vsnprintf
	_snprintf=snprintf
)

target_sources(demoplayer PRIVATE
	${COMMON_SRCS}
)

target_link_libraries(demoplayer PRIVATE
	dl
	m
)

set_target_properties(demoplayer PROPERTIES
	OUTPUT_NAME demoplayer
	PREFIX ""
	COMPILE_FLAGS ${COMPILE_FLAGS}
	LINK_FLAGS ${LINK_FLAGS}
	POSITION_INDEPENDENT_CODE OFF
)
