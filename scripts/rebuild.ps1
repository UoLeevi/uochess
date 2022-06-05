rm -r build
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local/uochess -DCMAKE_BUILD_TYPE=Debug -G "MinGW Makefiles"
cmake --build . --target install
