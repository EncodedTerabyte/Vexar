#include "Menu.hh"

void PrintVesion() {
    std::cout << "\nVexar (Built & Officially Owned by EncodedTerabyte) 0.1.0 INDEV \n";
    std::cout << "Copyright (C) 2025 Vexar Source License. \n";
}

void PrintHelpMenu() {
    std::cout << "Usage:\n";
    std::cout << "  vexar [options] <file>\n\n";

    std::cout << "Commands:\n";
    std::cout << "  --h, --help          Show this help message and exit\n";
    std::cout << "  --v --version        Show version information\n";
    std::cout << "  --t, --targets       Show available compilation targets\n\n";

    std::cout << "Compiler Options:\n";
    std::cout << "  -I, --include <dir>     Add directory to import/include path\n";
    std::cout << "  -o, --output <file>     Specify output file name\n";
    std::cout << "  -t, --target=<target>   Specify the compilation target (see --targets)\n";
    std::cout << "  -w, --no-warnings       Suppress warning messages\n";
    std::cout << "  -r, --run               Compile and run the program directly\n";
    std::cout << "  -O[level]               Set optimization level (0-5)\n\n";

    std::cout << "Analysis Options:\n";
    std::cout << "  -f, --full-analysis       Perform full module analysis (all options)\n";
    std::cout << "  -b, --bin                 Perform binary analysis\n";
    std::cout << "  -vec, --vectorization     Analyze vectorization opportunities\n";
    std::cout << "  -opt, --optimizations     Analyze optimizations applied\n";
    std::cout << "  -s, --symbols             Analyze module symbols\n";
    std::cout << "  -e, --memory              Analyze memory usage\n";
    std::cout << "  -m, --module              Analyze module structure\n";
    std::cout << "  -d, --dump                Dump assembler information\n";
    std::cout << "  -bc, --dump-bc            Dump bitcode\n";
    std::cout << "  -vbc, --dump-verbose-bc   Dump the verbose binary; bitcode\n";
    std::cout << "  -ir, --dump-ir            Dump the generated LLVM IR\n";
    std::cout << "  -vir, --dump-verbose-ir   Dump the disassembled LLVM IR\n";
    std::cout << "  -c, --check               Check syntax only (no codegen)\n";
    std::cout << "  -g, --debug               Enable debug mode\n";
    std::cout << "  -v, --verbose             Enable verbose output\n";
    std::cout << "  -t, --print-tokens        Print token stream\n";
    std::cout << "  -a, --print-ast           Print abstract syntax tree\n\n";
    
    std::cout << "Examples:\n";
    std::cout << "  vexar hello.vxr                 Compile 'hello.vxr' into an executable\n";
    std::cout << "  vexar -r hello.vxr              Compile and run immediately\n";
    std::cout << "  vexar -c test.vxr               Check syntax only\n";
    std::cout << "  vexar -a hello.vxr              Show AST of program\n";
    std::cout << "  vexar -t hello.vxr              Show tokens of program\n\n";
}

void PrintTargets() {
    std::cout << "Available Targets:\n\n";

    std::cout << "  interpret, i (use -r)            [Clang/LLVM/LLVM Toolchain]\n\n";
    
    std::cout << "Windows Targets:\n";
    std::cout << "  exe, win, windows, win64         [Clang/Windows]\n";
    std::cout << "  win32, windows32                 [Clang/Windows]\n";
    std::cout << "  win-arm64, windows-arm64         [Clang/Windows]\n";
    std::cout << "  win-gnu, mingw                   [Clang/Windows/MinGW]\n\n";

    std::cout << "Linux Targets:\n";
    std::cout << "  linux, linux64                   [Clang/Linux]\n";
    std::cout << "  linux32                          [Clang/Linux]\n";
    std::cout << "  linux-arm64                      [Clang/Linux]\n";
    std::cout << "  linux-arm                        [Clang/Linux]\n";
    std::cout << "  linux-riscv64                    [Clang/Linux]\n";
    std::cout << "  linux-musl                       [Clang/Linux]\n";
    std::cout << "  linux-arm64-musl                 [Clang/Linux]\n\n";

    std::cout << "macOS/iOS Targets:\n";
    std::cout << "  macos, darwin                    [Clang/macOS]\n";
    std::cout << "  macos-arm64, darwin-arm64        [Clang/macOS]\n";
    std::cout << "  ios                              [Clang/macOS]\n";
    std::cout << "  ios-arm64                        [Clang/macOS]\n\n";

    std::cout << "BSD/Unix Targets:\n";
    std::cout << "  freebsd                          [Clang/FreeBSD Toolchain]\n";
    std::cout << "  freebsd-arm64                    [Clang/FreeBSD Toolchain]\n";
    std::cout << "  openbsd                          [Clang/OpenBSD Toolchain]\n";
    std::cout << "  netbsd                           [Clang/NetBSD Toolchain]\n";
    std::cout << "  solaris                          [Clang/Solaris Toolchain]\n";
    std::cout << "  haiku                            [Clang/Haiku Toolchain]\n\n";

    std::cout << "ELF Bare Metal Targets:\n";
    std::cout << "  elf64                            [Clang/Linux Cross-Compiler]\n";
    std::cout << "  elf32                            [Clang/Linux Cross-Compiler]\n";
    std::cout << "  elf-arm64                        [Clang/Linux Cross-Compiler]\n";
    std::cout << "  elf-arm                          [Clang/Linux Cross-Compiler]\n\n";

    std::cout << "Alternative Architecture Targets:\n";
    std::cout << "  mips64, mips                     [Clang/MIPS Toolchain]\n";
    std::cout << "  ppc64, ppc64le                   [Clang/PowerPC Toolchain]\n";
    std::cout << "  s390x                            [Clang/s390x Toolchain]\n";
    std::cout << "  sparc64                          [Clang/SPARC Toolchain]\n";
    std::cout << "  fuchsia, fuchsia-arm64           [Clang/Fuchsia SDK]\n\n";

    std::cout << "WebAssembly Targets:\n";
    std::cout << "  wasm, webassembly, web           [Clang/Any OS]\n";
    std::cout << "  wasm64, webassembly64            [Clang/Any OS]\n\n";

    std::cout << "Assembly Output Targets:\n";
    std::cout << "  asm, assembly, s                 [Clang/Any OS]\n";
    std::cout << "  asm-intel, intel                 [Clang/Any OS]\n";
    std::cout << "  asm-att, att                     [Clang/Any OS]\n";
    std::cout << "  asm-arm, arm-asm                 [Clang/Any OS]\n";
    std::cout << "  asm-arm64, arm64-asm             [Clang/Any OS]\n";
    std::cout << "  asm-riscv64, riscv64-asm         [Clang/Any OS]\n";
    std::cout << "  asm-wasm, wasm-asm               [Clang/Any OS]\n";
    std::cout << "  asm-ptx, ptx-asm                 [Clang/CUDA Toolkit]\n\n";

    std::cout << "Intermediate Formats:\n";
    std::cout << "  llvm, ir                         [Clang/Any OS]\n";
    std::cout << "  bitcode, bc                      [Clang/Any OS]\n";
    std::cout << "  obj, object, o                   [Clang/Any OS]\n\n";

    std::cout << "GPU/CUDA Targets:\n";
    std::cout << "  cuda, ptx64, nvidia              [Clang/CUDA Toolkit]\n";
    std::cout << "  cuda32, ptx32                    [Clang/CUDA Toolkit]\n\n";

    std::cout << "Bare Metal/Embedded Targets:\n";
    std::cout << "  arm64-raw, aarch64               [Clang/Any OS]\n";
    std::cout << "  arm-raw, arm                     [Clang/Any OS]\n";
    std::cout << "  riscv64-raw, riscv64             [Clang/Any OS]\n";
    std::cout << "  riscv32-raw, riscv32             [Clang/Any OS]\n";
    std::cout << "  thumb-raw, thumb                 [Clang/Any OS]\n\n";

    std::cout << "Extended ARM/x86 Targets:\n";
    std::cout << "  aarch64_32, arm64_32             [Clang/ARM Toolchain]\n";
    std::cout << "  aarch64_be, arm64-be             [Clang/ARM Toolchain]\n";
    std::cout << "  armeb, arm-be                    [Clang/ARM Toolchain]\n";
    std::cout << "  thumbeb, thumb-be                [Clang/ARM Toolchain]\n";

    std::cout << "Kernel/eBPF Targets:\n";
    std::cout << "  bpf, ebpf, kernel                [Clang/Linux BPF Support]\n";
    std::cout << "  bpf-le, bpfel                    [Clang/Linux BPF Support]\n";
    std::cout << "  bpf-be, bpfeb                    [Clang/Linux BPF Support]\n\n";
}