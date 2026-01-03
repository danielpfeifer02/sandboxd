#include <iostream>

int main(int argc, char** argv) {
    std::cout << "Running sandbox with arguments: ";
    for (int i = 0; i < argc; i++) {
        std::cout << argv[i] << ' ';
    }
    std::cout << std::endl;
    return 0;
}