cmake_minimum_required(VERSION 3.1)
project(core CXX)

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
	set(COMPILE_FLAGS "${COMPILE_FLAGS} -mtune=generic -msse3 -flto\
		-fpermissive -fno-sized-deallocation\
		-Wno-unused-result -Wno-unknown-pragmas -Wno-unused-variable\
		-Wno-sign-compare -Wno-write-strings -Wno-strict-aliasing")

	# Check Clang compiler
	if ("$ENV{CXX}" MATCHES "clang")
		set(COMPILE_FLAGS "${COMPILE_FLAGS} -Wno-unused-private-field")
	else()
		# GCC >= 8.3
		if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 8.0)
			set(COMPILE_FLAGS "${COMPILE_FLAGS} -Wno-stringop-truncation -Wno-format-truncation")
		endif()
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

set(PROJECT_BZIP2_DIR
	"${PROJECT_SOURCE_DIR}/../../../dep/bzip2/include"
)

set(PROJECT_PUBLIC_DIR
	"${PROJECT_SOURCE_DIR}/../.."
	"${PROJECT_SOURCE_DIR}/../../engine"
	"${PROJECT_SOURCE_DIR}/../../common"
	"${PROJECT_SOURCE_DIR}/../../pm_shared"
	"${PROJECT_SOURCE_DIR}/../../public"
	"${PROJECT_SOURCE_DIR}/../../public/rehlds"
)

set(CORE_SRCS
	"src/BSPModel.cpp"
	"src/Delta.cpp"
	"src/NetSocket.cpp"
	"src/Network.cpp"
	"src/Server.cpp"
	"src/World.cpp"
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
	"../../HLTV/common/DemoFile.cpp"
	"../../HLTV/common/DirectorCmd.cpp"
	"../../HLTV/common/InfoString.cpp"
	"../../HLTV/common/mathlib.cpp"
	"../../HLTV/common/md5.cpp"
	"../../HLTV/common/munge.cpp"
	"../../HLTV/common/NetAddress.cpp"
	"../../HLTV/common/NetChannel.cpp"
	"../../HLTV/common/random.cpp"
	"../../engine/mem.cpp"
)

add_library(core SHARED ${CORE_SRCS})

target_include_directories(core PRIVATE
	${PROJECT_SRC_DIR}
	${PROJECT_BZIP2_DIR}
	${PROJECT_PUBLIC_DIR}
)

target_compile_definitions(core PRIVATE
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

target_sources(core PRIVATE
	${COMMON_SRCS}
)

target_link_libraries(core PRIVATE
	dl
	m
	bzip2
)

if (NOT TARGET bzip2)
	add_subdirectory(../../../dep/bzip2 lib)
endif()

set_target_properties(core PROPERTIES
	OUTPUT_NAME core
	PREFIX ""
	COMPILE_FLAGS ${COMPILE_FLAGS}
	LINK_FLAGS ${LINK_FLAGS}
	POSITION_INDEPENDENT_CODE OFF
)
