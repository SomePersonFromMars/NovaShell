CFLAGS=-I$(INC_DIR)
HDEPS=$(INC_DIR)/siparse.h $(INC_DIR)/config.h siparseutils.h

CSRC=siparseutils.c

all: siparseutils.o y.tab.o lex.yy.o
	ar rcs $(INSTALL_DIR)/siparse.a siparseutils.o lex.yy.o y.tab.o 

lex.yy.o: siparse.lex y.tab.o $(HDEPS)
	lex  siparse.lex
	$(CC) $(CFLAGS) lex.yy.c -c 

y.tab.o: siparse.y  $(HDEPS)
	byacc -d siparse.y
	$(CC) $(CFLAGS) y.tab.c -c 

siparseutils.o: siparseutils.c $(HDEPS)
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f lex.yy.c y.tab.c y.tab.h siparse.a  *.o 
	rm compile_commands.json
