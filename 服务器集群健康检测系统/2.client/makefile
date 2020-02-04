CC := gcc
SHDIR := ../1.common/
CFLAGS := -Wall -g -pthread
OBJS = client.o $(SHDIR)/common.o

client: $(OBJS)
	$(CC) -o client $(CFLAGS) $(OBJS)

client.o: client.c client.h
	$(CC) -c $(CFLAGS) -o client.o client.c

common.o: $(SHDIR)common.c $(SHDIR)common.h 
	$(CC) -c $(CFLAGS) -o $(SHDIR)common.o $(SHDIR)common.c

.PHONY: clean

clean :
	rm -f *.o client
	rm -f $(SHDIR)*o 
	echo "All cleaning!"

install :
	mkdir /opt/client 
	mkdir /opt/client/log
	mkdir /opt/client/script
	cp ./pihealthd_client.conf /opt/client/pihealthd_client.conf
	cp ./script/cpuinfo.sh /opt/client/script/cpuinfo.sh
	cp ./script/diskinfo.sh /opt/client/script/diskinfo.sh 
	cp ./script/meminfo.sh /opt/client/script/meminfo.sh
	cp ./script/osinfo.sh /opt/client/script/osinfo.sh 
	cp ./script/userinfo.sh /opt/client/script/userinfo.sh
	cp ./script/malic_process.sh /opt/client/script/malic_process.sh
	cp client /usr/bin/
	echo "Install OK!"

uninstall:
	rm -rf /opt/client/
	rm -rf /usr/bin/client
