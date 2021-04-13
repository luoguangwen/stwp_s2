CC=gcc
CFLAGS=-g  #-DLOGFILE=\"/tmp/\" 
LDFLAGS= -lpthread -ldl -lrt -luuid -lm `mysql_config --cflags --libs` #-lcrypto

OBJ_STWPD= stwp_init.o stwp_logdump.o \
    	stwp_util.o stwp_uievent.o cJSON.o \
		stwp_mysql.o stwp_p2.o stwp_multiring.o\
		stwp_md5.o stwp_user.o stwp_session.o stwp_dir_serv.o stwp_group.o


BIN_STWPD=stwp2


all:$(BIN_STWPD)

#output:elf file stwpd
$(BIN_STWPD):$(OBJ_STWPD)
	$(CC) $^ -o $@ $(LDFLAGS)
	#strip --strip-debug $@ 



%.o:%.cpp
	$(CC) -c $^ -o $@ $(CFLAGS)
.PHONY:
clean:
	rm *.o $(BIN_STWPD)  -rf
