#! /bin/bash

docker run -it --rm -h node$1 -v /home/ivan/dist-community/config/zoo.cfg:/conf/zoo.cfg \
	-v /home/ivan/dist-community/:/project -v /home/ivan/dist-community/config/hosts:/etc/hosts \
	-p 6969:6969 us-central1-docker.pkg.dev/fast-ability-399210/petnica-docker-repo/zklibs /bin/bash
