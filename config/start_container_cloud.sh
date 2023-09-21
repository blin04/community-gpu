#! /bin/bash

docker run -d --network=host -h node$1 --name node$1 -v /home/ivan/dist-community/config/zoo.cfg:/conf/zoo.cfg \
        -v /home/ivan/dist-community/:/project -v /home/ivan/dist-community/config/hosts:/etc/hosts \
        -v /home/ivan/dist-community/config/myid:/data/myid -v /home/ivan/dist-community/dataset/edges:/graph/edges \
		-v /home/ivan/dist-community/dataset/nodes:/graph/nodes us-central1-docker.pkg.dev/fast-ability-399210/petnica-docker-repo/zklibs

