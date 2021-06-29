#include <iostream>

using namespace std;

void do10(void(*todo)(int, void*), void* func_data = NULL){
	for(int i = 0; i < 10; ++i)
		todo(i, func_data);
}

int main(){
    // int loc = 11;
    // auto setToMaxWith = [](int num, void* v_target){
    //     int* target = (int*)v_target;
    //     *target = *target > num ? *target : num;
    // };
    // do10(setToMaxWith, &loc);
    // cout << "final is: " << loc << endl;

    int x = 0;
    int y = 0;
    int* ptr = &x;
    int* ptr2 = (int*)((char*)(ptr+1));
    printf("%lx\n", (size_t)ptr);
    printf("%lx\n", (size_t)ptr2);
    printf("%d\n", (size_t)ptr - (size_t)ptr2);
}