FROM zookeeper

RUN apt update 
RUN apt install -y build-essential
RUN apt install -y vim
RUN apt install libssl-dev

COPY ./apache-zookeeper-3.9.0 /apache-zookeeper-3.9.0