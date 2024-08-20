#include <iostream>
#include "voko.h"

int main()
{
    std::cout << "Welcome, voko!\n";
   
    voko* Voko = new voko();

    Voko->init();
    Voko->renderLoop();
    delete(Voko);


    std::cout << "Bye Bye, voko!\n";
    return 0;


}
