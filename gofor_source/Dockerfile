
FROM ubuntu:22.04

RUN apt-get update \
    && apt-get install -y python3.9 python3-pip g++-11 make \
    && rm -rf /var/lib/apt/lists/* \
    && apt clean

COPY evaluation/requirements.txt .

RUN pip install -r requirements.txt

#On windows, if you want to run the container without mounting a volume, you need to copy the files into the container
#so you have to uncomment the following line
#COPY . /home/user

EXPOSE 8888
VOLUME [ "/home/user" ]
WORKDIR /home/user/evaluation

CMD [ "jupyter", "notebook", "--allow-root", "--no-browser", "--ip", "0.0.0.0"]