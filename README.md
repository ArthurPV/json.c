# json.c

`json.c` is a [JSON](https://en.wikipedia.org/wiki/JSON) parser written in C, with `libc` as its only library dependency.

## Dependencies

- [CMake](https://cmake.org/)
- C compiler e.g. [GCC](https://gcc.gnu.org/), [Clang](https://clang.llvm.org/)
- Build system generator e.g. [Ninja](https://ninja-build.org/), [make](https://www.gnu.org/software/make/)

## Build

```sh
# From the root of the repository
mkdir -p build
cd build
cmake .. -G Ninja
ninja
```

## Example

Checkout the file [test.c](./test.c) for an example.

## References

- [RFC 8259](https://datatracker.ietf.org/doc/html/rfc8259) 
