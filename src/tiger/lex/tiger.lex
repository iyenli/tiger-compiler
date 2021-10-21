%filenames = "scanner"

 /*
  * Please don't modify the lines above.
  */

 /* You can add lex definitions here. */
%x COMMENT STR IGNORE

%%
 /* reserved words */
"if" {adjust(); return Parser::IF;}
"while" {adjust(); return Parser::WHILE;}
"for" {adjust(); return Parser::FOR;}
"else" {adjust(); return Parser::ELSE;}
"then" {adjust(); return Parser::THEN;}
"to" {adjust(); return Parser::TO;}
"do"  {adjust(); return Parser::DO;}
"let" {adjust(); return Parser::LET;}
"break" {adjust(); return Parser::BREAK;}
"of" {adjust(); return Parser::OF;}
"end" {adjust(); return Parser::END;}
"in" {adjust(); return Parser::IN;}
"nil" {adjust(); return Parser::NIL;}
"type" {adjust(); return Parser::TYPE;}
"array" {adjust(); return Parser::ARRAY;}
"var" {adjust(); return Parser::VAR;}
"function" {adjust(); return Parser::FUNCTION;}

 /* symbol used */
 ":=" {adjust(); return Parser::ASSIGN;}
"|"  {adjust(); return Parser::OR;}
"&"  {adjust(); return Parser::AND;}
">="  {adjust(); return Parser::GE;}
">"  {adjust(); return Parser::GT;}
"<="  {adjust(); return Parser::LE;}
"<"  {adjust(); return Parser::LT;}
"<>"  {adjust(); return Parser::NEQ;}
"="  {adjust(); return Parser::EQ;}
"/"  {adjust(); return Parser::DIVIDE;}
"*"  {adjust(); return Parser::TIMES;}
"-"  {adjust(); return Parser::MINUS;}
"+" {adjust(); return Parser::PLUS;}
"."  {adjust(); return Parser::DOT;}
"}"  {adjust(); return Parser::RBRACE;}
"{"  {adjust(); return Parser::LBRACE;}
"]"  {adjust(); return Parser::RBRACK;}
"["  {adjust(); return Parser::LBRACK;}
")"  {adjust(); return Parser::RPAREN;}
"("  {adjust(); return Parser::LPAREN;}
";"  {adjust(); return Parser::SEMICOLON;}
":"  {adjust(); return Parser::COLON;}
","  {adjust(); return Parser::COMMA;}

 /* ID and int */
[a-zA-Z_][a-zA-Z0-9_]* { adjust(); return Parser::ID; }
[0-9]+ { adjust(); return Parser::INT; }

 /* COM and Str */
\" { adjust(); begin(StartCondition__::STR); }
"/*" { adjust(); comment_level_ = 1; begin(StartCondition__::COMMENT); }

   /*
  * skip white space chars.
  * space, tabs and LF
  */
[ \t]+ {adjust();}
\n {adjust(); errormsg_->Newline();}

 /* illegal input */
. {adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal token");}

 /* MINI scanner */
 /* string scanner: try to add \ word to string buf */
 /* NO need to errormsg->newline in this part */
<STR> {
\" { adjustStr(); setMatched(string_buf_); string_buf_.clear(); begin(StartCondition__::INITIAL); return Parser::STRING; }
\\n { adjustStr(); string_buf_ += '\n'; }
\\t { adjustStr(); string_buf_ += '\t'; }
\\\^[A-Z] { adjustStr(); string_buf_ += (matched()[2] - 'A' + 1); }
\\[0-9]{3} { adjustStr(); string_buf_ += ((char)atoi(matched().c_str() + 1)); }
\\\" { adjustStr(); string_buf_ += '\"'; }
\\\\ { adjustStr(); string_buf_ += '\\'; }
\\[ \t\n\f]+\\ { adjustStr(); }
. {adjustStr(); string_buf_ += matched();}
}

 /* Comment: don't return as a token' */
<COMMENT> {
"*/" {--comment_level_; adjustStr(); if(comment_level_ == 0)begin(StartCondition__::INITIAL);}
"/*" {++comment_level_; adjustStr();}
\n {adjustStr(); errormsg_->Newline(); }
. {adjustStr();}
}