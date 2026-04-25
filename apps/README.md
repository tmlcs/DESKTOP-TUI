# Sample Applications

This directory contains sample applications demonstrating Desktop TUI features.

## Building

Enable samples in CMakeLists.txt:
```bash
cmake -DENABLE_SAMPLES=ON ..
make
```

## Samples

### demo_hello.cpp
Simple "Hello World" application showing basic window creation.

### demo_multiwindow.cpp
Multi-window demonstration with window cycling and management.

## Usage

```bash
./sample_hello
./sample_multiwindow
```

## Features Demonstrated

- Basic window creation
- Window positioning
- Widget layout
- Keybinding handling
- Multi-window management
