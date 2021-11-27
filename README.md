# Return to LibC Challenge

A simple ret2libc challenge that can be hosted with docker.
This challenge has been created for the "Hacker Contest" at Hochschule Darmstadt

## Challenge description

The "magic function finder" service has a function that will print the address of printf (located in libc)  
But the service is not ready yet, so the function is never called, but here a bufferoverflow in the feedback  
should enable us to call this function, which then leaks the address of libc. This address should due to ASLR  
change every time, but we can leak it and use it to find the base address of libc at runtime. There is also  
a webserver from which you can download the binary.  
The service also mentions that it runs on ubuntu 20.04, the attacker can setup a second docker container  
with the ubuntu 20.04 image and analyze the libc version to find all offsets needed for the exploit,  
an example container Dockerfile for this has been provided in the ubuntu-test-docker directory.

## Setup

```
git clone https://code.fbi.h-da.de/istmakimm/ret2libc.git 
cd ret2libc
docker build -t ret2libc .
docker run -p 80:80 -p 1024:1024 --name ret2libc ret2libc
```

At http://localhost/ there should now be a website  
With `nc localhost 1024` you should be greeted by the challenge

## Solution

A working exploit can be found at `solution/exploit.py`.  
A more detailed step by step explanation can be found at `solution/writeup.md`  

