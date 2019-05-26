#include <iostream>

int add(int a, int b)
{
    return a + b;
}

int main()
{
    std::cerr << "Hi there\n";
    std::cerr << "Hi yourself\n";
    return add(2, -2);
}
