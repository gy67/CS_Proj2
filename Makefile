# define C compiler & flags
CC = gcc
CFLAGS = -Wall -g 
# define libraries to be linked (for example -lm)
LDLIBS = 

# define sets of header source files and object files
SRC = main.c commands.c mailsOperation.c network.c utils.c

# OBJ is the same as SRC, just replace .c with .h
OBJ = $(SRC:.c=.o)
 
# define the executable name(s)
EXE = fetchmail

# the first target:
$(EXE): $(OBJ) 
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDLIBS) -lm

clean:
	rm -f $(OBJ) $(EXE)