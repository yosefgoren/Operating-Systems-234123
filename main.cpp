#include "malloc_1.h"
#include <iostream>
#include <string>

using namespace std; 

static bool passed_all;
static int intest_counter;
static string current_test_name;

void check(bool cond){
	intest_counter++;
	if(!cond)
		cout << "	" << current_test_name << ": FAILED assertion number: " << intest_counter << endl;
	passed_all = cond;
}

bool test_malloc_1(){
	passed_all = true;
	intest_counter = 0;
	current_test_name = "test_malloc_1";

	check(smalloc(0) == NULL);
	size_t p1 = (size_t)smalloc(16);
	size_t p2 = (size_t)smalloc(32);
	check(p2-p1 == 16);
	size_t p3 = (size_t)smalloc(256);
	check(p3-p2 == 32);
	check(smalloc(100000001) == NULL);
	
	cout << current_test_name << ": " << (passed_all ? "PASSED" : "FAILED") << endl;
}

int main(){
	test_malloc_1();
	
	return 0;
}
