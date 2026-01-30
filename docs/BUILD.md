How to build on Linux based on Debian
=======

Install packages:

- sudo apt-get install --quiet --assume-yes ninja-build build-essential libx11-dev libxext-dev libxrender-dev libxrandr-dev libxcursor-dev libxft-dev

git clone --recursive https://github.com/horsicq/HexViewer.git

cd HexViewer

Run build script: bash -x build_linux.sh

How to build on macOS
=======

Install packages:

- install xcode-select —install brew install cmake

git clone --recursive https://github.com/horsicq/HexViewer.git

cd HexViewer

Run build script: bash -x build_mac.sh
