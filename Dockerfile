FROM debian
run apt update
RUN apt-get -y install apt-utils
RUN apt-get -y install bash
RUN apt-get -y install autoconf automake m4 libtool build-essential
RUN apt-get -y install libSDL1.2-dev libSDL1.2
RUN apt-get -y install git

RUN git clone https://github.com/ukscone/marklomas_chuckie-egg.git
WORKDIR marklomas_chuckie-egg
RUN ./build.sh
ENTRYPOINT ["./chuckie-egg"]  

