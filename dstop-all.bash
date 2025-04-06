#!/bin/bash

container_list=$(sudo docker container list --all | tail -n +2 | cut -d ' ' -f1)

sudo docker container stop $container_list
sudo docker container rm $container_list