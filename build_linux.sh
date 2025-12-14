#!/bin/bash

echo "=== HexViewer Build Script ==="

if ! command -v cmake &>/dev/null; then
    echo "CMake not found. Installing..."
    sudo apt update
    sudo apt install -y cmake
else
    cmake_ver=$(cmake --version | head -n1 | awk '{print $3}')
    echo "CMake found: version $cmake_ver"
fi

CXX=""
if command -v g++ &>/dev/null; then
    CXX="g++"
elif command -v clang++ &>/dev/null; then
    CXX="clang++"
fi

if [ -n "$CXX" ]; then
    ver=$($CXX -dumpversion)
    echo "Compiler found: $CXX version $ver"
    echo "int main() { return 0; }" > test.cpp
    if $CXX -std=c++17 test.cpp -o test.out &>/dev/null; then
        echo "C++17 supported"
    else
        echo "$CXX does not support C++17. Installing g++-9..."
        sudo apt install -y g++-9
        CXX="g++-9"
    fi
    rm -f test.cpp test.out
else
    echo "No C++ compiler found. Installing g++..."
    sudo apt update
    sudo apt install -y g++
    CXX="g++"
fi

for pkg in libx11-dev make pkg-config; do
    dpkg -s $pkg &>/dev/null
    if [ $? -eq 0 ]; then
        echo "$pkg found"
    else
        echo "$pkg not found. Installing..."
        sudo apt install -y $pkg
    fi
done

dirs=("HexViewer/src" "HexViewer/include")
for d in "${dirs[@]}"; do
    if [ -d "$d" ]; then
        echo "Directory $d exists"
    else
        echo "Directory $d not found"
    fi
done

if [ ! -d build ]; then
    mkdir build
fi
cd build

echo "Running cmake .."
cmake ..

echo "Building HexViewer..."
make -j$(nproc)

echo "=== Build complete ==="

