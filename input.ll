; ModuleID = 'test.vexar'
source_filename = "test.vexar"

@0 = private unnamed_addr constant [14 x i8] c"Hello, World!\00", align 1
@str_fmt = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1

; Function Attrs: noinline
define i32 @user_main() #0 {
entry:
  %0 = call i32 (ptr, ...) @printf(ptr @str_fmt, ptr @0)
  ret i32 0
}

declare i32 @printf(ptr, ...)

define i32 @main() {
entry:
  %0 = call i32 @user_main()
  ret i32 %0
}

attributes #0 = { noinline }
