#!/bin/bash

sudo docker run -it --network db_default --rm mysql-bl mysql -hmysql_db --database example-database -unikita -p