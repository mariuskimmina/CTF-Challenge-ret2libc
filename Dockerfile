FROM ubuntu:20.04

RUN apt-get update
RUN apt-get install nginx -y

RUN useradd -d /home/ctf/ -m -p ctf -s /bin/bash ctf
RUN echo "ctf:ctf" | chpasswd

WORKDIR /home/ctf

COPY challenge/magic_function_finder .
COPY challenge/flag .
COPY challenge/ynetd .
COPY start.sh .

# Provide the user with everything they need
COPY challenge/index.html /var/www/html
COPY challenge/magic_function_finder /var/www/html

RUN chmod -R 777 /var/log/nginx
RUN chmod -R 777 /var/lib/nginx
RUN touch /run/nginx.pid
RUN chmod 777 /run/nginx.pid
RUN chmod +x start.sh


RUN chown -R root:root /home/ctf

USER ctf
EXPOSE 1024
EXPOSE 80
CMD ./start.sh
