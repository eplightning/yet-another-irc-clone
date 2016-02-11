TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS = common server \
    client

server.depends = common
client.depends = common
