#include<bits/stdc++.h>
using namespace std;

enum tokenType {
    keyword,    //0
    identifier, //1
    number,     //2
    operaTor,   //3
    separator,  //4
    stringtype, //5
    unknown     //6
};

struct token {
    tokenType type;
    string value;
    int line;
    int col;
};

unordered_set<string>keywords = {"int","float","double","long long","char","bool","string","if","else","for","while","true","false","return",
                                "void","break","continue","switch","case","default","cout","cin","using","namespace"
};

unordered_set<char>operators = {'+','-','*','/','=','%','&','|','<','>','!','^'};

unordered_set<char>separators = {'{' , '}' , ',' , '[' , ']' , '(' , ')' , ':' , ';' };

bool isNumber(const string &str) {
    return regex_match(str,regex("-?([0-9]+(\\.[0-9]+)?|\\.[0-9]+)"));
}

bool isIdentitfier(const string& str) {
    return regex_match(str,regex("[_a-zA-Z]+[_a-zA-Z0-9]*"));
}

vector<token> tokenize(const string &code) {
    vector<token>tokens;
    int i=0;
    int len = code.length();
    int line = 1;
    int column = 1;
    while(i < len) {
        char c = code[i];
        // Skip preprocessor directives
        if (c == '#') {
            while (i < len && code[i] != '\n') i++;
            continue;
        }
        //Ignore spaces
        if(isspace(c)) {
            if(c == '\n') {
                line++;
                column = 1;
            }
            else {
                column++;
            }
            i++;
            continue;
        }

        //Ignore single line comments
        if(c == '/' && i+1<len && code[i+1] =='/') {
            i+=2;
            while(i < len && code[i] != '\n')
                i++;
            line++;
            column = 1;
            i++;
            continue;
        }

        //Ignore multiline comments
        if(c == '/' && i+1<len && code[i+1] == '*') {
            i+=2;
            column+=2;
            while(i+1<len && code[i] != '*' && code[i+1] != '/') {
                if(code[i] == '\n') {
                    line++;
                    column = 1;
                }
                else
                    column++;
            }
            i+=2;
            column+=2;
            continue;
        } 

        //Now start generating tokens
        //Identifing string literals
        if(c == '"') {
            i++;
            int start = i;
            while(i < len && code[i] != '"') 
                i++;
            i++;
            tokens.push_back({tokenType::stringtype,code.substr(start,i-start-1),line,column});
            column += (i-start);
            continue;
        }

        //Identifying operators
        // Identifying operators (including << and >>)
    if (operators.count(c)) {
        string op(1, c);

        // Look ahead for <<, >>, <=, >=, ==, !=
        if (i + 1 < len) {
            char next = code[i + 1];
            if ((c == '<' && next == '<') || (c == '>' && next == '>') ||
                (c == '<' && next == '=') || (c == '>' && next == '=') ||
                (c == '=' && next == '=') || (c == '!' && next == '=')) {
                op += next;
                i++;
            }
        }

        tokens.push_back({tokenType::operaTor, op, line, column});
        column += op.length();
        i++;
        continue;
    }


        //Identifying separators
        if(separators.count(c)) {
            tokens.push_back({tokenType::separator,string(1,c),line,column});
            column++;
            i++;
            continue;
        }

        //Identifying numbers,identifiers,keywords
        if(isalnum(c) || c == '_' || c == '.') {
            int start = i;
            while(i < len && (isalnum(code[i]) || code[i] =='_' || code[i] == '.'))
                i++;
            string val = code.substr(start,i-start);
            tokenType type;
            if(keywords.count(val))
                type = tokenType::keyword;
            else if(isNumber(val))
                type = tokenType::number;
            else if(isIdentitfier(val))
                type = tokenType::identifier;
            else
                type = tokenType::unknown;
            tokens.push_back({type,val,line,column});
            column += (i-start);
            continue;
        }

        //if anything else is found then unknown
        tokens.push_back({tokenType::unknown,string(1,c),line,column});
        i++;
        column++;
    } 
    return tokens;
}

string tokenToString(tokenType t) {
    switch(t) {
        case keyword:return "Keyword   ";
        case identifier:return "Identifier";
        case number:return "Number   ";
        case operaTor:return "Operator";
        case separator:return "Separator";
        case stringtype:return "String   ";
        default:return "Unknown";
    }
}


