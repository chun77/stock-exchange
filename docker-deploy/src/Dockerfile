FROM ubuntu:latest

WORKDIR /app
COPY . /app

RUN apt-get update && apt-get install -y \
    apt-utils \
    build-essential \
    libtinyxml2-dev \
    libpqxx-dev

RUN make

EXPOSE 12345

CMD ["./main"]
