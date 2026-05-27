cmake_minimum_required(VERSION 3.16)

project(xv6 C ASM)

set(TOOLPREFIX "riscv64-elf-")

message(STATUS "=== xv6 CMake Build System ===")
message(STATUS "RISC-V Toolchain Prefix: ${TOOLPREFIX}")

# Tools
set(CC "${TOOLPREFIX}gcc")
set(AS "${TOOLPREFIX}gas")
set(LD "${TOOLPREFIX}ld")
set(OBJCOPY "${TOOLPREFIX}objcopy")
set(OBJDUMP "${TOOLPREFIX}objdump")
set(AR "${TOOLPREFIX}ar")

# Verify that the selected toolchain is available
find_program(CC_PATH NAMES ${CC})
if(CC_PATH)
  message(STATUS "C Compiler: ${CC_PATH}")
else()
  message(FATAL_ERROR "RISC-V C Compiler not found: ${CC}. Ensure the toolchain is installed and in PATH, or set -DTOOLPREFIX=<prefix>-.")
endif()

# Architecture and flags
set(MARCH "rv64gc")
set(MCMODEL "medany")

set(COMMON_FLAGS
  -Wall
  -Werror
  -Wno-unknown-attributes
  -O
  -fno-omit-frame-pointer
  -ggdb
  -gdwarf-2
  -march=${MARCH}
  -mcmodel=${MCMODEL}
  -ffreestanding
  -fno-common
  -nostdlib
  -fno-builtin-strncpy
  -fno-builtin-strncmp
  -fno-builtin-strlen
  -fno-builtin-memset
  -fno-builtin-memmove
  -fno-builtin-memcmp
  -fno-builtin-log
  -fno-builtin-bzero
  -fno-builtin-strchr
  -fno-builtin-exit
  -fno-builtin-malloc
  -fno-builtin-putc
  -fno-builtin-free
  -fno-builtin-memcpy
  -Wno-main
  -fno-builtin-printf
  -fno-builtin-fprintf
  -fno-builtin-vprintf
  -I${CMAKE_SOURCE_DIR}
  -MD
)

# Optional flags
execute_process(
  COMMAND ${CC} -fno-stack-protector -E -x c /dev/null
  OUTPUT_QUIET
  ERROR_QUIET
  RESULT_VARIABLE RESULT
)
if(RESULT EQUAL 0)
  list(APPEND COMMON_FLAGS -fno-stack-protector)
endif()

execute_process(
  COMMAND ${CC} -dumpspecs
  OUTPUT_VARIABLE SPECS
  ERROR_QUIET
)
if(SPECS MATCHES "no-pie")
  list(APPEND COMMON_FLAGS -fno-pie -no-pie)
endif()

# Convert list to string for command line
string(REPLACE ";" " " CFLAGS_STR "${COMMON_FLAGS}")

message(STATUS "Architecture: ${MARCH}")
message(STATUS "Code Model: ${MCMODEL}")
message(STATUS "Build directory: ${CMAKE_BINARY_DIR}")

# Create build directories
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/kernel")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/user")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/mkfs")

# Kernel object files
set(KERNEL_OBJS
  entry start console printf uart kalloc spinlock string main vm
  proc swtch trampoline trap syscall sysproc bio fs log sleeplock
  file pipe exec sysfile kernelvec plic virtio_disk
)

set(KERNEL_OBJ_PATHS "")
foreach(OBJ ${KERNEL_OBJS})
  list(APPEND KERNEL_OBJ_PATHS "${CMAKE_BINARY_DIR}/kernel/${OBJ}.o")
  
  set(SRC_PATH "${CMAKE_SOURCE_DIR}/kernel/${OBJ}")
  if(EXISTS "${SRC_PATH}.c")
    add_custom_command(
      OUTPUT "${CMAKE_BINARY_DIR}/kernel/${OBJ}.o"
      COMMAND ${CC} ${COMMON_FLAGS} -c -o "${CMAKE_BINARY_DIR}/kernel/${OBJ}.o" "${SRC_PATH}.c"
      DEPENDS "${SRC_PATH}.c"
      COMMENT "Compiling kernel/${OBJ}.c"
    )
  elseif(EXISTS "${SRC_PATH}.S")
    add_custom_command(
      OUTPUT "${CMAKE_BINARY_DIR}/kernel/${OBJ}.o"
      COMMAND ${CC} -march=${MARCH} -g -c -o "${CMAKE_BINARY_DIR}/kernel/${OBJ}.o" "${SRC_PATH}.S"
      DEPENDS "${SRC_PATH}.S"
      COMMENT "Assembling kernel/${OBJ}.S"
    )
  endif()
endforeach()

set(KERNEL_LINK_OBJ_PATHS "")
foreach(OBJ_PATH ${KERNEL_OBJ_PATHS})
  if(NOT OBJ_PATH MATCHES ".*/entry\\.o$")
    list(APPEND KERNEL_LINK_OBJ_PATHS "${OBJ_PATH}")
  endif()
endforeach()

# Kernel target
# add_custom_command(
#   OUTPUT "${CMAKE_BINARY_DIR}/kernel/kernel"
#   COMMAND ${LD} -z max-page-size=4096 -T ${CMAKE_SOURCE_DIR}/kernel/kernel.ld 
#           -o "${CMAKE_BINARY_DIR}/kernel/kernel" ${KERNEL_LINK_OBJ_PATHS}
#   COMMAND ${OBJDUMP} -S ${CMAKE_BINARY_DIR}/kernel/kernel > ${CMAKE_BINARY_DIR}/kernel/kernel.asm
#   COMMAND ${OBJDUMP} -t ${CMAKE_BINARY_DIR}/kernel/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > ${CMAKE_BINARY_DIR}/kernel/kernel.sym
#   DEPENDS ${KERNEL_OBJ_PATHS}
#   COMMENT "Linking kernel"
# )

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/kernel/kernel

    COMMAND ${LD}
        -z max-page-size=4096
        -T ${CMAKE_SOURCE_DIR}/kernel/kernel.ld
        -o ${CMAKE_BINARY_DIR}/kernel/kernel
        ${KERNEL_LINK_OBJ_PATHS}

    COMMAND /bin/sh -c "${OBJDUMP} -S ${CMAKE_BINARY_DIR}/kernel/kernel > ${CMAKE_BINARY_DIR}/kernel/kernel.asm"

    COMMAND /bin/sh -c
        "'${OBJDUMP}' -t '${CMAKE_BINARY_DIR}/kernel/kernel' | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > '${CMAKE_BINARY_DIR}/kernel/kernel.sym'"

    VERBATIM
    DEPENDS ${KERNEL_OBJ_PATHS}
    COMMENT "Linking kernel"
)

add_custom_target(xv6_kernel ALL
    DEPENDS ${CMAKE_BINARY_DIR}/kernel/kernel
)

# User library objects
set(ULIB_SRCS ulib usys printf umalloc)
set(ULIB_OBJS "")

foreach(LIB ${ULIB_SRCS})
  set(SRC_PATH "${CMAKE_SOURCE_DIR}/user/${LIB}")
  set(OBJ_PATH "${CMAKE_BINARY_DIR}/user/${LIB}.o")
  list(APPEND ULIB_OBJS "${OBJ_PATH}")
  
  if(EXISTS "${SRC_PATH}.c")
    add_custom_command(
      OUTPUT "${OBJ_PATH}"
      COMMAND ${CC} ${COMMON_FLAGS} -c -o "${OBJ_PATH}" "${SRC_PATH}.c"
      DEPENDS "${SRC_PATH}.c"
      COMMENT "Compiling user/${LIB}.c"
    )
  elseif(EXISTS "${SRC_PATH}.S")
    add_custom_command(
      OUTPUT "${OBJ_PATH}"
      COMMAND ${CC} ${COMMON_FLAGS} -c -o "${OBJ_PATH}" "${SRC_PATH}.S"
      DEPENDS "${SRC_PATH}.S"
      COMMENT "Assembling user/${LIB}.S"
    )
  endif()
endforeach()

# User programs
set(UPROGS
  cat echo forktest grep init kill ln ls mkdir rm sh stressfs
  usertests grind wc zombie logstress forphan dorphan
)

set(UPROGS_BINS "")
foreach(PROG ${UPROGS})
  set(SRC "${CMAKE_SOURCE_DIR}/user/${PROG}.c")
  set(OBJ "${CMAKE_BINARY_DIR}/user/${PROG}.o")
  set(BIN "${CMAKE_BINARY_DIR}/user/_${PROG}")
  
  list(APPEND UPROGS_BINS "${BIN}")
  
  add_custom_command(
    OUTPUT "${OBJ}"
    COMMAND ${CC} ${COMMON_FLAGS} -c -o "${OBJ}" "${SRC}"
    DEPENDS "${SRC}"
    COMMENT "Compiling user/${PROG}.c"
  )
  
  add_custom_command(
    OUTPUT "${BIN}"
    COMMAND ${LD} -z max-page-size=4096 -T${CMAKE_SOURCE_DIR}/user/user.ld 
            -o "${BIN}" "${OBJ}" ${ULIB_OBJS}
    COMMAND ${OBJDUMP} -S "${BIN}" > "${CMAKE_BINARY_DIR}/user/${PROG}.asm"
    COMMAND /bin/sh -c "${OBJDUMP} -t \"${BIN}\" | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^\$/d' > \"${CMAKE_BINARY_DIR}/user/${PROG}.sym\""
    VERBATIM
    DEPENDS "${OBJ}" ${ULIB_OBJS}
    COMMENT "Linking user/_${PROG}"
  )
endforeach()

# mkfs tool (native compilation)
add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/mkfs/mkfs"
  COMMAND gcc -Wno-unknown-attributes -I${CMAKE_SOURCE_DIR} 
          -o "${CMAKE_BINARY_DIR}/mkfs/mkfs" "${CMAKE_SOURCE_DIR}/mkfs/mkfs.c"
  VERBATIM        
  DEPENDS "${CMAKE_SOURCE_DIR}/mkfs/mkfs.c"
  COMMENT "Building mkfs"
)

# fs.img
add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/fs.img"
  COMMAND "${CMAKE_BINARY_DIR}/mkfs/mkfs" "${CMAKE_BINARY_DIR}/fs.img" "${CMAKE_SOURCE_DIR}/README.md" ${UPROGS_BINS}
  DEPENDS "${CMAKE_BINARY_DIR}/mkfs/mkfs" ${UPROGS_BINS} "${CMAKE_SOURCE_DIR}/README.md"
  COMMENT "Building filesystem image"
)

# Main targets
add_custom_target(user DEPENDS ${UPROGS_BINS})
add_custom_target(fs.img DEPENDS "${CMAKE_BINARY_DIR}/fs.img")
add_custom_target(mkfs DEPENDS "${CMAKE_BINARY_DIR}/mkfs/mkfs")

# Build all
add_custom_target(all_xv6 ALL
  DEPENDS 
    kernel
    user
    mkfs
    fs.img
  COMMENT "Build complete!"
)

# QEMU targets
if(NOT DEFINED CPUS)
  set(CPUS 3)
endif()

add_custom_target(qemu
  COMMAND qemu-system-riscv64 -machine virt -bios none -kernel "${CMAKE_BINARY_DIR}/kernel/kernel"
          -m 128M -smp ${CPUS} -nographic
          -global virtio-mmio.force-legacy=false
          -drive file="${CMAKE_BINARY_DIR}/fs.img",if=none,format=raw,id=x0
          -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
  DEPENDS all_xv6
  COMMENT "Running xv6 in QEMU..."
)

add_custom_target(qemu-gdb
  COMMAND qemu-system-riscv64 -machine virt -bios none -kernel "${CMAKE_BINARY_DIR}/kernel/kernel"
          -m 128M -smp ${CPUS} -nographic
          -global virtio-mmio.force-legacy=false
          -drive file="${CMAKE_BINARY_DIR}/fs.img",if=none,format=raw,id=x0
          -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
          -S -gdb tcp::1234
  DEPENDS all_xv6
  COMMENT "Running xv6 in QEMU with GDB..."
)

message(STATUS "")
message(STATUS "Build targets:")
message(STATUS "  cmake --build . --target kernel   # Build kernel only")
message(STATUS "  cmake --build . --target user     # Build user programs")
message(STATUS "  cmake --build . --target all_xv6  # Build everything")
message(STATUS "  cmake --build . --target qemu     # Run in QEMU")
message(STATUS "  cmake --build . --target qemu-gdb # Run with GDB")
message(STATUS "")
message(STATUS "Parallel build:")
message(STATUS "  cmake --build . -j4                # Build with 4 parallel jobs")
message(STATUS "  cmake --build . -j$(nproc)         # Use all cores")
message(STATUS "")
