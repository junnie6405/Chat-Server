 # Chat Server 
>This project demonstrates the usage of socket programming. 
>The server can accept clients through the listening sockets and the clients can talk
>to other clients on the same server.  

## Table of Contents
* [General Info](#general-information)
* [Technologies Used](#technologies-used)
* [Features](#features)
* [Setup](#setup)
* [Usage](#usage)
* [Room for Improvement](#room-for-improvement)


## General Information
The purpose of this program is to solidify my understanding of a Server-Client World. 
By the end of this project, I was able to fully grasp how an interprocess communication works on Linux. 


## Technologies Used
This program is written in C on Linux environment. A vim editor was used.  


## Features
1. The server can support more than multiple clients at the same time. Thus, this program supports the functionality of a group chat. 
2. This program supports nonblocking, which means a client does not need to wait for other clients to support. 
3.Poll function was used to monitor multiple clients and get their data effectively

## Setup
There are two files in this proejct: client.c and server.c. Make executable files by running MAKE command and run the server file first. Make sure to specify the port number. Then, run the executable client file with the same port number included as the argument. 


## Room for Improvement
1. I could add a feature to let users choose their own names in the chat room. 
2. When the number of clients' connections grow, the server should not use poll because it could be slow. In that case, I could use a libevent library to make a program that works even on a larger scale. 
