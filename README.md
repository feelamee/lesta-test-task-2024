# lesta-test-task-2024

### Build tests dependencies
- gn (`pacman -S gn`)
- clang* (`pacman -S clang`)
- ninja (`pacman -S ninja`)

*you can also use gcc.\
 Just choose default toolchain in `build/BUILDCONFIG.gn`
 
### BUILD tests
    gn gen target/
    ninja -C target/

### RUN tests
    ./target/tests
