import subprocess
from pathlib import Path
import os
from concurrent.futures import ProcessPoolExecutor, as_completed
import shutil

C_COMPILER = "clang"
CPP_COMPILER = "clang++"
LINKER = "clang++"

CFLAGS = [
   "-w",
   "-c",
   "-Wall",
   "-m64",
   "-std=c17",
   "-fms-compatibility",
   "-DNOMINMAX",
   "-IC:/Program Files/LLVM/include",
   "-Wno-microsoft-include",
   "-Wno-ignored-attributes",
   "-fms-extensions",
   "-D_CRT_SECURE_NO_WARNINGS",
    "-Os",
    "-flto",
    "-fno-unwind-tables",
    "-fno-asynchronous-unwind-tables",
    "-fomit-frame-pointer",
    "-fmerge-all-constants",
]

CPPFLAGS = [
   "-w",
   "-c", 
   "-Wall",
   "-m64",
   "-std=c++20",
   "-fms-compatibility",
   "-DNOMINMAX",
   "-IC:/Program Files/LLVM/include",
   "-Wno-microsoft-include",
   "-Wno-ignored-attributes",
   "-fms-extensions",
   "-D_CRT_SECURE_NO_WARNINGS",
   "-Os",
   "-flto",
]

LINKFLAGS = [
   "-flto",
   "-O3",
   "-s",
   "-fuse-ld=lld",
   "-Wl,/OPT:REF",
   "-Wl,/OPT:ICF",
   "-ffunction-sections",
   "-fdata-sections",
]

OUTPUT_EXE_NAME = "vexar.exe"

project_root = Path(".")
build_dir = project_root / "build"
objects_dir = build_dir / "obj"
dist_dir = build_dir / "dist"

def is_c_file(file_path: Path) -> bool:
   return file_path.suffix.lower() == ".c"

def is_cpp_file(file_path: Path) -> bool:
   return file_path.suffix.lower() in [".cpp", ".cc", ".c++", ".cxx"]

def compile_source(file_path: Path):
   obj_file = objects_dir / f"{file_path.stem}.o"
   if obj_file.exists() and obj_file.stat().st_mtime >= file_path.stat().st_mtime:
       return obj_file
   
   if is_c_file(file_path):
       compiler = C_COMPILER
       flags = CFLAGS
   else:
       compiler = CPP_COMPILER
       flags = CPPFLAGS
   
   cmd = [compiler] + flags + [str(file_path), "-o", str(obj_file)]
   subprocess.run(cmd, check=True)
   print(f"Compiled {file_path.name} -> {obj_file.name}")
   return obj_file

def copy_file(src: Path, dst: Path):
   dst.parent.mkdir(parents=True, exist_ok=True)
   shutil.copy2(src, dst)
   print(f"Copied {src.name} -> {dst.name}")

def main():
   objects_dir.mkdir(parents=True, exist_ok=True)
   dist_dir.mkdir(parents=True, exist_ok=True)

   sources = list(project_root.glob("**/*.c")) + \
             list(project_root.glob("**/*.cpp")) + \
             list(project_root.glob("**/*.cc")) + \
             list(project_root.glob("**/*.c++")) + \
             list(project_root.glob("**/*.cxx"))

   with ProcessPoolExecutor(max_workers=os.cpu_count()) as executor:
       futures = {executor.submit(compile_source, src): src for src in sources}
       object_files = [f.result() for f in as_completed(futures)]

   object_files_str = [str(f) for f in object_files]

   lib_dir = project_root / "lib"
   lib_files = list(lib_dir.rglob("*.lib"))
   lib_paths = [str(lib) for lib in lib_files]

   output_exe = dist_dir / OUTPUT_EXE_NAME
   needs_link = (
       not output_exe.exists() or
       any(f.stat().st_mtime > output_exe.stat().st_mtime for f in object_files) or
       any(lib.stat().st_mtime > output_exe.stat().st_mtime for lib in lib_files)
   )

   if needs_link:
       link_cmd = [LINKER] + LINKFLAGS + ["-o", str(output_exe)] + object_files_str + lib_paths
       subprocess.run(link_cmd, check=True)
       print(f"Linked executable -> {output_exe.name}")

   for dll in lib_dir.rglob("*.dll"):
       copy_file(dll, dist_dir / dll.name)

if __name__ == "__main__":
   from multiprocessing import freeze_support
   freeze_support()
   main()