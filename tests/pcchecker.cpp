#include <cstdio>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

using std::cin;
using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::sort;

int main() {
    string cmd;
    int number;
    vector<int> produced, consumed;
    while(cin >> cmd >> number) {
        if(cmd == "p")
            produced.push_back(number);
        if(cmd == "c")
            consumed.push_back(number);
    }
    sort(produced.begin(), produced.end());
    sort(consumed.begin(), consumed.end());
    if(produced != consumed)
        cout << "Wrong" << endl;
    else
        cout << "Correct" << endl;
    return 0;
}
