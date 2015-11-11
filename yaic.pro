TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS = common server \
    test_cli_client

server.depends = common
