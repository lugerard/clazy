# This Dockerfile creates the container for testing on Ubuntu
# You can run, for example: clazy/tests/docker/test_docker.py -b 1.6 , which will run the tests in all containers
# Or explicitly: docker run -i -t iamsergio/clazy-ubuntu-19.10-32bit sh /root/clazy/tests/docker/build-clazy.sh 1.6 -j12 none /usr/lib/llvm-9

FROM i386/ubuntu:19.10
MAINTAINER Sergio Martins (sergio.martins@kdab.com)

RUN apt-get update
RUN apt install -y g++ cmake clang-9 clang-tools-9 libclang-9-dev libclang-9-dev git-core python3 ninja-build qtbase5-dev qtdeclarative5-dev

WORKDIR /root
RUN git clone https://github.com/KDE/clazy.git
