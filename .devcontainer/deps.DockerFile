# [Operating System]
ARG base_image=ubuntu:22.04

FROM ${base_image} AS prereqs
SHELL ["/bin/bash", "-c"]
ARG toolchain=gcc11

# When a dialogue box would be needed during install, assume default configurations.
# Set here to avoid setting it for all install commands.
# Given as arg to make sure that this value is only set during build but not in the launched container.
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates && \
    apt-get autoremove -y --purge && apt-get clean && rm -rf /var/lib/apt/lists/*

## [Prerequisites]
# RUN apt-get update && apt-get install -y --no-install-recommends python3 && \
#     apt-get autoremove -y --purge && apt-get clean && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y locales && \
    sed -i '/en_US.UTF-8/s/^# //g' /etc/locale.gen && \
    locale-gen
ENV LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8

RUN apt-get update && apt-get install -y --no-install-recommends \
    wget ca-certificates

ARG TARGETARCH

RUN case "$TARGETARCH" in \
      amd64) CMAKE_ARCH="x86_64" ;; \
      arm64) CMAKE_ARCH="aarch64" ;; \
      *) echo "Unsupported TARGETARCH: $TARGETARCH" >&2; exit 1 ;; \
    esac  && \
    wget https://github.com/Kitware/CMake/releases/download/v3.29.0/cmake-3.29.0-linux-${CMAKE_ARCH}.tar.gz && \
    tar -xzf cmake-3.29.0-linux-${CMAKE_ARCH}.tar.gz -C /opt && \
    ln -sf /opt/cmake-3.29.0-linux-${CMAKE_ARCH}/bin/cmake /usr/local/bin/cmake && \
    ln -sf /opt/cmake-3.29.0-linux-${CMAKE_ARCH}/bin/ctest /usr/local/bin/ctest && \
    rm cmake-3.29.0-linux-${CMAKE_ARCH}.tar.gz

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      curl \
      ca-certificates \
      libssl-dev \
      zlib1g-dev \
      libbz2-dev \
      libreadline-dev \
      libsqlite3-dev \
      libncursesw5-dev \
      xz-utils \
      tk-dev \
      libxml2-dev \
      libxmlsec1-dev \
      libffi-dev \
      libzstd-dev \
      zstd \
      clangd \
      liblzma-dev

RUN apt-get update && \
    apt-get install -y \
    build-essential \
    libz3-dev \
    git \
    make \
    libtinfo-dev \
    lld \
    jq \
    gcc-11 \
    g++-11 \
    python3 \
    python3-pip \
    ninja-build \
    openssh-client \
    openssl \
    pkg-config \
    libeigen3-dev \
    libboost-program-options-dev \
    nano \
    vim \
    libzip-dev && \
    # Clean up cache to reduce image size
    rm -rf /var/lib/apt/lists/*

RUN python3 -m pip install lit

RUN apt-get update && apt-get install -y --no-install-recommends \
    locales
RUN python3 -m pip install sphinx myst-parser furo

RUN git config --global gc.auto 0


# 1. Same pinned LLVM/MLIR as FetchLLVM.cmake (MQSS_LLVM_VERSION)
RUN curl -LsSf https://github.com/munich-quantum-software/setup-mlir/releases/latest/download/setup-mlir.sh \
    | bash -s -- -v 22.1.0 -p /opt/deps/llvm

# 2. CUDA-Q, same clone/patch/configure/build/install steps as FindCUDAQ.cmake
RUN git clone --depth 1 --branch 0.15.0 https://github.com/NVIDIA/cuda-quantum.git /src/cudaq && \
    sed -i 's/add_subdirectory(nvqpp)/# patched out/' /src/cudaq/tools/CMakeLists.txt && \
    cmake -G Ninja -S /src/cudaq -B /build/cudaq \
      -DCMAKE_INSTALL_PREFIX=/opt/deps/cudaq -DCMAKE_BUILD_TYPE=Release \
      -DLLVM_DIR=/opt/deps/llvm/lib/cmake/llvm -DMLIR_DIR=/opt/deps/llvm/lib/cmake/mlir \
      -DCUDAQ_ENABLE_PYTHON=OFF -DCUDAQ_BUILD_TESTS=OFF -DCUDAQ_DISABLE_RUNTIME=ON \
      -DCMAKE_DISABLE_FIND_PACKAGE_Clang=TRUE && \
    cmake --build /build/cudaq --target QuakeDialect CCDialect QECDialect OptimBuilder OptCodeGen && \
    cmake --install /build/cudaq --component QuakeDialect && CCDialect && QECDialect && OptimBuilder && OptCodeGen

RUN git clone --depth 1 --branch v0.15.0 https://github.com/PennyLaneAI/catalyst.git /src/catalyst && \
    sed -i 's/add_subdirectory(Driver)/# patched out/' /src/catalyst/mlir/lib/CMakeLists.txt && \
    cmake -G Ninja -S /src/catalyst -B /build/catalyst \
      -DCMAKE_INSTALL_PREFIX=/opt/deps/catalyst -DCMAKE_BUILD_TYPE=Release \
      -DLLVM_DIR=/opt/deps/llvm/lib/cmake/llvm -DMLIR_DIR=/opt/deps/llvm/lib/cmake/mlir && \
    cmake --build /build/catalyst --target MLIRMBQC MLIRQRef MLIRQuantum

FROM scratch AS deps
COPY --from=builder /opt/deps /opt/deps
