SRCDIR=src
OBJDIR=obj
CXX=g++

NO_COLOR=\x1b[0m
OK_COLOR=\x1b[32;01m
ERROR_COLOR=\x1b[31;01m
WARN_COLOR=\x1b[33;01m
OK_STRING=$(OK_COLOR)[OK]$(NO_COLOR)

CFLAGS= -pthread -msse4.2 -Wall -Wuninitialized -std=c++11 
LDFLAGS= -pthread -msse4.2 -Wall -Wuninitialized -std=c++11 

EXE= acf4x1_force_swap acf4x1s_optimized acf2x4_force_swap acf4x1s_original acf4x1s_effect acf2x4_effect

OBJ_MAIN=$(EXE:%=$(OBJDIR)/%.o )
SRC=$(shell ls -R $(SRCDIR)/*.c*)
OBJ=$(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
OBJ_COMMON=$(filter-out $(OBJ_MAIN),$(OBJ))


all: $(EXE) 
 
$(EXE): $(OBJ)
	@$(CXX) $(OBJDIR)/$@.o $(OBJ_COMMON) -o $@  -lpthread  
	@echo -e '$(OK_COLOR)[*] Created executable  $@ $(NO_COLOR)'

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@$(CXX) $(CFLAGS) -c $< -o $@  -lpthread  
	@echo "[*] Compiled" $<

clean:
	@rm -f $(EXE) $(OBJ)
	@echo "[*] Directory $(CURDIR) cleaned"

