#include "pde_software_replication_core/hello.h"
#include <iostream>

int main() {
    const auto message = hello();
    std::cout << message << std::endl;
    return message == "Hello, World!" ? 0 : 1;
}
