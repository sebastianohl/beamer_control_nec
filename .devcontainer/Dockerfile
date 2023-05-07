ARG VARIANT="jammy"
FROM mcr.microsoft.com/vscode/devcontainers/base:${VARIANT}

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
    && apt-get install -y \
        git \
        python3-pip \
        gcc \
        git \
        wget \
        make \
        libncurses-dev \
        flex \
        bison \
        gperf \
        python3 \
        python3-serial \
        wget \
        cmake \
        screen \
        clang-format-14 \
    && rm -rf /var/lib/apt/lists/*  \
    && apt-get clean

RUN apt-get autoremove --purge -y

RUN mkdir -p /esp && cd esp \
    && wget -O - https://dl.espressif.com/dl/xtensa-lx106-elf-gcc8_4_0-esp-2020r3-linux-amd64.tar.gz | tar xvz
RUN cd /esp && git clone --branch v3.4 --recursive https://github.com/espressif/ESP8266_RTOS_SDK.git
RUN pip3 install -r /esp/ESP8266_RTOS_SDK/requirements.txt

RUN ln -s /usr/bin/python3 /usr/bin/python
