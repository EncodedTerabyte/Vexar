# Vexar

A fast, experimental compiled programming language with C-like syntax, native Windows API support, and inline assembly capabilities. Built for systems programming with modern language features.

> ⚠️ **Early Development**: Vexar is actively under development and not ready for production use.

## Features

- **Native Compilation**: Direct compilation to machine code via LLVM & Clang as a dependancy
- **Inline C/C++/Assembly**: Mix languages seamlessly for performance-critical code
- **(INDEV) Windows API Integration**: Built-in support for Win32 development
- **Simple Type System**: Clear, predictable types without complexity

## Quick Start

```vexar
func main() {
    println("Hello, World!")
}
```

**Compile and run:**
```bash
vexar build hello.vx
./hello.exe
```

## Language Examples

### Control Flow
```vexar
func classify_number(n: int): string {
    if (n > 0) {
        return "positive"
    } else if (n < 0) {
        return "negative"
    } else {
        return "zero"
    }
}
```

### Arrays and Loops
```vexar
func sum_array(): int {
    let numbers: int[] = {1, 2, 3, 4, 5}
    var total: int = 0
    var i: int = 0

    while (i < len(numbers)) {
        total += numbers[i]
        i += 1
    }

    return total  // Returns 15
}
```

### String Processing
```vexar
import std/string

func process_text(): void {
    let text: string = "Hello, Vexar!"
    
    println("Length: " + len(text))                    // 13
    println("Substring: " + substr(text, 0, 5))        // "Hello"
    println("Uppercase: " + to_upper(text))            // "HELLO, VEXAR!"
    println("Find 'Vexar': " + find(text, "Vexar"))   // 7
    println("Replace: " + replace(text, "Vexar", "World")) // "Hello, World!"
}
```

### Inline Assembly & C++ Integration
```vexar
func windows_message_box(message: string, title: string): void {
    inline __cpp__ {
        MessageBoxA(NULL, $message, $title, MB_OK | MB_ICONINFORMATION);
    }
}

func main() {
    windows_message_box("Hello, World!", "Test")
}
```

## Installation

**Prerequisites:**
- LLVM 15+ with Clang
- Windows 10/11 (Linux support coming soon)

**Build from source:**
```bash
git clone https://github.com/EncodedTerabyte/vexar
cd vexar
python build
./build/dist/vexar.exe --version
```

## Documentation

- [Language Reference](docs/language-reference.md)
- [Standard Library](docs/standard-library.md)
- [Inline Assembly Guide](docs/inline-assembly.md)
- [Windows API Integration](docs/windows-api.md)

## Contributing

Vexar is open source and welcomes contributions. See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## Roadmap

- [ ] **Returning Multiple Values**: e.g: ret x, y, z, w
- [ ] **Function Overloading**: Functions with same identifiers but different arguments
- [ ] **Int Types**: 8bit - 64bit integer support
- [ ] **Containers**: Namespaces
- [ ] **Type System**: Structs, enums, generics
- [ ] **Memory Management**: Optional garbage collection
- [ ] **Cross Platform**: Linux and macOS support
- [ ] **Package Manager**: Dependency management system
- [ ] **IDE Integration**: Better VS Code language server

## License

Licensed under the [Vexar Programming Language License](LICENSE) - a permissive license similar to MIT that allows commercial use while keeping compiler modifications open source.

## Disclaimer

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND. Vexar is experimental software under active development.