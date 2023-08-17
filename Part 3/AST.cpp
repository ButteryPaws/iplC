#include "AST.hh"
#include <iostream>

empty_astnode::empty_astnode()
{
    nodetype = EMPTY_ASTNODE;
}

void empty_astnode::print(int blanks)
{
    std::cout << "\"empty\"\n";
}

seq_astnode::seq_astnode()
{
    nodetype = SEQ_ASTNODE;
}

seq_astnode::seq_astnode(statement_astnode* arg)
{
    nodetype = SEQ_ASTNODE;
    this->seq.push_back(arg);
}

void seq_astnode::print(int blanks)
{
    std::cout << "{\n\"seq\": [\n";
    int idx = 0;
    for(auto x : this->seq){
        idx++;
        x->print(0);
        if(idx!=this->seq.size()){
            std::cout << ",";
        }
        std::cout << "\n";
    }
    std::cout << "\n]\n}";
}

assignS_astnode::assignS_astnode(exp_astnode* arg1, exp_astnode* arg2)
{
    this->left = arg1;
    this->right = arg2;
    nodetype = ASSIGNS_ASTNODE;
}

void assignS_astnode::print(int blanks)
{
    std::cout << "{\n\"assignS\": {\n\"left\": ";
    this->left->print(0);
    std::cout << ",\n\"right\": ";
    this->right->print(0);
    std::cout << "\n}\n}";
}

return_astnode::return_astnode(exp_astnode* arg1)
{
    this->arg = arg1;
    nodetype = RETURN_ASTNODE;
}

void return_astnode::print(int blanks)
{
    std::cout << "{\n\"return\": ";
    this->arg->print(0);
    std::cout << "\n}";
}

if_astnode::if_astnode(exp_astnode* arg1, statement_astnode* arg2, statement_astnode* arg3)
{
    this->cond = arg1;
    this->then = arg2;
    this->else_statement = arg3;
    nodetype = IF_ASTNODE;
}

void if_astnode::print(int blanks)
{
    std::cout << "{\n\"if\": {\n\"cond\": ";
    this->cond->print(0);
    std::cout << ",\n\"then\": ";
    this->then->print(0);
    std::cout << ",\n\"else\": ";
    this->else_statement->print(0);
    std::cout << "\n}\n}";
}

while_astnode::while_astnode(exp_astnode* arg1, statement_astnode* arg2)
{
    this->cond = arg1;
    this->stmt = arg2;
    nodetype = WHILE_ASTNODE;
}

void while_astnode::print(int blanks)
{
    std::cout << "{\n\"while\": {\n\"cond\": ";
    this->cond->print(0);
    std::cout << ",\n\"stmt\": ";
    this->stmt->print(0);
    std::cout << "\n}\n}";
}

for_astnode::for_astnode(exp_astnode* arg1, exp_astnode* arg2, exp_astnode* arg3, statement_astnode* arg4)
{
    this->init = arg1;
    this->guard = arg2;
    this->step = arg3;
    this->body = arg4;
    nodetype = FOR_ASTNODE;
}

void for_astnode::print(int blanks)
{
    std::cout << "{\n\"for\": {\n\"init\": ";
    this->init->print(0);
    std::cout << ",\n\"guard\": ";
    this->guard->print(0);
    std::cout << ",\n\"step\": ";
    this->step->print(0);
    std::cout << ",\n\"body\": ";
    this->body->print(0);
    std::cout << "\n}\n}";
}

proccall_astnode::proccall_astnode(vector<exp_astnode*> args)
{
    this->args = args;
    this->is_lvalue = 0;
    nodetype = PROCCALL_ASTNODE;
}

void proccall_astnode::print(int blanks)
{
    std::cout << "{\n\"proccall\": {\n\"fname\": ";
    this->args[0]->print(0);
    std::cout << ",\n\"params\": [";
    if(this->args.size()>1){
        std::cout << "\n";
        for(auto it=this->args.begin()+1; it!=this->args.end()-1; it++){
            (*it)->print(0);
            std::cout << ",\n";
        }
        this->args[this->args.size()-1]->print(0);
        std::cout << "\n";
    }
    std::cout << "]\n}\n}";
}

identifier_astnode::identifier_astnode(string arg)
{
    this->id = arg;
    this->type = declared_symbols[arg];
    this->is_lvalue = 1;
    nodetype = IDENTIFIER_ASTNODE;
}

void identifier_astnode::print(int blanks)
{
    std::cout << "{\n\"identifier\": \"" << this->id << "\"\n}";
}

arrayref_astnode::arrayref_astnode(exp_astnode* arg1, exp_astnode* arg2)
{
    this->array = arg1;
    this->index = arg2;
    this->type = arg1->type;
    this->is_lvalue = 1;
    nodetype = ARRAYREF_ASTNODE;
}

void arrayref_astnode::print(int blanks)
{
    std::cout << "{\n\"arrayref\": {\n\"array\": ";
    this->array->print(0);
    std::cout << ",\n\"index\": ";
    this->index->print(0);
    std::cout << "\n}\n}";
}


member_astnode::member_astnode(exp_astnode* arg1, identifier_astnode* arg2)
{
    this->struct_arg = arg1;
    this->field = arg2;
    this->type = arg2->type;
    this->is_lvalue = 1;
    nodetype = MEMBER_ASTNODE;
}

void member_astnode::print(int blanks)
{
    std::cout << "{\n\"member\": {\n\"struct\": ";
    this->struct_arg->print(0);
    std::cout << ",\n\"field\": ";
    this->field->print(0);
    std::cout << "\n}\n}";
}

arrow_astnode::arrow_astnode(exp_astnode* arg1, identifier_astnode* arg2)
{
    this->pointer = arg1;
    this->field = arg2;
    this->type = arg2->type;
    this->is_lvalue = 1;
    nodetype = ARROW_ASTNODE;
}

void arrow_astnode::print(int blanks)
{
    std::cout << "{\n\"arrow\": {\n\"pointer\": ";
    this->pointer->print(0);
    std::cout << ",\n\"field\": ";
    this->field->print(0);
    std::cout << "\n}\n}";
}

op_unary_astnode::op_unary_astnode(string arg1, exp_astnode* arg2)
{
    this->arg1 = arg1;
    this->arg2 = arg2;
    nodetype = OP_UNARY_ASTNODE;
    if(arg1=="DEREF"){
        // Bracket is present
        this->is_lvalue = 1;
        if(arg2->type.find('(')!=string::npos){
            int i=0;
            while(i<arg2->type.length() && arg2->type[i]!='('){
                i++;
            }
            string prefix = arg2->type.substr(0,i);
            string suffix = arg2->type.substr(i+3);
            this->type = prefix+suffix;
        }
        else if(arg2->type.find('[')!=string::npos){
            int i=0;
            while(i<arg2->type.length() && arg2->type[i]!='['){
                i++;
            }
            string prefix = arg2->type.substr(0,i);
            string suffix = arg2->type.substr(i+3);
            this->type = prefix+suffix;
        }
        else if(arg2->type.find("*")!=string::npos){
            this->type = arg2->type.substr(0,arg2->type.length()-1);
        }
    }
    else if(arg1=="ADDRESS"){
        this->is_lvalue = 0;
        if(arg2->type.find('[')!=string::npos){
            int i=0;
            while(i<arg2->type.length() && arg2->type[i]!='['){
                i++;
            }
            string prefix = arg2->type.substr(0,i);
            string suffix = arg2->type.substr(i);
            this->type = prefix + "(*)" + suffix;
        }
        else{
            this->type = arg2->type + "(*)";
        }
    }
    else if(arg1=="NOT"){
        this->type = "int";
    }
    else{
        this->type = arg2->type;
    }
    if(arg1=="UMINUS" || arg1=="NOT" || arg1=="PP"){
        this->is_lvalue = 0;
    }
}

void op_unary_astnode::print(int blanks)
{
    std::cout << "{\n\"op_unary\": {\"op\": \"" << this->arg1 << "\",\n\"child\": ";
    this->arg2->print(0);
    std::cout << "\n}\n}";
}

op_binary_astnode::op_binary_astnode(string arg1, exp_astnode* arg2, exp_astnode* arg3)
{
    this->op = arg1;
    this->arg1 = arg2;
    this->arg2 = arg3;
    this->is_lvalue = 0;
    this->value = -4;
    nodetype = OP_BINARY_ASTNODE;
}

void op_binary_astnode::print(int blanks)
{
    std::cout << "{\n\"op_binary\": {\"op\": \"" << this->op << "\",\n\"left\": ";
    this->arg1->print(0);
    std::cout << ",\n\"right\": ";
    this->arg2->print(0);
    std::cout << "\n}\n}";
}

assignE_astnode::assignE_astnode(exp_astnode* arg1, exp_astnode* arg2)
{ 
    this->left = arg1;
    this->right = arg2;
    nodetype = ASSIGNE_ASTNODE;
}

void assignE_astnode::print(int blanks)
{
    std::cout << "{\n\"assignE\": {\n\"left\": ";
    this->left->print(0);
    std::cout << ",\n\"right\": ";
    this->right->print(0);
    std::cout << "\n}\n}";
}

funcall_astnode::funcall_astnode(vector<exp_astnode*> args)
{
    this->args = args;
    this->is_lvalue = 0;
    nodetype = FUNCALL_ASTNODE;
}

void funcall_astnode::print(int blanks)
{
    std::cout << "{\n\"funcall\": {\n\"fname\": ";
    this->args[0]->print(0);
    std::cout << ",\n\"params\": [";
    if(this->args.size()!=1){
        cout << "\n";
        for(auto it=this->args.begin()+1; it!=this->args.end()-1; it++){
            (*it)->print(0);
            std::cout << ",\n";
        }
        this->args[this->args.size()-1]->print(0);
        std::cout << "\n]\n}\n}";
    }
    else{
        std::cout << "]\n}\n}";
    }
}

intconst_astnode::intconst_astnode(int arg1)
{
    this->id = arg1;
    this->type = "int";
    this->is_lvalue = 0;
    this->value = arg1;
    nodetype = INTCONST_ASTNODE;
}

void intconst_astnode::print(int blanks)
{
    std::cout << "{\n\"intconst\": " << this->id << "\n}";
}

floatconst_astnode::floatconst_astnode(float arg1)
{
    this->id = arg1;
    this->type = "float";
    this->is_lvalue = 0;
    this->value = arg1;
    nodetype = FLOATCONST_ASTNODE;
}

void floatconst_astnode::print(int blanks)
{
    std::cout << "{\n\"floatconst\": " << this->id << "\n}";
}

stringconst_astnode::stringconst_astnode(string arg1)
{
    this->id = arg1;
    this->type = "string";
    this->is_lvalue = 0;
    this->value = -1;
    nodetype = STRINGCONST_ASTNODE;
}

void stringconst_astnode::print(int blanks)
{
    std::cout << "{\n\"stringconst\": " << this->id << "\n}";
}

string get_base_type(string type){
    string base_type = type;
    // [] is present in the type
    if(base_type.find("[")!=string::npos){
        base_type = base_type.substr(0,base_type.find("["));
    }
    // (*) is present in the type
    if(base_type.find("(")!=string::npos){
        base_type = base_type.substr(0,base_type.find("("));
    }
    // * is present in the type
    if(base_type.find("*")!=string::npos){
        base_type = base_type.substr(0,base_type.find("*"));
    }
    return base_type;
}

int get_asterick_length(string type){
    int start = type.find("*");
    if(type[start+1]==')'){
        return 0;
    }
    else{
        int i = start;
        while(i<type.length() && type[i]=='*'){
            i++;
        }
        return i-start;
    }
}