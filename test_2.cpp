#include "malloc_2.cpp"
#include <iostream>
#include <string>

using namespace std; 

static bool passed_all;
static int intest_counter;
static string current_test_name;


void printMemory(void *start=first_block, bool onlyList=true) {
	Block* current = (Block*) start;
	size_t size = 0;
	int blocks = 0;
	if (!onlyList) {
		std::cout << "Printing Memory List\n";
	}
    Block* program_break = (Block*)sbrk(0);
    while (current != program_break && current != nullptr) {
        if (current->is_free) {
			std::cout << "|F:" << current->udata_size;
		} else {
			std::cout << "|U:" << current->udata_size;
		}
		size += current->udata_size;
		blocks++;
//		current = current->next;
        size_t effective_size = current->udata_size;
        char * temp_ptr = (char *)current;
        temp_ptr += effective_size + sizeof (Block);
        current =  (Block *)temp_ptr;
	}
	std::cout << "|";
	if (!onlyList) {
		std::cout << std::endl << "Memory Info:\nNumber Of Blocks: " << blocks << "\nTotal Size (without Metadata): " << size << std::endl;
		std::cout << "Size of Metadata: " << sizeof(Block) << std::endl;
	}
    cout << endl;
}


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

	int n = 6;
	char* arr[n];
	for(int i = 0; i < n; ++i){
		arr[i] = (char*)smalloc(i+1);
		*(arr[i]) = 'a';
	}
	printMemory();
	
	Block* b0 = blockFromUdata(arr[0]);
	Block* b1 = b0->getBlockAfter();
	Block* b2 = b1->getBlockAfter();
	Block* b3 = b2->getBlockAfter();
	Block* b4 = b3->getBlockAfter();
	Block* b5 = b4->getBlockAfter();
	
	for(int i = 0; i < n; i += 2){
		sfree(arr[i]);
	}
	printMemory();
	for(int i = 0; i < n; i += 2){
		arr[i] = (char*)smalloc(i+1);
		printMemory();
		*(arr[i]) = 'b';
	}
	printMemory();

	cout << current_test_name << ": " << (passed_all ? "PASSED" : "FAILED") << endl;
	return 0;
}
