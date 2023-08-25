FROM zookeeper

COPY ./apache-zookeeper-3.9.0 /apache-zookeeper-3.9.0

RUN apt update && apt install -y build-essential && apt install -y vim && apt install libssl-dev && cd /apache-zookeeper-3.9.0/zookeeper-client/zookeeper-client-c/ && make install
