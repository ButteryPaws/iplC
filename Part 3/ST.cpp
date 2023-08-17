#include "ST.hh"

SymTabEntry::SymTabEntry(string type, string loc,  int order, int l)
{
    this->loc = loc;
    this->type = type;
    this->order = order;
    this->l = l;
    set_size();
}

void SymTabEntry::set_size()
{
    int str_len = this->type.size();
    int i = 0;
    while(this->type[i] != '*' && this->type[i] != '[' && i < str_len) i++;
    // Eg : int, float, struct person
    if(i == str_len){
        if(this->type.substr(0, i) != "int" && this->type.substr(0, i) != "float")
        {
            this->size = tables.at(this->type.substr(0, i)).size;
        }
        else{
            this->size = 4;
        }
    map<int, string> ordered_attrs; //map from order of entries to their name
    }
    if(this->type[i] == '*'){
        this->size = 4;
    }
    else{
        if(this->type.substr(0, i) != "int" && this->type.substr(0, i) != "float")
        {
            this->size = tables.at(this->type.substr(0, i)).size;
        }
        else{
            this->size = 4;
        }
    } 
    while(this->type[i] != '[' && i < str_len) i++;
    if(i==str_len) return; // Eg : int*, struct s2***
    i++;
    while(i < str_len)
    {
        int j = i;
        while(this->type[j] != ']') j++;
        this->size *= stoi(this->type.substr(i, j - i));
        i = j + 2;
    }
}

SymTab::SymTab(string type, string ret_type, map<string, SymTabEntry> entries)
{
    this->type = type;
    this->entries = entries;
    size = 0;
    if(type == "fun") {
        this->ret_type = ret_type;
        int local_offset = 0;
        int param_offset = 12;
        map<int, string> ordered_locals;
        map<int, string> ordered_params;
        for(auto it: entries)
        {
            if(it.second.loc == "local") ordered_locals[it.second.order] = it.first;
            else ordered_params[it.second.order] = it.first;
        }
        for(auto it: ordered_locals)
        {
            local_offset -= this->entries.at(it.second).size;
            this->entries.at(it.second).offset = local_offset;
        }
        for(auto it = ordered_params.rbegin(); it != ordered_params.rend(); it++)
        {
            this->entries.at(it->second).offset = param_offset;
            param_offset += this->entries.at(it->second).size;
        }
    }
    else {
        map<int, string> ordered_attrs; //map from order of entries to their name
        for(auto it = entries.begin(); it != entries.end(); it++)
        {
            size += it->second.size;
            ordered_attrs[it->second.order] = it->first;
        }
        this->ret_type = "-";
        int curr_offset = 0;
        for(auto it = ordered_attrs.begin(); it != ordered_attrs.end(); it++)
        {
            this->entries.at(it->second).offset = curr_offset;
            curr_offset += this->entries.at(it->second).size;
        }
    }
}

int SymTab::get_param_length(){
    int count = 0;
    for(auto it = this->entries.begin(); it != this->entries.end(); it++){
        if((it->second).loc=="param"){
            count++;
        }
    }
    return count;
}