#include <cstring>
#include <cstddef>
#include <istream>
#include <iostream>
#include <fstream>

#include "scanner.hh"
#include "parser.tab.hh"
using namespace std;

// map<string, string> functions; 
// vector<string> struct_fun;
// vector<int> nature;
// map<string, map<string, string> > struct_localST; //struct name maps to map of (variable name to type)
// map<string, map<string, string> > func_var;
// map<string, map<string, string> > func_param;
// map<string, int> func_param_length;

// New Variables
map<string, SymTab> tables;
map<string, statement_astnode*> ASTs;
map<string, string> declared_symbols;
vector<string> scope;

void print_boilerplate();

int main(const int argc, const char **argv)
{

  using namespace std;
  fstream in_file;

  in_file.open(argv[1], ios::in);
  // Generate a scanner
  IPL::Scanner scanner(in_file);
  // Generate a Parser, passing the scanner as an argument.
  // Remember %parse-param { Scanner  &scanner  }
  IPL::Parser parser(scanner);
  
  #ifdef YYDEBUG
   parser.set_debug_level(1);
  #endif
  parser.parse();

  print_boilerplate();
}

void print_boilerplate()
{
  // Global Symbol Table
  cout << "{\n\"globalST\": [";
  if(tables.size()==1){
    auto start = tables.begin();
    cout << "[\n";
    cout << "\"" << start->first << "\",\n";
    cout << "\"" << start->second.type << "\",\n";
    cout << "\"global\",\n";
    cout << start->second.size << ",\n";
    cout << (start->second.type == "struct" ? "\"-\"" : "0") << ",\n";
    cout << "\"" << start->second.ret_type << "\"\n";
    cout << "]\n";
    cout << "],\n";
  }
  else{
    auto ultimate = prev(tables.end(), 1);
    for(auto it = tables.begin(); it != ultimate; it++){
      cout << "[\n";
      cout << "\"" << it->first << "\",\n";
      cout << "\"" << it->second.type << "\",\n";
      cout << "\"global\",\n";
      cout << it->second.size << ",\n";
      cout << (it->second.type == "struct" ? "\"-\"" : "0") << ",\n";
      cout << "\"" << it->second.ret_type << "\"\n";
      cout << "],\n";
    }
    cout << "[\n";
    cout << "\"" << ultimate->first << "\",\n";
    cout << "\"" << ultimate->second.type << "\",\n";
    cout << "\"global\",\n";
    cout << ultimate->second.size << ",\n";
    cout << (ultimate->second.type == "struct" ? "\"-\"" : "0") << ",\n";
    cout << "\"" << ultimate->second.ret_type << "\"\n";
    cout << "]\n";
    cout << "],\n";
  }

  // Struct List
  cout << "\"structs\": [";
  map<string, SymTab> struct_tabs;
  for(auto it: tables)
  {
    if(it.second.type == "struct") struct_tabs.insert({it.first, it.second});
  }
  if(struct_tabs.size()!=0){
    cout << "\n";
    auto ultimate_struct = prev(struct_tabs.end(), 1);
    for(auto it = struct_tabs.begin(); it != ultimate_struct; it++){
      cout << "{\n";
      cout << "\"name\": \"" << it->first << "\",\n";
      cout << "\"localST\": [\n"; 
      auto last_entry = std::prev(it->second.entries.end(), 1);
      for(auto it2 = it->second.entries.begin(); it2 != last_entry; it2++){
        cout << "[\n";
        cout << "\"" << it2->first << "\",\n";
        cout << "\"var\",\n";
        cout << "\"local\",\n";
        cout << it2->second.size << ",\n";
        cout << it2->second.offset << ",\n";
        cout << "\"" << it2->second.type << "\"\n";
        cout << "],\n";
      }
      cout << "[\n";
      cout << "\"" << last_entry->first << "\",\n";
      cout << "\"var\",\n";
      cout << "\"local\",\n";
      cout << last_entry->second.size << ",\n";
      cout << last_entry->second.offset << ",\n";
      cout << "\"" << last_entry->second.type << "\"\n";
      cout << "]\n]\n},\n";
    }
    cout << "{\n";
    cout << "\"name\": \"" << ultimate_struct->first << "\",\n";
    cout << "\"localST\": [\n"; 
    auto last_entry = std::prev(ultimate_struct->second.entries.end(), 1);
    for(auto it2 = ultimate_struct->second.entries.begin(); it2 != last_entry; it2++){
      cout << "[\n";
      cout << "\"" << it2->first << "\",\n";
      cout << "\"var\",\n";
      cout << "\"local\",\n";
      cout << it2->second.size << ",\n";
      cout << it2->second.offset << ",\n";
      cout << "\"" << it2->second.type << "\"\n";
      cout << "],\n";
    }
    cout << "[\n";
    cout << "\"" << last_entry->first << "\",\n";
    cout << "\"var\",\n";
    cout << "\"local\",\n";
    cout << last_entry->second.size << ",\n";
    cout << last_entry->second.offset << ",\n";
    cout << "\"" << last_entry->second.type << "\"\n";
    cout << "]\n]\n}\n],\n";
  }
  else{
    cout << "],\n";
  }

  // Functions List
  cout << "\"functions\": [\n";
  map<string, SymTab> func_tabs;
  for(auto it: tables)
  {
    if(it.second.type == "fun") func_tabs.insert({it.first, it.second});
  }
  if(func_tabs.size()!=0){
    auto last_function = std::prev(func_tabs.end(), 1);
    for(auto it=func_tabs.begin(); it!=last_function; it++){
      // func_param[it->first] : Symbol Table for parameters
      // func_var[it->first] : Symbol Table for variables
      cout << "{\n";

      // Name of the function
      cout << "\"name\": \"" << it->first << "\",\n";

      // Local Symbol Table
      cout << "\"localST\": [";

      if(it->second.entries.size()!=0){
        cout << "\n";
        auto last_var = std::prev(it->second.entries.end(), 1);
        for(auto it2=it->second.entries.begin(); it2!=last_var; it2++){
          cout << "[\n";
          cout << "\"" << it2->first << "\",\n";
          cout << "\"var\",\n";
          cout << "\"" << it2->second.loc << "\",\n";
          cout << it2->second.size << ",\n";
          cout << it2->second.offset << ",\n";
          cout << "\"" << it2->second.type << "\"\n";
          cout << "],\n";
        } 
        cout << "[\n";
        cout << "\"" << last_var->first << "\",\n";
        cout << "\"var\",\n";
        cout << "\"" << last_var->second.loc << "\",\n";
        cout << last_var->second.size << ",\n";
        cout << last_var->second.offset << ",\n";
        cout << "\"" << last_var->second.type << "\"\n";
        cout << "]\n";
        cout << "],\n";
      }
      else{
        cout << "],\n";
      }

      // AST
      cout << "\"ast\": ";
      ASTs[it->first]->print(0);
      cout << "\n},\n";
    }
    cout << "{\n";

    // Name of the function
    cout << "\"name\": \"" << last_function->first << "\",\n";

    // Local Symbol Table
    cout << "\"localST\": [\n";

    if(last_function->second.entries.size()!=0){
      auto last_var = std::prev(last_function->second.entries.end(), 1);
      for(auto it2=last_function->second.entries.begin(); it2!=last_var; it2++){
        cout << "[\n";
        cout << "\"" << it2->first << "\",\n";
        cout << "\"var\",\n";
        cout << "\"" << it2->second.loc << "\",\n";
        cout << it2->second.size << ",\n";
        cout << it2->second.offset << ",\n";
        cout << "\"" << it2->second.type << "\"\n";
        cout << "],\n";
      } 
      cout << "[\n";
      cout << "\"" << last_var->first << "\",\n";
      cout << "\"var\",\n";
      cout << "\"" << last_var->second.loc << "\",\n";
      cout << last_var->second.size << ",\n";
      cout << last_var->second.offset << ",\n";
      cout << "\"" << last_var->second.type << "\"\n";
      cout << "]\n";
      cout << "],\n";
    }
    else{
      cout << "],\n";
    }

    // AST
    cout << "\"ast\": ";
    ASTs[last_function->first]->print(0);
    cout << "\n}\n]\n}";
  }
}