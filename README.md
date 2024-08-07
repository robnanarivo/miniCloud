# Overview
This project intends to implement a small cloud platform that mimics the functionality of Google Apps, but only with email and storage service features. 

Our implementation is fully distributed, load balanced, and fault tolerant. It also supports recovery when a previously failed node is back online.

# Design
- Frontend: We have a load balancer that distributes traffic among several frontend servers, and each server is connected with all the backend servers (key-value store). The frontend servers will serve a web interface for users to interact with our platform.

- Backend: The backend functions as a key-value store, similar to Google Bigtable. All the data (user profiles, files, and emails) are stored here. An API interface layer is added on the key-value store for simpler interaction with the frontend. 

# How to Run
Run `start_service.sh` to start the cloud platform.
Run `stop_service.sh` to stop the cloud platfor
