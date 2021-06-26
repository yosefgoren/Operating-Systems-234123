#include "malloc_2.h"
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

int main(){
	passed_all = true;
	intest_counter = 0;
	current_test_name = "test_malloc_2";


	cout << current_test_name << ": " << (passed_all ? "PASSED" : "FAILED") << endl;
	return 0;
}
