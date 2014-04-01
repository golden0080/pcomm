CC=g++
SOCK_FLAGS=-lnsl
DEBUG_FLAGS=
EXECUTABLES=pcomm
all:
	make pcomm
pcomm: pcomm_main.o my_inet_utils.o
	$(CC) $(DEBUG_FLAGS) -o $@ $^ $(SOCK_FLAGS)
%.o: %.cc
	$(CC) -c $(DEBUG_FLAGS) -o $@ $^ $(SOCK_FLAGS)
tool:
	make pcomm
clean:
	rm -f *.o *~ $(EXECUTABLES)
