TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS = common server

server.depends = common
