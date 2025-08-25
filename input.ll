; ModuleID = 'test.vexar'
source_filename = "test.vexar"

@type_result = private unnamed_addr constant [4 x i8] c"int\00", align 1
@str_fmt = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@0 = private unnamed_addr constant [30 x i8] c"Enter a number for rounding: \00", align 1
@str_fmt.1 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@input_fmt = private unnamed_addr constant [10 x i8] c" %255[^\0A]\00", align 1
@1 = private unnamed_addr constant [10 x i8] c"Rounded: \00", align 1
@int_to_str_fmt = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@str_fmt.2 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1

; Function Attrs: noinline
define i32 @round(double %x) #0 {
entry:
  %x.addr = alloca double, align 8
  store double %x, ptr %x.addr, align 8
  br label %cond0

cond0:                                            ; preds = %entry
  %x1 = load double, ptr %x.addr, align 8
  %fcmp_ge = fcmp oge double %x1, 0.000000e+00
  br i1 %fcmp_ge, label %body0, label %else

body0:                                            ; preds = %cond0
  %x2 = load double, ptr %x.addr, align 8
  %addtmp = fadd double %x2, 5.000000e-01
  %0 = fptosi double %addtmp to i32
  ret i32 %0

else:                                             ; preds = %cond0
  %x3 = load double, ptr %x.addr, align 8
  %subtmp = fsub double %x3, 5.000000e-01
  %1 = fptosi double %subtmp to i32
  ret i32 %1
}

; Function Attrs: noinline
define i32 @user_main() #0 {
entry:
  %0 = call i32 (ptr, ...) @printf(ptr @str_fmt, ptr @type_result)
  %1 = call i32 (ptr, ...) @printf(ptr @str_fmt.1, ptr @0)
  %input_buffer = call ptr @malloc(i64 256)
  %2 = call i32 (ptr, ...) @scanf(ptr @input_fmt, ptr %input_buffer)
  %3 = call double @strtod(ptr %input_buffer, ptr null)
  %in = alloca ptr, align 8
  store double %3, ptr %in, align 8
  %in1 = load ptr, ptr %in, align 8
  %4 = call i32 @round(ptr %in1)
  %x = alloca i32, align 4
  store i32 %4, ptr %x, align 4
  %x2 = load i32, ptr %x, align 4
  %5 = call ptr @malloc(i64 32)
  %6 = call i32 (ptr, ptr, ...) @sprintf(ptr %5, ptr @int_to_str_fmt, i32 %x2)
  %7 = call i64 @strlen(ptr @1)
  %8 = call i64 @strlen(ptr %5)
  %9 = add i64 %7, %8
  %10 = add i64 %9, 1
  %11 = call ptr @malloc(i64 %10)
  %12 = call ptr @strcpy(ptr %11, ptr @1)
  %13 = call ptr @strcat(ptr %11, ptr %5)
  %14 = call i32 (ptr, ...) @printf(ptr @str_fmt.2, ptr %11)
  ret i32 0
}

declare i32 @printf(ptr, ...)

declare i32 @scanf(ptr, ...)

declare ptr @malloc(i64)

declare double @strtod(ptr, ptr)

declare i32 @sprintf(ptr, ptr, ...)

declare i64 @strlen(ptr)

declare ptr @strcpy(ptr, ptr)

declare ptr @strcat(ptr, ptr)

define i32 @main() {
entry:
  %0 = call i32 @user_main()
  ret i32 %0
}

attributes #0 = { noinline }
