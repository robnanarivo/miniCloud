#!/bin/bash

# kill master
fuser -k 8001/tcp
fuser -k 8002/tcp
fuser -k 8003/tcp

# kill slave
fuser -k 5000/tcp
fuser -k 5001/tcp
fuser -k 5002/tcp
fuser -k 5003/tcp

# kill frontend
fuser -k 8080/tcp
fuser -k 9000/tcp
fuser -k 8081/tcp
fuser -k 8082/tcp
fuser -k 8083/tcp
