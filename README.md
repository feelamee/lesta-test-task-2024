# lesta-test-task-2024

### Build tests dependencies
- gn
    pacman -S gn
- clang*
    pacman -S clang
- ninja
    pacman -S ninja

*you also can use gcc,\
 You can choose default toolchain in `build/BUILDCONFIG.gn`
### BUILD tests
    gn gen target/
    ninja -C target/

### RUN tests
    ./target/tests
