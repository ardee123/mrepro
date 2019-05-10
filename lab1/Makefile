BOT=bot
UDP=UDP_server

SOURCE_UDP=udp.c

SOURCE_BOT=bot.c
HEADERS_BOT=bot.h

CFLAGS=-Wall -g

OBJECTS_UDP=${SOURCE_UDP:.c=.o}
OBJECTS_BOT=${SOURCE_BOT:.c=.o}

$(UDP) $(BOT): $(OBJECTS_UDP) $(OBJECTS_BOT)
	$(CC) $(OBJECTS_UDP) -o $(UDP)
	$(CC) $(OBJECTS_BOT) -o $(BOT)
	
	
$(OBJECTS_UDP): $(HEADERS_UDP)
$(OBJECTS_BOT): $(HEADERS_BOT)

clean:
	-rm -f $(UDP) $(BOT) $(OBJECTS_UDP) $(OBJECTS_BOT) udp.o* *.core
