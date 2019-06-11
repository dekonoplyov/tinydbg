#include <iostream>

void a()
{
    std::cerr << "here again\n";
}

void b()
{
    a();
}

void c()
{
    a();
}

void d()
{
    c();
}

int main()
{
    b();
    c();
    d();
    return 0;
}
