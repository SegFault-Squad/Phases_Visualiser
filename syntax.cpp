// syntax.cpp
#include <bits/stdc++.h>
#include "lexical.cpp"
using namespace std;

// Define the AST node structure
struct ASTNode {
    string type;
    vector<ASTNode*> children;
    string value;
    ASTNode(string t, vector<ASTNode*> c = {}, string v = "") 
      : type(t), children(c), value(v) {}
};

class Parser {
    vector<token> tokens;
    int current = 0;
    ASTNode* root; // Root of the AST

public:
    Parser(const vector<token>& t) : tokens(t), root(nullptr) {}

    void parse() {
        root = new ASTNode{"program"};
        while (!isAtEnd()) {
            ASTNode* stmt = statement();
            if (stmt) 
                root->children.push_back(stmt);
        }
        printAST(root, 0); // Print the AST after parsing
    }

private:
    // Helper function to print the AST
    void printAST(ASTNode* node, int indent) {
        for (int i = 0; i < indent; i++) 
            cout << "  ";
        cout << node->type;
        if (!node->value.empty()) 
            cout << ": " << node->value;
        cout << "\n";
        for (auto child : node->children) {
            printAST(child, indent + 1);
        }
    }

    // Basic token utilities
    token peek() {
        return (current < (int)tokens.size())
               ? tokens[current]
               : token{ unknown, "", -1, -1 };
    }

    token peekNext(int offset = 1) {
        int idx = current + offset;
        if (idx < (int)tokens.size()) 
            return tokens[idx];
        return token{ unknown, "", -1, -1 };
    }

    token advance() {
        return tokens[current++];
    }

    bool match(tokenType type, const string& val = "") {
        if (current >= (int)tokens.size()) 
            return false;
        if (tokens[current].type == type &&
            (val.empty() || tokens[current].value == val)) 
        {
            current++;
            return true;
        }
        return false;
    }

    bool check(tokenType type, const string& val = "") {
        if (isAtEnd()) 
            return false;
        return (tokens[current].type == type) &&
               (val.empty() || tokens[current].value == val);
    }

    bool isAtEnd() {
        return current >= (int)tokens.size();
    }

    void error(const string& msg) {
        token t = peek();
        if (t.line == -1) {
            cerr << "Syntax Error: " << msg << " (unexpected end of input)\n";
        } else {
            cerr << "Syntax Error at line " << t.line 
                 << ", column " << t.col << ": " << msg << "\n";
        }
        exit(1);
    }

    void expect(const string& symbol, const string& errMsg) {
        if (!match(operaTor, symbol) && !match(separator, symbol)) {
            error(errMsg);
        }
    }

    bool isValidTypeKeyword() {
        return check(keyword, "int") || check(keyword, "float") ||
               check(keyword, "char") || check(keyword, "bool");
    }

    // Modified parsing functions to return ASTNode*
    ASTNode* statement() {
        // 1) Skip any preprocessor directive entirely:
        if (check(preprocessor)) {
            advance();
            return nullptr;
        }

        // 2) Function declaration (e.g., "int foo()" or "void bar()")
        if ((check(keyword, "int") || check(keyword, "void")) &&
            peekNext().type == identifier &&
            peekNext(2).type == separator && peekNext(2).value == "(") 
        {
            return function_decl();
        }

        // 3) using namespace std;
        if (match(keyword, "using")) {
            if (!match(keyword, "namespace")) 
                error("Expected 'namespace' after 'using'");
            if (!match(identifier, "std")) 
                error("Expected 'std' after 'namespace'");
            expect(";", "Expected ';' after using namespace std");
            return nullptr; // ignore this in the AST
        }

        // 4) Block: "{ ... }"
        if (check(separator, "{")) {
            return block();
        }

        // 5) Declaration: e.g., "int x;" or "float y = 3;"
        if (isValidTypeKeyword()) {
            ASTNode* decl = declaration();
            expect(";", "Expected ';' after declaration.");
            return decl;
        }

        // 6) if-statement
        if (check(keyword, "if")) {
            return if_stmt();
        }

        // 7) while-statement
        if (check(keyword, "while")) {
            return while_stmt();
        }

        // 8) for-statement
        if (check(keyword, "for")) {
            return for_stmt();
        }

        // 9) cout-statement
        if (check(keyword, "cout")) {
            advance(); // consume 'cout'
            ASTNode* coutNode = cout_stmt();
            expect(";", "Expected ';' after cout statement.");
            return coutNode;
        }

        // 10) cin-statement
        if (check(keyword, "cin")) {
            advance(); // consume 'cin'
            ASTNode* cinNode = cin_stmt();
            expect(";", "Expected ';' after cin statement.");
            return cinNode;
        }

        // 11) return-statement
        if (check(keyword, "return")) {
            return return_stmt();
        }

        // 12) assignment (identifier = expression;)
        if (check(identifier) && peekNext().type == operaTor && peekNext().value == "=") {
            ASTNode* assign = assignment();
            expect(";", "Expected ';' after assignment.");
            return assign;
        }

        // 13) Standalone semicolon or unknown separators can be skipped
        if (check(separator, ";")) {
            advance();
            return nullptr;
        }

        // 14) If we reach here and it's not the end, it's an unknown statement
        error("Unknown statement");
        return nullptr;
    }

    ASTNode* block() {
        expect("{", "Expected '{' to begin block.");
        ASTNode* blockNode = new ASTNode{"block"};
        // Collect statements until matching "}"
        while (!check(separator, "}") && !isAtEnd()) {
            ASTNode* stmt = statement();
            if (stmt) 
                blockNode->children.push_back(stmt);
        }
        expect("}", "Expected '}' to close block.");
        return blockNode;
    }

    ASTNode* cout_stmt() {
        ASTNode* coutNode = new ASTNode{"cout"};
        if (!match(operaTor, "<<")) 
            error("Expected '<<' after 'cout'");
        ASTNode* val = cout_value();
        coutNode->children.push_back(val);
        while (match(operaTor, "<<")) {
            val = cout_value();
            coutNode->children.push_back(val);
        }
        return coutNode;
    }

    ASTNode* cin_stmt() {
        ASTNode* cinNode = new ASTNode{"cin"};
        if (!match(operaTor, ">>")) 
            error("Expected '>>' after 'cin'");
        if (!match(identifier)) 
            error("Expected identifier after '>>'");
        cinNode->children.push_back(new ASTNode{"identifier", {}, tokens[current-1].value});
        while (match(operaTor, ">>")) {
            if (!match(identifier)) 
                error("Expected identifier after '>>'");
            cinNode->children.push_back(new ASTNode{"identifier", {}, tokens[current-1].value});
        }
        return cinNode;
    }

    ASTNode* cout_value() {
        if (match(stringtype)) {
            return new ASTNode{"string", {}, tokens[current-1].value};
        }
        else if (match(identifier)) {
            return new ASTNode{"identifier", {}, tokens[current-1].value};
        }
        else if (match(number)) {
            return new ASTNode{"number", {}, tokens[current-1].value};
        }
        else {
            error("Expected string, identifier, or number in cout");
            return nullptr;
        }
    }

    ASTNode* return_stmt() {
        match(keyword, "return");
        ASTNode* returnNode = new ASTNode{"return"};
        if (!check(separator, ";")) {
            ASTNode* expr = expression();
            returnNode->children.push_back(expr);
        }
        expect(";", "Expected ';' after return statement.");
        return returnNode;
    }

    ASTNode* declaration() {
        string typeStr = peek().value;
        type();
        if (!match(identifier)) 
            error("Expected identifier in declaration.");
        string id = tokens[current-1].value;
        ASTNode* declNode = new ASTNode{"declaration"};
        declNode->children.push_back(new ASTNode{"type", {}, typeStr});
        declNode->children.push_back(new ASTNode{"identifier", {}, id});
        if (match(operaTor, "=")) {
            ASTNode* expr = expression();
            declNode->children.push_back(expr);
        }
        return declNode;
    }

    ASTNode* assignment() {
        if (!match(identifier)) 
            error("Expected identifier in assignment.");
        string id = tokens[current-1].value;
        expect("=", "Expected '=' in assignment.");
        ASTNode* expr = expression();
        ASTNode* assignNode = new ASTNode{"assignment"};
        assignNode->children.push_back(new ASTNode{"identifier", {}, id});
        assignNode->children.push_back(expr);
        return assignNode;
    }

    ASTNode* if_stmt() {
        match(keyword, "if");
        expect("(", "Expected '(' after 'if'.");
        ASTNode* condition = comparison();
        expect(")", "Expected ')' after condition.");
        ASTNode* thenStmt = statement();
        ASTNode* ifNode = new ASTNode{"if"};
        ifNode->children.push_back(condition);
        ifNode->children.push_back(thenStmt);
        if (match(keyword, "else")) {
            ASTNode* elseStmt = statement();
            ifNode->children.push_back(elseStmt);
        }
        return ifNode;
    }

    ASTNode* while_stmt() {
        match(keyword, "while");
        expect("(", "Expected '(' after 'while'.");
        ASTNode* condition = comparison();
        expect(")", "Expected ')' after condition.");
        ASTNode* body = statement();
        ASTNode* whileNode = new ASTNode{"while"};
        whileNode->children.push_back(condition);
        whileNode->children.push_back(body);
        return whileNode;
    }

    ASTNode* for_stmt() {
        match(keyword, "for");
        expect("(", "Expected '(' after 'for'.");
        ASTNode* init = assignment();
        expect(";", "Expected ';' after init assignment.");
        ASTNode* condition = comparison();
        expect(";", "Expected ';' after loop condition.");
        ASTNode* increment = assignment();
        expect(")", "Expected ')' after increment.");
        ASTNode* body = statement();
        ASTNode* forNode = new ASTNode{"for"};
        forNode->children.push_back(init);
        forNode->children.push_back(condition);
        forNode->children.push_back(increment);
        forNode->children.push_back(body);
        return forNode;
    }

    ASTNode* comparison() {
        ASTNode* left = expression();
        while (match(operaTor, "<") || match(operaTor, ">") ||
               match(operaTor, "==") || match(operaTor, "!=") ||
               match(operaTor, "<=") || match(operaTor, ">=")) 
        {
            string op = tokens[current-1].value;
            ASTNode* right = expression();
            ASTNode* compNode = new ASTNode{"comparison", {left, right}, op};
            left = compNode;
        }
        return left;
    }

    ASTNode* expression() {
        ASTNode* left = term();
        while (match(operaTor, "+") || match(operaTor, "-")) {
            string op = tokens[current-1].value;
            ASTNode* right = term();
            left = new ASTNode{"binary", {left, right}, op};
        }
        return left;
    }

    ASTNode* term() {
        ASTNode* left = factor();
        while (match(operaTor, "*") || match(operaTor, "/")) {
            string op = tokens[current-1].value;
            ASTNode* right = factor();
            left = new ASTNode{"binary", {left, right}, op};
        }
        return left;
    }

    ASTNode* factor() {
        if (match(number)) {
            return new ASTNode{"number", {}, tokens[current-1].value};
        }
        else if (match(identifier)) {
            return new ASTNode{"identifier", {}, tokens[current-1].value};
        }
        else if (match(separator, "(")) {
            ASTNode* expr = expression();
            expect(")", "Expected ')' after expression.");
            return expr;
        }
        else {
            error("Expected number, identifier or '('");
            return nullptr;
        }
    }

    ASTNode* function_decl() {
        string returnType = peek().value;
        type();
        if (!match(identifier)) 
            error("Expected function name after return type");
        string funcName = tokens[current-1].value;
        expect("(", "Expected '(' after function name");
        expect(")", "Expected ')' after function parameters");
        ASTNode* body = block();
        ASTNode* funcNode = new ASTNode{"function"};
        funcNode->children.push_back(new ASTNode{"returnType", {}, returnType});
        funcNode->children.push_back(new ASTNode{"identifier", {}, funcName});
        funcNode->children.push_back(body);
        return funcNode;
    }

    void type() {
        if (!(match(keyword, "int")   ||
              match(keyword, "float") ||
              match(keyword, "char")  ||
              match(keyword, "bool")  ||
              match(keyword, "void"))) 
        {
            error("Expected type (int, float, char, bool, void)");
        }
    }
};


/*int main() {
    ifstream file("input.cpp");
    if (!file.is_open()) {
        cerr << "Error opening file\n";
        return 1;
    }
    string code((istreambuf_iterator<char>(file)), {});
    vector<token> toks = tokenize(code);

    //Assuming your syntax analyzer has already run successfully:
    Parser p(toks);
    p.parse();

   /* SemanticAnalyzer sem(toks);
    sem.analyze();
    return 0;
}*/
