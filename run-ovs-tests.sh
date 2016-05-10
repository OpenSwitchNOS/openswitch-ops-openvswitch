#!/bin/bash
# Copyright (C) 2016 Hewlett-Packard Development Company, L.P.
# All Rights Reserved.
#

OVSTESTDIR="/ovstest"
DOCKERIMAGE="openswitch/ubuntudev"
JEMALLOC=0
YAML=0
JEMALLOC=$(ldconfig -p | grep -c jemalloc)
YAML=$(ldconfig -p | grep -c yaml)
if [ $1 ]; then
    YAML=0
fi

if [ ${JEMALLOC} -gt 0 ] && [ ${YAML} -gt 0 ]; then
    echo "Running on local host"
    ./ovs-test.sh;
else
    echo "One of jemalloc or yaml libraries not found, running on docker"
    echo "Loading docker"
# If docker image is not available, it will be downloaded from openswitch docker hub.
if [ $(docker images | grep -ic $DOCKERIMAGE) -le 1 ]; then
        docker pull $DOCKERIMAGE
        if [ $(echo $?) -eq 1 ]; then
            echo "Docker image not found"
            exit 1;
        fi
        until [ $(docker images | grep -ic $DOCKERIMAGE) -eq 1 ]; do
           sleep 0.1;
        done;
    fi
    docker run --rm -v $(pwd):${OVSTESTDIR} --name test ${DOCKERIMAGE} &
    sleep 2
    echo "Waiting docker to be ready"
    until [ "`/usr/bin/docker inspect -f {{.State.Running}} test`"=="true" ]; do
        sleep 0.1;
    done;
    echo "Configuring options in docker"
    docker exec -it test groupadd -g $(id -g ${USER}) $(stat -c "%G" .)
    docker exec -it test useradd  -u $(id -u ${USER}) ${USER}
    echo "Running tests in docker"
    docker exec -ti test /bin/sh -c "cd ${OVSTESTDIR} && ./ovs-test.sh"
    docker stop test
fi
