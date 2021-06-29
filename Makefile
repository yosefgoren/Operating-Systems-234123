CPPC=g++
COMP_FLAGS= -Wall -Wextra -Werror -g
SUBMIT=malloc_1.cpp malloc_2.cpp malloc_3.cpp malloc_4.cpp submmitters.txt

all: test_1.out test_2.out free.out

test_1.out: test_1.cpp malloc_1.cpp malloc_1.h
	$(CPPC) $(COMP_FLAGS) test_1.cpp malloc_1.cpp -o $@

test_2.out: test_2.cpp malloc_2.cpp malloc_2.h
	$(CPPC) $(COMP_FLAGS) test_2.cpp malloc_2.cpp -o $@

free.out: free.cpp
	$(CPPC) $(COMP_FLAGS) $^ -o $@

free: free.out
	./free.out

test:
	./test_1.out

clean:
	rm -rf *.o *.out
