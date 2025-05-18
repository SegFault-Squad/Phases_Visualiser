#include <bits/stdc++.h>
#include "lexical.cpp"
using namespace std;

class Parser {
    vector<token> tokens;
    int current = 0;

public:
    Parser(const vector<token>& t) : tokens(t) {}

    void parse() {
        while (!isAtEnd()) {
            statement();
        }
        cout << "Syntax Analysis Successful.\n";
    }

private:
    token peek() {
        return current < tokens.size() ? tokens[current] : token{unknown, "", -1, -1};
    }

    token peekNext(int offset = 1) {
        int idx = current + offset;
        if (idx < (int)tokens.size()) return tokens[idx];
        return token{unknown,"",-1,-1};
    }

    token advance() {
        return tokens[current++];
    }

    bool match(tokenType type, const string& val = "") {
        if (current >= tokens.size()) return false;
        if (tokens[current].type == type && (val.empty() || tokens[current].value == val)) {
            current++;
            return true;
        }
        return false;
    }

    bool check(tokenType type, const string& val = "") {
        if (isAtEnd()) return false;
        return tokens[current].type == type && (val.empty() || tokens[current].value == val);
    }

    bool isAtEnd() {
        return current >= tokens.size();
    }

    void error(const string& msg) {
        token t = peek();
        if (t.line == -1)
            cerr << "Syntax Error: " << msg << " (unexpected end of input)\n";
        else
            cerr << "Syntax Error at line " << t.line << ", column " << t.col << ": " << msg << "\n";
        exit(1);
    }


    void block() {
        if (!match(separator, "{"))
            error("Expected '{' to begin block.");

        while (!check(separator, "}") && !isAtEnd()) {
            statement();
        }

        if (!match(separator, "}"))
            error("Expected '}' to close block.");
    }

    void cout_stmt() {
        if (!match(operaTor, "<<"))
            error("Expected '<<' after 'cout'");

        cout_value();

        while (match(operaTor, "<<")) {
            cout_value();
        }
    }

    void cin_stmt() {
        if (!match(operaTor, ">>"))
            error("Expected '>>' after 'cin'");

        if (!match(identifier))
            error("Expected identifier after '>>'");

        while (match(operaTor, ">>")) {
            if (!match(identifier))
                error("Expected identifier after '>>'");
        }
    }

    void cout_value() {
        if (!(match(stringtype) || match(identifier) || match(number))) {
            error("Expected string, identifier, or number in cout");
        }
    }

    // <return_stmt> → return [ <expression> ] ;
    void return_stmt() {
        // consume the 'return' keyword
        if (!match(keyword, "return"))
            error("Expected 'return' keyword");

        // if the next token is not ';', parse an expression
        if (!check(separator, ";")) {
            expression();
        }

        // now require the trailing semicolon
        expect(";", "Expected ';' after return statement.");
    }


    // <statement> → <declaration> ; | <assignment> ; | <if_stmt> | <while_stmt> | <for_stmt>
    void statement() {
        // Handle function definition first
        if ((check(keyword,"int") || check(keyword,"void")) &&
            peekNext().type == identifier &&
            peekNext(2).type == separator && peekNext(2).value == "(") {
            function_decl();
            return;
        }
        if (match(keyword, "using")) {
            if (!match(keyword, "namespace"))
                error("Expected 'namespace' after 'using'");
            if (!match(identifier, "std"))
                error("Expected 'std' after 'namespace'");
            expect(";", "Expected ';' after using namespace std");
            return;  // skip it
        }

        if (check(separator, "{")) {
            block();
        }
        else if (isValidTypeKeyword()) {
            declaration();
            expect(";", "Expected ';' after declaration.");
        }
        else if (check(keyword,"if")) {
            if_stmt();
        }
        else if (check(keyword,"while")) {
            while_stmt();
        }
        else if (check(keyword,"for")) {
            for_stmt();
        }
        else if (check(keyword,"cout")) {   // updated to check keyword
            advance();  // consume 'cout'
            cout_stmt();
            expect(";", "Expected ';' after cout statement.");
        }
        else if (check(keyword,"cin")) {
            advance();  // consume 'cin'
            cin_stmt();
            expect(";", "Expected ';' after cin statement.");
        }
        else if (check(keyword,"return")) {
            return_stmt(); // you already have this
        }
        else if (check(identifier)) {
            assignment();
            expect(";", "Expected ';' after assignment.");
        }
        else {
            error("Unknown statement");
        }
    }

    bool isValidTypeKeyword() {
        return check(keyword, "int") ||
            check(keyword, "float") ||
            check(keyword, "char") ||
            check(keyword, "bool");
    }

    // <declaration> → <type> ID [= <expression>]
    // <declaration> → <type> ID [= <expression>]
    void declaration() {
        type();
        if (!match(identifier)) error("Expected identifier in declaration.");
        if (match(operaTor, "=")) {
            expression();
        }
    }

    // <assignment> → ID = <expression>
    void assignment() {
        if (!match(identifier)) error("Expected identifier in assignment.");
        expect("=", "Expected '=' in assignment.");
        expression();
    }

    // <type> → int | float | char | bool | void
    void type() {
        if (!(match(keyword,"int")   ||
              match(keyword,"float") ||
              match(keyword,"char")  ||
              match(keyword,"bool")  ||
              match(keyword,"void"))) {
            error("Expected type (int, float, char, bool, void)");
        }
    }

    // <if_stmt> → if ( <expression> ) <statement> [else <statement>]
    void if_stmt() {
        match(keyword, "if");
        expect("(", "Expected '(' after 'if'.");
        comparison();
        expect(")", "Expected ')' after condition.");
        statement();
        if (match(keyword, "else")) {
            statement();
        }
    }

    // <while_stmt> → while ( <expression> ) <statement>
    void while_stmt() {
        match(keyword, "while");
        expect("(", "Expected '(' after 'while'.");
        comparison();
        expect(")", "Expected ')' after condition.");
        statement();
    }

    // <for_stmt> → for ( <assignment> ; <expression> ; <assignment> ) <statement>
    void for_stmt() {
        match(keyword, "for");
        expect("(", "Expected '(' after 'for'.");
        assignment();
        expect(";", "Expected ';' after init assignment.");
        comparison();
        expect(";", "Expected ';' after loop condition.");
        assignment();
        expect(")", "Expected ')' after increment.");
        statement();
    }

    // <comparison> → <expression> [ ("<" | ">" | "<=" | ">=" | "==" | "!=") <expression> ]
    void comparison() {
        expression();
        while (match(operaTor, "<") || match(operaTor, ">") ||
            match(operaTor, "==") || match(operaTor, "!=") ||
            match(operaTor, "<=") || match(operaTor, ">=")) {
            expression(); // right-hand side
        }
    }


    // <expression> → <term> { (+|-) <term> }
    void expression() {
        term();
        while (match(operaTor, "+") || match(operaTor, "-")) {
            term();
        }
    }

    // <term> → <factor> { (*|/) <factor> }
    void term() {
        factor();
        while (match(operaTor, "*") || match(operaTor, "/")) {
            factor();
        }
    }

    // <factor> → NUMBER | ID | ( <expression> )
    void factor() {
        if (match(number) || match(identifier)) {
            return;
        }
        if (match(separator, "(")) {
            expression();
            expect(")", "Expected ')' after expression.");
            return;
        }
        error("Expected number, identifier or '('");
    }

    void function_decl() {
        type();  // consume int or void
        if (!match(identifier))
            error("Expected function name after return type");
        expect("(", "Expected '(' after function name");
        expect(")", "Expected ')' after function parameters");
        block(); // parse the { ... } body
    }


    void expect(const string& symbol, const string& errMsg) {
        if (!match(operaTor, symbol) && !match(separator, symbol)) {
            error(errMsg);
        }
    }
};