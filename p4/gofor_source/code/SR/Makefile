SRCDIR=src
OBJDIR=obj
INCLUDE_PATH=include

SRC := $(wildcard $(SRCDIR)/*.c)
HEADERS := $(wildcard $(INCLUDE_PATH)/*.h)
OBJ  = $(SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

PG := #-pg
FLAGS := -Wall -Wextra -Werror -O3
#$(PG)
CC ?= gcc

LIBS :=

vertclair=\033[1;32m
neutre=\033[0;m
cyanfonce=\033[0;36m

info="[${vertclair}INFO${neutre}]"
eval="[${cyanfonce}EVAL${neutre}]"

GBLX=glibx-connex
#name_of_gblx
FRANCOIS=sprint-modif-zipf.isp
WORST_SPRINT=sprint-modif-zipf.isp
#sprint-modif70.isp
REALISTIC_SPRINT=geoSprint
#sprint-modif70.isp
#sprint-real.isp

USE_CASE?=USE_CASE_DCLC

all : objects samcra

unitFlex : $(OBJ) unitFlex.o
	$(CC) $(FLAGS) -o $@ $^ $(LIBS)
	@echo ${info} "Test unit flex succesfully compiled"

unitFlex.o: test_unit_flex.c
	$(CC) $(FLAGS) -Iinclude -o $@ -c $^ $(LIBS)

unitBinHeap : $(OBJ) $(OBJDIR)/BinHeap.o
	$(CC) $(FLAGS) -o $@ $^ $(LIBS)
	@echo ${info} "Test unit bin heap succesfully compiled"

BinHeap.o: test_unit_binheap.c
	$(CC) $(FLAGS) -Iinclude -o $@ -c $^ $(LIBS)

samcra : $(OBJ) main.o
	$(CC) $(FLAGS) -o $@ $^ $(LIBS)
	@echo ${info} "Samcra succesfully compiled"

samcraLeastDelay : clean-objects
	USE_CASE=USE_CASE_LEAST_DELAY make objects
	$(CC) $(FLAGS) -o $@ $(OBJ) main.o $(LIBS)
	@echo ${info} "Samcra succesfully compiled"

samcraDCLC : clean-objects
	USE_CASE=USE_CASE_DCLC make objects
	$(CC) $(FLAGS) -o $@ $(OBJ) main.o $(LIBS)
	@echo ${info} "Samcra succesfully compiled"

samcraFRR : clean-objects
	USE_CASE=USE_CASE_FRR make objects
	$(CC) $(FLAGS) -o $@ $(OBJ) main.o $(LIBS)
	@echo ${info} "Samcra succesfully compiled"


main.o: main.c
	$(CC) $(FLAGS) -D$(USE_CASE) -Iinclude -o $@ -c $^ $(LIBS)

$(OBJDIR)/%.o : $(SRCDIR)/%.c $(HEADERS)
	@mkdir -p obj
	$(CC) $(FLAGS) -D$(USE_CASE) -Iinclude -o $@ -c $<  $(LIBS)


objects: $(OBJ) main.o


clean-objects: 
	@rm -rf obj/*.o
	@rm -rf main.o
	@rm -rf eval.o
	@rm -rf unitFlex.o
	@rm -rf BinHeap.o
	@rm -rf genRandom.o
	@rm -rf randomTopo.o
	@rm -rf randomSpread.o

clean:
	@rm -rf obj/*.o
	@rm -rf samcra
	@rm -rf samcraLeastDelay
	@rm -rf samcraDCLC
	@rm -rf samcraFRR
	@rm -rf main.o
	@rm -rf eval
	@rm -rf eval.o
	@rm -rf gmon.out
	@rm -rf profiler.txt
	@rm -rf unitFlex
	@rm -rf unitFlex.o
	@rm -rf unitBinHeap
	@rm -rf BinHeap.o
	@rm -rf genRandom.o
	@rm -rf genRandom
	@rm -rf randomTopo.o
	@rm -rf randomTopo
	@rm -rf randomSpread.o
	@rm -rf randomSpread
	@echo ${info} "Main program clean succesfully done"


.PHONY: clean-objects clean
