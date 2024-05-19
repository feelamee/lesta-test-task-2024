# lesta-test-task-2024

### Build tests dependencies
- gn (`pacman -S gn`)
- clang/gcc (`pacman -S clang`, `pacman -S gcc`)
- ninja (`pacman -S ninja`)

Choose default toolchain in `build/BUILDCONFIG.gn`
 
### BUILD tests
    gn gen target/
    ninja -C target/

### RUN tests
    ./target/tests
