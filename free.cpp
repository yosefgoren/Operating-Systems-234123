#include <iostream>

using namespace std;

struct A
{   
    int x;
    int y;
    int z;
    void* w;
    bool h;
    bool n;
};


void do10(void(*todo)(int, void*), void* func_data = NULL){
	for(int i = 0; i < 10; ++i)
		todo(i, func_data);
}

int main(){
    //cout << 11e6 << endl;
}