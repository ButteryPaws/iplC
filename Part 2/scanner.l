%{
// This part goes directly into the generated parser

#include "scanner.hh"
#include "parser.tab.hh"

// YY_DECL is bound to the type of the scanner. The default is 
// #define YY_DECL int Scanner::yylex()
// which is generated anyway.

//If you want the type of the generated scanner to be different, do this

#undef  YY_DECL
#define YY_DECL int IPL::Scanner::yylex( IPL::Parser::semantic_type * const lval, IPL::Parser::location_type *loc )

//We want these to be the first action after a token is recognized. 

#define YY_USER_ACTION loc->step(); loc->columns(yyleng);

// token_type is a short hand for IPL::Parser::token

using token_type = IPL::Parser::token;

%}


/* %option debug */
%option nodefault
%option yyclass="Scanner"
%option noyywrap
%option c++

%%

%{ /** Code executed at the beginning of yylex **/

      yylval = lval;
      // loc = location;

%}

[ \t]                             //skip
[\n]|[\r\n]                       {loc->lines();}
"//"([^\\\n]|[\\][^\\\n])*        //skip
"/*"([^*]|[*][^/])*"*/"           //skip
[1-9][0-9]*         {   
       yylval->emplace< std::string >( YYText() );
       return token_type::INT_CONSTANT;
}
0[Bb][01]*              {   
       yylval->emplace< std::string >( YYText() );
       return token_type::INT_CONSTANT;
}
0[0-7]*                 {   
       yylval->emplace< std::string >( YYText() );
       return token_type::INT_CONSTANT;
}
0[xX][0-9a-fA-F]*       {   
       yylval->emplace< std::string >( YYText() );
       return token_type::INT_CONSTANT;
}
[0-9]+"."[0-9]*     {   
       yylval->emplace< std::string >( YYText() );
       return token_type::FLOAT_CONSTANT;
}
[0-9]*"."[0-9]+     {   
       yylval->emplace< std::string >( YYText() );
       return token_type::FLOAT_CONSTANT;
}
[0-9]+"."[0-9]*[eE][+-][0-9]+     {   
       yylval->emplace< std::string >( YYText() );
       return token_type::FLOAT_CONSTANT;
}
[0-9]*"."[0-9]+[eE][+-][0-9]+     {   
       yylval->emplace< std::string >( YYText() );
       return token_type::FLOAT_CONSTANT;
}
[0-9]+"."[0-9]*[eE][0-9]+     {   
       yylval->emplace< std::string >( YYText() );
       return token_type::FLOAT_CONSTANT;
}
[0-9]*"."[0-9]+[eE][0-9]+     {   
       yylval->emplace< std::string >( YYText() );
       return token_type::FLOAT_CONSTANT;
}
"."[0-9]*[eE][+-][0-9]+     {   
       yylval->emplace< std::string >( YYText() );
       return token_type::FLOAT_CONSTANT;
}
\"([^\\\"]|[\\n]|[\\t]|[\\\\]|[\\][\"])*\"  {   
       yylval->emplace< std::string >( YYText() );
       return token_type::STRING_LITERAL;
}
"void"                                    {
      yylval->emplace< std::string >( YYText() );
      return token_type::VOID;
}
"int"                                    {
      yylval->emplace< std::string >( YYText() );
      return token_type::INT;
}
"float"                                    {
      yylval->emplace< std::string >( YYText() );
      return token_type::FLOAT;
}
"struct"                                    {
      yylval->emplace< std::string >( YYText() );
      return token_type::STRUCT;
}
"return"                                    {
      yylval->emplace< std::string >( YYText() );
      return token_type::RETURN;
}
"if"                                    {
      yylval->emplace< std::string >( YYText() );
      return token_type::IF;
}
"else"                                    {
      yylval->emplace< std::string >( YYText() );
      return token_type::ELSE;
}
"while"                                    {
      yylval->emplace< std::string >( YYText() );
      return token_type::WHILE;
}
"for"                                    {
      yylval->emplace< std::string >( YYText() );
      return token_type::FOR;
}
[a-zA-Z_][a-zA-Z0-9_]*                      {
      yylval->emplace< std::string >( YYText() );
      return token_type::IDENTIFIER;
}
"||"                                    {
      yylval->emplace< std::string >( YYText() );
      return token_type::OR_OP;
}
"&&"                                    {
      yylval->emplace< std::string >( YYText() );
      return token_type::AND_OP;
}
"=="                                    {
      yylval->emplace< std::string >( YYText() );
      return token_type::EQ_OP;
}
"!="                                    {
      yylval->emplace< std::string >( YYText() );
      return token_type::NE_OP;
}
"<="                                    {
      yylval->emplace< std::string >( YYText() );
      return token_type::LE_OP;
}
">="                                    {
      yylval->emplace< std::string >( YYText() );
      return token_type::GE_OP;
}
"->"                                    {
      yylval->emplace< std::string >( YYText() );
      return token_type::PTR_OP;
}
"++"                                    {
      yylval->emplace< std::string >( YYText() );
      return token_type::INC_OP;
}
"+"                                         return '+';
"-"                                         return '-';
"*"                                         return '*';
"/"                                         return '/';
"="                                         return '=';
">"                                         return '>';
"<"                                         return '<';
"."                                         return '.';
","                                         return ',';
"!"                                         return '!';
"&"                                         return '&';
"("                                         return '(';
")"                                         return ')';
"{"                                         return '{';
"}"                                         return '}';
"["                                         return '[';
"]"                                         return ']';
";"                                         return ';';
.                                           return token_type::OTHERS;
<<EOF>>         yyterminate();
%%

