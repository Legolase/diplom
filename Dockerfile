FROM ubuntu:noble

RUN apt update && \
    apt upgrade -y && \
    apt install -y \
        build-essential \
        ninja-build \
        python3-pip \
        python3-venv \
        python3-dev curl gnupg apt-transport-https \
        zlib1g && \
    apt clean && \
    rm -rf /var/lib/apt/lists/*

RUN pip3 install --no-cache-dir --break-system-packages conan==2.15.0 pytest==6.2.5 cmake && \
    conan profile detect --force && \
    conan remote add otterbrix http://conan.otterbrix.com

WORKDIR /app

COPY ./conanfile.txt ./conanfile.txt
RUN conan install . --output-folder= --build=missing

COPY ./include ./include
COPY ./src ./src
COPY ./CMakeLists.txt ./CMakeLists.txt
COPY ./CMakeUserPresets.json ./CMakeUserPresets.json


WORKDIR /app/build/Release

RUN cmake -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=generators/conan_toolchain.cmake -S/app -B/app/build/Release -DCMAKE_BUILD_TYPE=Release
RUN cmake --build . --parallel $(nproc) --target diplom

# ENTRYPOINT ["/app/build/Release/diplom"]