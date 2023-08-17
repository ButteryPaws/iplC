#include <cstring>
#include <cstddef>
#include <istream>
#include <iostream>
#include <fstream>

#include "scanner.hh"
#include "parser.tab.hh"
using namespace std;

// New Variables
map<string, SymTab> tables;
map<string, statement_astnode*> ASTs;
map<string, string> declared_symbols;
vector<string> scope;

//Global code strings
vector<string> print_codes;
uint max_temp = 0;
uint curr_temp = 0;
uint jump_label = 2;
string code = "";

struct member_info{
  string type;
  int offset;
  string base;
};

struct array_info{
  string type;
  int base_value;
  int offset;
  string base;
};

void print_boilerplate();

void print_start(string filepath);
bool check_if_comparison(op_binary_astnode* node);
void genASTcode(map<string, statement_astnode*>::iterator it);
void sethi_ullman(exp_astnode* tree_root, string funcname, vector<string> &rstack);
void label_nodes(exp_astnode* tree_root);
void gencode(exp_astnode* n, vector<string> &rstack, string funcname);
void gen_statement_code(statement_astnode* stmt, map<string, statement_astnode*>::iterator it);
void gen_assignment_code(assignE_astnode* node, map<string, statement_astnode*>::iterator it);
int get_type_size(string type);
member_info get_struct_offset(member_astnode* n, string funcname, vector<string> &rstack);
member_info get_array_offset(arrayref_astnode* n, string funcname, vector<string> &rstack);
void recursive_deref(op_unary_astnode* deref_node, int depth, string reg, string funcname);
string deref_string(string type, int depth);
string recursive_add_or(op_binary_astnode* node, string funcname, vector<string> &rstack, int jump);

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
  for(auto it = ASTs.begin(); it != ASTs.end(); it++)
  {
    tables.at(it->first).size = get_type_size(tables.at(it->first).ret_type);
  }

  for(auto it = ASTs.begin(); it != ASTs.end(); it++)
  {
    genASTcode(it);
  }
  if(print_codes.size() > 0)
  {
    print_codes[print_codes.size()-1].append("\t.text\n");
  }
  print_start(string(argv[1]));
  for(int i = 0; i < print_codes.size(); i++)
  {
    cout << ".LC" << i << ":" << endl;
    cout << print_codes[i];
  }
  cout << code;
  cout << "\t.ident\t\"GCC: (Ubuntu 8.1.0-9ubuntu1~16.04.york1) 8.1.0\"\n\t.section\t.note.GNU-stack,\"\",@progbits" << endl;

  // print_boilerplate();
}

void print_start(string filepath)
{
  size_t start = filepath.find_last_of('/');
  string filename = start == string::npos ? filepath : filepath.substr(start+1);
  cout << "\t.file\t\"" << filename << "\"" << endl;
  cout << "\t.text" << endl;
  for(uint i = 0; i < max_temp; i++)
  {
    cout << "\t.comm\t.t" << i << ",4,4" << endl;
  }
  if(print_codes.size() > 0)
  {
    cout << "\t.section\t.rodata\n\t.align 4" << endl;
  }
}

void genASTcode(map<string, statement_astnode*>::iterator it)
{
  code.append("\t.globl\t" + it->first + "\n\t.type\t" + it->first + ", @function\n");
  code.append(it->first + ":\n");
  code.append("\tpushl\t%ebp\n");
  code.append("\tmovl\t%esp, %ebp\n");
  //Calculating max offset
  int offset_min = 0;
  for(auto it2 = tables.at(it->first).entries.begin(); it2 != tables.at(it->first).entries.end(); it2++)
  {
    offset_min = min(offset_min, it2->second.offset);
  }
  code.append("\tsubl\t$" + to_string(-offset_min) + ", %esp\n");
  //it->second is a pointer to a seq_astnode
  seq_astnode* seq_statements = (seq_astnode *) it->second;
  for(auto stmt: seq_statements->seq)
  {
    gen_statement_code(stmt, it);
  }
  if(it->first == "main") code.append("\tmovl\t$0, %eax\n\tleave\n\tret\n\t.size\t" + it->first + ", .-" + it->first + "\n");
  else code.append("\tleave\n\tret\n\t.size\t" + it->first + ", .-" + it->first + "\n");
}

void gen_statement_code(statement_astnode* stmt, map<string, statement_astnode*>::iterator it)
{
  if(stmt->nodetype == PROCCALL_ASTNODE)
  {
    proccall_astnode* proc_stmt = (proccall_astnode *) stmt;
    identifier_astnode* fname = (identifier_astnode *) (proc_stmt->args[0]);
    if(fname->id == "printf")
    {
      int num_args = proc_stmt->args.size() - 1;
      if(num_args > 1)
      {
        for(int i = num_args; i > 1; i--)
        {
          if(proc_stmt->args[i]->nodetype == INTCONST_ASTNODE)
          {
            code.append("\tpushl\t$" + to_string(proc_stmt->args[i]->value));
            code.append("\n");
          }
          else if(proc_stmt->args[i]->nodetype == IDENTIFIER_ASTNODE)
          {
            identifier_astnode* var_astnode = (identifier_astnode *) proc_stmt->args[i];
            string var_name = var_astnode->id;
            code.append("\tpushl\t" + to_string(tables.at(it->first).entries.at(var_name).offset));
            code.append("(%ebp)\n");
          }
          else
          {
            exp_astnode* exp_tree = (exp_astnode *) (proc_stmt->args[i]);
            vector<string> rstack {"%eax", "%edx", "%ebx", "%ecx", "%esi", "%edi"};
            sethi_ullman(exp_tree, it->first, rstack);
            code.append("\tpushl\t%edi\n");
          }
        }
        code.append("\tpushl\t$.LC" + to_string(print_codes.size()) + "\n");
        stringconst_astnode* format_string_astnode = (stringconst_astnode *) (proc_stmt->args[1]);
        print_codes.push_back("\t.string\t" + format_string_astnode->id + "\n");
        code.append("\tcall\tprintf\n");
        code.append("\taddl\t$" + to_string(4 * num_args) + ", %esp\n");
      }
      else
      {
        code.append("\tpushl\t$.LC" + to_string(print_codes.size()) + "\n");
        stringconst_astnode* format_string_astnode = (stringconst_astnode *) (proc_stmt->args[1]);
        if(format_string_astnode->id.back() == '\n')
        {
          print_codes.push_back("\t.string\t" + format_string_astnode->id.substr(0, format_string_astnode->id.length() - 1) + "\n");
          code.append("\tcall\tputs\n");
        }
        else
        {
          print_codes.push_back("\t.string\t" + format_string_astnode->id + "\n");
          code.append("\tcall\tprintf\n");
        }
        code.append("\taddl\t$4");
        code.append(", %esp\n");
      }
    }
    else
    {
      //make space for return value
      if((tables.at(fname->id).size) > 0) code.append("\tsubl\t$" + to_string(tables.at(fname->id).size) + ", %esp\n");
      //push args
      int num_args = proc_stmt->args.size() - 1;
      for(int i = 1; i <= num_args; i++)
      {
        if(proc_stmt->args[i]->nodetype == INTCONST_ASTNODE)
        {
          code.append("\tpushl\t$" + to_string(proc_stmt->args[i]->value));
          code.append("\n");
        }
        else if(proc_stmt->args[i]->nodetype == IDENTIFIER_ASTNODE)
        {
          identifier_astnode* var_astnode = (identifier_astnode *) proc_stmt->args[i];
          string var_name = var_astnode->id;
          int var_offset = tables.at(it->first).entries.at(var_name).offset;
          int var_size = tables.at(it->first).entries.at(var_name).size;
          int iter = var_offset + var_size - 4;
          while(iter >= var_offset)
          {
            code.append("\tpushl\t" + to_string(iter) + "(%ebp)\n");
            iter -= 4;
          }
        }
        else
        {
          //Check if argument is an int or struct
          string arg_type;
          for(map<string, SymTabEntry>::iterator entry = tables.at(fname->id).entries.begin(); entry != tables.at(fname->id).entries.end(); entry++)
          {
            if(entry->second.order == i - 1 && entry->second.loc == "param") arg_type = entry->second.type;
          }
          exp_astnode* exp_tree = (exp_astnode *) (proc_stmt->args[i]);
          vector<string> rstack {"%eax", "%edx", "%ebx", "%ecx", "%esi", "%edi"};
          sethi_ullman(exp_tree, it->first, rstack);
          if(tables.find(arg_type) == tables.end())
          {
            code.append("\tpushl\t%edi\n");
          }
          // else if(tables.find(arg_type) != tables.end()) //struct arg type
          else
          {
            int var_size = tables.at(arg_type).size;
            int iter = var_size - 4;
            while(iter >= 0)
            {
              code.append("\tpushl\t" + to_string(iter) + "(%edi)\n");
              iter -= 4;
            }
          }
        }
      }
      code.append("\tsubl\t$4, %esp\n");
      code.append("\tcall\t" + fname->id + "\n");
      //Get size of arguments pushed
      int size = 0;
      for(map<string, SymTabEntry>::iterator fun_vars = tables.at(fname->id).entries.begin(); fun_vars != tables.at(fname->id).entries.end(); fun_vars++)
      {
        if(fun_vars->second.loc == "param") size += fun_vars->second.size;
      }
      code.append("\taddl\t$" + to_string(size + 4 + tables.at(fname->id).size) + ", %esp\n");
    }
  }
  else if(stmt->nodetype == ASSIGNS_ASTNODE)
  {
    assignS_astnode* full_stmt = (assignS_astnode *) stmt;
    assignE_astnode* fullE_stmt = new assignE_astnode(full_stmt->left, full_stmt->right);
    gen_assignment_code(fullE_stmt, it);
  }
  else if (stmt->nodetype == IF_ASTNODE)
  {
    if_astnode* node = (if_astnode *) stmt;
    if (node->cond->nodetype == IDENTIFIER_ASTNODE)
    {
      identifier_astnode* cond_id = (identifier_astnode *) node->cond;
      string cond_id_id = cond_id->id;
      int offset = tables.at(it->first).entries.at(cond_id_id).offset;
      int else_label = jump_label;
      int out_label = else_label + 1;
      jump_label += 2;
      code.append("\tcmpl\t$0, " + to_string(offset) + "(%ebp)\n");
      code.append("\tje\t.L" + to_string(else_label) + "\n");
      gen_statement_code(node->then, it);
      code.append("\tjmp\t.L" + to_string(out_label) + "\n");
      code.append(".L" + to_string(else_label) + ":\n");
      gen_statement_code(node->else_statement, it);
      code.append("\tjmp\t.L" + to_string(out_label) + "\n");
      code.append(".L" + to_string(out_label) + ":\n");
    }
    else if (node->cond->nodetype == INTCONST_ASTNODE)
    {
      intconst_astnode* cond_num = (intconst_astnode *) node->cond;
      int cond_num_id = cond_num->id;
      if(cond_num_id == 0)
      {
        gen_statement_code(node->else_statement, it);
        return;
      }
      else
      {
        gen_statement_code(node->then, it);
        return;
      }
    }
    else if(node->cond->nodetype == OP_BINARY_ASTNODE)
    {
      op_binary_astnode* binary_exp = (op_binary_astnode *) node->cond;
      vector<string> rstack {"%eax", "%edx", "%ebx", "%ecx", "%esi", "%edi"};
      sethi_ullman(binary_exp, it->first, rstack);
      int else_label = jump_label;
      int out_label = else_label + 1;
      jump_label += 2;
      code.append("\tcmpl\t$0, %edi\n");
      code.append("\tje\t.L" + to_string(else_label) + "\n");
      gen_statement_code(node->then, it);
      code.append("\tjmp\t.L" + to_string(out_label) + "\n");
      code.append(".L" + to_string(else_label) + ":\n");
      gen_statement_code(node->else_statement, it);
      code.append("\tjmp\t.L" + to_string(out_label) + "\n");
      code.append(".L" + to_string(out_label) + ":\n");
    }
  }
  else if (stmt->nodetype == WHILE_ASTNODE)
  {
    while_astnode* while_stmt = (while_astnode *) stmt;
    if(while_stmt->cond->nodetype == INTCONST_ASTNODE)
    {
      int loop_label = jump_label++;
      intconst_astnode* cond_num = (intconst_astnode *) while_stmt->cond;
      int cond_num_id = cond_num->id;
      if(cond_num_id)
      {
        code.append(".L" + to_string(loop_label) + "\n");
        gen_statement_code(while_stmt->stmt, it);
        code.append("jmp\t.L" + to_string(loop_label) + "\n");
        return;
      }
      else
      {
        return;
      }
    }
    int true_label = jump_label;
    int cond_label = true_label + 1;
    jump_label += 2;
    code.append("\tjmp\t.L" + to_string(cond_label) + "\n");
    code.append(".L" + to_string(true_label) + ":\n");
    gen_statement_code(while_stmt->stmt, it);
    code.append(".L" + to_string(cond_label) + ":\n");
    if (while_stmt->cond->nodetype == IDENTIFIER_ASTNODE)
    {
      identifier_astnode* cond_id = (identifier_astnode *) while_stmt->cond;
      string cond_id_id = cond_id->id;
      int offset = tables.at(it->first).entries.at(cond_id_id).offset;
      code.append("\tcmpl\t$0, " + to_string(offset) + "(%ebp)\n");
      code.append("\tjne\t.L" + to_string(true_label) + "\n");
    }
    else if(while_stmt->cond->nodetype == OP_BINARY_ASTNODE)
    {
      op_binary_astnode* binary_exp = (op_binary_astnode *) while_stmt->cond;
      vector<string> rstack {"%eax", "%edx", "%ebx", "%ecx", "%esi", "%edi"};
      sethi_ullman(binary_exp, it->first, rstack);
      code.append("\tcmpl\t$0, %edi\n");
      code.append("\tjne\t.L" + to_string(true_label) + "\n");
    }
  }
  else if (stmt->nodetype == FOR_ASTNODE)
  {
    for_astnode* for_stmt = (for_astnode *) stmt;
    gen_assignment_code((assignE_astnode *)(for_stmt->init), it);
    int guard_label = jump_label;
    int body_label = guard_label + 1;
    jump_label += 2;
    code.append("\tjmp\t.L" + to_string(guard_label) + "\n");
    code.append(".L" + to_string(body_label) + ":\n");
    gen_statement_code(for_stmt->body, it);
    gen_assignment_code((assignE_astnode *)(for_stmt->step), it);
    code.append(".L" + to_string(guard_label) + ":\n");
    vector<string> rstack {"%eax", "%edx", "%ebx", "%ecx", "%esi", "%edi"};
    sethi_ullman(for_stmt->guard, it->first, rstack);
    code.append("\tcmpl\t$0, %edi\n");
    code.append("\tjne\t.L" + to_string(body_label) + "\n");
  }
  else if (stmt->nodetype == RETURN_ASTNODE && it->first != "main")
  {
    return_astnode* return_node = (return_astnode *) stmt;
    vector<string> rstack {"%eax", "%edx", "%ebx", "%ecx", "%esi", "%edi"};
    sethi_ullman(return_node->arg, it->first, rstack);
    int params_size = 0;
    for(auto temp = tables.at(it->first).entries.begin(); temp != tables.at(it->first).entries.end(); temp++)
    {
      if(temp->second.loc == "param") params_size += temp->second.size;
    }
    if (tables.at(it->first).ret_type == "int")
    {
      code.append("\tmovl\t%edi," + to_string(12 + params_size) + "(%ebp)\n");
    }
    else if (tables.find(tables.at(it->first).ret_type) != tables.end())
    {
      int var_size = tables.at(tables.at(it->first).ret_type).size;
      int iter = 0;
      while(iter < var_size)
      {
        code.append("\tmovl\t" + to_string(iter) + "(%edi), %esi\n");
        code.append("\tmovl\t%esi, " + to_string(12 + params_size + iter) + "(%ebp)\n");
        iter += 4;
      }
    }
  }
  else if (stmt->nodetype == SEQ_ASTNODE)
  {
    seq_astnode* seq = (seq_astnode *) stmt;
    for(auto seq_stmt: seq->seq)
    {
      gen_statement_code(seq_stmt, it);
    }
  }
}

int get_type_size(string type)
{
  int size = 4;
  if(type == "void") return 0;
  int str_len = type.size();
  int i = 0;
  while(type[i] != '*' && type[i] != '[' && i < str_len) i++;
  // Eg : int, float, struct person
  if(i == str_len){
      if(type.substr(0, i) != "int" && type.substr(0, i) != "void")
      {
          size = tables.at(type.substr(0, i)).size;
      }
      else{
          size = 4;
      }
  map<int, string> ordered_attrs; //map from order of entries to their name
  }
  if(type[i] == '*'){
      size = 4;
  }
  else{
      if(type.substr(0, i) != "int" && type.substr(0, i) != "void")
      {
          size = tables.at(type.substr(0, i)).size;
      }
      else{
          size = 4;
      }
  } 
  while(type[i] != '[' && i < str_len) i++;
  if(i==str_len) return size; // Eg : int*, struct s2***
  i++;
  while(i < str_len)
  {
      int j = i;
      while(type[j] != ']') j++;
      size *= stoi(type.substr(i, j - i));
      i = j + 2;
  }
  return size;
}

void gen_assignment_code(assignE_astnode* node, map<string, statement_astnode*>::iterator it)
{
  //Trivial case
  if(node->right->nodetype == INTCONST_ASTNODE)
  {
    intconst_astnode* rhs = (intconst_astnode *) (node->right);
    if(node->left->nodetype == IDENTIFIER_ASTNODE)
    {
      identifier_astnode* lhs = (identifier_astnode *) (node->left);
      string lhs_id = lhs->id;
      int offset = tables.at(it->first).entries.at(lhs_id).offset;
      code.append("\tmovl\t$" + to_string(rhs->id) + ", " + to_string(offset) + "(%ebp)\n");
      return;
    }
  }
  vector<string> rstack{ "%eax", "%edx", "%ebx", "%ecx", "%esi", "%edi" };
  sethi_ullman(node->right, it->first, rstack);
  string R = rstack.back();
  rstack.pop_back();
  if(node->left->nodetype == IDENTIFIER_ASTNODE)
  {
    identifier_astnode* lhs = (identifier_astnode *) (node->left);
    string lhs_id = lhs->id;
    int offset = tables.at(it->first).entries.at(lhs_id).offset;
    // if(tables.at(it->first).entries.at(lhs_id).type == "int")
    if(tables.find(tables.at(it->first).entries.at(lhs_id).type) == tables.end())
    {
      code.append("\tmovl\t" + R + ", " + to_string(offset) + "(%ebp)\n");
    }
    // else if(tables.find(tables.at(it->first).entries.at(lhs_id).type) != tables.end()) //lhs is struct or not
    else
    {
      int size = tables.at(it->first).entries.at(lhs_id).size;
      int iter = 0;
      string temp = rstack.back();
      while(iter < size)
      {
        code.append("\tmovl\t" + to_string(iter) + "(" + R + "), " + temp + "\n");
        code.append("\tmovl\t" + temp + ", " + to_string(offset + iter) + "(%ebp)\n");
        iter += 4;
      }
    }
    return;
  }
  else if(node->left->nodetype == MEMBER_ASTNODE || node->left->nodetype == ARROW_ASTNODE)
  {
    if(node->left->nodetype == ARROW_ASTNODE)
    {
      arrow_astnode* arrow_node = (arrow_astnode *) (node->left);
      exp_astnode* temp = arrow_node->pointer;
      op_unary_astnode* inter = new op_unary_astnode("DEREF", temp);
      ((member_astnode *)(node->left))->struct_arg = inter;
      ((member_astnode *)(node->left))->nodetype = MEMBER_ASTNODE;
      inter->nodetype = OP_UNARY_ASTNODE;
      inter->treenodetype = UNARYNODE;
      inter->type = deref_string(inter->arg2->type, 1);
    }
    member_info lhs_info = get_struct_offset((member_astnode *)node->left, it->first, rstack);
    // if(lhs_info.type == "int")
    if(tables.find(lhs_info.type) == tables.end())
    {
      code.append("\tmovl\t%edi, " + to_string(lhs_info.offset) + "(" + lhs_info.base + ")\n");
    }
    // else if (tables.find(lhs_info.type) != tables.end())
    else
    {
      int size = get_type_size(lhs_info.type);
      int iter = 0;
      while(iter < size)
      {
        code.append("\tmovl\t" + to_string(iter) + "(%edi), %esi\n");
        code.append("\tmovl\t%esi, " + to_string(lhs_info.offset + iter) + "(%ebp)\n");
        iter += 4;
      }
    }
  }
  else if(node->left->nodetype == OP_UNARY_ASTNODE)
  {
    op_unary_astnode* deref_node = (op_unary_astnode *) (node->left);
    recursive_deref(deref_node, 0, R, it->first);
  }
}

void recursive_deref(op_unary_astnode* deref_node, int depth, string reg, string funcname)
{
  if(deref_node->arg2->nodetype == OP_UNARY_ASTNODE)
  {
    op_unary_astnode* child = (op_unary_astnode *) deref_node->arg2;
    if(child->arg1 == "DEREF")
    {
      recursive_deref(child, depth + 1, reg, funcname);
    }
    else if(child->arg1 == "ADDRESS")
    {
      recursive_deref(child, depth - 1, reg, funcname);
    }
  }
  else if(deref_node->arg2->nodetype == IDENTIFIER_ASTNODE)
  {
    identifier_astnode* child = (identifier_astnode *) deref_node->arg2;
    if(depth == -1)
    {
      if(tables.find(tables.at(funcname).entries.at(child->id).type) == tables.end())
      {
        code.append("\tmovl\t" + reg + ", " + to_string(tables.at(funcname).entries.at(child->id).offset) + "(%ebp)\n");
        return;
      }
      else
      {
        int size = get_type_size(tables.at(funcname).entries.at(child->id).type);
        int iter = 0;
        while(iter < size)
        {
          code.append("\tmovl\t" + to_string(iter) + "(" + reg + "), %esi\n");
          code.append("\tmovl\t%esi, " + to_string(tables.at(funcname).entries.at(child->id).offset + iter) + "(%ebp)\n");
          iter += 4;
        }
        return;
      }
    }
    if(tables.find(deref_string(tables.at(funcname).entries.at(child->id).type, depth + 1)) == tables.end())
    {
      code.append("\tmovl\t" + to_string(tables.at(funcname).entries.at(child->id).offset) + "(%ebp), %eax\n");
      for(int i = 0; i < depth; i++)
      {
        code.append("\tmovl\t(%eax), %eax\n");
      }
      code.append("\tmovl\t" + reg + ", " + "(%eax)\n");
    }
    else
    {
      code.append("\tmovl\t" + to_string(tables.at(funcname).entries.at(child->id).offset) + "(%ebp), %eax\n");
      for(int i = 0; i < depth; i++)
      {
        code.append("\tmovl\t(%eax), %eax\n");
      }
      int size = get_type_size(tables.at(funcname).entries.at(child->id).type);
      int iter = 0;
      while(iter < size)
      {
        code.append("\tmovl\t" + to_string(iter) + "(" + reg + "), %esi\n");
        code.append("\tmovl\t%esi, " + to_string(tables.at(funcname).entries.at(child->id).offset + iter) + "(%ebp)\n");
        iter += 4;
      }
    }
  }
  else if(deref_node->arg2->nodetype == MEMBER_ASTNODE)
  {
    vector<string> rstack{ "%eax", "%edx", "%ebx", "%ecx", "%esi" };
    member_info lhs_info = get_struct_offset((member_astnode *)deref_node->arg2, funcname, rstack);
    // if(lhs_info.type == "int")
    if(depth == -1)
    {
      if(tables.find(lhs_info.type) == tables.end())
      {
        code.append("\tmovl\t" + reg + ", " + to_string(lhs_info.offset) + "(%ebp)\n");
        return;
      }
      else
      {
        int size = get_type_size(lhs_info.type);
        int iter = 0;
        while(iter < size)
        {
          code.append("\tmovl\t" + to_string(iter) + "(" + reg + "), %esi\n");
          code.append("\tmovl\t%esi, " + to_string(lhs_info.offset + iter) + "(%ebp)\n");
          iter += 4;
        }
        return;
      }
    }
    string new_type = deref_string(lhs_info.type, depth + 1);
    if(tables.find(new_type) == tables.end())
    {
      code.append("\tmovl\t" + to_string(lhs_info.offset) + "(%ebp), %eax\n");
      for(int i = 0; i < depth; i++)
      {
        code.append("\tmovl\t(%eax), %eax\n");
      }
      code.append("\tmovl\t" + reg + ", " + "(%eax)\n");
    }
    // else if (tables.find(lhs_info.type) != tables.end())
    else
    {
      code.append("\tmovl\t" + to_string(lhs_info.offset) + "(%ebp), %eax\n");
      for(int i = 0; i < depth; i++)
      {
        code.append("\tmovl\t(%eax), %eax\n");
      }
      int size = get_type_size(new_type);
      int iter = 0;
      while(iter < size)
      {
        code.append("\tmovl\t" + to_string(iter) + "(" + reg + "), %esi\n");
        code.append("\tmovl\t%esi, " + to_string(iter) + "(%eax)\n");
        iter += 4;
      }
    }
  }
}

bool check_if_comparison(op_binary_astnode* node)
{
  if(node->op.find("EQ_OP") != string::npos || node->op.find("NE_OP") != string::npos || node->op.find("LT_OP") != string::npos || node->op.find("GT_OP") != string::npos || node->op.find("LE_OP") != string::npos || node->op.find("GE_OP") != string::npos) return true;
  return false;
}

void sethi_ullman(exp_astnode* tree_root, string funcname, vector<string> &rstack)
{
  //Stage 1: Labelling;
  label_nodes(tree_root);
  //Stage 2: Generate code
  curr_temp = 0;
  tree_root->treenodetype = LEFTCHILD;
  gencode(tree_root, rstack, funcname);
}

void label_nodes(exp_astnode* tree_root)
{
  if(tree_root->nodetype == OP_BINARY_ASTNODE)
  {
    op_binary_astnode* binary_node = (op_binary_astnode *) tree_root;
    binary_node->arg1->treenodetype = LEFTCHILD;
    binary_node->arg2->treenodetype = RIGHTCHILD;
    if(get_asterick_length(binary_node->arg1->type)>0 && get_asterick_length(binary_node->arg2->type)==0 && binary_node->arg2->treenodetype==IDENTIFIER_ASTNODE)
    {
      label_nodes(binary_node->arg1);
      binary_node->arg2->label = 1;
    }
    else
    {
      label_nodes(binary_node->arg1);
      label_nodes(binary_node->arg2);
    }
    if(binary_node->arg1->label == binary_node->arg2->label)
    {
      binary_node->label = binary_node->arg1->label + 1;
    }
    else
    {
      binary_node->label = max(binary_node->arg1->label, binary_node->arg2->label);
    }
  }
  else if(tree_root->nodetype == FUNCALL_ASTNODE)
  {
    funcall_astnode* fun_node = (funcall_astnode *) tree_root;
    fun_node->treenodetype = FUNCALL;
    for(int i = 1; i < fun_node->args.size(); i++)
    {
      fun_node->args[i]->treenodetype = FUNCARG;
      label_nodes(fun_node->args[i]);
    }
    fun_node->label = 1;
  }
  else if(tree_root->nodetype == MEMBER_ASTNODE)
  {
    member_astnode* mem_node = (member_astnode *) tree_root;
    mem_node->treenodetype = STRUCT_MEMBER;
    label_nodes(mem_node->struct_arg);
    label_nodes(mem_node->field);
    mem_node->label = mem_node->struct_arg->label;
  }
  else if(tree_root->nodetype == ARROW_ASTNODE)
  {
    arrow_astnode* arrow_node = (arrow_astnode *) tree_root;
    exp_astnode* temp = arrow_node->pointer;
    op_unary_astnode* inter = new op_unary_astnode("DEREF", temp);
    ((member_astnode *)(tree_root))->struct_arg = inter;
    ((member_astnode *)(tree_root))->nodetype = MEMBER_ASTNODE;
    inter->nodetype = OP_UNARY_ASTNODE;
    inter->treenodetype = UNARYNODE;
    inter->type = deref_string(inter->arg2->type, 1);
    label_nodes(tree_root);
  }
  else if(tree_root->nodetype == OP_UNARY_ASTNODE)
  {
    op_unary_astnode* unary_node = (op_unary_astnode *) tree_root;
    // if(unary_node->arg1 == "ADDRESS")
    // {
    //   unary_node->treenodetype = UNARYNODE;
    //   unary_node->arg2->label = LEFTCHILD;
    //   label_nodes(unary_node->arg2);
    //   unary_node->label = unary_node->arg2->label;
    // }
    // else if(unary_node->arg1 == "DEREF")
    // {
    //   unary_node->treenodetype = UNARYNODE;
    //   unary_node->arg2->treenodetype = LEFTCHILD;
    //   label_nodes(unary_node->arg2);
    //   unary_node->label = unary_node->arg2->label;
    // }
    // else if(unary_node->arg1 == "NOT" || unary_node->arg1 == "UMINUS" || unary_node->arg1 == "PP")
    // {
      unary_node->treenodetype = UNARYNODE;
      unary_node->arg2->treenodetype = LEFTCHILD;
      label_nodes(unary_node->arg2);
      unary_node->label = unary_node->arg2->label;
    // }
  }
  else if(tree_root->nodetype == ARRAYREF_ASTNODE)
  {
    arrayref_astnode* array_node = (arrayref_astnode *)(tree_root);
    array_node->treenodetype = ARRAY_REF;
    label_nodes(array_node->array);
    label_nodes(array_node->index);
    array_node->label = array_node->array->label;
  }
  else
  {
    if(tree_root->treenodetype == RIGHTCHILD) tree_root->label = 0;
    else tree_root->label = 1;
  }
}

void gencode(exp_astnode* n, vector<string> &rstack, string funcname)
{
  //Case 1
  if(n->treenodetype == LEFTCHILD && n->nodetype == INTCONST_ASTNODE)
  {
    intconst_astnode* leaf = (intconst_astnode *) n;
    code.append("\tmovl\t$" + to_string(leaf->id) + ", " + rstack.back() + "\n");
  }
  else if(n->treenodetype == LEFTCHILD && n->nodetype == IDENTIFIER_ASTNODE)
  {
    identifier_astnode* leaf = (identifier_astnode *) n;
    int offset = tables.at(funcname).entries.at(leaf->id).offset;
    if(tables.find(tables.at(funcname).entries.at(leaf->id).type) == tables.end()) //int, int*, struct*
    {
      code.append("\tmovl\t" + to_string(offset) + "(%ebp), " + rstack.back() + "\n");
    }
    // else if(tables.find(tables.at(funcname).entries.at(leaf->id).type) != tables.end()) //struct
    else
    {
      code.append("\tleal\t" + to_string(offset) + "(%ebp), " + rstack.back() + "\n");
    }
  }
  //Case 2
  else if(n->nodetype == OP_BINARY_ASTNODE)
  {
    op_binary_astnode* node = (op_binary_astnode *) n;

    string op;
    if (node->op.find("PLUS") != string::npos) op = "addl";
    else if (node->op.find("MINUS") != string::npos) op = "subl";
    else if (node->op.find("MULT") != string::npos) op = "imull";
    else if (node->op.find("DIV") != string::npos) op = "idivl";
    else if (check_if_comparison(node)) op = "cmpl";

    if(node->arg2->treenodetype == RIGHTCHILD && (node->arg2->nodetype == INTCONST_ASTNODE || node->arg2->nodetype == IDENTIFIER_ASTNODE))
    {
      gencode(node->arg1, rstack, funcname);
      if(op == "idivl")
      {
        if(rstack.back() == "%eax")
        {
          if(node->arg2->nodetype == IDENTIFIER_ASTNODE)
          {
            identifier_astnode* temp = (identifier_astnode *) (node->arg2);
            string temp_id = temp->id;
            int offset = tables.at(funcname).entries.at(temp_id).offset;
            code.append("\tcltd\n");
            code.append("\tidivl\t" + to_string(offset) + "(%ebp)\n");
          }
          else if(node->arg2->nodetype == INTCONST_ASTNODE)
          {
            intconst_astnode* temp = (intconst_astnode *) (node->arg2);
            int temp_id = temp->id;
            string T = ".t" + to_string(curr_temp);
            curr_temp += 1;
            max_temp = max(max_temp, curr_temp);
            code.append("\tmovl\t$" + to_string(temp_id) + ", " + T + "\n");
            code.append("\tcltd\n");
            code.append("\tidivl\t" + T + "\n");
            curr_temp -= 1;
          }
        }
        else
        {
          string T = ".t" + to_string(curr_temp);
          curr_temp += 1;
          max_temp = max(max_temp, curr_temp);
          code.append("\tmovl\t%eax, " + T + "\n");
          code.append("\tmovl\t" + rstack.back() + ", %eax\n");
          if(node->arg2->nodetype == IDENTIFIER_ASTNODE)
          {
            identifier_astnode* temp = (identifier_astnode *) (node->arg2);
            string temp_id = temp->id;
            int offset = tables.at(funcname).entries.at(temp_id).offset;
            code.append("\tcltd\n");
            code.append("\tidivl\t" + to_string(offset) + "(%ebp)\n");
          }
          else if(node->arg2->nodetype == INTCONST_ASTNODE)
          {
            intconst_astnode* temp = (intconst_astnode *) (node->arg2);
            int temp_id = temp->id;
            string T2 = ".t" + to_string(curr_temp);
            curr_temp += 1;
            max_temp = max(max_temp, curr_temp);
            code.append("\tmovl\t$" + to_string(temp_id) + ", " + T2 + "\n");
            code.append("\tcltd\n");
            code.append("\tidivl\t" + T2 + "\n");
            curr_temp -= 1;
          }
          code.append("\tmovl\t%eax, " + rstack.back() + "\n");
          code.append("\tmovl\t" + T + ", %eax\n");
          curr_temp -= 1;
        }
        return;
      }
      else if (node->op == "AND_OP"){
        code.append("\tcmpl\t$0, " + rstack.back() + "\n"); 
        code.append("\tje .L" + to_string(jump_label) + "\n");
        if(node->arg2->nodetype == INTCONST_ASTNODE)
        {
          intconst_astnode* temp = (intconst_astnode *) (node->arg2);
          int temp_id = temp->id;
          if(temp_id){
            code.append("\tjmp .L" + to_string(jump_label+1) + "\n");
          }
        }
        else{
          identifier_astnode* temp = (identifier_astnode *) (node->arg2);
          string temp_id = temp->id;
          int offset = tables.at(funcname).entries.at(temp_id).offset;
          code.append("\tcmpl\t$0, " + to_string(offset) + "(%ebp)\n"); 
          code.append("\tjne .L" + to_string(jump_label+1) + "\n");
        }
        code.append(".L" + to_string(jump_label) + ":\n"); 
        code.append("\tmovl	$0, " + rstack.back() + "\n");
        code.append("\tjmp	.L" + to_string(jump_label+2) + "\n");
        code.append(".L" + to_string(jump_label+1) + ":\n"); 
        code.append("\tmovl	$1, " + rstack.back() + "\n");
        code.append(".L" + to_string(jump_label+2) + ":\n"); 
        jump_label += 3;
        return;
      }
      else if (node->op == "OR_OP"){
        code.append("\tcmpl\t$0, " + rstack.back() + "\n"); 
        code.append("\tjne .L" + to_string(jump_label) + "\n");
        if(node->arg2->nodetype == INTCONST_ASTNODE)
        {
          intconst_astnode* temp = (intconst_astnode *) (node->arg2);
          int temp_id = temp->id;
          if(!temp_id){
            code.append("\tjmp .L" + to_string(jump_label+1) + "\n");
          }
        }
        else{
          identifier_astnode* temp = (identifier_astnode *) (node->arg2);
          string temp_id = temp->id;
          int offset = tables.at(funcname).entries.at(temp_id).offset;
          code.append("\tcmpl\t$0, " + to_string(offset) + "(%ebp)\n"); 
          code.append("\tje .L" + to_string(jump_label+1) + "\n");
        }
        code.append(".L" + to_string(jump_label) + ":\n"); 
        code.append("\tmovl	$1, " + rstack.back() + "\n");
        code.append("\tjmp	.L" + to_string(jump_label+2) + "\n");
        code.append(".L" + to_string(jump_label+1) + ":\n"); 
        code.append("\tmovl	$0, " + rstack.back() + "\n");
        code.append(".L" + to_string(jump_label+2) + ":\n"); 
        jump_label += 3;
        return;
      }
      //addl leaf, top(rstack) type
      string op1;
      if(node->arg2->nodetype == INTCONST_ASTNODE)
      {
        intconst_astnode* temp = (intconst_astnode *) (node->arg2);
        int temp_id = temp->id;
        // Left child is a pointer
        if(get_asterick_length(node->arg1->type)>0 && (op=="addl" || op=="subl"))
        {
          temp_id = temp_id*get_type_size(deref_string(node->arg1->type,1));
        }
        op1 = "$" + to_string(temp_id);
      }
      else if(node->arg2->nodetype == IDENTIFIER_ASTNODE)
      {
        identifier_astnode* temp = (identifier_astnode *) (node->arg2);
        string temp_id = temp->id;
        int offset = tables.at(funcname).entries.at(temp_id).offset;
        op1 = to_string(offset) + "(%ebp)";
        // Left child is a pointer
        if(get_asterick_length(node->arg1->type)>0 && (op=="addl" || op=="subl"))
        {
          string R = rstack.back();
          rstack.pop_back();
          code.append("\tmovl\t" + to_string(offset) + "(%ebp), " + rstack.back() + "\n");
          code.append("\timul\t$" + to_string(get_type_size(deref_string(node->arg1->type,1))) + ", " + rstack.back() + "\n");
          op1 = rstack.back();
          rstack.push_back(R); 
        }
      }
      code.append("\t" + op + "\t" + op1 + ", " + rstack.back() + "\n");
      if(op == "cmpl")
      {
        if (node->op.find("EQ_OP") != string::npos)
        {
          code.append("\tsete\t%al\n");
        }
        else if (node->op.find("NE_OP") != string::npos)
        {
          code.append("\tsetne\t%al\n");
        }
        else if (node->op.find("LT_OP") != string::npos)
        {
          code.append("\tsetl\t%al\n");
        }
        else if (node->op.find("LE_OP") != string::npos)
        {
          code.append("\tsetle\t%al\n");
        }
        else if (node->op.find("GT_OP") != string::npos)
        {
          code.append("\tsetg\t%al\n");
        }
        else if (node->op.find("GE_OP") != string::npos)
        {
          code.append("\tsetge\t%al\n");
        }
        code.append("\tmovzbl\t%al, " + rstack.back() + "\n");
      }
    }
    //Case 3
    else if((node->arg1->label < node->arg2->label) && (node->arg1->label < 6))
    {
      gencode(node->arg2, rstack, funcname);
      string R = rstack.back();
      rstack.pop_back();
      gencode(node->arg1, rstack, funcname);
      if(op == "idivl")
      {
        if(rstack.back() == "%eax")
        {
          code.append("\tcltd\n");
          code.append("\tidivl\t" + R + "\n");
        }
        else
        {
          string T = ".t" + to_string(curr_temp);
          curr_temp += 1;
          max_temp = max(max_temp, curr_temp);
          code.append("\tmovl\t%eax, " + T + "\n");
          code.append("\tmovl\t" + rstack.back() + ", %eax\n");
          code.append("\tcltd\n");
          code.append("\tidivl\t" + R + "\n");
          code.append("\tmovl\t%eax, " + rstack.back() + "\n");
          code.append("\tmovl\t" + T + ", %eax\n");
          curr_temp -= 1;
        }
        rstack.push_back(R);
        swap(rstack[rstack.size() - 1], rstack[rstack.size() - 2]);
        return;
      }
      if(node->op=="AND_OP")
      {
        code.append("\tcmpl\t$0, " + rstack.back() + "\n"); 
        code.append("\tje .L" + to_string(jump_label) + "\n");
        code.append("\tcmpl\t$0, " + R + "\n"); 
        code.append("\tjne .L" + to_string(jump_label+1) + "\n");
        code.append(".L" + to_string(jump_label) + ":\n"); 
        code.append("\tmovl	$0, " + rstack.back() + "\n");
        code.append("\tjmp	.L" + to_string(jump_label+2) + "\n");
        code.append(".L" + to_string(jump_label+1) + ":\n"); 
        code.append("\tmovl	$1, " + rstack.back() + "\n");
        code.append(".L" + to_string(jump_label+2) + ":\n"); 
        jump_label += 3;
        rstack.push_back(R);
        swap(rstack[rstack.size() - 1], rstack[rstack.size() - 2]);
        return;
      }
      else if(node->op=="OR_OP")
      {
        code.append("\tcmpl\t$0, " + rstack.back() + "\n"); 
        code.append("\tjne .L" + to_string(jump_label) + "\n");
        code.append("\tcmpl\t$0, " + R + "\n"); 
        code.append("\tje .L" + to_string(jump_label+1) + "\n");
        code.append(".L" + to_string(jump_label) + ":\n"); 
        code.append("\tmovl	$1, " + rstack.back() + "\n");
        code.append("\tjmp	.L" + to_string(jump_label+2) + "\n");
        code.append(".L" + to_string(jump_label+1) + ":\n"); 
        code.append("\tmovl	$0, " + rstack.back() + "\n");
        code.append(".L" + to_string(jump_label+2) + ":\n"); 
        rstack.push_back(R);
        swap(rstack[rstack.size() - 1], rstack[rstack.size() - 2]);
        jump_label += 3;
        return;
      }
      // Left child is a pointer
      if(get_asterick_length(node->arg1->type)>0 && (op=="addl" || op=="subl"))
      {
        code.append("\timul\t$" + to_string(get_type_size(deref_string(node->arg1->type, 1))) + ", " + R + "\n");  
      }
      code.append("\t" + op + "\t" + R + ", " + rstack.back() + "\n");
      if(op == "cmpl")
      {
        if (node->op.find("EQ_OP") != string::npos)
        {
          code.append("\tsete\t%al\n");
        }
        else if (node->op.find("NE_OP") != string::npos)
        {
          code.append("\tsetne\t%al\n");
        }
        else if (node->op.find("LT_OP") != string::npos)
        {
          code.append("\tsetl\t%al\n");
        }
        else if (node->op.find("LE_OP") != string::npos)
        {
          code.append("\tsetle\t%al\n");
        }
        else if (node->op.find("GT_OP") != string::npos)
        {
          code.append("\tsetg\t%al\n");
        }
        else if (node->op.find("GE_OP") != string::npos)
        {
          code.append("\tsetge\t%al\n");
        }
        code.append("\tmovzbl\t%al, " + rstack.back() + "\n");
      }
      rstack.push_back(R);
      swap(rstack[rstack.size() - 1], rstack[rstack.size() - 2]);
    }
    //Case 4
    else if((node->arg1->label >= node->arg2->label) && (node->arg2->label < 6))
    {
      gencode(node->arg1, rstack, funcname);
      string R = rstack.back();
      rstack.pop_back();
      gencode(node->arg2, rstack, funcname);
      if(op == "idivl")
      {
        if(R == "%eax")
        {
          code.append("\tcltd\n");
          code.append("\tidivl\t" + rstack.back() + "\n");
        }
        else
        {
          string T = ".t" + to_string(curr_temp);
          curr_temp += 1;
          max_temp = max(max_temp, curr_temp);
          code.append("\tmovl\t%eax, " + T + "\n");
          code.append("\tmovl\t" + R + ", %eax\n");
          code.append("\tcltd\n");
          code.append("\tidivl\t" + rstack.back() + "\n");
          code.append("\tmovl\t%eax, " + R + "\n");
          code.append("\tmovl\t" + T + ", %eax\n");
          curr_temp -= 1;
        }
        rstack.push_back(R);
        return;
      }
      if(node->op=="AND_OP")
      {
        code.append("\tcmpl\t$0, " + rstack.back() + "\n"); 
        code.append("\tje .L" + to_string(jump_label) + "\n");
        code.append("\tcmpl\t$0, " + R + "\n"); 
        code.append("\tjne .L" + to_string(jump_label+1) + "\n");
        code.append(".L" + to_string(jump_label) + ":\n"); 
        code.append("\tmovl	$0, " + R + "\n");
        code.append("\tjmp	.L" + to_string(jump_label+2) + "\n");
        code.append(".L" + to_string(jump_label+1) + ":\n"); 
        code.append("\tmovl	$1, " + R + "\n");
        code.append(".L" + to_string(jump_label+2) + ":\n"); 
        jump_label += 3;
        rstack.push_back(R);
        return;
      }
      else if(node->op=="OR_OP")
      {
        code.append("\tcmpl\t$0, " + rstack.back() + "\n"); 
        code.append("\tjne .L" + to_string(jump_label) + "\n");
        code.append("\tcmpl\t$0, " + R + "\n"); 
        code.append("\tje .L" + to_string(jump_label+1) + "\n");
        code.append(".L" + to_string(jump_label) + ":\n"); 
        code.append("\tmovl	$1, " + R + "\n");
        code.append("\tjmp	.L" + to_string(jump_label+2) + "\n");
        code.append(".L" + to_string(jump_label+1) + ":\n"); 
        code.append("\tmovl	$0, " + R + "\n");
        code.append(".L" + to_string(jump_label+2) + ":\n"); 
        rstack.push_back(R);
        jump_label += 3;
        return;
      }
      // Left child is a pointer
      if(get_asterick_length(node->arg1->type)>0 && (op=="addl" || op=="subl"))
      {
        code.append("\timul\t$" + to_string(get_type_size(deref_string(node->arg1->type, 1))) + ", " + rstack.back() + "\n");  
      }
      code.append("\t" + op + "\t" + rstack.back() + ", " + R + "\n");
      if(op == "cmpl")
      {
        if (node->op.find("EQ_OP") != string::npos)
        {
          code.append("\tsete\t%al\n");
        }
        else if (node->op.find("NE_OP") != string::npos)
        {
          code.append("\tsetne\t%al\n");
        }
        else if (node->op.find("LT_OP") != string::npos)
        {
          code.append("\tsetl\t%al\n");
        }
        else if (node->op.find("LE_OP") != string::npos)
        {
          code.append("\tsetle\t%al\n");
        }
        else if (node->op.find("GT_OP") != string::npos)
        {
          code.append("\tsetg\t%al\n");
        }
        else if (node->op.find("GE_OP") != string::npos)
        {
          code.append("\tsetge\t%al\n");
        }
        code.append("\tmovzbl\t%al, " + R + "\n");
      }
      rstack.push_back(R);
    }
    //Case 5
    else if((node->arg1->label >= 6) && (node->arg2->label >= 6))
    {
      gencode(node->arg2, rstack, funcname);
      string T = ".t" + to_string(curr_temp);
      curr_temp += 1;
      max_temp = max(max_temp, curr_temp);
      code.append("\tmovl\t" + rstack.back() + ", " + T + "\n");
      gencode(node->arg1, rstack, funcname);
      if(op == "idivl")
      {
        if(rstack.back() == "%eax")
        {
          code.append("\tcltd\n");
          code.append("\tidivl\t" + T + "\n");
        }
        else
        {
          string T1 = ".t" + to_string(curr_temp);
          curr_temp += 1;
          max_temp = max(max_temp, curr_temp);
          code.append("\tmovl\t%eax, " + T1 + "\n");
          code.append("\tmovl\t" + rstack.back() + ", %eax\n");
          code.append("\tcltd\n");
          code.append("\tidivl\t" + T + "\n");
          code.append("\tmovl\t%eax, " + rstack.back() + "\n");
          code.append("\tmovl\t" + T1 + ", %eax\n");
          curr_temp -= 1;
        }
        return;
      }
      curr_temp -= 1;
      if(node->op=="AND_OP")
      {
        code.append("\tcmpl\t$0, " + rstack.back() + "\n"); 
        code.append("\tje .L" + to_string(jump_label) + "\n");
        code.append("\tcmpl\t$0, " + T + "\n"); 
        code.append("\tjne .L" + to_string(jump_label+1) + "\n");
        code.append(".L" + to_string(jump_label) + ":\n"); 
        code.append("\tmovl	$0, " + rstack.back() + "\n");
        code.append("\tjmp	.L" + to_string(jump_label+2) + "\n");
        code.append(".L" + to_string(jump_label+1) + ":\n"); 
        code.append("\tmovl	$1, " + rstack.back() + "\n");
        code.append(".L" + to_string(jump_label+2) + ":\n"); 
        jump_label += 3;
        return;
      }
      else if(node->op=="OR_OP")
      {
        code.append("\tcmpl\t$0, " + rstack.back() + "\n"); 
        code.append("\tjne .L" + to_string(jump_label) + "\n");
        code.append("\tcmpl\t$0, " + T + "\n"); 
        code.append("\tje .L" + to_string(jump_label+1) + "\n");
        code.append(".L" + to_string(jump_label) + ":\n"); 
        code.append("\tmovl	$1, " + rstack.back() + "\n");
        code.append("\tjmp	.L" + to_string(jump_label+2) + "\n");
        code.append(".L" + to_string(jump_label+1) + ":\n"); 
        code.append("\tmovl	$0, " + rstack.back() + "\n");
        code.append(".L" + to_string(jump_label+2) + ":\n");
        jump_label += 3;
        return;
      }
      // Left child is a pointer
      if(get_asterick_length(node->arg1->type)>0 && (op=="addl" || op=="subl"))
      {
        code.append("\timul\t$" + to_string(get_type_size(deref_string(node->arg1->type, 1))) + ", " + T + "\n");  
      }
      code.append("\t" + op + "\t" + T + ", " + rstack.back() + "\n");
      if(op == "cmpl")
      {
        if (node->op.find("EQ_OP") != string::npos)
        {
          code.append("\tsete\t%al\n");
        }
        else if (node->op.find("NE_OP") != string::npos)
        {
          code.append("\tsetne\t%al\n");
        }
        else if (node->op.find("LT_OP") != string::npos)
        {
          code.append("\tsetl\t%al\n");
        }
        else if (node->op.find("LE_OP") != string::npos)
        {
          code.append("\tsetle\t%al\n");
        }
        else if (node->op.find("GT_OP") != string::npos)
        {
          code.append("\tsetg\t%al\n");
        }
        else if (node->op.find("GE_OP") != string::npos)
        {
          code.append("\tsetge\t%al\n");
        }
        code.append("\tmovzbl\t%al, " + rstack.back() + "\n");
      }
    }
  }
  //Case 6
  else if(n->nodetype == FUNCALL_ASTNODE)
  {
    funcall_astnode* fun_node = (funcall_astnode *) n;
    vector<string> used_registers;
    vector<string> all_registers { "%eax", "%ebx", "%ecx", "%edx", "%esi", "%edi" };
    for(int i = 0; i < all_registers.size(); i++)
    {
      bool found = false;
      for(int j = 0; j < rstack.size(); j++)
      {
        if(rstack[j] == all_registers[i])
        {
          found = true;
          break;
        }
      }
      if(!found) used_registers.push_back(all_registers[i]);
    }
    //push used registers
    for(string i: used_registers)
    {
      code.append("\tpushl\t" + i + "\n");
    }
    identifier_astnode* fun_name = (identifier_astnode *) fun_node->args[0];
    //make space for ret value
    code.append("\tsubl\t$" + to_string(tables.at(fun_name->id).size) + ", %esp\n");
    //push args
    for(int i = 1; i < fun_node->args.size(); i++)
    {
      if(fun_node->args[i]->nodetype == INTCONST_ASTNODE)
      {
        code.append("\tpushl\t$" + to_string(fun_node->args[i]->value) + "\n");
      }
      else if (fun_node->args[i]->nodetype == IDENTIFIER_ASTNODE)
      {
        identifier_astnode* var_astnode = (identifier_astnode *) fun_node->args[i];
        string var_name = var_astnode->id;
        int var_offset = tables.at(funcname).entries.at(var_name).offset;
        int var_size = tables.at(funcname).entries.at(var_name).size;
        int iter = var_offset + var_size - 4;
        while(iter >= var_offset)
        {
          code.append("\tpushl\t" + to_string(iter) + "(%ebp)\n");
          iter -= 4;
        }
      }
      else
      {
        //Check if argument is an int or struct
        string arg_type;
        identifier_astnode* fname = (identifier_astnode *) fun_node->args[0];
        for(map<string, SymTabEntry>::iterator entry = tables.at(fname->id).entries.begin(); entry != tables.at(fname->id).entries.end(); entry++)
        {
          if(entry->second.order == i - 1) arg_type = entry->second.type;
        }
        gencode(fun_node->args[i], rstack, funcname);
        if(arg_type == "int")
        {
          code.append("\tpushl\t" + rstack.back() + "\n");
        }
        else if(tables.find(arg_type) != tables.end())
        {
          int var_size = tables.at(arg_type).size;
          int iter = var_size - 4;
          while(iter >= 0)
          {
            code.append("\tpushl\t" + to_string(iter) + "(" + rstack.back() + ")\n");
            iter -= 4;
          }
        }
      }
    }
    //make space for SL
    code.append("\tsubl\t$4, %esp\n");
    code.append("\tcall\t" + fun_name->id + "\n");
    //Return value
    int size = 0;
    for(map<string, SymTabEntry>::iterator fun_vars = tables.at(fun_name->id).entries.begin(); fun_vars != tables.at(fun_name->id).entries.end(); fun_vars++)
    {
      if(fun_vars->second.loc == "param") size += fun_vars->second.size;
    }
    if(tables.at(fun_name->id).ret_type == "int")
    {
      code.append("\tmovl\t" + to_string(4 + size) + "(%esp), " + rstack.back() + "\n");
    }
    else if(tables.find(tables.at(fun_name->id).ret_type) != tables.end())
    {
      code.append("\tleal\t" + to_string(4 + size) + "(%esp), " + rstack.back() + "\n");
    }
    //Remove the functions arguments. Pop return value and used registers iff returning int, else popping those is responsibility of caller
    code.append("\taddl\t$" + to_string(size + 4 + tables.at(fun_name->id).size) + ", %esp\n");
    //Pop used registers in reverse order
    for(int i = used_registers.size() - 1; i >= 0; i--)
    {
      code.append("\tpopl\t" + used_registers[i] + "\n");
    }
  }
  //Case 7
  else if(n->nodetype == MEMBER_ASTNODE)
  {
    member_astnode* mem_node = (member_astnode *) n;
    member_info node_info = get_struct_offset(mem_node, funcname, rstack);
    if(node_info.type == "int")
    {
      code.append("\tmovl\t" + to_string(node_info.offset) + "(" + node_info.base + "), " + rstack.back() + "\n");
    }
    else
    {
      code.append("\tleal\t" + to_string(node_info.offset) + "(" + node_info.base + "), " + rstack.back() + "\n");
    }
  }
  //Case 8
  else if(n->nodetype == OP_UNARY_ASTNODE)
  {
    op_unary_astnode* op_node = (op_unary_astnode *) n;
    if(op_node->arg1 == "ADDRESS")
    {
      if(op_node->arg2->nodetype == IDENTIFIER_ASTNODE)
      {
        identifier_astnode* leaf = (identifier_astnode *)(op_node->arg2);
        int offset = tables.at(funcname).entries.at(leaf->id).offset;
        code.append("\tleal\t" + to_string(offset) + "(%ebp), " + rstack.back() + "\n");
      }
      else if(op_node->arg2->nodetype == MEMBER_ASTNODE)
      {
        member_astnode* mem_node = (member_astnode *)(op_node->arg2);
        member_info node_info = get_struct_offset(mem_node, funcname, rstack);
        code.append("\tleal\t" + to_string(node_info.offset) + "(" + node_info.base + "), " + rstack.back() + "\n");
      }
      else if(op_node->arg2->nodetype == ARROW_ASTNODE)
      {
        arrow_astnode* arrow_node = (arrow_astnode *)(op_node->arg2);
        //TODO
      }
    }
    else if(op_node->arg1 == "DEREF")
    {
      if(op_node->arg2->nodetype == IDENTIFIER_ASTNODE)
      {
        identifier_astnode* leaf = (identifier_astnode *)(op_node->arg2);
        if(tables.at(funcname).entries.at(leaf->id).type.find("struct") == string::npos)
        {
          gencode(op_node->arg2, rstack, funcname);
          code.append("\tmovl\t(" + rstack.back() + "), " + rstack.back() + "\n");
        }
      }
      else if(op_node->arg2->nodetype == MEMBER_ASTNODE)
      {
        member_astnode* mem_node = (member_astnode *)(op_node->arg2);
        member_info node_info = get_struct_offset(mem_node, funcname, rstack);
        if(node_info.type.find("struct") == string::npos)
        {
          code.append("\tmovl\t" + to_string(node_info.offset) + "(" + node_info.base + "), " + rstack.back() + "\n");
        }
        else
        {
          code.append("\tleal\t" + to_string(node_info.offset) + "(" + node_info.base + "), " + rstack.back() + "\n");
        }
      }
      else if(op_node->arg2->nodetype == OP_UNARY_ASTNODE)
      {
        op_unary_astnode* op_node = (op_unary_astnode *) op_node->arg2;
        if(op_node->arg1 == "DEREF")
        {
          gencode(op_node->arg2, rstack, funcname);
          code.append("\tmovl\t(" + rstack.back() + "), " + rstack.back() + "\n");
        }
      }
    }
    else if(op_node->arg1 == "UMINUS")
    {
      gencode(op_node->arg2, rstack, funcname);
      code.append("\tnegl\t" + rstack.back() + "\n");
    }
    else if(op_node->arg1 == "PP")
    {
      if(op_node->arg2->nodetype == IDENTIFIER_ASTNODE)
      {
        identifier_astnode* id_node = (identifier_astnode *) op_node->arg2;
        code.append("\tmovl\t" + to_string(tables.at(funcname).entries.at(id_node->id).offset) + "(%ebp), " + rstack.back() + "\n");
        if(id_node->type == "int")
        {
          code.append("\taddl\t$1, " + to_string(tables.at(funcname).entries.at(id_node->id).offset) + "(%ebp)\n");
        }
        else
        {
          int move_size = get_type_size(deref_string(id_node->type, 1));
          code.append("\taddl\t$" + to_string(move_size) + ", " + to_string(tables.at(funcname).entries.at(id_node->id).offset) + "(%ebp)\n");
        }
      }
      else if(op_node->arg2->nodetype == MEMBER_ASTNODE)
      {
        member_astnode* mem_node = (member_astnode *) op_node->arg2;
        member_info info = get_struct_offset(mem_node, funcname, rstack);
        code.append("\tmovl\t" + to_string(info.offset) + "(" + info.base + "), " + rstack.back() + "\n");
        code.append("\taddl\t$1, " + to_string(info.offset) + "(" + info.base + ")\n");
      }
    }
    else if(op_node->arg1 == "NOT")
    {
      if(op_node->arg2->nodetype == INTCONST_ASTNODE)
      {
        intconst_astnode * int_const = (intconst_astnode *) op_node->arg2;
        if(int_const->id == 0)
        {
          code.append("\tmovl\t$1, " + rstack.back() + "\n");
        }
        else
        {
          code.append("\tmovl\t$0, " + rstack.back() + "\n");
        }
      }
      else if(op_node->arg2->nodetype == IDENTIFIER_ASTNODE)
      {
        identifier_astnode *identifier_node = (identifier_astnode *) op_node->arg2;
        int offset = tables.at(funcname).entries.at(identifier_node->id).offset;
        code.append("\tcmpl\t$0, " + to_string(offset) + "(%ebp)\n");
        code.append("\tsete\t%al\n");
        code.append("\tmovzbl\t%al, " + rstack.back() + "\n");
      }
      else
      {
        gencode(op_node->arg2, rstack, funcname);
        code.append("\tcmpl\t$0, " + rstack.back() + "\n");
        code.append("\tsete\t%al\n");
        code.append("\tmovzbl\t%al, " + rstack.back() + "\n");
      }
    }
  }
  //Case 7
  else if(n->nodetype == ARRAYREF_ASTNODE)
  {
    arrayref_astnode* arr_node = (arrayref_astnode *)n;
    member_info node_info = get_array_offset(arr_node, funcname, rstack);
    if(node_info.type == "int")
    {
      code.append("\tmovl\t" + to_string(node_info.offset) + "(" + node_info.base + "), " + rstack.back() + "\n");
    }
    else
    {
      code.append("\tleal\t" + to_string(node_info.offset) + "(" + node_info.base + "), " + rstack.back() + "\n");
    }
  }
}

member_info get_array_offset(arrayref_astnode* n, string funcname, vector<string> &rstack)
{
  if(n->array->nodetype == IDENTIFIER_ASTNODE)
  {
    identifier_astnode* array_name = (identifier_astnode *)(n->array);
    int base = tables.at(funcname).entries.at(array_name->id).offset;
    return {array_name->type, base, "%ebp"};
  }
  else if(n->array->nodetype == IDENTIFIER_ASTNODE){
    arrayref_astnode* arr = (arrayref_astnode *) n;
    string type = arr->type;
    int first = type.find_last_of('[');
    string later = type.substr(first+1);
    int last = later.find_last_of(']');
    int nk = stoi(later.substr(0, last));
    member_info temp = get_array_offset((arrayref_astnode *) (n->array), funcname, rstack);
    // int base = temp.base;
    int offset = temp.offset;
  }
}

member_info get_struct_offset(member_astnode* n, string funcname, vector<string> &rstack)
{
  //base case
  if(n->struct_arg->nodetype == IDENTIFIER_ASTNODE)
  {
    identifier_astnode* struct_name = (identifier_astnode *)(n->struct_arg);
    int base = tables.at(funcname).entries.at(struct_name->id).offset;
    string struct_type = tables.at(funcname).entries.at(struct_name->id).type;
    int offset = tables.at(struct_type).entries.at(n->field->id).offset;
    string node_type = tables.at(struct_type).entries.at(n->field->id).type;
    return {node_type, base + offset, "%ebp"};
  }
  //recursion
  else if(n->struct_arg->nodetype == MEMBER_ASTNODE)
  {
    member_astnode* mem_node = (member_astnode *) n;
    member_info temp = get_struct_offset((member_astnode *)(mem_node->struct_arg), funcname, rstack);
    string field = mem_node->field->id;
    int base = temp.offset;
    string node_type = tables.at(temp.type).entries.at(field).type;
    int offset = tables.at(temp.type).entries.at(field).offset;
    return {node_type, base + offset, "%ebp"};
  }
  else if(n->struct_arg->nodetype == FUNCALL_ASTNODE)
  {
    funcall_astnode* func_node = (funcall_astnode *) (n->struct_arg);
    sethi_ullman(func_node, funcname, rstack); //rstack.back() now stores address of struct start
    string node_type = tables.at(((identifier_astnode *)(func_node->args[0]))->id).ret_type;
    return {tables.at(node_type).entries.at(n->field->id).type, tables.at(node_type).entries.at(n->field->id).offset, rstack.back()};
  }
  else if(n->struct_arg->nodetype == OP_UNARY_ASTNODE)
  {
    op_unary_astnode* star_node = (op_unary_astnode *)(n->struct_arg);
    if(star_node->arg1 == "DEREF")
    {
      sethi_ullman(star_node->arg2, funcname, rstack);
      return {tables.at(star_node->type).entries.at(n->field->id).type, tables.at(star_node->type).entries.at(n->field->id).offset, rstack.back()};
    }
  }
}

string deref_string(string type, int depth){
  int start = type.find("*");
  int i = start;
  while(i<type.length() && type[i]=='*'){
      i++;
  }
  string remainder = type.substr(i);
  string initial = type.substr(0,i-depth);
  return initial+remainder;
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