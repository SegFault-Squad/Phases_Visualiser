// semantic.cpp
#include <bits/stdc++.h>
#include "syntax.cpp"   // brings in Parser, tokenize(), token, etc.
using namespace std;

// —————————————————————————————————————————————————————————————
// A “Symbol” entry: stores type, scope level, memory address, and value.
// —————————————————————————————————————————————————————————————
struct Symbol {
    string type;             // e.g. "int", "float", "char", ...
    int scopeLevel;          // 0 = global, 1 = first nested block, etc.
    string memoryAddress;    // mock hex address, e.g. "0x1000"
    string value;            // either literal or "Uninitialized"
};

// —————————————————————————————————————————————————————————————
// The SemanticAnalyzer builds a vector of (name → Symbol) entries
// and, at the end, prints a 5-column ASCII table: Name | Type | Scope | Memory Address | Value
// —————————————————————————————————————————————————————————————
class SemanticAnalyzer {
    vector<token> tokens;
    int current = 0;
    int currentScopeLevel = 0;

    // Stack of “maps” from (name → Symbol) for each scope level:
    vector<map<string, Symbol>> symbolTableStack;

    // Preserve insertion order so we can print in declaration order:
    vector<pair<string, Symbol>> symbolEntries;

    // Next mock address (4-byte increments) starting at 0x1000:
    unsigned int nextAddress = 0x1000;

public:
    SemanticAnalyzer(const vector<token>& t)
      : tokens(t)
    {
        // Start with one global scope (level 0):
        symbolTableStack.push_back({});
    }

    void analyze() {
        // Walk all top-level statements/blocks:
        while (!isAtEnd()) {
            statement();
        }
        // Print the 5-column symbol table:
        printSymbolTable();
        cout << "Semantic Analysis Successful.\n";
    }

private:
    // ————————————————————————————— Token Utilities —————————————————————————————
    token peek() {
        if (current < (int)tokens.size())
            return tokens[current];
        return token{ unknown, "", -1, -1 };
    }

    token advance() {
        return tokens[current++];
    }

    bool match(tokenType ty, const string& v = "") {
        if (current < (int)tokens.size() &&
            tokens[current].type == ty &&
            (v.empty() || tokens[current].value == v))
        {
            current++;
            return true;
        }
        return false;
    }

    bool check(tokenType ty, const string& v = "") {
        if (current >= (int)tokens.size()) return false;
        return (tokens[current].type == ty) &&
               (v.empty() || tokens[current].value == v);
    }

    bool isAtEnd() {
        return current >= (int)tokens.size();
    }

    token peekNext(int offset = 1) {
        int idx = current + offset;
        if (idx < (int)tokens.size())
            return tokens[idx];
        return token{ unknown, "", -1, -1 };
    }

    void error(const string& msg) {
        token t = peek();
        if (t.line == -1) {
            cerr << "Semantic Error: " << msg << " (unexpected end of input)\n";
        } else {
            cerr << "Semantic Error at line " << t.line
                 << ", column " << t.col << ": " << msg << "\n";
        }
        exit(1);
    }

    // Return reference to current (innermost) scope’s map:
    map<string, Symbol>& currentScope() {
        return symbolTableStack.back();
    }

    // Search through scopes to see if name was declared:
    bool isDeclared(const string& name) {
        for (int i = (int)symbolTableStack.size() - 1; i >= 0; --i) {
            if (symbolTableStack[i].count(name)) return true;
        }
        return false;
    }

    // Look up type of name from innermost → outermost:
    string getType(const string& name) {
        for (int i = (int)symbolTableStack.size() - 1; i >= 0; --i) {
            auto& tbl = symbolTableStack[i];
            if (tbl.count(name)) {
                return tbl[name].type;
            }
        }
        return "";
    }

    // int ↔ float compatibility; otherwise must match exactly:
    bool typesCompatible(const string& lhs, const string& rhs) {
        if (lhs == rhs) return true;
        const set<string> num = { "int", "float" };
        if (num.count(lhs) && num.count(rhs)) return true;
        return false;
    }

    // ————————————————————————————— Print 5-Column Symbol Table —————————————————————————————
    void printSymbolTable() {
        if (symbolEntries.empty()) {
            cout << "No symbols declared.\n";
            return;
        }

        // Determine max width of each column:
        size_t nameW   = strlen("Name");
        size_t typeW   = strlen("Type");
        size_t scopeW  = strlen("Scope");
        size_t addrW   = strlen("Memory Address");
        size_t valueW  = strlen("Value");

        for (auto& pr : symbolEntries) {
            const string& nm  = pr.first;
            const Symbol& sym = pr.second;
            nameW   = max(nameW, nm.size());
            typeW   = max(typeW, sym.type.size());
            // Scope as string
            string sScope = to_string(sym.scopeLevel);
            scopeW  = max(scopeW, sScope.size());
            addrW   = max(addrW, sym.memoryAddress.size());
            valueW  = max(valueW, sym.value.size());
        }

        // Build a horizontal border: +--nameW--+--typeW--+--scopeW--+--addrW--+--valueW--+
        auto mkBorder = [&]() {
            string border = "+";
            border += string(nameW  + 2, '-') + "+";
            border += string(typeW  + 2, '-') + "+";
            border += string(scopeW + 2, '-') + "+";
            border += string(addrW  + 2, '-') + "+";
            border += string(valueW + 2, '-') + "+";
            return border;
        };

        string border = mkBorder();

        // Header row:
        cout << border << "\n";
        cout << "| " << left << setw(nameW)   << "Name"
             << " | " << left << setw(typeW)   << "Type"
             << " | " << right << setw(scopeW) << "Scope"
             << " | " << left << setw(addrW)   << "Memory Address"
             << " | " << left << setw(valueW)  << "Value"
             << " |\n";
        cout << border << "\n";

        // Each entry row:
        for (auto& pr : symbolEntries) {
            const string& nm = pr.first;
            const Symbol& sym = pr.second;
            string sScope = to_string(sym.scopeLevel);

            cout << "| " << left << setw(nameW)   << nm
                 << " | " << left << setw(typeW)   << sym.type
                 << " | " << right << setw(scopeW) << sScope
                 << " | " << left << setw(addrW)   << sym.memoryAddress
                 << " | " << left << setw(valueW)  << sym.value
                 << " |\n";
        }
        cout << border << "\n";
    }

    // ————————————————————————————— Top-Level Statement Dispatcher —————————————————————————————
    void statement() {
        // 1) Block “{ … }”
        if (match(separator, "{")) {
            symbolTableStack.push_back({});   // new nested scope
            currentScopeLevel++;

            while (!match(separator, "}")) {
                if (isAtEnd()) {
                    error("Unclosed block in semantic analysis");
                }
                statement();
            }

            symbolTableStack.pop_back();
            currentScopeLevel--;
        }
        // 2) Declaration: int x;  or  float y = 3;
        else if (check(keyword, "int")   ||
                 check(keyword, "float") ||
                 check(keyword, "char")  ||
                 check(keyword, "bool"))
        {
            declaration();
            match(separator, ";");  // consume trailing “;” if present
        }
        // 3) Assignment: x = expr;
        else if (check(identifier) &&
                 peekNext().type == operaTor &&
                 peekNext().value == "=")
        {
            assignment();
            match(separator, ";");
        }
        // 4) Otherwise skip (including stray semicolons)
        else {
            advance();
        }
    }

    // ————————————————————————————— Declarations —————————————————————————————
    void declaration() {
        // Next token is a type keyword
        string varType = advance().value;  // e.g. "int", "float", …

        if (!match(identifier)) {
            error("Expected variable name after type");
        }
        string varName = tokens[current - 1].value;

        // Redeclaration check (current scope only)
        if (currentScope().count(varName)) {
            error("Variable '" + varName + "' redeclared in same scope");
        }

        // Mock memory address: allocate 4 bytes at nextAddress
        char buf[20];
        snprintf(buf, sizeof(buf), "0x%04X", nextAddress);
        string addrStr = buf;
        nextAddress += 4;

        // Initializer value (literal or “Uninitialized”)
        string initVal = "Uninitialized";
        if (match(operaTor, "=")) {
            if (check(number)) {
                initVal = tokens[current].value;
                advance();
            }
            else if (check(identifier)) {
                string rhsName = tokens[current].value;
                if (!isDeclared(rhsName)) {
                    error("Variable '" + rhsName + "' used before declaration in initializer");
                }
                initVal = rhsName;
                advance();
            }
            else {
                error("Only single-literal or identifier initialization supported");
            }
        }

        // Create Symbol, insert into current scope and symbolEntries
        Symbol sym;
        sym.type          = varType;
        sym.scopeLevel    = currentScopeLevel;
        sym.memoryAddress = addrStr;
        sym.value         = initVal;

        currentScope()[varName] = sym;
        symbolEntries.push_back({ varName, sym });
    }

    // ————————————————————————————— Assignments (type-check only) —————————————————————————————
    void assignment() {
        advance();  // consume identifier
        string varName = tokens[current - 1].value;

        if (!isDeclared(varName)) {
            error("Variable '" + varName + "' used before declaration");
        }
        string lhsType = getType(varName);

        match(operaTor, "=");

        // Determine RHS type (single literal or identifier)
        string rhsType;
        if (check(number)) {
            string rhsVal = tokens[current].value;
            rhsType = (rhsVal.find('.') != string::npos) ? "float" : "int";
            advance();
        }
        else if (check(identifier)) {
            string rhsName = tokens[current].value;
            if (!isDeclared(rhsName)) {
                error("Variable '" + rhsName + "' used before declaration in assignment");
            }
            rhsType = getType(rhsName);
            advance();
        }
        else {
            error("Only single-literal or identifier assignment supported");
        }

        if (!typesCompatible(lhsType, rhsType)) {
            error("Cannot assign type '" + rhsType + "' to variable '" 
                   + varName + "' (" + lhsType + ")");
        }

        // NOTE: We do NOT update `symbolEntries[].second.value` here,
        // so declaration-time “Uninitialized” remains if there was no initializer.
    }

    // ————————————————————————————— Expression-Type Evaluation (unused for “value”) —————————————————————————————
    string evaluateExpressionType() {
        string res = primaryType();
        while (check(operaTor, "*") || check(operaTor, "/")) {
            advance();
            string r = primaryType();
            res = (res == "float" || r == "float") ? "float" : "int";
        }
        while (check(operaTor, "+") || check(operaTor, "-")) {
            advance();
            string r = primaryType();
            res = (res == "float" || r == "float") ? "float" : "int";
        }
        return res;
    }

    string primaryType() {
        if (match(number)) {
            string lit = tokens[current - 1].value;
            return (lit.find('.') != string::npos) ? "float" : "int";
        }
        if (match(identifier)) {
            string name = tokens[current - 1].value;
            if (!isDeclared(name)) {
                error("Variable '" + name + "' used before declaration in expression");
            }
            return getType(name);
        }
        if (match(separator, "(")) {
            string inner = evaluateExpressionType();
            match(separator, ")");
            return inner;
        }
        error("Invalid expression in semantic analysis");
        return "";
    }
};
