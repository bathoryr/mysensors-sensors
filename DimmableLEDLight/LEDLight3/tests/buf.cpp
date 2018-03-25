#include <iostream>
#include "buffer.h"

using namespace std;

int main(int argc, char** argv)
{
    buffer<int> buff;
    for (int i = 3; i < 40; i++)
    {
        buff.add(i);
    }
    cout << "OK" << endl;
    for (int i = 0; i < buff.size(); i++)
    {
        cout << i << ": " << buff.get(i) << endl;
    }
}