#include <vector>
#include <cstring>
#include <map>

using namespace std;

enum typeExp
{
    STATEMENT_ASTNODE,
    EMPTY_ASTNODE,
    SEQ_ASTNODE,
    ASSIGNS_ASTNODE,
    RETURN_ASTNODE,
    IF_ASTNODE,
    WHILE_ASTNODE,
    FOR_ASTNODE,
    PROCCALL_ASTNODE,
    EXP_ASTNODE,
    REF_ASTNODE,
    IDENTIFIER_ASTNODE,
    ARRAYREF_ASTNODE,
    MEMBER_ASTNODE,
    ARROW_ASTNODE,
    OP_BINARY_ASTNODE,
    OP_UNARY_ASTNODE,
    ASSIGNE_ASTNODE,
    FUNCALL_ASTNODE,
    INTCONST_ASTNODE,
    FLOATCONST_ASTNODE,
    STRINGCONST_ASTNODE,
};

class abstract_astnode
{
    public:
    int is_lvalue;
    virtual void print(int blanks) = 0;
    enum typeExp astnode_type;
};

class statement_astnode: public abstract_astnode
{
    public:
    virtual void print(int blanks) = 0;
};

class exp_astnode: public abstract_astnode
{
    public:
    string type;
    int value;
    virtual void print(int blanks) = 0;
};

class ref_astnode: public exp_astnode
{
    public:
    virtual void print(int blanks) = 0;
};

class empty_astnode: public statement_astnode
{
    public:
    empty_astnode();
    void print(int blanks);
};

class seq_astnode: public statement_astnode
{
    public:
    vector<statement_astnode*> seq;
    seq_astnode(statement_astnode* args);
    seq_astnode();
    void print(int blanks);
};

class assignS_astnode: public statement_astnode
{
    public:
    exp_astnode *left, *right;
    assignS_astnode(exp_astnode* arg1, exp_astnode* arg2);
    void print(int blanks);
};

class return_astnode: public statement_astnode
{
    public:
    exp_astnode* arg;
    return_astnode(exp_astnode* arg1);
    void print(int blanks);
};

class if_astnode: public statement_astnode
{
    public:
    exp_astnode* cond;
    statement_astnode* then;
    statement_astnode* else_statement;
    if_astnode(exp_astnode* arg1, statement_astnode* arg2, statement_astnode* arg3);
    void print(int blanks);
};

class while_astnode: public statement_astnode
{
    public:
    exp_astnode* cond;
    statement_astnode* stmt;
    while_astnode(exp_astnode* arg1, statement_astnode* arg2);
    void print(int blanks);
};

class for_astnode: public statement_astnode
{
    public:
    exp_astnode* init;
    exp_astnode* guard;
    exp_astnode* step;
    statement_astnode* body;
    for_astnode(exp_astnode* arg1, exp_astnode* arg2, exp_astnode* arg3, statement_astnode* arg4);
    void print(int blanks);
};

class proccall_astnode: public statement_astnode
{
    public:
    vector<exp_astnode*> args;
    proccall_astnode(vector<exp_astnode*> args);
    void print(int blanks);
};

class identifier_astnode: public ref_astnode
{
    public:
    string id;
    identifier_astnode(string arg);
    void print(int blanks);
};

class arrayref_astnode: public ref_astnode
{
    public:
    exp_astnode* array;
    exp_astnode* index;
    arrayref_astnode(exp_astnode* arg1, exp_astnode* arg2);
    void print(int blanks);
};

class member_astnode: public ref_astnode
{
    public:
    exp_astnode* struct_arg;
    identifier_astnode* field;
    member_astnode(exp_astnode* arg1, identifier_astnode* arg2);
    void print(int blanks);
};

class arrow_astnode: public ref_astnode
{
    public:
    exp_astnode* pointer;
    identifier_astnode* field;
    arrow_astnode(exp_astnode* arg1, identifier_astnode* arg2);
    void print(int blanks);
};

class op_binary_astnode: public exp_astnode
{
    public:
    string op;
    exp_astnode* arg1;
    exp_astnode* arg2;
    op_binary_astnode(string arg1, exp_astnode* arg2, exp_astnode* arg3);
    void print(int blanks);
};

class op_unary_astnode: public exp_astnode
{
    public:
    string arg1;
    exp_astnode* arg2;
    op_unary_astnode(string arg1, exp_astnode* arg2);
    void print(int blanks);
};

class assignE_astnode: public exp_astnode
{
    public:
    exp_astnode *left, *right;
    assignE_astnode(exp_astnode* arg1, exp_astnode* arg2);
    void print(int blanks);
};

class funcall_astnode: public exp_astnode
{
    public:
    vector<exp_astnode*> args;
    funcall_astnode(vector<exp_astnode*> args);
    void print(int blanks);
};

class intconst_astnode: public exp_astnode
{
    public:
    int id;
    intconst_astnode(int arg1);
    void print(int blanks);
};

class floatconst_astnode: public exp_astnode
{
    public:
    float id;
    floatconst_astnode(float arg1);
    void print(int blanks);
};

class stringconst_astnode: public exp_astnode
{
    public:
    string id;
    stringconst_astnode(string arg1);
    void print(int blanks);
};

extern map<string, string> declared_symbols;
// extern map<string, string> functions;
// extern vector<string> struct_fun;
// extern vector<int> nature;
// extern map<string, map<string, string> > struct_localST;
// extern map<string, map<string, string> > func_var;
// extern map<string, map<string, string> > func_param;
// extern vector<statement_astnode*> my_statement_list;
extern vector<string> scope;
// extern map<string, int> func_param_length;
// extern string return_type;
// extern int is_return;   
extern map<string, statement_astnode*> ASTs;