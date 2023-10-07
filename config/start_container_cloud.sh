#! /bin/bash

docker run -d --rm --network=host -h node$1 --name node$1 -v /home/ivan/dist-community/config/zoo.cfg:/conf/zoo.cfg \
        -v /home/ivan/dist-community/system:/project -v /home/ivan/dist-community/config/hosts:/etc/hosts \
        -v /home/ivan/dist-community/config/myid:/data/myid -v /home/ivan/dist-community/dataset:/graph \
		us-central1-docker.pkg.dev/fast-ability-399210/petnica-docker-repo/zklibs

