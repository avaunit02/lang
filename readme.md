# lang
## depends
Flex and Bison are used for tokenising and parsing the language. Many distros will provide flex and bison by default, in Arch they are in package group base-devel.

```# pacman -S flex bison```

LLVM (for LLVM IR backend) and glslang (for SPIR-V backend) for code generation.

```# pacman -S llvm-libs glslang```

## usage
```
$ make
$ out/compiler input.lang output.ir
```

## testing
```
$ make test #will run all tests
$ make test debug=1 #as above and will drop you into a GDB session when a test fails
```

## ideas
- halide and futhark inspiration
  - futhark is a language compiler written in haskell generating opencl
  - halide is an eDSL in C++ using LLVM as a backend, then generating x86, ARM, CUDA
  - futhark has a much nicer language/workflow versus halide
  - futhark and halide are both functional
  - halide algorithms are completely pure, taking pixel coordinates and saying what input and output pixel coordinates to operate on
  - "not turing complete"
  - because theres no recursion in halide
  - seperate out the algorithm from the schedule
  - give fine grained control over which and what shape cores to use
  - use an explicit parser and code generator, don't piggyback on C++ like halide
- sorting for locality 
  - halide doesnt (I think) sort the input for locality
  - could morton sort multidimensional input
  - greatest common subset of functionality between GPU, CPU, CPU SIMD
    - no recursion or virtual functions because GPUs dont support it
    - no strings
- interfacing
  - just generate blobs of object code that take pointers or short arrays of ints/bytes and return the same
  - maybe generate or parse C headers... probably hard
  - not sure how to manage the blobs, uploading to the GPU, spawning the threads etc
  - just operate on blobs in memory, no need for input/output operations, and especially no string operations
- execution
  - allow complete control of when code is compiled and where code is executed 
  - compilation modes: offline, JIT, REPL
  - _where_ the code executes
    - backends LLVM IR and SPIR-V give us basically all platforms
    - SIMD (SSE, AVX, NEON, etc)
    - FPU
    - GPU
    - multiple cores, sockets, machines...
    - handle the data transfer in a default sensible way
  - would be _incredible_ for debugging experience to REPL code on the GPU
- better operators
  - first class support for
    - vectors and matrices
    - bitwise and bytewise shifts, rotates, shuffles, etc
    - all the kinds of atomics and synchronisation primitives in LLVM IR and SPIRV
  - clearer modulo and remainder differentiation
    - and allow "always positive modulo" with builtin
