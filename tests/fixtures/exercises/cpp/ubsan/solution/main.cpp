#include <climits>
#include <iostream>
int main(int argc, char **) { int x = INT_MAX; x += argc; std::cout << 42 << "\n"; return x == 0; }
