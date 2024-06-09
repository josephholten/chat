# chat: Simple UNIX Networking Experiments

I am following 'Beej's Guide to Network Programming' to implement some simple experiments trying to do network programming.

## Build

Simple cmake build system. No external dependencies.
Should only work on UNIX systems.

Example:
```
mkdir build
cmake -B build
cmake --build build
```

## Usage

Example:
```
build/server &
build/client &
```
