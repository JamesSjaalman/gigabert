## Makefile
#CFLAGS=-Wall -pg -g3 -I /opt/postgres/include -L /opt/postgres/lib -lpq
CFLAGS=-Wall -pg -g3 -I/opt/postgres/include
LDFLAGS=-L/opt/postgres/lib
LIBS=pq
OBJS=pgstuff.o

all: hubertc # gigahal

clean:
	@rm hubertc.o gigahal.o $(OBJS) hubertc gigahal

test4: hubertc
	./hubertc -m 4 0 < data/test2.dat

test7: hubertc
	./hubertc -m 7 0 < data/test2.dat

hubertc: hubertc.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $< $(OBJS) -l$(LIBS)

hubertc.o: hubertc.c

pgstuff.o: pgstuff.c

#gigahal: gigahal.o $(OBJS)
#	$(CC) $(LDFLAGS) -o $@ $< $(OBJS) -l$(LIBS)

#gigahal.o: gigahal.c

#### Eof
