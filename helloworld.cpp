// helloworld.cpp
#include <iostream>
using namespace std;
int main(int argc, char **argv){
    for (int i=0; i<argc; i++) {
        printf("%s\n", argv[i]);
    }
    
	cout<<"hello world"<<endl;
}