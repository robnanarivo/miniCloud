#!/bin/bash

# build
[ ! -d "build" ] && mkdir build
cd build && cmake .. && cmake --build . && cd ..

# start backend master
cd build/storeserver
./master_server ../../master_config.txt |& tee m_db.txt

# sleep(1)

# start backend slave
# ./tablet_server ../../tabserver_config.txt 1 |& tee t1_db.txt &
# (./tablet_server ../../tabserver_config.txt 2 |& tee t2_db.txt &)
# (./tablet_server ../../tabserver_config.txt 3 |& tee t3_db.txt &)


