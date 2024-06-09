cmake_minimum_required(VERSION 3.13)
project(sampling)

# Set directories for convenience
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(TEST_DIR ${CMAKE_SOURCE_DIR}/tests)
set(KERNEL_DIR ${CMAKE_SOURCE_DIR}/kernels)
set(SCRIPT_DIR ${CMAKE_SOURCE_DIR}/scripts)
set(PLATFORM xilinx_u2_gen3x4_xdma_gc_2_202110_1)
set(TEMP_DIR temp)
set(PACKAGE_DIR package)

if(DEFINED ENV{XILINX_XRT})
    set(XILINX_XRT $ENV{XILINX_XRT})
else()
    set(XILINX_XRT "/opt/xilinx/xrt")
endif()

if(DEFINED ENV{XILINX_VIVADO})
    set(XILINX_VIVADO $ENV{XILINX_VIVADO})
elseif(EXISTS "/tools/Xilinx/Vivado/2021.2")
    set(XILINX_VIVADO "/tools/Xilinx/Vivado/2021.2")
elseif(EXISTS "/opt/tools/Xilinx/Vivado/2021.2")
    set(XILINX_VIVADO "/opt/tools/Xilinx/Vivado/2021.2")
else()
    message(FATAL_ERROR "Cannot find a valid XILINX_VIVADO path. Make sure you source the enrionment setup script.")
endif()

if(DEFINED ENV{XILINX_VITIS})
    set(XILINX_VITIS $ENV{XILINX_VITIS})
elseif(EXISTS "/tools/Xilinx/Vitis/2021.2")
    set(XILINX_VITIS "/tools/Xilinx/Vitis/2021.2")
elseif(EXISTS "/opt/tools/Xilinx/Vitis/2021.2")
    set(XILINX_VITIS "/opt/tools/Xilinx/Vitis/2021.2")
else()
    message(FATAL_ERROR "Cannot find a valid XILINX_VITIS path. Make sure you source the enrionment setup script.")
endif()

if(DEFINED ENV{XILINX_VITIS_HLS})
    set(XILINX_VITIS_HLS $ENV{XILINX_VITIS_HLS})
elseif(EXISTS "/tools/Xilinx/Vitis_HLS/2021.2")
    set(XILINX_VITIS_HLS "/tools/Vitis_HLS/Vitis/2021.2")
elseif(EXISTS "/opt/tools/Xilinx/Vitis_HLS/2021.2")
    set(XILINX_VITIS_HLS "/opt/tools/Xilinx/Vitis_HLS/2021.2")
else()
    message(FATAL_ERROR "Cannot find a valid XILINX_VITIS path. Make sure you source the enrionment setup script.")
endif()

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

find_package(OpenMP REQUIRED)

# Create directory to store the temporary file
add_custom_command(
    OUTPUT ${TEMP_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TEMP_DIR}
    COMMENT "Creating temp directory"
)

add_custom_command(
    OUTPUT ${PACKAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PACKAGE_DIR}
    COMMENT "Creating package directory"
)

# create custom targets for kernels
file(GLOB_RECURSE KERNEL_SOURCE_FILE ${KERNEL_DIR}/*.cpp)
foreach(CUR_KERNEL_FILE ${KERNEL_SOURCE_FILE})
    get_filename_component(KERNEL_NAME ${CUR_KERNEL_FILE} NAME_WE)

    # Compile the kernel, generate .xo file
    add_custom_command(
        OUTPUT ${TEMP_DIR}/${KERNEL_NAME}.xo
        COMMAND ${XILINX_VITIS}/bin/v++ 
                -c 
                # --save-temps 
                -t hw 
                --platform ${PLATFORM} 
                -k ${KERNEL_NAME} 
                # --temp_dir ${TEMP_DIR} 
                -I${SRC_DIR} 
                -o temp/${KERNEL_NAME}.xo 
                ${CUR_KERNEL_FILE} 
        DEPENDS ${TEMP_DIR} ${CUR_KERNEL_FILE} 
    )
    
    # Link the kernel, generate .link.xclbin file
    add_custom_command(
        OUTPUT ${KERNEL_NAME}.link.xclbin
        COMMAND ${XILINX_VITIS}/bin/v++ 
                -l 
                # --save-temps 
                -t hw 
                --platform ${PLATFORM}
                # --temp_dir ${TEMP_DIR}
                -o ${KERNEL_NAME}.link.xclbin
                --hls.jobs  32
                --vivado.impl.jobs 32
                --vivado.synth.jobs 32
                ${TEMP_DIR}/${KERNEL_NAME}.xo
        DEPENDS ${TEMP_DIR}/${KERNEL_NAME}.xo
    )

    # pack up the kernel, generate .xclbin file
    add_custom_command(
    OUTPUT ${KERNEL_NAME}.xclbin
    COMMAND ${XILINX_VITIS}/bin/v++ 
            -p ${KERNEL_NAME}.link.xclbin 
            # --save-temps 
            -t hw
            --platform ${PLATFORM} 
            --package.out_dir ${PACKAGE_DIR}
            -o ${KERNEL_NAME}.xclbin
    DEPENDS ${PACKAGE_DIR} ${KERNEL_NAME}.link.xclbin
    )

    add_custom_target(${KERNEL_NAME} DEPENDS ${KERNEL_NAME}.xclbin)

endforeach()


# Host program compilation
file(GLOB_RECURSE SOURCES ${SRC_DIR}/*.cpp)
list(REMOVE_ITEM SOURCES ${KERNEL_SOURCE_FILE})

include_directories(${XILINX_XRT}/include
                    ${XILINX_VIVADO}/include
                    ${XILINX_VITIS_HLS}/include
                    )

add_executable(main
               ${SOURCES}
               )

target_link_libraries(main
                      pthread
                      OpenCL
                      rt
                      stdc++
                      uuid
                      xrt_coreutil)


target_compile_options(main PRIVATE -Wall -O0 -g -std=c++1y -fmessage-length=0)

target_link_directories(main PRIVATE ${XILINX_XRT}/lib)

# Main is host
add_custom_target(host)
add_dependencies(host main)

# Add CTest support
enable_testing()
include(CTest)

# Add the test files
file(GLOB_RECURSE TEST_SOURCE_FILES tests/*.cpp)

# Get all source files from the src directory, except the main file
file(GLOB_RECURSE SRC_FILES ${SRC_DIR}/*.cpp)
list(REMOVE_ITEM SRC_FILES ${SRC_DIR}/host.cpp)
list(REMOVE_ITEM SRC_FILES ${KERNEL_SOURCE_FILE})

# Create test executables and link with the appropriate source files
foreach(test_file ${TEST_SOURCE_FILES})
  get_filename_component(test_name ${test_file} NAME_WE)

  add_executable(${test_name} 
        ${test_file} 
        ${SRC_FILES})
  target_include_directories(
    ${test_name} PRIVATE 
    ${CMAKE_SOURCE_DIR}/src)
  target_link_libraries(
    ${test_name} PRIVATE 
    pthread 
    OpenCL 
    rt 
    stdc++ 
    uuid 
    xrt_coreutil)
  target_link_directories(${test_name} PRIVATE ${XILINX_XRT}/lib)
  if(OpenMP_CXX_FOUND)
    target_link_libraries(${test_name} PUBLIC OpenMP::OpenMP_CXX)
  endif()
  add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# Add cpp files in the scripts
file(GLOB_RECURSE SCRIPT_FILES ${SCRIPT_DIR}/*.cpp)
foreach(script_file ${SCRIPT_FILES})
  get_filename_component(file_name ${script_file} NAME_WE)

  add_executable(${file_name} 
        ${script_file} 
        )
  target_link_libraries(
    ${test_name} PRIVATE 
    stdc++ )
endforeach()

# add_custom_target(all_default ALL)
# add_dependencies(all_default host kernel)

# Add custom target for code formatting using clang-format
find_program(CLANG_FORMAT "clang-format")

if(CLANG_FORMAT)
  file(GLOB_RECURSE ALL_SOURCE_FILES ${SRC_DIR}/*.cpp ${SRC_DIR}/*.h ${SRC_DIR}/*.hpp ${TEST_DIR}/*.cpp ${TEST_DIR}/*.h ${TEST_DIR}/*.hpp ${KERNEL_DIR}/*.cpp ${KERNEL_DIR}/*.h ${KERNEL_DIR}/*.hpp ${SCRIPT_DIR}/*.cpp)
  add_custom_target(
    format
    COMMAND ${CLANG_FORMAT}
    -i
    -style=LLVM
    ${ALL_SOURCE_FILES}
    COMMENT "Formatting source code with LLVM style..."
    VERBATIM
  )
endif()