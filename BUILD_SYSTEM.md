# xv6 CMake Build System

A **CMake-based parallel build system** for xv6 that works alongside the original Makefile.

## Features

✅ **Parallel Compilation** - Build multiple files simultaneously with `-j` flag  
✅ **Original Makefile Untouched** - Both systems work independently  
✅ **RISC-V Toolchain Support** - Works with any RISC-V cross-compiler  
✅ **Clean Separation** - CMake artifacts in `build/` directory  
✅ **Simple Configuration** - One command to configure  
✅ **QEMU Integration** - Run and debug directly from build system  

## Installation

### 1. Install CMake
```bash
# macOS
brew install cmake

# Ubuntu/Debian
sudo apt-get install cmake

# CentOS/RHEL
sudo yum install cmake
```

### 2. Install RISC-V Toolchain
The CMake build requires a RISC-V cross-compiler. Install one of:

```bash
# macOS - using Homebrew
brew tap SiFive/riscv
brew install riscv-tools

# Ubuntu/Debian
sudo apt-get install gcc-riscv64-unknown-elf
# or
sudo apt-get install gcc-riscv64-linux-gnu

# From source
git clone https://github.com/riscv-collab/riscv-gnu-toolchain
cd riscv-gnu-toolchain
./configure --prefix=/opt/riscv
make -j$(nproc)
```

## Quick Start

### Using CMake (Recommended)

```bash
# Configure build (auto-detects toolchain)
mkdir -p build && cd build
cmake -DTOOLPREFIX=riscv64-unknown-elf- ..

# Build with parallel jobs
cmake --build . -j4        # 4 parallel jobs
cmake --build . -j$(nproc) # All available cores

# Run in QEMU
cmake --build . --target qemu

# Run with GDB debugging
cmake --build . --target qemu-gdb
```

### Using Original Makefile

```bash
# Clean and build
make clean
make -j4          # Parallel build

# Run
make qemu         # Run in QEMU
make qemu-gdb     # Run with GDB
```

## Build Targets

### CMake Targets

```bash
# Build everything (default)
cmake --build .

# Build only kernel
cmake --build . --target kernel

# Build only user programs
cmake --build . --target user

# Build mkfs tool
cmake --build . --target mkfs

# Create filesystem image
cmake --build . --target fs.img

# Run in QEMU
cmake --build . --target qemu

# Run with GDB debugging
cmake --build . --target qemu-gdb
```

## Parallel Build Performance

CMake enables efficient parallel compilation. Use `-j` flag:

```bash
# Build with specified number of jobs
cmake --build . -j4

# Use all available CPU cores
cmake --build . -j$(nproc)

# Build verbosely to see all commands
cmake --build . -j4 --verbose
```

## Configuration Options

### Specify RISC-V Toolchain Prefix

```bash
# Using riscv64-unknown-elf
cmake -DTOOLPREFIX=riscv64-unknown-elf- ..

# Using riscv64-linux-gnu
cmake -DTOOLPREFIX=riscv64-linux-gnu- ..

# Using custom path
cmake -DTOOLPREFIX=/opt/riscv/bin/riscv64-unknown-elf- ..
```

### Configure QEMU CPUs

```bash
# Default: 3 CPUs
cmake -DCPUS=8 ..

# Run with different CPU count
cmake --build . --target qemu
```

## Directory Structure

```
.
├── CMakeLists.txt              # Main CMake configuration
├── cmake/
│   └── RiscVToolchain.cmake    # Toolchain detection module
├── kernel/                      # Kernel source files
├── user/                        # User program source
├── mkfs/                        # Filesystem tool source
├── Makefile                     # Original build system (still works!)
├── build/                       # CMake build artifacts (created by cmake)
│   ├── kernel/
│   │   └── kernel              # Final kernel binary
│   ├── user/
│   │   ├── _cat, _echo, ...    # User program binaries
│   │   └── *.asm, *.sym        # Assembly/symbol files
│   ├── mkfs/
│   │   └── mkfs                # Filesystem creation tool
│   └── fs.img                  # Filesystem disk image
└── README.md, BUILD_SYSTEM.md
```

## Compiler Flags

Both CMake and Makefile use identical RISC-V compiler flags:

**Architecture**: `-march=rv64gc` (64-bit RISC-V with G+C extensions)  
**Code Model**: `-mcmodel=medany` (position-independent)  
**Bare-metal**: `-ffreestanding -nostdlib` (no libc)  
**Optimizations**: `-O` with frame pointers and DWARF-2 debug info  

## Build Files Generated

### Kernel Build Artifacts
- `build/kernel/kernel` - Final kernel binary
- `build/kernel/kernel.asm` - Disassembly
- `build/kernel/kernel.sym` - Symbol table

### User Program Artifacts
- `build/user/_<prog>` - User program binaries
- `build/user/<prog>.asm` - Program disassembly
- `build/user/<prog>.sym` - Program symbols

### Filesystem Image
- `build/fs.img` - Bootable filesystem image

## Troubleshooting

### CMake not found
```bash
# Verify CMake is installed
cmake --version

# Install CMake
brew install cmake  # macOS
apt-get install cmake  # Ubuntu
```

### RISC-V toolchain not found
```bash
# Check which toolchain you have
which riscv64-unknown-elf-gcc
which riscv64-linux-gnu-gcc

# Install toolchain or specify path
cmake -DTOOLPREFIX=/path/to/toolchain/riscv64-unknown-elf- ..
```

### Build fails with compiler errors
```bash
# Check compilation commands
cmake --build . -j1 --verbose

# Rebuild from scratch
rm -rf build
mkdir build && cd build
cmake -DTOOLPREFIX=riscv64-unknown-elf- ..
cmake --build .
```

### CMake caching issues
```bash
# Clear CMake cache and rebuild
rm -rf build
mkdir build && cd build
cmake -DTOOLPREFIX=riscv64-unknown-elf- ..
cmake --build .
```

## Both Build Systems Work Together

You can use **either** system - they're completely independent:

| System | Output Location | Pros |
|--------|-----------------|------|
| Makefile | `kernel/`, `user/`, `./*.img` | Simple, no dependencies |
| CMake | `build/kernel/`, `build/user/`, `build/*.img` | Parallel, modern, organized |

Build artifacts don't conflict because they go to different directories.

## Using xv6

### Boot into QEMU
```bash
cmake --build . --target qemu
# Press Ctrl+A then X to exit QEMU
```

### Debug with GDB
```bash
# Terminal 1: Start QEMU with GDB support
cmake --build . --target qemu-gdb

# Terminal 2: Start GDB and connect
riscv64-unknown-elf-gdb build/kernel/kernel
(gdb) target remote localhost:1234
(gdb) break main
(gdb) continue
```

### Useful GDB Commands
```gdb
info reg            # Show registers
info mem            # Show memory layout
dis main            # Disassemble main function
stepi               # Single-step instruction
nexti               # Step over
continue            # Resume execution
bt                  # Show backtrace
```

## Advanced Usage

### Custom build directory
```bash
cmake -B build-release -DTOOLPREFIX=riscv64-unknown-elf- -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

### Use Ninja instead of Make
```bash
cmake -G Ninja -DTOOLPREFIX=riscv64-unknown-elf- ..
cmake --build .  # Uses ninja automatically
```

### Verbose build output
```bash
cmake --build . --verbose

# Or set environment
VERBOSE=1 cmake --build .
```

## Performance Tips

1. **Use parallel builds** - Always use `-j` with CMake:
   ```bash
   cmake --build . -j$(nproc)
   ```

2. **Incremental builds** - Only changed files are recompiled

3. **Check dependencies** - CMake tracks header file changes automatically

4. **Link-time optimization** - Can be enabled with CMake flags

## Files in This Repository

**Created/Modified:**
- `CMakeLists.txt` - Main CMake build configuration
- `cmake/RiscVToolchain.cmake` - RISC-V toolchain detection
- `BUILD_SYSTEM.md` - This documentation

**Unchanged:**
- `Makefile` - Original build system (fully functional)
- All source files - No modifications to kernel/, user/, mkfs/

## Summary

The CMake build system provides:
- ✅ Parallel compilation for faster builds
- ✅ Clean separation of build artifacts
- ✅ Automatic RISC-V toolchain detection
- ✅ Modern build system practices
- ✅ Compatible with original Makefile

Choose whichever build system works best for your workflow!
