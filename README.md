# lesta-test-task-2024

### BUILD

#### BUILD dependencies
- conan (`paru -S conan`)

#### SETUP environment & configure
    git clone https://github.com/feelamee/lesta-test-task-2024
    cd lesta-test-task-2024
    conan install . --build=missing
    source build/[build type]/generators/conanbuild.sh
    cmake --config conan-[build type]

[See](https://superuser.com/questions/826333/is-there-a-way-to-source-a-sh-script-from-the-fish-shell) how to source `sh` if you use `fish`

#### BUILD tests
    cmake --build build/[build type] --target tests -j

#### RUN tests
    ./build/[build type]/tests/tests

Now there is issues with consume/produce test of `tt::lock_free_ringbuf`.
So, this is okay, if it will fail. 

#### BUILD benchmarks
    cmake --build build/[build type] --target bench-sort -j

#### RUN benchmarks
    ./build/[build type]/bench/bench-sort
use `--help` for options

#### PLOT graph of benchmarks
    TODO
