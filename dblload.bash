#!/bin/bash

binlog_path_command=$(sudo docker exec -it dbms_container bash -c "find /var/log/mysql -type f -name 'mysql-bin.*' | grep -E 'mysql-bin.[0-9]+$' | sort | tail -n 1")
binlog_path_command="${binlog_path_command%?}" # remove last '\r' symbol. How it located there? I don't know

file_name="$(echo $binlog_path_command | rev | cut -d '/' -f1 | rev)"

container_id_command=$(sudo docker container ls | tail -n +2 | grep 'dbms_image' | cut -d ' ' -f 1)

final_command="sudo docker exec -i $container_id_command cat $binlog_path_command > ./static/binlog/$file_name"

echo $final_command
eval "$final_command"
