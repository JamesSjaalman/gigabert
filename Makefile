## Makefile
#CFLAGS=-Wall -pg -g3 -I /opt/postgres/include -L /opt/postgres/lib -lpq
CFLAGS=-Wall -pg -g3 -I/opt/postgres/include
LDFLAGS=-L/opt/postgres/lib
LIBS=pq
OBJS=pgstuff.o

all: hubertc # gigahal

clean:
	@rm hubertc.o gigahal.o $(OBJS) hubertc gigahal

hubertc: hubertc.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $< $(OBJS) -l$(LIBS)

#gigahal: gigahal.o $(OBJS)
#	$(CC) $(LDFLAGS) -o $@ $< $(OBJS) -l$(LIBS)

hubertc.o: hubertc.c

#gigahal.o: gigahal.c

pgstuff.o: pgstuff.c

#### Eof
