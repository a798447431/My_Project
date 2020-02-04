CC := gcc
SHDIR := ../1.common/
CFLAGS := -Wall -g -pthread
OBJS = master.o $(SHDIR)/common.o

master: $(OBJS)
	$(CC) -o master $(CFLAGS) $(OBJS)

master.o: master.c master.h
	$(CC) -c $(CFLAGS) -o master.o master.c

common.o: $(SHDIR)common.c $(SHDIR)common.h
	$(CC) -c $(CFLAGS) -o $(SHDIR)common.o $(SHDIR)common.c

.PHONY: clean

clean :
	rm -f *o master
	rm -f $(SHDIR)*o
	echo "All clean!"

install :
	mkdir /opt/master 
	mkdir /opt/master/log
	cp pihealthd_master.conf /opt/master
	cp master /usr/bin/ 
	echo "Install success!"

uninstall :
	rm -r /opt/master/
	rm /usr/bin/master
	

