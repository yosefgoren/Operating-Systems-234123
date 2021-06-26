CPPC=g++
COMP_FLAGS= -Wall -Wextra -Werror
SOL=utils.cpp utils.h malloc_1.cpp malloc_1.h

main.out: main.o utils.o utils.h malloc_1.o malloc_1.h malloc_2.o malloc_2.h
	$(CPPC) $(COMP_FLAGS) $^ -o  $@

main.o: main.cpp
	$(CPPC) $^ -c -o $@

utils.o: utils.cpp utils.h
	$(CPPC) $(COMP_FLAGS) utils.cpp -c -o $@

malloc_1.o: malloc_1.cpp malloc_1.h
	$(CPPC) $(COMP_FLAGS) malloc_1.cpp -c -o $@

malloc_2.o: malloc_2.cpp malloc_2.h
	$(CPPC) $(COMP_FLAGS) malloc_2.cpp -c -o $@

clean:
	rm -rf *.o *.out

