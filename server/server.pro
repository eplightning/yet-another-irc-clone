TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS = common master slave

master.depends = common
