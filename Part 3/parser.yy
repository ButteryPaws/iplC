%skeleton "lalr1.cc"
%require  "3.0.1"
%locations

%defines 
%define api.namespace {IPL}
%define api.parser.class {Parser}

%define parse.trace

%code requires{
   #include "AST.hh"
   #include "ST.hh"
   #include "location.hh"
   namespace IPL {
      class Scanner;
   }


  // # ifndef YY_NULLPTR
  // #  if defined __cplusplus && 201103L <= __cplusplus
  // #   define YY_NULLPTR nullptr
  // #  else
  // #   define YY_NULLPTR 0
  // #  endif
  // # endif

}

%printer { std::cerr << $$; } IDENTIFIER
%printer { std::cerr << $$; } VOID
%printer { std::cerr << $$; } INT
%printer { std::cerr << $$; } FLOAT
%printer { std::cerr << $$; } STRUCT
%printer { std::cerr << $$; } INT_CONSTANT
%printer { std::cerr << $$; } RETURN
%printer { std::cerr << $$; } OR_OP
%printer { std::cerr << $$; } AND_OP
%printer { std::cerr << $$; } EQ_OP
%printer { std::cerr << $$; } NE_OP
%printer { std::cerr << $$; } LE_OP
%printer { std::cerr << $$; } GE_OP
%printer { std::cerr << $$; } PTR_OP
%printer { std::cerr << $$; } INC_OP
%printer { std::cerr << $$; } FLOAT_CONSTANT
%printer { std::cerr << $$; } IF
%printer { std::cerr << $$; } ELSE
%printer { std::cerr << $$; } WHILE
%printer { std::cerr << $$; } FOR
%printer { std::cerr << $$; } OTHERS
%printer { std::cerr << $$; } STRING_LITERAL

%parse-param { Scanner  &scanner  }
%code{
   #include <iostream>
   #include <cstdlib>
   #include <fstream>
   #include <string>
   #include <vector>
   #include <utility>
   #include <algorithm>

   #include "scanner.hh"
   string return_type;
   int is_return = 1;
   string temp;
   int order_local = 0;
   int order_param = 0;
   // int nodeCount = 0;

#undef yylex
#define yylex IPL::Parser::scanner.yylex

}

%define api.value.type variant
%define parse.assert

%start translation_unit

%token '\n'
%token <std::string> IDENTIFIER
%token <std::string> VOID 
%token <std::string> INT 
%token <std::string> FLOAT 
%token <std::string> STRUCT 
%token <std::string> INT_CONSTANT 
%token <std::string> RETURN
%token <std::string> OR_OP
%token <std::string> AND_OP
%token <std::string> EQ_OP
%token <std::string> NE_OP
%token <std::string> LE_OP
%token <std::string> GE_OP
%token <std::string> PTR_OP
%token <std::string> INC_OP
%token <std::string> FLOAT_CONSTANT
%token <std::string> IF
%token <std::string> ELSE
%token <std::string> WHILE
%token <std::string> FOR
%token <std::string> OTHERS
%token <std::string> STRING_LITERAL
%token '+' '-' '*' '/' '=' '>' '<' '.' ',' '!' '&' '(' ')' '{' '}' '[' ']' ';'

%nterm <int> struct_specifier translation_unit
%nterm<exp_astnode*> primary_expression postfix_expression unary_expression multiplicative_expression additive_expression relational_expression equality_expression logical_and_expression expression
%nterm<assignE_astnode*> assignment_expression
%nterm<seq_astnode*> statement_list
%nterm<statement_astnode*> assignment_statement statement iteration_statement function_definition
%nterm<pair<statement_astnode*, map<std::string, SymTabEntry>>> compound_statement 
%nterm<proccall_astnode*> procedure_call
%nterm<vector<exp_astnode*>> expression_list
%nterm<if_astnode*> selection_statement
%nterm<std::string> unary_operator declarator_arr type_specifier declarator 
%nterm<pair<std::string, map<std::string, SymTabEntry>>> fun_declarator
%nterm<vector<std::string>> declarator_list
%nterm<map<std::string, SymTabEntry>> parameter_declaration parameter_list declaration declaration_list
%%

translation_unit: 
       struct_specifier
       {
       } 
       | function_definition
       {
       }
       | translation_unit struct_specifier
       {
       }
       | translation_unit function_definition
       {
       }
       ;

struct_specifier: 
         STRUCT IDENTIFIER {
            temp = "struct " + $2;
         }
          '{' declaration_list '}' ';'
         {
           string struct_name = "struct " + $2; 
           // Check 1 : If the struct has a previous definition
           if(tables.find(struct_name)==tables.end()){
              SymTab struct_ST("struct", "-", $5);
              tables.insert({struct_name, struct_ST});
              order_local = 0;
              is_return = 1;
              scope.clear();
              temp.clear();
           }
           else{
              string err_mesg = "\"struct " + $2 + "\" has a previous definition";
              error(@2, err_mesg);
           }
         }
       ;

function_definition:
         type_specifier fun_declarator 
         {
           if(tables.find($2.first)==tables.end()){
            // Check 2 : If the return type of the function is struct then the struct should be defined before
            if($1.find("struct")==0){
              string struct_name = "struct " + $1.substr($1.find(" ")+1);
              if(tables.find(struct_name)==tables.end()){
                string err_mesg = "\"" + struct_name + "\" not declared";
                error(@$, err_mesg);
              }
            }
            map<std::string, SymTabEntry> temp;
            temp.insert($2.second.begin(), $2.second.end());
            SymTab function_ST("fun", $1, temp);
            tables.insert({$2.first, function_ST});
           }
         }
         compound_statement 
       { 
            $4.second.insert($2.second.begin(), $2.second.end());
            SymTab function_ST("fun", $1, $4.second);
            tables.erase($2.first);
            tables.insert({$2.first, function_ST});
            ASTs[$2.first] = $4.first;
            scope.clear();
            is_return = 1;
       } 
       ;

type_specifier:
          VOID
       {    
         $$ = "void";
         if(is_return){
           return_type = $$;
           is_return = 0;
         }
       }
       |  INT
       {
         $$ = "int";
         if(is_return){
           return_type = $$;
           is_return = 0;
         }
       }
       | FLOAT
       {
         $$ = "float";
         if(is_return){
           return_type = $$;
           is_return = 0;
         }
       }
       | STRUCT IDENTIFIER
       {
         $$ = "struct " + $2; 
         if(is_return){
           return_type = $$;
           is_return = 0;
         }
       }
       ;
     
fun_declarator:
          IDENTIFIER '(' parameter_list ')'
       {
         order_param = 0;
         $$.first = $1;
         $$.second = $3;
       }
       |  IDENTIFIER '(' ')'
       {
         $$.first = $1;
       }
       ;

parameter_list:
          parameter_declaration
       {
          $$ = $1;
       }
       |  parameter_list ',' parameter_declaration
       {
          $$ = $1;
          $$.insert($3.begin(), $3.end());
       }
       ;

parameter_declaration:
          type_specifier declarator
       {
          // Parsing variable name
          int i=0;
          while(i<$2.length() && $2[i]=='*'){
              i++;
          }
          string prefix = $2.substr(0,i);
          int j=i;
          while(j<$2.length() && $2[j]!='['){
            j++;
          }
          string identifier = $2.substr(i,j-i);
          string suffix = $2.substr(j);
          string type = $1 + prefix + suffix;

          // Check 1 : Cannot set the type of variable to void
          if(type=="void"){
            error(@$,"Cannot declare the type of a parameter as \"void\"");
          }
          else{
            declared_symbols[identifier] = type;
            SymTabEntry entry(type, "param", order_param, @$.begin.line);
            order_param++;
            $$.insert({identifier, entry});
            scope.push_back(identifier);
          }
       }
       ;

declarator_arr:
      IDENTIFIER
       {
         $$ = $1;
       }
       | declarator_arr '[' INT_CONSTANT ']'
       {
         $$ = $1 + '[' + $3 + ']';
       }
       ;

declarator:
       declarator_arr
       {
         $$ = $1;
       }
       | '*' declarator
       {
         $$ = "*" + $2;
       }
       ;

compound_statement:
          '{' '}'
          {
            $$.first = new seq_astnode();
          }
          | '{' statement_list '}'
          {
            $$.first = $2;
          }
          | '{' declaration_list '}'
          {
            $$.first = new seq_astnode();
            $$.second = $2;
            order_local = 0;
          }
          | '{' declaration_list statement_list '}'
          {
            $$.first = $3;
            $$.second = $2;
            order_local = 0;
          }
          ;

statement_list: 
          statement
          {
            $$ = new seq_astnode($1);
          }
          | statement_list statement
          {
            $1->seq.push_back($2);
            $$ = $1;
          }
          ;

statement: 
          ';'
          {
            $$ = new empty_astnode();
          }
          | '{' statement_list '}'
          {
             $$ = $2;
          }
          | selection_statement
          {
             $$ = $1;
          }
          | iteration_statement
          {
             $$ = $1;
          }
          | assignment_statement
          {
            $$ = $1;
          }
          | procedure_call
          {
             $$ = $1;
          }
          | RETURN expression ';'
          {
            // Check 1a : Return type is a struct and type of the expression is not struct
            // Check 1b : Return type is not a struct and type of the expression is a struct
            // Check 2 : Return type is void
             if((return_type.find("struct")==string::npos && $2->type.find("struct")==0) || (return_type.find("struct")==0 && $2->type.find("struct")==string::npos) || (return_type == "void"))
             {
               is_return = 1;
               string err_msg = "Incompatible type \"" + $2->type + "\" returned, expected \"" + return_type + "\"";
               error(@1, err_msg);
             }
             // Check 3 : Compatible types but not the same
             else if((return_type=="float" && $2->type=="int") || (return_type=="int" && $2->type=="float")){
               // Performing typecasting
               string cast;
               if(return_type=="int"){
                 cast = "TO_INT";
               }
               else{
                 cast = "TO_FLOAT";
               }
               exp_astnode* type_cast = new op_unary_astnode(cast, $2);
               $$ = new return_astnode(type_cast);
             }
             else if(return_type==$2->type){
               $$ = new return_astnode($2);
             }
             else{
               string err_msg = "Incompatible type \"" + $2->type + "\" returned, expected \"" + return_type + "\"";
               error(@$, err_msg);
             }
          }
          ;

assignment_expression: 
          unary_expression '=' expression
          {
            exp_astnode* arg1; // Represents the unary_expression
            exp_astnode* arg2; // Represents the expression

            // Case 1a : Cannot assign to an array (first condition)
            // Case 1b : Cannot assign from a pointer to an array (second condition)
            if(($1->type.find("[")!=string::npos) || ($3->type.find("[")!=string::npos && $3->type.find("(")!=string::npos)){
              string err_msg = "Incompatible assignment when assigning to type \"" + $1->type + "\" from type \"" + $3->type + "\"";
              error(@$, err_msg);
            }

            string arg1_base = get_base_type($1->type);
            string arg2_base = get_base_type($3->type);

            string arg1_type = $1->type;
            string arg2_type = $3->type;

            // Transformation 1 : (*) becomes * not followed by []
            if(arg2_type.find("(")!=string::npos){
              arg2_type = arg2_type.substr(0,arg2_type.find("(")) + "*";
            }

            int first_pointer_length = get_asterick_length(arg1_type);
            int second_pointer_length = get_asterick_length(arg2_type);

            // Case 2 : Pointer and array
            if(arg2_type.find("[")!=string::npos && arg2_type.find("*")==string::npos){
              second_pointer_length++;
            }

            // Case 3 : Pointer can be assigned 0
            if(first_pointer_length>0 && second_pointer_length == 0 && $3->value == 0){
              arg1 = $1;
              arg2 = $3;
            }
            // Case 4 : void* pointer
            else if((arg1_type.compare("void*")==0 && arg2_base!=arg2_type) || (arg2_type.compare("void*")==0 && arg1_base!=arg1_type)){
              arg1 = $1;
              arg2 = $3;
            }
            // Case 5 : Compatible Types
            else if(first_pointer_length==second_pointer_length){
              // Case 5a : Base types are same
              if(arg1_base.compare(arg2_base)==0){
                arg1 = $1;
                arg2 = $3;
              }
              // Case 5b : INT and FLOAT
              else if(arg1_base.compare("int")==0 && arg2_base.compare("float")==0 && first_pointer_length==0 && second_pointer_length==0){
                arg1 = $1;
                arg2 = new op_unary_astnode("TO_INT", $3);
              }
              // Case 5c : FLOAT and INT
              else if(arg1_base.compare("float")==0 && arg2_base.compare("int")==0 && first_pointer_length==0 && second_pointer_length==0){
                arg1 = $1;
                arg2 = new op_unary_astnode("TO_FLOAT", $3);
              }
              // Case 5d : Base types are not the same and cannot be typecasted
              else{
                string err_msg = "Incompatible assignment when assigning to type \"" + arg1_type + "\" from type \"" + arg2_type + "\"";
                error(@$, err_msg);
              }
            }
            // Case 6 : Incompatible types
            else{
              string err_msg = "Incompatible assignment when assigning to type \"" + arg1_type + "\" from type \"" + arg2_type + "\"";
              error(@$, err_msg);
            }

            // Case 7 : Left operand of assignment should have an lvalue
            if(!$1->is_lvalue){
              string err_msg = "Left operand of assignment should have an lvalue";
              error(@$, err_msg);
            }
            $$ = new assignE_astnode(arg1, arg2);
          }
          ;

assignment_statement: 
          assignment_expression ';'
          {
             $$ = new assignS_astnode($1->left, $1->right);
          }
          ;
     
procedure_call: 
          IDENTIFIER '(' ')' ';'
          {
            if(tables.find($1)==tables.end()){
              string err_msg = "Procedure \"" + $1 + "\" not declared";
              error(@$, err_msg);
            }
            else{
              // Check 2 : The number of function parameters match
              if(tables.at($1).get_param_length()!=0){
                string err_msg = "Procedure \"" + $1 + "\" called with too many arguments";
                error(@$, err_msg);
              }
              identifier_astnode* id = new identifier_astnode($1);
              vector<exp_astnode*> args;
              args.push_back(id);
              $$ = new proccall_astnode(args);
            }
          }   
          | IDENTIFIER '(' expression_list ')' ';'
          {
            // Check 1 : Function is not declared before
            if((tables.find($1)==tables.end()) && ($1!="scanf") && ($1!="printf")){
              string err_msg = "Procedure \"" + $1 + "\" not declared";
              error(@$, err_msg);
            }
            else{
              // Check 2 : The number of function parameters match
              if($1 == "scanf")
              {
                if($3.size() < 2)
                {
                  string error_msg = "Procedure \"scanf\" must have at least 2 arguments";
                  error(@$, error_msg);
                }
              }
              else if($1 == "printf")
              {
                if($3.size() == 0)
                {
                  string error_msg = "Procedure \"printf\" must have at least 1 argument";
                  error(@$, error_msg);
                }
              }
              else{
                if(tables.at($1).get_param_length()>$3.size()){
                  string err_msg = "Procedure \"" + $1 + "\" called with too few arguments";
                  error(@$, err_msg);
                }
                else if(tables.at($1).get_param_length()<$3.size()){
                  string err_msg = "Procedure \"" + $1 + "\" called with too many arguments";
                  error(@$, err_msg);
                }
                int i=0;
                for(auto it = tables.at($1).entries.rbegin(); it!= tables.at($1).entries.rend(); it++){
                  if(it->second.loc=="param"){
                    string actual_type = $3[i]->type;
                    string formal_type = it->second.type;

                    // Transformation 1 : INT[10] -> INT*
                    if(std::count(actual_type.begin(), actual_type.end(), '[')==1 && actual_type.find("(")==string::npos){
                      actual_type = actual_type.substr(0,actual_type.find('[')) + '*';
                    }
                    if(std::count(formal_type.begin(), formal_type.end(), '[')==1 && formal_type.find("(")==string::npos){
                      formal_type = formal_type.substr(0,formal_type.find('[')) + '*';
                    }

                    // Transformation 2 : INT[10][20] -> INT(*)[20]
                    if(std::count(actual_type.begin(), actual_type.end(), '[')>1 && actual_type.find("(")==string::npos){
                      actual_type = actual_type.substr(0,actual_type.find('[')) + "(*)" + actual_type.substr(actual_type.find(']')+1);
                    }
                    if(std::count(formal_type.begin(), formal_type.end(), '[')>1 && formal_type.find("(")==string::npos){
                      formal_type = formal_type.substr(0,formal_type.find('[')) + "(*)" + formal_type.substr(formal_type.find(']')+1);
                    }

                    // Transformation 3 : INT(*) -> INT* (Only for Actual)
                    if(std::count(actual_type.begin(), actual_type.end(), '[')==0 && actual_type.find("(")!=string::npos){
                      actual_type = actual_type.substr(0,actual_type.find('(')) + '*';
                    }

                    // Get Base types of Actual and Formal Parameters
                    string actual_base = get_base_type(actual_type);
                    string formal_base = get_base_type(formal_type);

                    int actual_pointer_length = get_asterick_length(actual_type);
                    int formal_pointer_length = get_asterick_length(formal_type);

                    // Check 1 : Typecasting
                    if(actual_type=="int" && formal_type=="float"){
                      $3[i] = new op_unary_astnode("TO_FLOAT", $3[i]);
                    }
                    else if(actual_type=="float" && formal_type=="int"){
                      $3[i] = new op_unary_astnode("TO_INT", $3[i]);
                    }
                    // Check 2 : void* pointer
                    else if(formal_type.compare("void*")==0){
                      if(actual_base==actual_type){
                        string err_msg = "Expected \"" + formal_type + "\" but argument is of type \"" + actual_type + "\"";
                        error(@$, err_msg);
                      }
                    }
                    // Check 3 : Actual = Formal
                    else if(actual_type!=formal_type){
                      string err_msg = "Expected \"" + formal_type + "\" but argument is of type \"" + actual_type + "\"";
                      error(@$, err_msg);
                    }
                    i++;
                  }
                }
              }
              identifier_astnode* id = new identifier_astnode($1);
              $3.insert($3.begin(), id);
              $$ = new proccall_astnode($3);
            }
          }
          ;

expression: 
          logical_and_expression
          {
             $$ = $1;
          }
          | expression OR_OP logical_and_expression
          {
            $$ = new op_binary_astnode("OR_OP", $1, $3);
            $$->type = "int";
          }
          ;

logical_and_expression: 
          equality_expression
          {
             $$ = $1;
          }
          | logical_and_expression AND_OP equality_expression
          {
             $$ = new op_binary_astnode("AND_OP", $1, $3);
             $$->type = "int";
          }
          ;

equality_expression: 
          relational_expression
          {
             $$ = $1;
          }        
          | equality_expression EQ_OP relational_expression
          {
            string op = "EQ_OP";
            exp_astnode* arg1;
            exp_astnode* arg2;

            string arg1_base = get_base_type($1->type);
            string arg2_base = get_base_type($3->type);

            string arg1_type = $1->type;
            string arg2_type = $3->type;

            // Transformation 1 : Convert (*) to * if not followed by []
            if(arg1_type.find('(')!=string::npos && arg1_type.find('[')==string::npos){
              arg1_type = arg1_type.substr(0,arg1_type.find("(")) + "*";
            }
            if(arg2_type.find('(')!=string::npos && arg2_type.find('[')==string::npos){
              arg2_type = arg2_type.substr(0,arg2_type.find("(")) + "*";
            }

            int first_pointer_length = get_asterick_length(arg1_type);
            int second_pointer_length = get_asterick_length(arg2_type);

            string first_modified_type = arg1_type.substr(arg1_base.length()+first_pointer_length);
            string second_modified_type = arg2_type.substr(arg2_base.length()+second_pointer_length);

            // Transformation 2a : Convert [] to * if not followed by another [] and should not have (*)
            if(first_modified_type.find('(')==string::npos && std::count(first_modified_type.begin(), first_modified_type.end(), '[')==1){
              first_modified_type = "";
              first_pointer_length++;
            }
            // Transformation 2 : Convert the first [] into (*) only if there is not an (*) already present and there are more than one []
            else if(first_modified_type.find('(')==string::npos && first_modified_type.find('[')!=string::npos && std::count(first_modified_type.begin(), first_modified_type.end(), '[')>1){
              first_modified_type = "(*)" + first_modified_type.substr(first_modified_type.find("]")+1);
            }
            if(second_modified_type.find('(')==string::npos && std::count(second_modified_type.begin(), second_modified_type.end(), '[')==1){
              second_modified_type = "";
              second_pointer_length++;
            }
            else if(second_modified_type.find('(')==string::npos && second_modified_type.find('[')!=string::npos && std::count(second_modified_type.begin(), second_modified_type.end(), '[')>1){
              second_modified_type = "(*)" + second_modified_type.substr(second_modified_type.find("]")+1);
            }

            // Case 1 : Structs cannot be compared
            if(arg1_type.find("struct")==0 || arg2_type.find("struct")==0)
            {
              string err_msg = "Invalid operands types for binary ==, \"" + arg1_type + "\" and \"" + arg2_type + "\"";
              error(@$, err_msg);
            }
            if(first_pointer_length==second_pointer_length && first_modified_type==second_modified_type)
            {
              if(arg1_base=="float" || arg2_base=="float"){
                op = op + "_FLOAT";
              }
              else{
                op = op + "_INT";
              }
              // Case 3a : Compatible types
              if(arg1_base==arg2_base)
              {
                arg1 = $1;
                arg2 = $3;
              }
              // Case 3b : Typecasting
              else if (arg1_base.compare("int")==0 && arg2_base.compare("float")==0 && first_pointer_length==0 && second_pointer_length==0)
              {
                arg1 = new op_unary_astnode("TO_FLOAT", $1);
                arg2 = $3;
              }
              else if (arg1_base.compare("float")==0 && arg2_base.compare("int")==0 && first_pointer_length==0 && second_pointer_length==0)
              {
                arg1 = $1;
                arg2 = new op_unary_astnode("TO_FLOAT", $3);
              }
              else{
                string err_msg = "Invalid operands types for binary ==, \"" + arg1_type + "\" and \"" + arg2_type + "\"";
                error(@$, err_msg);
              }
            }
            else{
              string err_msg = "Invalid operands types for binary ==, \"" + arg1_type + "\" and \"" + arg2_type + "\"";
              error(@$, err_msg);
            }
            
            $$ = new op_binary_astnode(op, arg1, arg2);
            $$->type = "int";
          }
          | equality_expression NE_OP relational_expression
          {
            string op = "NE_OP";
            exp_astnode* arg1;
            exp_astnode* arg2;

            string arg1_type = $1->type;
            string arg2_type = $3->type;

            string arg1_base = get_base_type(arg1_type);
            string arg2_base = get_base_type(arg2_type);

            // Transformation 1 : Convert (*) to * if not followed by []
            if(arg1_type.find('(')!=string::npos && arg1_type.find('[')==string::npos){
              arg1_type = arg1_type.substr(0,arg1_type.find("(")) + "*";
            }
            if(arg2_type.find('(')!=string::npos && arg2_type.find('[')==string::npos){
              arg2_type = arg2_type.substr(0,arg2_type.find("(")) + "*";
            }

            int first_pointer_length = get_asterick_length(arg1_type);
            int second_pointer_length = get_asterick_length(arg2_type);

            string first_modified_type = arg1_type.substr(arg1_base.length()+first_pointer_length);
            string second_modified_type = arg2_type.substr(arg2_base.length()+second_pointer_length);

            // Transformation 2a : Convert [] to * if not followed by another [] and should not have (*)
            if(first_modified_type.find('(')==string::npos && std::count(first_modified_type.begin(), first_modified_type.end(), '[')==1){
              first_modified_type = "";
              first_pointer_length++;
            }
            // Transformation 2 : Convert the first [] into (*) only if there is not an (*) already present and there are more than one []
            else if(first_modified_type.find('(')==string::npos && first_modified_type.find('[')!=string::npos && std::count(first_modified_type.begin(), first_modified_type.end(), '[')>1){
              first_modified_type = "(*)" + first_modified_type.substr(first_modified_type.find("]")+1);
            }
            if(second_modified_type.find('(')==string::npos && std::count(second_modified_type.begin(), second_modified_type.end(), '[')==1){
              second_modified_type = "";
              second_pointer_length++;
            }
            else if(second_modified_type.find('(')==string::npos && second_modified_type.find('[')!=string::npos && std::count(second_modified_type.begin(), second_modified_type.end(), '[')>1){
              second_modified_type = "(*)" + second_modified_type.substr(second_modified_type.find("]")+1);
            }

            // Case 1 : Structs cannot be compared
            if(arg1_type.find("struct")==0 || arg2_type.find("struct")==0)
            {
              string err_msg = "Invalid operands types for binary !=, \"" + arg1_type + "\" and \"" + arg2_type + "\"";
              error(@$, err_msg);
            }
            if(first_pointer_length==second_pointer_length && first_modified_type==second_modified_type)
            {
              if(arg1_base=="float" || arg2_base=="float"){
                op = op + "_FLOAT";
              }
              else{
                op = op + "_INT";
              }
              // Case 3a : Compatible types
              if(arg1_base==arg2_base)
              {
                arg1 = $1;
                arg2 = $3;
              }
              // Case 3b : Typecasting
              else if (arg1_base.compare("int")==0 && arg2_base.compare("float")==0 && first_pointer_length==0 && second_pointer_length==0)
              {
                arg1 = new op_unary_astnode("TO_FLOAT", $1);
                arg2 = $3;
              }
              else if (arg1_base.compare("float")==0 && arg2_base.compare("int")==0 && first_pointer_length==0 && second_pointer_length==0)
              {
                arg1 = $1;
                arg2 = new op_unary_astnode("TO_FLOAT", $3);
              }
              else{
                string err_msg = "Invalid operands types for binary !=, \"" + arg1_type + "\" and \"" + arg2_type + "\"";
                error(@$, err_msg);
              }
            }
            else{
              string err_msg = "Invalid operands types for binary !=, \"" + arg1_type + "\" and \"" + arg2_type + "\"";
              error(@$, err_msg);
            }

            $$ = new op_binary_astnode(op, arg1, arg2);
            $$->type = "int";
          }
          ;

relational_expression: 
          additive_expression
          {
             $$ = $1;
          }
          | relational_expression '<' additive_expression
          {
            string op = "LT_OP";
            exp_astnode* arg1;
            exp_astnode* arg2;

            string arg1_type = $1->type;
            string arg2_type = $3->type;

            string arg1_base = get_base_type(arg1_type);
            string arg2_base = get_base_type(arg2_type);

            // Transformation 1 : Convert (*) to * if not followed by []
            if(arg1_type.find('(')!=string::npos && arg1_type.find('[')==string::npos){
              arg1_type = arg1_type.substr(0,arg1_type.find("(")) + "*";
            }
            if(arg2_type.find('(')!=string::npos && arg2_type.find('[')==string::npos){
              arg2_type = arg2_type.substr(0,arg2_type.find("(")) + "*";
            }
            
            int first_pointer_length = get_asterick_length(arg1_type);
            int second_pointer_length = get_asterick_length(arg2_type);

            string first_modified_type = arg1_type.substr(arg1_base.length()+first_pointer_length);
            string second_modified_type = arg2_type.substr(arg2_base.length()+second_pointer_length);

            // Transformation 1 : Convert the first [] into (*) only if there is not an (*) already present
            if(first_modified_type.find('(')==string::npos && first_modified_type.find('[')!=string::npos){
              first_modified_type = "(*)" + first_modified_type.substr(first_modified_type.find("]")+1);
            }
            if(second_modified_type.find('(')==string::npos && second_modified_type.find('[')!=string::npos){
              second_modified_type = "(*)" + second_modified_type.substr(second_modified_type.find("]")+1);
            }

            // Case 1 : Structs cannot be compared
            if(arg1_type.find("struct")==0 || arg2_type.find("struct")==0)
            {
              string err_msg = "Invalid operands types for binary <, \"" + arg1_type + "\" and \"" + arg2_type + "\"";
              error(@$, err_msg);
            }
            // Case 2 : * and []
            if(abs(second_pointer_length-first_pointer_length)==1 && arg1_base==arg2_base && ((arg2_type.find('[')!=string::npos && arg2_type.find('(')==string::npos && arg1_type.find('[')==string::npos && arg1_type.find('(')==string::npos) || (arg1_type.find('[')!=string::npos && arg1_type.find('(')==string::npos && arg2_type.find('[')==string::npos && arg2_type.find('(')==string::npos)))
            {
              if(arg1_base=="int"){
                op = op + "_INT";
              }
              else{
                op = op + "_FLOAT";
              }
              arg1 = $1;
              arg2 = $3;
            }
            else if(first_pointer_length==second_pointer_length && first_modified_type==second_modified_type)
            {
              if(arg1_base=="float" || arg2_base=="float"){
                op = op + "_FLOAT";
              }
              else{
                op = op + "_INT";
              }
              // Case 3a : Compatible types
              if(arg1_base==arg2_base)
              {
                arg1 = $1;
                arg2 = $3;
              }
              // Case 3b : Typecasting
              else if (arg1_base.compare("int")==0 && arg2_base.compare("float")==0 && first_pointer_length==0 && second_pointer_length==0)
              {
                arg1 = new op_unary_astnode("TO_FLOAT", $1);
                arg2 = $3;
              }
              else if (arg1_base.compare("float")==0 && arg2_base.compare("int")==0 && first_pointer_length==0 && second_pointer_length==0)
              {
                arg1 = $1;
                arg2 = new op_unary_astnode("TO_FLOAT", $3);
              }
              else{
                string err_msg = "Invalid operands types for binary <, \"" + arg1_type + "\" and \"" + arg2_type + "\"";
                error(@$, err_msg);
              }
            }
            else{
              string err_msg = "Invalid operands types for binary <, \"" + arg1_type + "\" and \"" + arg2_type + "\"";
              error(@$, err_msg);
            }
            
            $$ = new op_binary_astnode(op, arg1, arg2);
            $$->type = "int";
          }
          | relational_expression '>' additive_expression
          {
            string op = "GT_OP";
            exp_astnode* arg1;
            exp_astnode* arg2;

            string arg1_type = $1->type;
            string arg2_type = $3->type;

            string arg1_base = get_base_type(arg1_type);
            string arg2_base = get_base_type(arg2_type);

            // Transformation 1 : Convert (*) to * if not followed by []
            if(arg1_type.find('(')!=string::npos && arg1_type.find('[')==string::npos){
              arg1_type = arg1_type.substr(0,arg1_type.find("(")) + "*";
            }
            if(arg2_type.find('(')!=string::npos && arg2_type.find('[')==string::npos){
              arg2_type = arg2_type.substr(0,arg2_type.find("(")) + "*";
            }
            
            int first_pointer_length = get_asterick_length(arg1_type);
            int second_pointer_length = get_asterick_length(arg2_type);

            string first_modified_type = arg1_type.substr(arg1_base.length()+first_pointer_length);
            string second_modified_type = arg2_type.substr(arg2_base.length()+second_pointer_length);

            // Transformation 1 : Convert the first [] into (*) only if there is not an (*) already present
            if(first_modified_type.find('(')==string::npos && first_modified_type.find('[')!=string::npos){
              first_modified_type = "(*)" + first_modified_type.substr(first_modified_type.find("]")+1);
            }
            if(second_modified_type.find('(')==string::npos && second_modified_type.find('[')!=string::npos){
              second_modified_type = "(*)" + second_modified_type.substr(second_modified_type.find("]")+1);
            }

            // Case 1 : Structs cannot be compared
            if(arg1_type.find("struct")==0 || arg2_type.find("struct")==0)
            {
              string err_msg = "Invalid operands types for binary >, \"" + arg1_type + "\" and \"" + arg2_type + "\"";
              error(@$, err_msg);
            }
            // Case 2 : * and []
            if(abs(second_pointer_length-first_pointer_length)==1 && arg1_base==arg2_base && ((arg2_type.find('[')!=string::npos && arg2_type.find('(')==string::npos && arg1_type.find('[')==string::npos && arg1_type.find('(')==string::npos) || (arg1_type.find('[')!=string::npos && arg1_type.find('(')==string::npos && arg2_type.find('[')==string::npos && arg2_type.find('(')==string::npos)))
            {
              if(arg1_base=="int"){
                op = op + "_INT";
              }
              else{
                op = op + "_FLOAT";
              }
              arg1 = $1;
              arg2 = $3;
            }
            else if(first_pointer_length==second_pointer_length && first_modified_type==second_modified_type)
            {
              if(arg1_base=="float" || arg2_base=="float"){
                op = op + "_FLOAT";
              }
              else{
                op = op + "_INT";
              }
              // Case 3a : Compatible types
              if(arg1_base==arg2_base)
              {
                arg1 = $1;
                arg2 = $3;
              }
              // Case 3b : Typecasting
              else if (arg1_base.compare("int")==0 && arg2_base.compare("float")==0 && first_pointer_length==0 && second_pointer_length==0)
              {
                arg1 = new op_unary_astnode("TO_FLOAT", $1);
                arg2 = $3;
              }
              else if (arg1_base.compare("float")==0 && arg2_base.compare("int")==0 && first_pointer_length==0 && second_pointer_length==0)
              {
                arg1 = $1;
                arg2 = new op_unary_astnode("TO_FLOAT", $3);
              }
              else{
                string err_msg = "Invalid operands types for binary >, \"" + arg1_type + "\" and \"" + arg2_type + "\"";
                error(@$, err_msg);
              }
            }
            else{
              string err_msg = "Invalid operands types for binary >, \"" + arg1_type + "\" and \"" + arg2_type + "\"";
              error(@$, err_msg);
            }

            $$ = new op_binary_astnode(op, arg1, arg2);
            $$->type = "int";
          }
          | relational_expression LE_OP additive_expression
          {
            string op = "LE_OP";
            exp_astnode* arg1;
            exp_astnode* arg2;

            string arg1_type = $1->type;
            string arg2_type = $3->type;

            string arg1_base = get_base_type(arg1_type);
            string arg2_base = get_base_type(arg2_type);

            // Transformation 1 : Convert (*) to * if not followed by []
            if(arg1_type.find('(')!=string::npos && arg1_type.find('[')==string::npos){
              arg1_type = arg1_type.substr(0,arg1_type.find("(")) + "*";
            }
            if(arg2_type.find('(')!=string::npos && arg2_type.find('[')==string::npos){
              arg2_type = arg2_type.substr(0,arg2_type.find("(")) + "*";
            }
            
            int first_pointer_length = get_asterick_length(arg1_type);
            int second_pointer_length = get_asterick_length(arg2_type);

            string first_modified_type = arg1_type.substr(arg1_base.length()+first_pointer_length);
            string second_modified_type = arg2_type.substr(arg2_base.length()+second_pointer_length);

            // Transformation 1 : Convert the first [] into (*) only if there is not an (*) already present
            if(first_modified_type.find('(')==string::npos && first_modified_type.find('[')!=string::npos){
              first_modified_type = "(*)" + first_modified_type.substr(first_modified_type.find("]")+1);
            }
            if(second_modified_type.find('(')==string::npos && second_modified_type.find('[')!=string::npos){
              second_modified_type = "(*)" + second_modified_type.substr(second_modified_type.find("]")+1);
            }

            // Case 1 : Structs cannot be compared
            if(arg1_type.find("struct")==0 || arg2_type.find("struct")==0)
            {
              string err_msg = "Invalid operands types for binary <=, \"" + arg1_type + "\" and \"" + arg2_type + "\"";
              error(@$, err_msg);
            }
            // Case 2 : * and []
            if(abs(second_pointer_length-first_pointer_length)==1 && arg1_base==arg2_base && ((arg2_type.find('[')!=string::npos && arg2_type.find('(')==string::npos && arg1_type.find('[')==string::npos && arg1_type.find('(')==string::npos) || (arg1_type.find('[')!=string::npos && arg1_type.find('(')==string::npos && arg2_type.find('[')==string::npos && arg2_type.find('(')==string::npos)))
            {
              if(arg1_base=="int"){
                op = op + "_INT";
              }
              else{
                op = op + "_FLOAT";
              }
              arg1 = $1;
              arg2 = $3;
            }
            else if(first_pointer_length==second_pointer_length && first_modified_type==second_modified_type)
            {
              if(arg1_base=="float" || arg2_base=="float"){
                op = op + "_FLOAT";
              }
              else{
                op = op + "_INT";
              }
              // Case 3a : Compatible types
              if(arg1_base==arg2_base)
              {
                arg1 = $1;
                arg2 = $3;
              }
              // Case 3b : Typecasting
              else if (arg1_base.compare("int")==0 && arg2_base.compare("float")==0 && first_pointer_length==0 && second_pointer_length==0)
              {
                arg1 = new op_unary_astnode("TO_FLOAT", $1);
                arg2 = $3;
              }
              else if (arg1_base.compare("float")==0 && arg2_base.compare("int")==0 && first_pointer_length==0 && second_pointer_length==0)
              {
                arg1 = $1;
                arg2 = new op_unary_astnode("TO_FLOAT", $3);
              }
              else{
                string err_msg = "Invalid operands types for binary <=, \"" + arg1_type + "\" and \"" + arg2_type + "\"";
                error(@$, err_msg);
              }
            }
            else{
              string err_msg = "Invalid operands types for binary <=, \"" + arg1_type + "\" and \"" + arg2_type + "\"";
              error(@$, err_msg);
            }

            $$ = new op_binary_astnode(op, arg1, arg2);
            $$->type = "int";
          }
          | relational_expression GE_OP additive_expression
          {
            string op = "GE_OP";
            exp_astnode* arg1;
            exp_astnode* arg2;

            string arg1_type = $1->type;
            string arg2_type = $3->type;

            string arg1_base = get_base_type(arg1_type);
            string arg2_base = get_base_type(arg2_type);

            // Transformation 1 : Convert (*) to * if not followed by []
            if(arg1_type.find('(')!=string::npos && arg1_type.find('[')==string::npos){
              arg1_type = arg1_type.substr(0,arg1_type.find("(")) + "*";
            }
            if(arg2_type.find('(')!=string::npos && arg2_type.find('[')==string::npos){
              arg2_type = arg2_type.substr(0,arg2_type.find("(")) + "*";
            }
            
            int first_pointer_length = get_asterick_length(arg1_type);
            int second_pointer_length = get_asterick_length(arg2_type);

            string first_modified_type = arg1_type.substr(arg1_base.length()+first_pointer_length);
            string second_modified_type = arg2_type.substr(arg2_base.length()+second_pointer_length);

            // Transformation 2 : Convert the first [] into (*) only if there is not an (*) already present
            if(first_modified_type.find('(')==string::npos && first_modified_type.find('[')!=string::npos){
              first_modified_type = "(*)" + first_modified_type.substr(first_modified_type.find("]")+1);
            }
            if(second_modified_type.find('(')==string::npos && second_modified_type.find('[')!=string::npos){
              second_modified_type = "(*)" + second_modified_type.substr(second_modified_type.find("]")+1);
            }

            // Case 1 : Structs cannot be compared
            if(arg1_type.find("struct")==0 || arg2_type.find("struct")==0)
            {
              string err_msg = "Invalid operands types for binary >=, \"" + arg1_type + "\" and \"" + arg2_type + "\"";
              error(@$, err_msg);
            }
            // Case 2 : * and []
            if(abs(second_pointer_length-first_pointer_length)==1 && arg1_base==arg2_base && ((arg2_type.find('[')!=string::npos && arg2_type.find('(')==string::npos && arg1_type.find('[')==string::npos && arg1_type.find('(')==string::npos) || (arg1_type.find('[')!=string::npos && arg1_type.find('(')==string::npos && arg2_type.find('[')==string::npos && arg2_type.find('(')==string::npos)))
            {
              if(arg1_base=="int"){
                op = op + "_INT";
              }
              else{
                op = op + "_FLOAT";
              }
              arg1 = $1;
              arg2 = $3;
            }
            else if(first_pointer_length==second_pointer_length && first_modified_type==second_modified_type)
            {
              if(arg1_base=="float" || arg2_base=="float"){
                op = op + "_FLOAT";
              }
              else{
                op = op + "_INT";
              }
              // Case 3a : Compatible types
              if(arg1_base==arg2_base)
              {
                arg1 = $1;
                arg2 = $3;
              }
              // Case 3b : Typecasting
              else if (arg1_base.compare("int")==0 && arg2_base.compare("float")==0 && first_pointer_length==0 && second_pointer_length==0)
              {
                arg1 = new op_unary_astnode("TO_FLOAT", $1);
                arg2 = $3;
              }
              else if (arg1_base.compare("float")==0 && arg2_base.compare("int")==0 && first_pointer_length==0 && second_pointer_length==0)
              {
                arg1 = $1;
                arg2 = new op_unary_astnode("TO_FLOAT", $3);
              }
              else{
                string err_msg = "Invalid operands types for binary >=, \"" + arg1_type + "\" and \"" + arg2_type + "\"";
                error(@$, err_msg);
              }
            }
            else{
              string err_msg = "Invalid operands types for binary >=, \"" + arg1_type + "\" and \"" + arg2_type + "\"";
              error(@$, err_msg);
            }

            $$ = new op_binary_astnode(op, arg1, arg2);
            $$->type = "int";
          }
          ;

additive_expression: 
          multiplicative_expression
          {
             $$ = $1;
          }
          | additive_expression '+' multiplicative_expression
          {
            string op = "PLUS";
            string type;
            exp_astnode* arg1;
            exp_astnode* arg2;

            string arg1_type = $1->type;
            string arg2_type = $3->type;

            string arg1_base = get_base_type(arg1_type);
            string arg2_base = get_base_type(arg2_type);

            // Transformation 1 : Convert (*) to * if not followed by []
            if(arg1_type.find('(')!=string::npos && arg1_type.find('[')==string::npos){
              arg1_type = arg1_type.substr(0,arg1_type.find("(")) + "*";
            }
            if(arg2_type.find('(')!=string::npos && arg2_type.find('[')==string::npos){
              arg2_type = arg2_type.substr(0,arg2_type.find("(")) + "*";
            }

            // Case 1 : Pointer and int
            if(arg1_type.compare("int")==0 && arg2_type!=arg2_base)
            {
              op = op + "_INT";
              arg1 = $1;
              arg2 = $3;
              // Single [] becomes * while first [] becomes (*)
              // No change in case of (*)[10]
              if(arg2_type.find("(")==string::npos){
                if(std::count(arg2_type.begin(), arg2_type.end(), '[')==1){
                  type = arg2_type.substr(0,arg2_type.find("[")) + "*";
                }
                else if(std::count(arg2_type.begin(), arg2_type.end(), '[')>1){
                  type = arg2_type.substr(0,arg2_type.find("[")) + "(*)" + arg2_type.substr(arg2_type.find("]")+1);
                }
              }
              else{
                type = arg2_type;
              }
            }
            else if(arg2_type.compare("int")==0 && arg1_type!=arg1_base)
            {
              op = op + "_INT";
              arg1 = $1;
              arg2 = $3;
              // Single [] becomes * while first [] becomes (*)
              // No change in case of (*)[10]
              if(arg1_type.find("(")==string::npos){
                if(std::count(arg1_type.begin(), arg1_type.end(), '[')==1){
                  type = arg1_type.substr(0,arg1_type.find("[")) + "*";
                }
                else if(std::count(arg1_type.begin(), arg1_type.end(), '[')>1){
                  type = arg1_type.substr(0,arg1_type.find("[")) + "(*)" + arg1_type.substr(arg1_type.find("]")+1);
                }
                else{
                  type = arg1_type;
                }
              }
              else{
                type = arg1_type;
              }
            }
            // Check 2 :INT and INT
            else if(arg1_type.compare("int")==0 && arg2_type.compare("int")==0){
              op = op + "_INT";
              arg1 = $1;
              arg2 = $3;
              type = "int";
            }
            // Check 3 : INT and FLOAT
            else if(arg1_type.compare("int")==0 && arg2_type.compare("float")==0){
              op = op + "_FLOAT";
              arg1 = new op_unary_astnode("TO_FLOAT", $1);
              arg2 = $3;
              type = "float";
            }
            // Check 4 : FLOAT and INT
            else if(arg1_type.compare("float")==0 && arg2_type.compare("int")==0){
              op = op + "_FLOAT";
              arg1 = $1;
              arg2 = new op_unary_astnode("TO_FLOAT", $3);
              type = "float";
            }
            // Check 5 : FLOAT and FLOAT
            else if(arg1_type.compare("float")==0 && arg2_type.compare("float")==0){
              op = op + "_FLOAT";
              arg1 = $1;
              arg2 = $3;
              type = "float";
            }
            else{
              string err_msg = "Invalid operands types for binary +, \"" + arg1_type + "\" and \"" + arg2_type + "\"";
              error(@$, err_msg);
            }

            $$ = new op_binary_astnode(op, arg1, arg2);
            $$->type = type;
          }
          | additive_expression '-' multiplicative_expression
          {
            string op = "MINUS";
            string type;
            exp_astnode* arg1;
            exp_astnode* arg2;

            string arg1_type = $1->type;
            string arg2_type = $3->type;

            string arg1_base = get_base_type(arg1_type);
            string arg2_base = get_base_type(arg2_type);

            string arg1_temp = $1->type;
            string arg2_temp = $3->type;

            // Transformation 1 : Convert (*) to * if not followed by []
            if(arg1_type.find('(')!=string::npos && arg1_type.find('[')==string::npos){
              arg1_type = arg1_type.substr(0,arg1_type.find("(")) + "*";
            }
            if(arg2_type.find('(')!=string::npos && arg2_type.find('[')==string::npos){
              arg2_type = arg2_type.substr(0,arg2_type.find("(")) + "*";
            }

            // Transformation 2 : Convert (*)[20] to *[20]
            if(arg1_temp.find('(')!=string::npos && arg1_temp.find('[')!=string::npos){
              arg1_temp = arg1_temp.substr(0,arg1_temp.find("(")) + "*" + arg1_temp.substr(arg1_temp.find("["));
            }
            // Transformation 3 : Convert [10][20] to *[20]
            else if(arg1_temp.find('(')==string::npos && arg1_temp.find('[')!=string::npos){
              arg1_temp = arg1_temp.substr(0,arg1_temp.find("[")) + "*" + arg1_temp.substr(arg1_temp.find("]")+1);
            }
            // Transformation 2 : Convert (*)[20] to *[20]
            if(arg2_temp.find('(')!=string::npos && arg2_temp.find('[')!=string::npos){
              arg2_temp = arg2_temp.substr(0,arg2_temp.find("(")) + "*" + arg2_temp.substr(arg2_temp.find("["));
            }
            // Transformation 3 : Convert [10][20] to *[20]
            else if(arg2_temp.find('(')==string::npos && arg2_temp.find('[')!=string::npos){
              arg2_temp = arg2_temp.substr(0,arg2_temp.find("[")) + "*" + arg2_temp.substr(arg2_temp.find("]")+1);
            }

            // Case 1 : Pointer and int
            if(arg2_type.compare("int")==0 && arg1_type!=arg1_base)
            {
              op = op + "_INT";
              arg1 = $1;
              arg2 = $3;
              // Single [] becomes * while first [] becomes (*)
              // No change in case of (*)[10]
              if(arg1_type.find("(")==string::npos){
                if(std::count(arg1_type.begin(), arg1_type.end(), '[')==1){
                  type = arg1_type.substr(0,arg1_type.find("[")) + "*";
                }
                else if(std::count(arg1_type.begin(), arg1_type.end(), '[')>1){
                  type = arg1_type.substr(0,arg1_type.find("[")) + "(*)" + arg1_type.substr(arg1_type.find("]")+1);
                }
                else{
                  type = arg1_type;
                }
              }
              else{
                type = arg1_type;
              }
            }
            // Check 2 :INT and INT
            else if(arg1_type.compare("int")==0 && arg2_type.compare("int")==0){
              op = op + "_INT";
              arg1 = $1;
              arg2 = $3;
              type = "int";
            }
            // Check 3 : INT and FLOAT
            else if(arg1_type.compare("int")==0 && arg2_type.compare("float")==0){
              op = op + "_FLOAT";
              arg1 = new op_unary_astnode("TO_FLOAT", $1);
              arg2 = $3;
              type = "float";
            }
            // Check 4 : FLOAT and INT
            else if(arg1_type.compare("float")==0 && arg2_type.compare("int")==0){
              op = op + "_FLOAT";
              arg1 = $1;
              arg2 = new op_unary_astnode("TO_FLOAT", $3);
              type = "float";
            }
            // Check 5 : FLOAT and FLOAT
            else if(arg1_type.compare("float")==0 && arg2_type.compare("float")==0){
              op = op + "_FLOAT";
              arg1 = $1;
              arg2 = $3;
              type = "float";
            }
            // Check 6a : INT[10] - INT[10]
            // Check 6b : INT[10] - INT* OR INT* - INT[10]
            // Check 6c : INT* - INT*
            // Check 6d : INT(*)[20] - INT[10][20]
            else if(arg1_temp==arg2_temp){
              op = op + "_INT";
              arg1 = $1;
              arg2 = $3;
              type = "int";
            }
            else{
              string err_msg = "Invalid operands types for binary -, \"" + arg1_type + "\" and \"" + arg2_type + "\"";
              error(@$, err_msg);
            }

            $$ = new op_binary_astnode(op, arg1, arg2);
            $$->type = type;
          }
          ;

unary_expression: 
          postfix_expression
          {
            $$ = $1;
          }
          | unary_operator unary_expression
          {
            if($1=="DEREF" && $2->type.find("*")==string::npos && $2->type.find("[")==string::npos){
              string err_msg = "Invalid operand type \"" + $2->type + "\" of unary *";
              error(@$, err_msg);
            }
            else if($1=="UMINUS" && $2->type!="int" && $2->type!="float"){
              string err_msg = "Operand of unary - should be an int or float";
              error(@$, err_msg);
            }
            else if($1=="ADDRESS" && !$2->is_lvalue){
              string err_msg = "Operand of & should have lvalue";
              error(@$, err_msg);
            }
            $$ = new op_unary_astnode($1, $2);
          }
          ;

multiplicative_expression: 
          unary_expression
          {
            $$ = $1;
          }
          | multiplicative_expression '*' unary_expression
          {
            string op = "MULT";
            string type;
            exp_astnode* arg1;
            exp_astnode* arg2;

            if($1->type.find("struct")==0 || $3->type.find("struct")==0)
            {
                string err_msg = "Invalid operands types for binary *, \"" + $1->type + "\" and \"" + $3->type + "\"";
                error(@$, err_msg);
            }
            // Check 2 :INT and INT
            else if($1->type.compare("int")==0 && $3->type.compare("int")==0){
              op = op + "_INT";
              arg1 = $1;
              arg2 = $3;
              type = "int";
            }
            // Check 3 : INT and FLOAT
            else if($1->type.compare("int")==0 && $3->type.compare("float")==0){
              op = op + "_FLOAT";
              arg1 = new op_unary_astnode("TO_FLOAT", $1);
              arg2 = $3;
              type = "float";
            }
            // Check 4 : FLOAT and INT
            else if($1->type.compare("float")==0 && $3->type.compare("int")==0){
              op = op + "_FLOAT";
              arg1 = $1;
              arg2 = new op_unary_astnode("TO_FLOAT", $3);
              type = "float";
            }
            // Check 5 : FLOAT and FLOAT
            else if($1->type.compare("float")==0 && $3->type.compare("float")==0){
              op = op + "_FLOAT";
              arg1 = $1;
              arg2 = $3;
              type = "float";
            }
            else{
              string err_msg = "Invalid operands types for binary *, \"" + $1->type + "\" and \"" + $3->type + "\"";
              error(@$, err_msg);
            }

            $$ = new op_binary_astnode(op, arg1, arg2);
            $$->type = type;
          }
          | multiplicative_expression '/' unary_expression
          {
            string op = "DIV";
            string type;
            exp_astnode* arg1;
            exp_astnode* arg2;

            if($1->type.find("struct")==0 || $3->type.find("struct")==0)
            {
                string err_msg = "Invalid operands types for binary *, \"" + $1->type + "\" and \"" + $3->type + "\"";
                error(@$, err_msg);
            }
            // Check 2 :INT and INT
            else if($1->type.compare("int")==0 && $3->type.compare("int")==0){
              op = op + "_INT";
              arg1 = $1;
              arg2 = $3;
              type = "int";
            }
            // Check 3 : INT and FLOAT
            else if($1->type.compare("int")==0 && $3->type.compare("float")==0){
              op = op + "_FLOAT";
              arg1 = new op_unary_astnode("TO_FLOAT", $1);
              arg2 = $3;
              type = "float";
            }
            // Check 4 : FLOAT and INT
            else if($1->type.compare("float")==0 && $3->type.compare("int")==0){
              op = op + "_FLOAT";
              arg1 = $1;
              arg2 = new op_unary_astnode("TO_FLOAT", $3);
              type = "float";
            }
            // Check 5 : FLOAT and FLOAT
            else if($1->type.compare("float")==0 && $3->type.compare("float")==0){
              op = op + "_FLOAT";
              arg1 = $1;
              arg2 = $3;
              type = "float";
            }
            else{
              string err_msg = "Invalid operands types for binary /, \"" + $1->type + "\" and \"" + $3->type + "\"";
              error(@$, err_msg);
            }

            $$ = new op_binary_astnode(op, arg1, arg2);
            $$->type = type;
          }
          ;

postfix_expression: 
          primary_expression
          {
            $$ = $1;
          }
          | postfix_expression '[' expression ']'
          {
            if($3->type!="int"){
              error(@3, "Array subscript is not an integer");
            }
            else{
              $$ = new arrayref_astnode($1, $3);
              int i = $1->type.length() - 1;
              while($1->type[i] != '[') i--;
              $$->type = $1->type.substr(0, i);
            }
          }
          | IDENTIFIER '(' ')'
          {
            // Check 1 : Function is not declared before
            if(tables.find($1)==tables.end()){
              string err_msg = "Function \"" + $1 + "\" not declared";
              error(@$, err_msg);
            }
            else{
              // Check 2 : The number of function parameters match
              if(tables.at($1).get_param_length()!=0){
                string err_msg = "Function \"" + $1 + "\" called with too many arguments";
                error(@$, err_msg);
              }
              identifier_astnode* id = new identifier_astnode($1);
              vector<exp_astnode*> args;
              args.push_back(id);
              $$ = new funcall_astnode(args);
              $$->type = tables.at($1).ret_type;
            }
          }
          | IDENTIFIER '(' expression_list ')'
          {
            // Check 1 : Function is not declared before
            if(tables.find($1)==tables.end()){
              string err_msg = "Function \"" + $1 + "\" not declared";
              error(@$, err_msg);
            }
            else{
              // Check 2 : The number of function parameters match
              if(tables.at($1).get_param_length()>$3.size()){
                string err_msg = "Function \"" + $1 + "\" called with too few arguments";
                error(@$, err_msg);
              }
              else if(tables.at($1).get_param_length()<$3.size()){
                string err_msg = "Function \"" + $1 + "\" called with too many arguments";
                error(@$, err_msg);
              }
              int i=0;
              for(auto it = tables.at($1).entries.begin(); it!= tables.at($1).entries.end(); it++){
                if(it->second.loc=="param"){
                  string actual_type = $3[i]->type;
                  string formal_type = it->second.type;

                  // Transformation 1 : INT[10] -> INT*
                  if(std::count(actual_type.begin(), actual_type.end(), '[')==1 && actual_type.find("(")==string::npos){
                    actual_type = actual_type.substr(0,actual_type.find('[')) + '*';
                  }
                  if(std::count(formal_type.begin(), formal_type.end(), '[')==1 && formal_type.find("(")==string::npos){
                    formal_type = formal_type.substr(0,formal_type.find('[')) + '*';
                  }

                  // Transformation 2 : INT[10][20] -> INT(*)[20]
                  if(std::count(actual_type.begin(), actual_type.end(), '[')>1 && actual_type.find("(")==string::npos){
                    actual_type = actual_type.substr(0,actual_type.find('[')) + "(*)" + actual_type.substr(actual_type.find(']')+1);
                  }
                  if(std::count(formal_type.begin(), formal_type.end(), '[')>1 && formal_type.find("(")==string::npos){
                    formal_type = formal_type.substr(0,formal_type.find('[')) + "(*)" + formal_type.substr(formal_type.find(']')+1);
                  }

                  // Transformation 3 : INT(*) -> INT* (Only for Actual)
                  if(std::count(actual_type.begin(), actual_type.end(), '[')==0 && actual_type.find("(")!=string::npos){
                    actual_type = actual_type.substr(0,actual_type.find('(')) + '*';
                  }

                  // Get Base types of Actual and Formal Parameters
                  string actual_base = get_base_type(actual_type);
                  string formal_base = get_base_type(formal_type);

                  int actual_pointer_length = get_asterick_length(actual_type);
                  int formal_pointer_length = get_asterick_length(formal_type);

                  // Check 1 : Typecasting
                  if(actual_type=="int" && formal_type=="float"){
                    $3[i] = new op_unary_astnode("TO_FLOAT", $3[i]);
                  }
                  else if(actual_type=="float" && formal_type=="int"){
                    $3[i] = new op_unary_astnode("TO_INT", $3[i]);
                  }
                  // Check 2 : void* pointer
                  else if(formal_type.compare("void*")==0){
                    if(actual_type==actual_base){
                      string err_msg = "Expected \"" + formal_type + "\" but argument is of type \"" + actual_type + "\"";
                      error(@$, err_msg);
                    }
                  }
                  // Check 3 : Actual = Formal
                  else if(actual_type!=formal_type){
                    string err_msg = "Expected \"" + formal_type + "\" but argument is of type \"" + actual_type + "\"";
                    error(@$, err_msg);
                  }
                  i++;
                }
              }
              identifier_astnode* id = new identifier_astnode($1);
              $3.insert($3.begin(), id);
              $$ = new funcall_astnode($3);
              $$->type = tables.at($1).ret_type;
            }
          }
          | postfix_expression '.' IDENTIFIER
          {
            //Check if postfix expression is a struct which contains identifier
            bool check = false;
            for(auto it: tables)
            {
              if(it.first == $1->type && it.second.type == "struct")
              {
                check = true;
                break;
              }
            }
            if(!check)
            {
              string error_msg = "Cannot get attribute \"" + $3 + "\" from type \"" + $1->type + "\"";
              error(@3, error_msg);
            } 
            if(tables.at($1->type).entries.find($3) == tables.at($1->type).entries.end())
            {
              string error_msg = "No attribute named \"" + $3 + "\" in struct of type \"" + $1->type + "\"";
              error(@3, error_msg);
            }
            else
            {
              identifier_astnode* id = new identifier_astnode($3);
              $$ = new member_astnode($1, id);
              $$->type = tables.at($1->type).entries.at($3).type;
            }
          }
          | postfix_expression PTR_OP IDENTIFIER
          {
            //check if postfix expression can be deref to struct
            int i = $1->type.length() - 1;
            string deref_type;
            if($1->type[i] == '*')
            {
              deref_type = $1->type.substr(0, i);
            }
            else if($1->type[i] == ']')
            {
              while($1->type[i] != '[') i--;
              deref_type = $1->type.substr(0, i);
            }
            else
            {
              string error_msg = "Cannot use arrow operator on type \"" + $1->type + "\"";
              error(@1, error_msg);
            }
            bool check = false;
            for(auto it: tables)
            {
              if(it.first == deref_type && it.second.type == "struct")
              {
                check = true;
                break;
              }
            }
            if(!check)
            {
              string error_msg = "Cannot get attribute \"" + $3 + "\" from type \"" + deref_type + "\"";
              error(@3, error_msg);
            } 
            if(tables.at(deref_type).entries.find($3) == tables.at(deref_type).entries.end())
            {
              string error_msg = "No attribute named \"" + $3 + "\" in struct of type \"" + deref_type + "\"";
              error(@3, error_msg);
            }
            else
            {
              identifier_astnode* id = new identifier_astnode($3);
              $$ = new arrow_astnode($1, id);
              $$->type = tables.at(deref_type).entries.at($3).type;
            }
          }
          | postfix_expression INC_OP
          {
            if($1->type.find("*")==string::npos && $1->type!="int" && $1->type!="float"){
              string error_msg = "Operand of \"++\" should be a int, float or pointer";
              error(@$, error_msg);
            }
            $$ = new op_unary_astnode("PP", $1);
          }
          ;

primary_expression: 
          IDENTIFIER
          {
            int found = 0;
            for(string x : scope){
              if(x==$1){
                found = 1;
              }
            }
            if(found){
              $$ = new identifier_astnode($1);
              $$->value = -3;
            }
            else{
              string err_msg = "Variable \"" + $1 + "\" not declared";
              error(@$, err_msg);
            }
          }
          | INT_CONSTANT
          {
            $$ = new intconst_astnode(stoi($1));
            $$->value = stoi($1);
          }
          | FLOAT_CONSTANT
          {
            $$ = new floatconst_astnode(stof($1));
            $$->value = -1;
          }
          | STRING_LITERAL
          {
            $$ = new stringconst_astnode($1);
            $$->value = -2;
          }
          | '(' expression ')'
          {
            $$ = $2;
          }
          ;

expression_list: 
          expression
          {
             vector<exp_astnode*> exp;
             exp.push_back($1);
             $$ = exp;
          }
          | expression_list ',' expression
          {
             $$ = $1;
             $$.push_back($3);
          }
          ;

unary_operator: 
        '-'
        {
            $$ = "UMINUS";
        }
        | '!'
        {
            $$ = "NOT";
        }
        | '&'
        {
            $$ = "ADDRESS";
        }
        | '*'
        {
            $$ = "DEREF";
        }
        ;

selection_statement: 
        IF '(' expression ')' statement ELSE statement
        {
           $$ = new if_astnode($3, $5, $7);
        }
        ;

iteration_statement:
        WHILE '(' expression ')' statement
        {
           $$ = new while_astnode($3, $5);
        }
        | FOR '(' assignment_expression ';' expression ';' assignment_expression ')' statement
        {
           $$ = new for_astnode($3, $5, $7, $9);
        }
        ;

declaration_list:
        declaration
        {
          $$ = $1;
        }
        | declaration_list declaration
        {
          $$ = $1;
          $$.insert($2.begin(), $2.end());
        }
        ;

declaration:
        type_specifier declarator_list ';'
        {
          int isVoid = 0;
          if($1=="void"){
            isVoid = 1;
          }
          // Check 1 : The struct has been defined before or not
          int isStruct = 0;
          string struct_name;
          if($1.find("struct")==0){
            struct_name = "struct " + $1.substr($1.find(" ")+1);
            if(tables.find(struct_name)==tables.end() && struct_name!=temp){
              string err_mesg = "\"" + struct_name + "\" is not defined";
              error(@$, err_mesg);
            }
            isStruct = 1;
          }
          for(string x : $2)
          {
            int i=0;
            while(i<x.length() && x[i]=='*'){
                i++;
            }
            string prefix = x.substr(0,i);
            int j=i;
            while(j<x.length() && x[j]!='['){
              j++;
            }
            string identifier = x.substr(i,j-i);
            string suffix = x.substr(j);

            // Check 2 : Can't set the type of variable to void
            if(isVoid && prefix.length()==0){
              string err_msg = "Cannot declare variable of type \"void\"";
              error(@$, err_msg);
            }
            // Check 3 : Pointer to the underlying struct
            if(isStruct && prefix.length()==0 && struct_name==temp){
              string err_mesg = "\"" + struct_name + "\" is not defined";
              error(@$, err_mesg);
            }
            // Check 4 : The variable has a previous declaration or not
            if(std::find(scope.begin(), scope.end(), identifier) != scope.end()){
              string err_msg = "\"" + identifier + "\" has a previous declaration";
              error(@$, err_msg);
            }
            else{
              declared_symbols[identifier] = $1 + prefix + suffix;
              scope.push_back(identifier);
              SymTabEntry entry($1 + prefix + suffix, "local", order_local, @$.begin.line);
              order_local++;
              $$.insert({identifier, entry});
            }
          }
        }
        ;

declarator_list:
        declarator
        {
           vector<std::string> init;
           init.push_back($1);
           $$ = init;
        }
        | declarator_list ',' declarator
        {
           $$ = $1;
           $$.push_back($3);
        }
        ;
          
%%
void IPL::Parser::error( const location_type &l, const std::string &err_message )
{
   std::cout << "Error at line " << l.begin.line << ": " << err_message << "\n";
   exit(1);
}


