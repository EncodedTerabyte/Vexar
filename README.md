# Vexar

A small experimental & compiled programming language with C-like & Go-like syntax, simple types, and built-in string/array manipulation. Designed to be readable, minimal, and easy to extend.

# If Statements
```go
func check(n: int): string {
    if (n > 0) {
        ret "positive"
    }
    if (n < 0) {
        ret "negative"
    }
    ret "zero"
}
```

# While Loops & Raw Arrays
```go
func sum_array(): int {
    var arr: int[] = {1, 2, 3, 4, 5}
    var i: int = 0
    var total: int = 0

    while (i < 5) {
        total = total + arr[i]
        i = i + 1
    }

    ret total // 15
}
```

# String Functions
```go
import std/string

func demo_strings(): int {
    var text: string = "hello world"

    println(len(text))           // 11
    println(substr(text, 0, 5))  // hello
    println(strrev(text))        // dlrow olleh
    println(strfind(text, "or")) // 7
    println(strreplace(text, "world", "Vexar")) // hello Vexar

    ret 0
}
```

```go
always_inline func doSum(x: int, y: int) {
    inline __cc__ {
        std::cout << "inline c++" << std::endl;

        int result;
        __asm__ volatile (
            "movl %1, %%eax\n\t"
            "addl %2, %%eax\n\t"
            "movl %%eax, %0"
            : "=r" (result)
            : "r" ($x), "r" ($y)
            : "eax"
        );
        std::cout << "Value: " << result << std::endl;
    }
}

inline func main() {
    print("Enter number: ")
    var x = (int)readLine()
    println("")

    print("Enter number: ")
    var y = (int)readLine()
    println("")
    
    doSum(x, y);
}
```