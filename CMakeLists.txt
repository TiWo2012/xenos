cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_COMPILER clang++ CACHE STRING "C++ compiler" FORCE)
set(CMAKE_ASM_NASM_COMPILER nasm CACHE STRING "NASM assembler" FORCE)
set(CMAKE_LINKER ld.lld CACHE STRING "Linker" FORCE)

project(xenos LANGUAGES CXX ASM_NASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(POLICY CMP0156)
  cmake_policy(SET CMP0156 NEW)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ASM_NASM_FLAGS "-f elf64")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g -O0 -ffreestanding -fno-exceptions -fno-rtti -m64 -fno-stack-protector")

file(GLOB_RECURSE CPP_SOURCES src/*.cpp)
file(GLOB_RECURSE ASM_SOURCES src/*.asm)
file(GLOB_RECURSE MODULE_SOURCES src/*.cppm)

add_executable(kernel.elf ${CPP_SOURCES} ${ASM_SOURCES} ${MODULE_SOURCES})

target_sources(kernel.elf
  PRIVATE
    FILE_SET CXX_MODULES
    BASE_DIRS ${CMAKE_SOURCE_DIR}/src
    FILES
      ${CMAKE_SOURCE_DIR}/src/utils.cppm
      ${CMAKE_SOURCE_DIR}/src/drivers/binio.cppm
      ${CMAKE_SOURCE_DIR}/src/drivers/serial.cppm
      ${CMAKE_SOURCE_DIR}/src/drivers/font.cppm
      ${CMAKE_SOURCE_DIR}/src/drivers/vga.cppm
      ${CMAKE_SOURCE_DIR}/src/drivers/mem/pmm.cppm
      ${CMAKE_SOURCE_DIR}/src/drivers/idt.cppm
      ${CMAKE_SOURCE_DIR}/src/drivers/irq/pit.cppm
      ${CMAKE_SOURCE_DIR}/src/drivers/mem/heap.cppm
      ${CMAKE_SOURCE_DIR}/src/drivers/mem/vmm.cppm
      ${CMAKE_SOURCE_DIR}/src/drivers/terminal/terminal.cppm
      ${CMAKE_SOURCE_DIR}/src/drivers/irq/kbd.cppm
      ${CMAKE_SOURCE_DIR}/src/tests.cppm
)

set_target_properties(kernel.elf PROPERTIES POSITION_INDEPENDENT_CODE OFF)

target_include_directories(kernel.elf PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/src/drivers
)

target_link_options(kernel.elf PRIVATE
  -T ${CMAKE_SOURCE_DIR}/linker.ld
  -nostdlib -no-pie
)

set(ISO_DIR ${CMAKE_BINARY_DIR}/iso)

add_custom_target(iso ALL
  COMMAND ${CMAKE_COMMAND} -E rm -rf ${ISO_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${ISO_DIR}/boot/grub
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:kernel.elf> ${ISO_DIR}/boot/kernel.elf
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/grub.cfg ${ISO_DIR}/boot/grub/grub.cfg
  COMMAND grub-mkrescue -o ${CMAKE_BINARY_DIR}/os.iso ${ISO_DIR}
  DEPENDS kernel.elf
)

add_custom_target(run
  COMMAND qemu-system-x86_64 -cdrom ${CMAKE_BINARY_DIR}/os.iso -serial stdio
  DEPENDS iso
)

add_custom_target(clean-all
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
)
