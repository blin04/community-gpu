FROM zookeeper

RUN apt update && apt install -y libzookeeper-mt-dev
RUN apt install -y build-essential
RUN apt install -y vim