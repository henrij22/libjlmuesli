# SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
# SPDX-License-Identifier: MIT

# Use Ubuntu 24.04 as the base image
FROM ubuntu:24.04

LABEL maintainer=jakob@ibb.uni-stuttgart.de

# Install system packages, including build-essential, cmake, gcc, clang, and julia
RUN apt-get update && apt-get install -y \
    build-essential \
    python3 \
    python3-pip \
    cmake \
    gcc \
    clang \
    curl \
    git \
    wget \
    apt-utils \
    gfortran \
    gdb  \
    nano \
    htop \
    libopenblas-dev \
    ninja-build \
    # clang-format \
    # cmake-format \
    && rm -rf /var/lib/apt/lists/*

RUN  update-alternatives --install /usr/bin/python python /usr/bin/python3.11 11

RUN  python -m pip install --upgrade pip --break-system-packages && \
    python -m pip install --no-cache-dir --break-system-packages \
    cmakelang==0.6.13 \
    clang-format==18.1.8 \

RUN git clone --depth=1 https://gitlab.com/libeigen/eigen.git && \
    cd eigen && \
    mkdir build && \
    cd build && \
    cmake ../ && \
    make -j"$(nproc)" install   && \
    cd ~ && \
    rm -rf eigen

RUN curl -fsSL https://install.julialang.org | sh -s -- -y

ENV PATH=/root/.juliaup/bin:${PATH}

RUN julia -e 'using Pkg; Pkg.add("CxxWrap"); import CxxWrap; open("/tmp/prefix.txt", "w") do f write(f, CxxWrap.prefix_path()) end'


RUN git clone https://bitbucket.org/ignromero/muesli.git && \
    cd muesli && \
    git checkout 27e8204971602cb042d633b8b5f87761272b10df
COPY patches/CMakeLists.txt /muesli
COPY patches/MuesliConfig.cmake.in /muesli
RUN cd muesli && \
    mkdir build &&  cd build && \
    cmake .. && \
    cmake --build . --target install --parallel "$(nproc)" 


# -l makes it a "login" shell
SHELL ["/bin/bash", "-l", "-c"]  
