#include "Lexer.hpp"

vector<Token*> lex(string& code)
{
  vector<Token*> tokens;
  //note: i is incremented various amounts depending on the tokens
  for(size_t i = 0; i < code.size();)
  {
    //scan to start of next token (ignoring whitespace)
    if(code[i] == ' ' || code[i] == '\t' || code[i] == '\n')
    {
      i++;
      continue;
    }
    //start a token here
    size_t tokStart = i;
    //scan to end of token
    size_t iter = tokStart + 1;
    if(code[tokStart] == '"')
    {
      while(code[iter] != '"')
      {
        if(code[iter] == '\\')
        {
          iter++;
        }
        iter++;
      }
      iter++;
      i = iter;
      addToken(tokens, code.substr(tokStart, iter - tokStart), STRING_LITERAL);
    }
    else if(code[tokStart] == '\'')
    {
      if(code[tokStart + 1] == '\\')
      {
        addToken(tokens, code.substr(tokStart, 4), CHAR_LITERAL);
        i += 4;
      }
      else
      {
        addToken(tokens, code.substr(tokStart, 3), CHAR_LITERAL);
        i += 3;
      }
    }
    else if(ispunct(code[tokStart]))
    {
      //make token out of one or two punct chars
      if(code[tokStart] == '=')
      {
        if(code[tokStart + 1] == '=')
        {
          tokens.push_back(new Oper(CMPEQ));
          i += 2;
        }
        else
        {
          tokens.push_back(new Oper(ASSIGN));
          i++;
        }
      }
      else if(code[tokStart] == '<')
      {
        if(code[tokStart + 1] == '=')
        {
          tokens.push_back(new Oper(CMPLE));
          i += 2;
        }
        else if(code[tokStart + 1] == '<')
        {
          tokens.push_back(new Oper(SHL));
          i += 2;
        }
        else
        {
          tokens.push_back(new Oper(CMPL));
          i++;
        }
      }
      else if(code[tokStart] == '>')
      {
        if(code[tokStart + 1] == '=')
        {
          tokens.push_back(new Oper(CMPGE));
          i += 2;
        }
        else if(code[tokStart + 1] == '>')
        {
          tokens.push_back(new Oper(SHR));
          i += 2;
        }
        else
        {
          tokens.push_back(new Oper(CMPG));
          i++;
        }
      }
      else if(code[tokStart] == '|')
      {
        if(code[tokStart + 1] == '|')
        {
          tokens.push_back(new Oper(LOR));
          i += 2;
        }
        else
        {
          tokens.push_back(new Oper(BOR));
          i++;
        }
      }
      else if(code[tokStart] == '&')
      {
        if(code[tokStart + 1] == '&')
        {
          tokens.push_back(new Oper(LAND));
          i += 2;
        }
        else
        {
          tokens.push_back(new Oper(BAND));
          i++;
        }
      }
      else if(code[tokStart] == '!')
      {
        if(code[tokStart + 1] == '=')
        {
          tokens.push_back(new Oper(CMPNEQ));
          i += 2;
        }
        else
        {
          tokens.push_back(new Oper(LNOT));
          i++;
        }
      }
      else if(code[tokStart] == '+')
      {
        if(code[tokStart + 1] == '=')
        {
          tokens.push_back(new Oper(PLUSEQ));
          i += 2;
        }
        else
        {
          tokens.push_back(new Oper(PLUS));
          i++;
        }
      }
      else if(code[tokStart] == '-')
      {
        if(code[tokStart + 1] == '=')
        {
          tokens.push_back(new Oper(SUBEQ));
          i += 2;
        }
        else
        {
          tokens.push_back(new Oper(SUB));
          i++;
        }
      }
      else if(code[tokStart] == '*')
      {
        if(code[tokStart + 1] == '=')
        {
          tokens.push_back(new Oper(MULEQ));
          i += 2;
        }
        else
        {
          tokens.push_back(new Oper(MUL));
          i++;
        }
      }
      else if(code[tokStart] == '/')
      {
        if(code[tokStart + 1] == '=')
        {
          tokens.push_back(new Oper(DIVEQ));
          i += 2;
        }
        else
        {
          tokens.push_back(new Oper(DIV));
          i++;
        }
      }
      else
      {
        //must be a 1-character punctuation
        string token = string("") + code[tokStart];
        addToken(tokens, token, PUNCTUATION);
        i++;
      }
    }
    else if(isdigit(code[i]))
    {
      //int literal
      while(isdigit(code[iter]))
      {
        iter++;
      }
      addToken(tokens, code.substr(tokStart, iter - tokStart), INT_LITERAL);
      i = iter;
    }
    else if(isalpha(code[i]))
    {
      //keyword, type or identifier; read to end of [a-z, 0-9, _]
      while(isalpha(code[iter]) || isdigit(code[iter]) || code[iter] == '_')
      {
        iter++;
      }
      addToken(tokens, code.substr(tokStart, iter - tokStart), IDENTIFIER);
      i += (iter - tokStart);
    }
    else
    {
      //???
      cout << &code[i] << '\n';
      errAndQuit(string("Error: lexer could not identify token at index ") + to_string(i));
    }
  }
  return tokens;
}

void addToken(vector<Token*>& tokList, string token, int hint)
{
  if(hint == IDENTIFIER)
  {
    int kw = isKeyword(token);
    if(kw != -1)
    {
      tokList.push_back(new Keyword(kw));
    }
    else
    {
      tokList.push_back(new Ident(token));
    }
  }
  else if(hint == STRING_LITERAL)
  {
    string val = "";
    for(size_t i = 1; i < token.size() - 1; i++)
    {
      if(token[i] == '\\')
      {
        if(i == token.size() - 1)
        {
          errAndQuit("String literal ends with backslash.");
        }
        val += getEscapedChar(token[i + 1]);
        i++;
      }
      else
        val += token[i];
    }
    tokList.push_back(new StrLit(val));
  }
  else if(hint == CHAR_LITERAL)
  {
    char val = token[1];
    if(val == '\\')
      val = getEscapedChar(token[2]);
    tokList.push_back(new CharLit(val));
  }
  else if(hint == INT_LITERAL)
  {
    //token is a copy outside of code stream and is null-terminated
    int val;
    sscanf(&token[1], "%i\n", &val);
    tokList.push_back(new IntLit(val));
  }
  else if(hint == PUNCTUATION)
  {
    char tok = token[0];
    //Structure punctuation
    if(tok == '(')
      tokList.push_back(new Punct(LPAREN));
    else if(tok == ')')
      tokList.push_back(new Punct(RPAREN));
    else if(tok == '{')
      tokList.push_back(new Punct(LBRACE));
    else if(tok == '}')
      tokList.push_back(new Punct(RBRACE));
    else if(tok == '.')
      tokList.push_back(new Punct(DOT));
    else if(tok == ',')
      tokList.push_back(new Punct(COMMA));
    else if(tok == ';')
      tokList.push_back(new Punct(SEMICOLON));
    //Operators
    else if(token == "^")
      tokList.push_back(new Oper(BXOR));
    else if(token == "[")
      tokList.push_back(new Oper(LBRACK));
    else if(token == "]")
      tokList.push_back(new Oper(RBRACK));
    else
      errAndQuit(string("Invalid or unknown punctuation token: \"") + token + "\"");
  }
}

char getEscapedChar(char ident)
{
  if(ident == 'n')
    return '\n';
  if(ident == 't')
    return '\t';
  if(ident == '0')
    return 0;
  if(ident == '\\')
    return '\\';
  errAndQuit(string("Unknown escape sequence: \\") + ident);
  return ' ';
}

