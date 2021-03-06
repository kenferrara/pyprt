# Copyright (c) 2012-2020 Esri R&D Center Zurich
 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
 
#   http://www.apache.org/licenses/LICENSE-2.0
 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# A copy of the license is available in the repository's LICENSE file.

cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0015 NEW)


### setup build target

add_library(${CODEC_TARGET} SHARED
		codecs_Py.cpp
		encoder/PyEncoder.cpp)

target_compile_features(${CODEC_TARGET} PRIVATE
		cxx_std_14) # C++14

target_compile_definitions(${CODEC_TARGET} PRIVATE
		-DPRT_VERSION_MAJOR=${PRT_VERSION_MAJOR}
		-DPRT_VERSION_MINOR=${PRT_VERSION_MINOR})

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	target_compile_options(${CODEC_TARGET} PRIVATE
			-bigobj -GR -EHsc)

elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	target_compile_options(${CODEC_TARGET} PRIVATE
			-D_GLIBCXX_USE_CXX11_ABI=0 # we follow RHEL and use the pre-C++11 libstdc++ mode
			-march=nocona
			-fvisibility=hidden -fvisibility-inlines-hidden
			-Wl,--exclude-libs,ALL
			-Wall -Wextra -Wunused-parameter)

elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	target_compile_options(${CODEC_TARGET} PRIVATE
			-stdlib=libc++
			-fvisibility=hidden -fvisibility-inlines-hidden
			-Wl,--exclude-libs,ALL
			-Wall -Wextra -Wunused-parameter)
endif()


### add prt dependency

target_link_libraries(${CODEC_TARGET} PRIVATE ${PRT_LINK_LIBRARIES})
target_include_directories(${CODEC_TARGET} PRIVATE ${PRT_INCLUDE_PATH})


### install target

install(TARGETS ${CODEC_TARGET} RUNTIME DESTINATION lib LIBRARY DESTINATION lib)
