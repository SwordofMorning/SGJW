PROJECT(SGJW)

CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

# ===== Setp 1 : Set Cross Compiler Path =====

SET(CMAKE_C_COMPILER aarch64-none-linux-gnu-gcc)

# ===== Setp 2 : Set Flags =====

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s -O3 -lrt")

# ===== Setp 3 : Set Application Name =====

SET(APP_NAME "sgjw")

# ===== Setp 4 : Add Subdirectory =====

ADD_SUBDIRECTORY(src bin)