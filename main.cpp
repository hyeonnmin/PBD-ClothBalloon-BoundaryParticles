#include <iostream>
#include <memory>
#include <windows.h>

#include "ExampleApp.h"

using namespace std;


int main() {
    jhm::ExampleApp exampleApp;

    if (!exampleApp.Initialize()) {
        cout << "Initialization failed." << endl;
        return -1;
    }

    return exampleApp.Run();
}
