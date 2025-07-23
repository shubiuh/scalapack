## ECOMS (OS version is too old, cann only use oneapi<=2023)
On ECOMS, load `module load intel-oneapi/2023.1.0`

then modify SLmake.inc if necessary (user can change to other mpi Fortran and C compilers)

At last, run `make lib` to build `libscalapack.a` library.

# lab-V100 (ubuntu 24.04)

On lab-v100, intel-oneapi version is 2025.1 (/home/shubin/intel/oneapi/compiler/2025.1)

modify the SLmake.inc to use `mpiifx` and `mpiicx` and use `std=c89` flag to avoid error for old C code

At last, run `make lib` to build `libscalapack.a` library.