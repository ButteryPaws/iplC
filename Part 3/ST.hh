#include <string>
#include <map>
#include <iostream>
#include <vector>

using namespace std;

class SymTabEntry;
class SymTab;

extern map<string, SymTab> tables;

class SymTabEntry
{
    public:
    int order;
    string loc; //local or param
    int size;
    int offset = 200;
    string type;
    int l;

    SymTabEntry(string type, string loc,  int order, int l);

    void set_size();
};

class SymTab
{
    public:
    string type; // fun or struct
    int size;
    map<string, SymTabEntry> entries;
    string ret_type;

    SymTab(string type, string ret_type, map<string, SymTabEntry> entries);

    int get_param_length();
};