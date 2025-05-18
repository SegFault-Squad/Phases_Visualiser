// semantic.cpp
#include <bits/stdc++.h>
#include "syntax.cpp"
using namespace std;

struct Symbol {
    string type;
    int scopeLevel;
};

class SemanticAnalyzer {
    vector<token> tokens;
    int current = 0;
    int currentScopeLevel = 0;
    vector<unordered_map<string, Symbol>> symbolTableStack;

public:
    SemanticAnalyzer(const vector<token>& t)
      : tokens(t)
    {
        symbolTableStack.push_back({});  // global scope
    }

    void analyze() {
        while (!isAtEnd()) {
            statement();
        }
        cout << "Semantic Analysis Successful.\n";
    }

private:
    // ——— Basic token utilities ———
    token peek()              { return current < tokens.size() ? tokens[current] : token{unknown,"",-1,-1}; }
    token advance()           { return tokens[current++]; }
    bool match(tokenType ty, const string& v="") {
        if (current < tokens.size() &&
            tokens[current].type == ty &&
            (v.empty() || tokens[current].value == v))
        {
            current++;
            return true;
        }
        return false;
    }
    bool check(tokenType ty, const string& v="") {
        return (current < tokens.size() &&
                tokens[current].type == ty &&
                (v.empty() || tokens[current].value == v));
    }
    bool isAtEnd() { return current >= tokens.size(); }

    void error(const string& msg) {
        auto t = peek();
        cerr << "Semantic Error at line " << t.line
             << ", column " << t.col << ": " << msg << "\n";
        exit(1);
    }

    // ——— Scope management ———
    void enterScope() {
        symbolTableStack.push_back({});
        currentScopeLevel++;
    }
    void exitScope() {
        symbolTableStack.pop_back();
        currentScopeLevel--;
    }
    auto& currentScope() {
        return symbolTableStack.back();
    }

    bool isDeclared(const string& name) {
        for (int i = symbolTableStack.size() - 1; i >= 0; --i)
            if (symbolTableStack[i].count(name))
                return true;
        return false;
    }
    string getType(const string& name) {
        for (int i = symbolTableStack.size() - 1; i >= 0; --i)
            if (symbolTableStack[i].count(name))
                return symbolTableStack[i][name].type;
        return "";
    }
    bool typesCompatible(const string& lhs, const string& rhs) {
        if (lhs == rhs) return true;
        // int ↔ float both ways allowed
        const set<string> num = {"int","float"};
        if (num.count(lhs) && num.count(rhs)) return true;
        return false;
    }

    // ——— Top-level dispatcher ———
    void statement() {
        if (match(separator, "{")) {
            enterScope();
            while (!match(separator, "}")) {
                if (isAtEnd()) error("Unclosed block in semantic pass");
                statement();
            }
            exitScope();
        }
        else if (check(keyword, "int")  ||
                 check(keyword, "float")||
                 check(keyword, "char") ||
                 check(keyword, "bool"))
        {
            declaration();
            // optionally consume trailing ';'
            if (match(separator, ";")) {}
        }
        else if (check(identifier) && peekNext().type == operaTor && peekNext().value == "=") {
            assignment();
            if (match(separator, ";")) {}
        }
        else {
            // skip any other tokens (including ';')
            advance();
        }
    }

    token peekNext(int off = 1) {
        int idx = current + off;
        return idx < tokens.size() ? tokens[idx] : token{unknown,"",-1,-1};
    }

    // ——— Declarations ———
    void declaration() {
        string varType = advance().value;  // consume type keyword
        if (!match(identifier))
            error("Expected variable name in declaration");
        string varName = tokens[current - 1].value;

        // duplicate in same scope?
        if (currentScope().count(varName))
            error("Variable '" + varName + "' redeclared in same scope");

        currentScope()[varName] = {varType, currentScopeLevel};

        // optional initializer
        if (match(operaTor, "=")) {
            string rhs = evaluateExpressionType();
            if (!typesCompatible(varType, rhs))
                error("Cannot initialize '" + varName +
                      "' (" + varType + ") with " + rhs);
        }
    }

    // ——— Assignments ———
    void assignment() {
        advance();  // identifier
        string varName = tokens[current - 1].value;

        if (!isDeclared(varName))
            error("Variable '" + varName + "' used before declaration");

        string lhs = getType(varName);
        match(operaTor, "=");
        string rhs = evaluateExpressionType();
        if (!typesCompatible(lhs, rhs))
            error("Cannot assign " + rhs + " to '" + varName + "' (" + lhs + ")");
    }

    // ——— Expression type eval ———
    string evaluateExpressionType() {
        // primary
        string res = primaryType();

        // * /
        while (check(operaTor, "*") || check(operaTor, "/")) {
            advance();
            string r = primaryType();
            res = (res=="float"||r=="float") ? "float" : "int";
        }
        // + -
        while (check(operaTor, "+") || check(operaTor, "-")) {
            advance();
            string r = primaryType();
            res = (res=="float"||r=="float") ? "float" : "int";
        }
        return res;
    }

    // ——— Primary (number | identifier | '(' expr ')' ) ———
    string primaryType() {
        if (match(number)) {
            string lit = tokens[current - 1].value;
            return (lit.find('.') != string::npos) ? "float" : "int";
        }
        if (match(identifier)) {
            string name = tokens[current - 1].value;
            if (!isDeclared(name))
                error("Variable '" + name + "' used before declaration");
            return getType(name);
        }
        if (match(separator, "(")) {
            string t = evaluateExpressionType();
            match(separator, ")");
            return t;
        }
        error("Invalid expression in semantic pass");
        return "";
    }
};

int main() {
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

    SemanticAnalyzer sem(toks);
    sem.analyze();
    return 0;
}
