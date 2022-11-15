++ = g++

OBJ := command.o pipe.o process.o build-in_command.o np_simple.o np_single_proc.o system_broadcast.o shm.o  np_multi_proc.o
np_simple_OBJ := command.o pipe.o process.o build-in_command.o np_simple.o
np_single_OBJ := command.o pipe.o process.o build-in_command.o np_single_proc.o
np_multi_OBJ  := system_broadcast.o shm.o command.o pipe.o process.o build-in_command.o np_multi_proc.o
np_simple_exe := np_simple
np_single_exe := np_single_proc
np_multi_exe  := np_multi_proc
CFLAGS=-O1 -g -Wall

all: $(np_simple_exe) $(np_single_exe) $(np_multi_exe)

%.o:%.cpp
	$(++) -c $(CFLAGS) $< -o $@

$(np_simple_exe):$(np_simple_OBJ)
	$(++) -o $(np_simple_exe) $^

$(np_single_exe):$(np_single_OBJ)
	$(++) -o $(np_single_exe) $^

$(np_multi_exe):$(np_multi_OBJ)
	$(++) -o $(np_multi_exe) $^

.PHONY:clean

clean:
	rm -rf $(OBJ) $(np_simple_exe) $(np_single_exe) $(np_multi_exe)