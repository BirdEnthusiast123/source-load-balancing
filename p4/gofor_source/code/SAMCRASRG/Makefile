SRCDIR=src
OBJDIR= obj
SRC := $(wildcard $(SRCDIR)/*.c)
OBJ  = $(SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
#PG := -pg
#FLAGS := -Wall -Wextra -Werror -O3
#$(PG)


CC ?= gcc

# UNAME_S := $(shell uname -s)
# ifeq ($(UNAME_S),Darwin)
# 	OMPFLAGS := -L/usr/local/lib/ -lomp
# endif

#CC ?= /usr/local/opt/llvm/bin/clang

## for M1
LIBS := $(OMPFLAGS) -lm
FLAGS := -Wall -Wextra -Werror -O3 -Xpreprocessor -fopenmp -I/opt/homebrew/include -Iinclude

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



all : objects samcra



scriptCstr : $(OBJ) scriptCstr.o
	$(CC) $(FLAGS) -o $@ $^ $(LIBS)
	@echo ${info} "Constraint script succesfully compiled"

scriptCstr.o: scriptCstr.c
	$(CC) $(FLAGS) -Iinclude -o $@ -c $^ $(LIBS)

worstSprint : $(OBJ) worstSprint.o
	$(CC) $(FLAGS) -o $@ $^ $(LIBS)
	@echo ${info} "Worst sprint succesfully compiled"

worstSprint.o: worstSprint.c
	$(CC) $(FLAGS) -Iinclude -o $@ -c $^ $(LIBS)

genRandom : $(OBJ) genRandom.o
	$(CC) $(FLAGS) -o $@ $^ $(LIBS)
	@echo ${info} "Worst sprint succesfully compiled"

genRandom.o: generate_random.c
	$(CC) $(FLAGS) -Iinclude -o $@ -c $^ $(LIBS)

unitFlex : $(OBJ) unitFlex.o
	$(CC) $(FLAGS) -o $@ $^ $(LIBS)
	@echo ${info} "Test unit flex succesfully compiled"

unitFlex.o: test_unit_flex.c
	$(CC) $(FLAGS) -Iinclude -o $@ -c $^ $(LIBS)

unitBinHeap : $(OBJ) BinHeap.o
	$(CC) $(FLAGS) -o $@ $^ $(LIBS)
	@echo ${info} "Test unit bin heap succesfully compiled"

BinHeap.o: test_unit_binheap.c
	$(CC) $(FLAGS) -Iinclude -o $@ -c $^ $(LIBS)

worstRandom : $(OBJ) worstRandom.o
	$(CC) $(FLAGS) -o $@ $^ $(LIBS)
	@echo ${info} "Worst random succesfully compiled"

worstRandom.o: worstRandom.c
	$(CC) $(FLAGS) -Iinclude -o $@ -c $^ $(LIBS)

randomTopo : $(OBJ) randomTopo.o
	$(CC) $(FLAGS) -o $@ $^ $(LIBS)
	@echo ${info} "randomTopo succesfully compiled"

randomTopo.o: randomTopo.c
	$(CC) $(FLAGS) -Iinclude -o $@ -c $^ $(LIBS)

randomSpread : $(OBJ) randomSpread.o
	$(CC) $(FLAGS) -o $@ $^ $(LIBS)
	@echo ${info} "randomSpread succesfully compiled"

randomSpread.o: randomSpread.c
	$(CC) $(FLAGS) -Iinclude -o $@ -c $^ $(LIBS)

crashTest : clean objects eval

eval : $(OBJ) eval.o
	$(CC) $(FLAGS) -o $@ $^ $(LIBS)
	@echo ${info} "Crash test succesfully compiled"

eval.o: eval.c
	$(CC) $(FLAGS) -Iinclude -o $@ -c $^ $(LIBS)

samcra : $(OBJ) main.o
	$(CC) $(FLAGS) -o $@ $^ $(LIBS)
	@echo ${info} "Best2cop succesfully compiled"

%.o: %.c
	$(CC) $(FLAGS) -o $@ -c $^

$(OBJDIR)/%.o : $(SRCDIR)/%.c
	@mkdir -p obj
	$(CC) $(FLAGS) -o $@ -c $^


objects:
	@mkdir -p obj

clean:
	@rm -rf obj/*.o
	@rm -rf samcra
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


ct_nodesdirs:
	@mkdir -p CT_NODES

CT_NODES: crashTest ct_nodesdirs
	@echo ${eval} "Start eval for Ct_nodes"
	@echo "NB_NODES NB_PL NB_THREADS TIME" > CT_NODES/results.txt
	@for i in 100 200 300 400 500 600 700 800 900 1000 ; do \
		./eval $$i 2 1000 1 >> CT_NODES/results.txt ; \
		./eval $$i 2 1000 4 >> CT_NODES/results.txt ; \
		./eval $$i 2 1000 8 >> CT_NODES/results.txt ; \
		./eval $$i 2 1000 12 >> CT_NODES/results.txt ; \
	done;
	@echo ${eval} "Eval for Ct_nodes completed"

ct_nodesclean:
	@rm -rf CT_NODES
	@rm -rf eval
	@rm -rf eval.o


ct_threadsdirs:
	@mkdir -p CT_THREADS

CT_THREADS: crashTest ct_threadsdirs
	@echo ${eval} "Start eval for Ct_threads"
	@echo "NB_NODES NB_PL NB_THREADS TIME" > CT_THREADS/results.txt
	@for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 ; do \
		./eval 1000 2 1000 $$i >> CT_THREADS/results.txt ; \
	done;
	@echo ${eval} "Eval for Ct_threads completed"

ct_threadsclean:
	@rm -rf CT_THREADS
	@rm -rf eval
	@rm -rf eval.o

rnddirs:
	@for i in 100 500 1000 ; do \
		for j in $$(seq 1 10) ; do \
			mkdir -p RND/resultsSpread$$i/resultsTime$$(($$j * 100)) ; \
		done; \
	done;

rnddirsGen:
	@for i in 100 500 1000 ; do \
		for j in $$(seq 1 10) ; do \
			mkdir -p RND/resultsSpread$$i/topo$$(($$j * 100)) ; \
		done; \
	done;

RND: rnddirs randomTopo
	@echo ${eval} "Start eval for RND"
	@./randomTopo 100
	@./randomTopo 500
	@./randomTopo 1000
	@echo ${eval} "Eval for RND completed"

rndclean:
	@rm -rf RND
	@rm -rf randomTopo
	@rm -rf randomTopo.o

rltdirs:
	@mkdir -p RLT

RLT: best2cop rltdirs
	@echo ${eval} "Start eval for RLT"
	@./best2cop --file topos/${WORST_SPRINT} --topo --labels --all-nodes --output RLT/worst_sprint_res.txt --threads 12
	#@./best2cop --file topos/${FRANCOIS} --topo --labels --all-nodes --output --threads 12 RLT/topo_francois_res.txt
	@./best2cop --file topos/${GBLX} --topo --labels --all-nodes --output RLT/gblx_res.txt --threads 12
	@echo ${eval} "Eval for RLT completed"

rltclean:
	@rm -rf RLT
	@rm -rf best2cop
	@rm -rf main.o

# figure6dirs:
# 	@mkdir -p figure6

# figure6: figure6dirs worstSprint
# 	@echo ${eval} "Start eval for Figure 6"
# 	@./worstSprint
# 	@echo ${eval} "Eval for Figure 6 completed"

# figure6clean:
# 	@rm -rf figure6
# 	@rm -rf worstSprint
# 	@rm -rf worstSprint.o

# figure7dirs:
# 	@mkdir -p figure7

# figure7: figure7dirs randomSpread
# 	@echo ${eval} "Start eval for Figure 7"
# 	@./randomSpread figure7/results.txt
# 	@echo ${eval} "Eval for Figure 7 completed"

# figure7clean:
# 	@rm -rf figure7
# 	@rm -rf randomSpread.o
# 	@rm -rf randomSpread

# figure8dirs:
# 	@mkdir -p figure8

# figure8: figure8dirs scriptCstr
# 	@echo ${eval} "Start eval for Figure 8"
# 	@./scriptCstr topos/sprint-modif70.isp figure8/res_cstr.txt
# 	@echo ${eval} "Eval for Figure 8 completed"

# figure8clean:
# 	@rm -rf figure8
# 	@rm -rf scriptCstr.o
# 	@rm -rf scriptCstr

# figure9dirs:
# 	@mkdir -p figure9

# figure9: figure9dirs worstRandom
# 	@echo ${eval} "Start eval for Figure 9"
# 	@./worstRandom figure4/resultsSpread500/worstTopo1000.isp figure9/res.txt
# 	@echo ${eval} "Eval for Figure 9 completed"

# figure9clean:
# 	@rm -rf figure9
# 	@rm -rf worstRandom.o
# 	@rm -rf worstRandom

nbsegdirs:
	@mkdir -p NBSEG

NBSEG: nbsegdirs best2cop
	@echo ${eval} "Start eval for NBSEG"
	@for i in 5 10 50 100 ; do \
		echo -e ${eval} "Start eval for $$i"; \
		./best2cop --file sr_topo21.isp --sr --id --cstr1 $$(($$i * 10)) --accu 0 --all-nodes --DCLC --output NBSEG/res_rand_dclc$$i.txt; \
		./best2cop --file ${REALISTIC_SPRINT} --topo --labels --cstr1 $$i --all-nodes --DCLC --output NBSEG/res_sprint_dclc$$i.txt; \
	done;
	@echo ${eval} "Eval for NBSEG completed"

FOR_FRANCOIS: best2cop nbsegdirs scriptCstr rltdirs
	@echo ${eval} "Start eval for FRANCOIS"
	@./best2cop --file topos/${FRANCOIS} --topo --labels --all-nodes --output RLT/topo_francois_res.txt --threads 12
	@./scriptCstr topos/${FRANCOIS} NBSEG/topo_francois_res.txt
	@echo ${eval} "Eval for FRANCOIS completed"

nbsegclean:
	@rm -rf NBSEG
	@rm -rf main.o
	@rm -rf best2cop

cleanFigures: ct_nodesclean ct_threadsclean rndclean rltclean nbsegclean

allFigures: CT_NODES CT_THREADS RND RLT NBSEG
