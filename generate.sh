rm ./build-arm32/* -rf
rm ./build/* -rf
cmake -S . -B build-arm32 -DBUILD_FOR_ARM32=ON -DBUILD_FOR_AMD64=OFF
cmake -S . -B build -DBUILD_FOR_ARM32=OFF -DBUILD_FOR_AMD64=ON