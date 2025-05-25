#include <iostream>
#include <fstream>
#include <string>
#include "semantic.cpp"  // Includes syntax.cpp → lexical.cpp

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: analyzer <mode> <input_file>\n";
        return 1;
    }

    string mode = argv[1];
    string filename = argv[2];

    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Could not open input file.\n";
        return 1;
    }

    string code((istreambuf_iterator<char>(file)), {});
    vector<token> toks = tokenize(code);

    if (mode == "lexical") {
        for (const auto& t : toks) {
            cout << "Type: " << tokenToString(t.type)
                 << ", Value: '" << t.value
                 << "', Line: " << t.line
                 << ", Column: " << t.col << "\n";
        }
    } else if (mode == "syntax") {
        Parser p(toks);
        p.parse();
    } else if (mode == "semantic") {
        //Parser p(toks);
        //p.parse();
        SemanticAnalyzer sem(toks);
        sem.analyze();
    } else {
        cerr << "Invalid mode.\n";
        return 1;
    }

    return 0;
}
