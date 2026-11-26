
#include <bits/stdc++.h>
using namespace std;

struct Token {
    string text, kind, file;
    size_t pos;
    int line, col;
};

struct Node {
    string kind, type = "";
    Token t;
    vector<Node> children;
};

vector<string> errors;

void err(Token t, string s) {
    errors.push_back(
        t.file + ":" + to_string(t.line) + ":" + to_string(t.col) + ": " + s
    );
}

vector<Token> lex(const string& s, const string& filename) {
    vector<Token> v;
    size_t i = 0;
    int l = 1, c = 1;

    auto advance = [&]() {
        if (s[i++] == '\n') {
            l++;
            c = 1;
        } else {
            c++;
        }
    };

    set<string> kw = {
        "array", "boolean", "char", "else", "false", "for",
        "function", "if", "integer", "print", "return",
        "string", "true", "void", "while"
    };

    while (i < s.size()) {
        if (isspace((unsigned char)s[i])) {
            advance();
            continue;
        }

        Token t{"", "", filename, i, l, c};
        char a = s[i];

        if (s.compare(i, 2, "//") == 0) {
            while (i < s.size() && s[i] != '\n') {
                advance();
            }

            t.kind = "comment";
        } else if (s.compare(i, 2, "/*") == 0) {
            advance();
            advance();

            while (i < s.size() && s.compare(i, 2, "*/")) {
                advance();
            }

            if (i == s.size()) {
                err(t, "unterminated comment");
            } else {
                advance();
                advance();
            }

            t.kind = "comment";
        } else if (isalpha((unsigned char)a) || a == '_') {
            while (
                i < s.size() &&
                (isalnum((unsigned char)s[i]) || s[i] == '_')
            ) {
                advance();
            }

            t.kind = "id";

            if (i - t.pos > 256) {
                err(t, "identifier exceeds 256 characters");
            }
        } else if (isdigit((unsigned char)a)) {
            while (i < s.size() && isdigit((unsigned char)s[i])) {
                advance();
            }

            t.kind = "lit";

            try {
                stoll(s.substr(t.pos, i - t.pos));
            } catch (...) {
                err(t, "integer overflow");
            }
        } else if (a == '\'' || a == '"') {
            advance();

            int n = 0;

            while (i < s.size() && s[i] != a && s[i] != '\n') {
                if (s[i] == '\\') {
                    advance();

                    if (i == s.size()) {
                        break;
                    }
                }

                advance();
                n++;
            }

            if (i < s.size() && s[i] == a) {
                advance();
            } else {
                err(t, "unterminated literal");
            }

            if ((a == '\'' && n != 1) || (a == '"' && n > 256)) {
                err(t, "invalid literal length");
            }

            t.kind = "lit";
        } else {
            string two = s.substr(i, 2);

            if (
                set<string>{
                    "++", "--", "<=", ">=", "=<", "=>",
                    "==", "!=", "&&", "||"
                }.count(two)
            ) {
                advance();
                advance();
                t.kind = "op";
            } else {
                advance();

                t.kind =
                    string("+-*/%^!=<>").find(a) != string::npos
                        ? "op"
                        : string("()[]{};:,").find(a) != string::npos
                            ? "punct"
                            : "invalid";

                if (t.kind == "invalid") {
                    err(t, "invalid character");
                }
            }
        }

        t.text = s.substr(t.pos, i - t.pos);

        if (kw.count(t.text)) {
            t.kind =
                (t.text == "true" || t.text == "false")
                    ? "lit"
                    : set<string>{
                        "integer", "boolean", "char", "string",
                        "void", "array", "function"
                    }.count(t.text)
                        ? "type"
                        : "kw";
        }

        v.push_back(t);
    }

    v.push_back({"", "eof", filename, i, l, c});

    return v;
}

struct Parser {
    vector<Token> v;
    size_t i = 0;

    Parser(vector<Token> a) {
        for (auto t : a) {
            if (t.kind != "comment") {
                v.push_back(t);
            }
        }
    }

    Token cur() {
        return v[i];
    }

    bool at(string s) {
        return cur().text == s;
    }

    Token take() {
        auto t = cur();

        if (t.kind != "eof") {
            i++;
        }

        return t;
    }

    bool eat(string s) {
        if (!at(s)) {
            return false;
        }

        take();
        return true;
    }

    void fail(string s) {
        err(cur(), s);
        throw runtime_error(s);
    }

    Token need(string s) {
        if (!at(s)) {
            fail("expected " + s);
        }

        return take();
    }

    Token id() {
        if (cur().kind != "id") {
            fail("expected identifier");
        }

        return take();
    }

    Node node(string k, Token t, vector<Node> c = {}) {
        return {k, "", t, c};
    }

    int prec(string s) {
        if (s == "=") return 1;
        if (s == "||") return 2;
        if (s == "&&") return 3;
        if (s == "==" || s == "!=") return 4;

        if (
            s == "<" || s == ">" ||
            s == "<=" || s == ">=" ||
            s == "=<" || s == "=>"
        ) {
            return 5;
        }

        if (s == "+" || s == "-") return 6;
        if (s == "*" || s == "/" || s == "%") return 7;
        if (s == "^") return 8;

        return 0;
    }

    Node expr(int p = 1) {
        auto t = cur();
        Node a;

        if (at("-") || at("!")) {
            take();
            a = node("UnaryExpr", t, {expr(9)});
        } else if (eat("(")) {
            a = expr();
            need(")");
        } else if (t.kind == "id" || t.kind == "lit") {
            take();
            a = node(t.kind == "id" ? "Identifier" : "Literal", t);
        } else {
            fail("expected expression");
        }

        while (true) {
            auto op = cur();

            if (eat("(")) {
                vector<Node> ch{a};

                if (!at(")")) {
                    do {
                        ch.push_back(expr());
                    } while (eat(","));
                }

                need(")");
                a = node("CallExpr", a.t, ch);
            } else if (eat("[")) {
                auto b = expr();
                need("]");
                a = node("IndexExpr", a.t, {a, b});
            } else if (eat("++") || eat("--")) {
                a = node("PostfixExpr", op, {a});
            } else {
                break;
            }
        }

        while (prec(cur().text) >= p) {
            auto op = take();
            int q = prec(op.text);

            auto b = expr(
                q + ((op.text == "=" || op.text == "^") ? 0 : 1)
            );

            a = node("BinaryExpr", op, {a, b});
        }

        return a;
    }

    Node type() {
        auto t = cur();

        if (eat("array")) {
            vector<Node> ch;

            need("[");

            if (!at("]")) {
                ch.push_back(expr());
            }

            need("]");
            ch.push_back(type());

            return node("ArrayType", t, ch);
        }

        if (eat("function")) {
            vector<Node> ch{type()};

            need("(");

            if (!at(")")) {
                do {
                    auto n = id();
                    need(":");

                    ch.push_back(
                        node("Param", n, {type()})
                    );
                } while (eat(","));
            }

            need(")");

            return node("FunctionType", t, ch);
        }

        if (
            !set<string>{
                "integer", "boolean", "char", "string", "void"
            }.count(t.text)
        ) {
            fail("expected type");
        }

        take();

        return node("Type", t);
    }

    Node init() {
        auto t = cur();

        if (!eat("{")) {
            return expr();
        }

        vector<Node> ch;

        if (!at("}")) {
            do {
                ch.push_back(init());
            } while (eat(","));
        }

        need("}");

        return node("Initializer", t, ch);
    }

    Node decl(bool global) {
        auto t = id();

        need(":");

        auto ty = type();
        bool f = ty.kind == "FunctionType";

        vector<Node> ch{ty};

        if (f && !global) {
            fail("nested function");
        }

        if (eat("=")) {
            if (f) {
                ch.push_back(block());
                return node("FuncDecl", t, ch);
            }

            ch.push_back(init());
        }

        need(";");

        return node(f ? "FuncDecl" : "VarDecl", t, ch);
    }

    void sync(bool inside) {
        while (
            cur().kind != "eof" &&
            !at(";") &&
            !at("}")
        ) {
            take();
        }

        if (at(";") || (!inside && at("}"))) {
            take();
        }
    }

    Node block() {
        auto t = need("{");
        vector<Node> ch;

        while (!at("}") && cur().kind != "eof") {
            try {
                ch.push_back(stmt());
            } catch (runtime_error&) {
                sync(true);
            }
        }

        need("}");

        return node("Block", t, ch);
    }

    Node stmt() {
        auto t = cur();

        if (at("{")) {
            return block();
        }

        if (
            t.kind == "id" &&
            i + 1 < v.size() &&
            v[i + 1].text == ":"
        ) {
            return decl(false);
        }

        if (eat("if")) {
            need("(");

            auto a = expr();

            need(")");

            vector<Node> ch{a, stmt()};

            if (eat("else")) {
                ch.push_back(stmt());
            }

            return node("IfStmt", t, ch);
        }

        if (eat("for")) {
            need("(");

            vector<Node> ch;

            for (auto end : {";", ";", ")"}) {
                ch.push_back(
                    at(end) ? node("Empty", cur()) : expr()
                );

                need(end);
            }

            ch.push_back(stmt());

            return node("ForStmt", t, ch);
        }

        if (eat("return") || eat("print")) {
            vector<Node> ch;

            if (!at(";")) {
                ch.push_back(expr());

                if (t.text == "print") {
                    while (eat(",")) {
                        ch.push_back(expr());
                    }
                }
            }

            need(";");

            return node(
                t.text == "return" ? "ReturnStmt" : "PrintStmt",
                t,
                ch
            );
        }

        if (eat(";")) {
            return node("Empty", t);
        }

        auto a = expr();

        need(";");

        return node("ExprStmt", t, {a});
    }

    Node run() {
        auto root = node("Program", cur());

        while (cur().kind != "eof") {
            try {
                root.children.push_back(decl(true));
            } catch (runtime_error&) {
                sync(false);
            }
        }

        return root;
    }
};

void dump(const Node& n, int d = 0) {
    cout
        << string(d, ' ')
        << n.kind
        << " "
        << quoted(n.t.text)
        << " @"
        << n.t.file
        << ":"
        << n.t.line
        << ":"
        << n.t.col
        << " type="
        << (n.type.empty() ? "<null>" : n.type)
        << '\n';

    for (auto& c : n.children) {
        dump(c, d + 2);
    }
}

void mark(const Node& n, map<size_t, string>& m) {
    if (n.kind == "FuncDecl") {
        m[n.t.pos] = "func";
    }

    if (n.kind == "VarDecl" || n.kind == "Param") {
        m[n.t.pos] = "var";
    }

    if (
        n.kind == "CallExpr" &&
        !n.children.empty() &&
        n.children[0].kind == "Identifier"
    ) {
        m[n.children[0].t.pos] = "func";
    }

    for (auto& c : n.children) {
        mark(c, m);
    }
}

string escapeHtml(string s) {
    string r;

    for (char c : s) {
        if (c == '&') {
            r += "&amp;";
        } else if (c == '<') {
            r += "&lt;";
        } else if (c == '>') {
            r += "&gt;";
        } else {
            r += c;
        }
    }

    return r;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        cerr << "usage: bminor --tokens|--ast|--ansi|--html file\n";
        return 2;
    }

    ifstream f(argv[2], ios::binary);

    if (!f) {
        cerr << "cannot open input\n";
        return 2;
    }

    string s(
        (istreambuf_iterator<char>(f)),
        {}
    );

    string mode = argv[1];

    auto v = lex(s, argv[2]);

    if (mode == "--tokens") {
        for (auto t : v) {
            cout
                << t.file
                << ":"
                << t.line
                << ":"
                << t.col
                << " "
                << t.kind
                << " "
                << quoted(t.text)
                << '\n';
        }
    } else {
        Parser p(v);
        auto n = p.run();

        if (mode == "--ast") {
            dump(n);
        } else if (mode == "--ansi" || mode == "--html") {
            bool h = mode == "--html";

            map<size_t, string> m;

            mark(n, m);

            map<string, string> colors = {
                {"kw", "34;1"},
                {"type", "36"},
                {"lit", "33"},
                {"comment", "32"},
                {"func", "35;1"},
                {"var", "37;1"},
                {"invalid", "31"},
                {"op", "31"},
                {"id", "37"},
                {"punct", "37"}
            };

            if (h) {
                cout
                    << "<!doctype html><meta charset=utf-8>"
                    << "<style>body{background:#171717;color:#eee}"
                    << ".kw{color:#80aaff}"
                    << ".type{color:#60dada}"
                    << ".lit{color:#eac76a}"
                    << ".comment{color:#82b982}"
                    << ".func{color:#e5a0ff}"
                    << ".var{color:#f0f0f0;font-weight:bold}"
                    << ".invalid,.op{color:#ff8585}</style><pre>";
            }

            size_t pos = 0;

            for (auto t : v) {
                auto gap = s.substr(pos, t.pos - pos);

                cout << (h ? escapeHtml(gap) : gap);

                if (t.kind == "eof") {
                    break;
                }

                auto k = m.count(t.pos) ? m[t.pos] : t.kind;

                if (h) {
                    cout
                        << "<span class=\""
                        << k
                        << "\">"
                        << escapeHtml(t.text)
                        << "</span>";
                } else {
                    cout
                        << "\033["
                        << colors[k]
                        << "m"
                        << t.text
                        << "\033[0m";
                }

                pos = t.pos + t.text.size();
            }

            if (h) {
                cout << "</pre>\n";
            }
        } else {
            cerr << "unknown mode\n";
            return 2;
        }
    }

    for (auto& e : errors) {
        cerr << e << '\n';
    }

    return errors.empty() ? 0 : 1;
}
