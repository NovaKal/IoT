FROM ubuntu

RUN apt update
RUN apt install python3 -y
RUN apt install python3-pip -y
RUN pip3 install dash
RUN pip3 install scipy
RUN pip3 install dash_bootstrap_components
RUN pip3 install pandas

ADD ["app.py", "/home/"]

EXPOSE 80

