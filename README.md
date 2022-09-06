
## Build with CMake

```
mkdir build
cmake -B build
cmake --build build
```

## Run frag decoder

```
./build/frag_decoder ../helloworld-stm32wl_with_fragments.signed.bin ../firmware_decoded.bin 61 1 5
```

## Compare results

diff -y <(xxd ../helloworld-stm32wl.signed.bin) <(xxd firmware_decoded.bin) | less
