#include "PassUtils.h"

uint64_t get_ts_ms() {
    uint64_t us = chrono::system_clock::now().time_since_epoch().count();
    return us / 1000;
}

vector<string> split(const string& s, const string& d) {
    size_t pos_start = 0;
    size_t pos_end;
    size_t delim_len = d.length();
    string token;
    vector<string> res;
    while ((pos_end = s.find(d, pos_start)) != string::npos) {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }
    token = s.substr(pos_start);
    if (token.length() > 0) {
      res.push_back(token);
    }
    return res;
}

bool startswith(const string& str, const string& prefix) {
    return prefix.size() <= str.size() && equal(prefix.cbegin(), prefix.cend(), str.cbegin());
}

string ts_to_date(int64_t ts) {
    chrono::duration<int64_t, ratio<1,1>> dur(ts);
    auto dc = chrono::duration_cast<chrono::system_clock::duration>(dur);
    auto tp = chrono::system_clock::time_point(dc);
    time_t in_time_t = chrono::system_clock::to_time_t(tp);
    string format = "%Y-%m-%d %H:%M:%S";
    char buff[64];
    strftime(buff, 64, format.c_str(), localtime(&in_time_t));
    return buff;
}

string get_module_file(Module& M) {
    string file = M.getSourceFileName();
    if (file.empty()) {
        file = M.getName().str();
    }
    return file;
}

bool isNameReserved(Value* V) {
    StringRef name = V->getName();
    if (name.starts_with("llvm.") || name.starts_with("clang.")) {
        return true;
    }
    return false;
}

