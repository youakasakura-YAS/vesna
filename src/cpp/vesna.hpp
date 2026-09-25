// vesna.hpp — Vesna 1.0.0 C++ 实现：声明
// 从 src/vesna.py 移植，保持语言语义一致
#pragma once

#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <unordered_map>
#include <set>
#include <utility>
#include <variant>
#include <vector>

namespace vesna {

extern const std::string VERSION;  // "1.0.0"

// ============================================================
// 错误与信号
// ============================================================
class VesnaError : public std::exception {
public:
    std::string msg;
    int line;
    VesnaError(std::string m, int l = -1) : msg(std::move(m)), line(l) {}
    const char* what() const noexcept override { return msg.c_str(); }
    std::string str() const {
        if (line < 0) return msg;
        return "[第 " + std::to_string(line) + " 行] " + msg;
    }
};

class BreakSignal : public std::exception {};
class ContinueSignal : public std::exception {};
class ExitSignal : public std::exception {
public:
    int code;
    ExitSignal(int c = 0) : code(c) {}
};

// ============================================================
// 值
// ============================================================
struct ListVal;
struct GroupVal;
struct DictVal;

struct Value {
    // 判别联合：T 顺序与 variant index 严格对应（NONE=0 ... DICT=7）
    enum class T { NONE, BOOL, INT, FLOAT, STR, LIST, GROUP, DICT };
    std::variant<std::monostate, bool, int64_t, double, std::string,
                 std::shared_ptr<ListVal>, std::shared_ptr<GroupVal>,
                 std::shared_ptr<DictVal>> v;

    Value() = default;
    T t() const { return static_cast<T>(v.index()); }
    bool& b() { return std::get<bool>(v); }
    const bool& b() const { return std::get<bool>(v); }
    int64_t& i() { return std::get<int64_t>(v); }
    const int64_t& i() const { return std::get<int64_t>(v); }
    double& f() { return std::get<double>(v); }
    const double& f() const { return std::get<double>(v); }
    std::string& s() { return std::get<std::string>(v); }
    const std::string& s() const { return std::get<std::string>(v); }
    std::shared_ptr<ListVal>& list() { return std::get<std::shared_ptr<ListVal>>(v); }
    const std::shared_ptr<ListVal>& list() const { return std::get<std::shared_ptr<ListVal>>(v); }
    std::shared_ptr<GroupVal>& group() { return std::get<std::shared_ptr<GroupVal>>(v); }
    const std::shared_ptr<GroupVal>& group() const { return std::get<std::shared_ptr<GroupVal>>(v); }
    std::shared_ptr<DictVal>& dict() { return std::get<std::shared_ptr<DictVal>>(v); }
    const std::shared_ptr<DictVal>& dict() const { return std::get<std::shared_ptr<DictVal>>(v); }
};

struct ListVal { std::vector<Value> items; };
struct GroupVal { std::vector<Value> items; };
struct DictVal { std::vector<std::pair<Value, Value>> pairs; };

class ReturnSignal : public std::exception {
public:
    Value value;
    explicit ReturnSignal(Value v) : value(std::move(v)) {}
};

// 值工厂
Value mkNone();
Value mkBool(bool b);
Value mkInt(int64_t i);
Value mkFloat(double f);
Value mkStr(std::string s);
Value mkList();
Value mkGroup();
Value mkDict();

int64_t internId(const std::string& s);          // 标识符→整数 ID（跨会话表）
const std::string& internName(int64_t id);             // ID→名字（错误消息用）

bool isNum(const Value& v);          // int 或 float（不含 bool）
bool vesnaEq(const Value& a, const Value& b);
bool truthy(const Value& v);
std::string fmt(const Value& v);
std::string typeName(const Value& v);

// ============================================================
// AST
// ============================================================
struct Expr {
    enum class K {
        NUM, STR, BOOL, NONE, VAR, INTERP, NEG, NOT, AND, OR, BIN,
        GROUP, LIST, DICT, INDEX, BUILTIN, INTO, CALL
    } k;
    bool is_float = false;
    int64_t inum = 0;
    double fnum = 0.0;
    std::string str;                 // STR/INTERP/VAR/BUILTIN/CALL 名称、INTO 类型
    int64_t nid = 0;                 // VAR/INTERP 的 intern 化标识符
    std::string op;                  // BIN 运算符（"PLUS"/"MINUS"/...）
    bool bval = false;
    std::shared_ptr<Expr> a, b;      // 一元/二元/索引操作数
    std::vector<std::shared_ptr<Expr>> args;              // BUILTIN/CALL/GROUP/LIST 元素
    std::vector<std::pair<std::shared_ptr<Expr>, std::shared_ptr<Expr>>> pairs;  // DICT
};

struct Stmt {
    enum class K {
        ASSIGN, EXPR, BACK, BREAK, CONTINUE, IF, WHILE, FOR,
        DEF, TRY, IMPORT, ERR
    } k;
    int line = -1;                   // 语句起始物理行号（调试器用）
    // ASSIGN
    std::string lv_name;
    int64_t lv_nid = 0;
    std::shared_ptr<Expr> lv_idx;    // null → 变量赋值；否则下标赋值
    std::string op_kind;             // "ASSIGN" 或 "PLUS"/"MINUS"/"STAR"/"SLASH"
    std::shared_ptr<Expr> val;
    // EXPR / BACK
    std::shared_ptr<Expr> expr;
    // IF
    std::vector<std::pair<std::shared_ptr<Expr>, std::vector<std::shared_ptr<Stmt>>>> branches;
    // WHILE / FOR
    std::shared_ptr<Expr> cond;
    std::string var;                 // FOR 变量（错误消息用）
    int64_t var_nid = 0;
    std::vector<std::shared_ptr<Stmt>> body;
    // DEF
    std::string fname;
    std::vector<std::pair<int64_t, std::shared_ptr<Expr>>> params;     // (名称ID, 默认值或null)
    // TRY
    std::vector<std::shared_ptr<Stmt>> try_body, catch_body;
    int64_t catch_nid = 0;
    // IMPORT
    std::string import_name;
    // ERROR
    std::string err_msg;
    int err_line = -1;
};

struct Env;
struct Function {
    std::string name;
    std::vector<std::pair<int64_t, std::shared_ptr<Expr>>> params;
    std::vector<std::shared_ptr<Stmt>> body;
    std::weak_ptr<Env> closure;
};

struct Env {
    std::unordered_map<int64_t, Value> vars;
    std::unordered_map<int64_t, std::shared_ptr<Function>> funcs;
    std::shared_ptr<Env> parent;

    explicit Env(std::shared_ptr<Env> par = nullptr) : parent(std::move(par)) {}
    const Value* getRef(int64_t nid) const;  // 未定义返回 nullptr
    void set(int64_t nid, const Value& v);
    std::shared_ptr<Function> getFunc(int64_t nid) const;
    void setFunc(int64_t nid, const std::shared_ptr<Function>& fn);
};

// ============================================================
// 词法
// ============================================================
enum TokKind {
    TK_NUMBER, TK_STRING, TK_BOOL, TK_NONE, TK_IDENT, TK_BUILTIN, TK_TYPE,
    TK_INTERP, TK_EOF, TK_KW,          // TK_KW: value 存关键字单词
    TK_OR, TK_AND, TK_NOT,             // 由 TK_KW 派生（仅表达式层用）
    TK_EQ, TK_NE, TK_LT, TK_GT, TK_LE, TK_GE,
    TK_PLUS, TK_MINUS, TK_STAR, TK_SLASH, TK_IDIV, TK_FDIV, TK_MOD,
    TK_ASSIGN, TK_PLUSEQ, TK_MINUSEQ, TK_STAREQ, TK_SLASHEQ,
    TK_LPAREN, TK_RPAREN, TK_LBRACK, TK_RBRACK, TK_LBRACE, TK_RBRACE,
    TK_COMMA, TK_SEMI, TK_COLON
};

struct Token {
    TokKind kind = TK_EOF;
    std::string value;                 // 名称/字符串内容/运算符符号/关键字
    bool bval = false;                 // BOOL
    int line = 1;
};

std::vector<Token> lexExpr(const std::string& s, int ln);

// ============================================================
// 预处理
// ============================================================
struct Line {
    int indent = 0;
    std::string content;
    bool block_start = false;
    int line_no = 1;
};

std::vector<Line> preprocess(const std::string& src);

// ============================================================
// 解析
// ============================================================
std::shared_ptr<Expr> parseExpr(const std::string& s, int ln);

struct Parser {
    std::vector<Line> lines;
    size_t pos = 0;
    std::string filename;
    std::vector<std::pair<int, std::string>> errors;

    explicit Parser(std::vector<Line> lns, std::string fn = "<stdin>")
        : lines(std::move(lns)), filename(std::move(fn)) {}

    std::vector<std::shared_ptr<Stmt>> parse();

private:
    std::shared_ptr<Stmt> stmt(int min_indent);
    std::shared_ptr<Stmt> stmtInner(int min_indent);
    std::shared_ptr<Stmt> parseIf(const Line& ln);
    std::shared_ptr<Stmt> parseWhile(const Line& ln);
    std::shared_ptr<Stmt> parseFor(const Line& ln);
    std::shared_ptr<Stmt> parseDef(const Line& ln);
    std::shared_ptr<Stmt> parseTry(const Line& ln);
    std::shared_ptr<Stmt> parseSimple(const std::string& content, int ln_no);
};

// ============================================================
// 运行时
// ============================================================
struct Interp {
    std::shared_ptr<Env> g;
    std::vector<std::string> argv;
    std::string script_dir;
    std::vector<std::shared_ptr<Env>> kept_;  // 保活被闭包引用的环境

    // ---- 调试器状态 ----
    bool dbg = false;
    std::string dbg_file;
    std::set<int> dbg_breaks;
    int dbg_mode = 0;                  // 0=运行 1=step 2=next
    int dbg_next_depth = 0;
    std::shared_ptr<Env> dbg_env;
    int dbg_line = 0;
    std::string dbg_fname;
    std::vector<std::pair<std::string, int>> frames;   // 调用栈（名, 当前行）
    void dbgCheck(const std::shared_ptr<Stmt>& stmt, const std::shared_ptr<Env>& env);
    void dbgLoop();
    void dbgPrintFrames();
    void dbgPrintList();
    void dbgHelp();

    explicit Interp(std::vector<std::string> a = {}, std::string dir = ".")
        : g(std::make_shared<Env>()), argv(std::move(a)), script_dir(std::move(dir)) {}

    void run(const std::vector<std::shared_ptr<Stmt>>& program);
    void exec(const std::shared_ptr<Stmt>& stmt, const std::shared_ptr<Env>& env);
    Value eval(const std::shared_ptr<Expr>& e, const std::shared_ptr<Env>& env);
    void assign(int64_t nid, const std::shared_ptr<Expr>& idx, const std::string& op_kind,
                const Value& val, const std::shared_ptr<Env>& env);
    Value binop(const std::string& op, const Value& l, const Value& r);
    Value call(const std::string& name, int64_t nid, const std::vector<std::shared_ptr<Expr>>& args,
               const std::shared_ptr<Env>& env);
    Value into(const std::string& t, const Value& v);
    std::string interpStr(const std::string& tpl, const std::shared_ptr<Env>& env);
    void doImport(const std::string& name, const std::shared_ptr<Env>& env);
    Value builtin(const std::string& name, const std::vector<std::shared_ptr<Expr>>& args,
                  const std::shared_ptr<Env>& env);
};

// ============================================================
// 入口辅助
// ============================================================
bool fileExists(const std::string& path);
std::string readFileUtf8(const std::string& path);
std::string findVesnaHome();
int runSource(const std::string& src, const std::vector<std::string>& argv,
              const std::string& script_dir, const std::string& filename, bool catch_exit,
              bool dbg = false, const std::string& dbg_file = "");
int runFile(const std::string& path, const std::vector<std::string>& argv);
int runFileDbg(const std::string& path, const std::vector<std::string>& argv);
void repl();
int mainCli(int argc, char** argv);

}  // namespace vesna
