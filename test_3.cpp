#include <iostream>
#include "malloc_3.h"

using namespace std;

int main(){
    int* ptr = (int*)smalloc(sizeof(int));
    *ptr = 3;
    cout << *ptr << endl;
}