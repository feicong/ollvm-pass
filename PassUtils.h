#ifndef __PASS_UTILS__
#define __PASS_UTILS__

#include "common.h"
#include <json.hpp>

uint64_t get_ts_ms();
vector<string> split(const string& s, const string& d);
bool startswith(const string& str, const string& prefix);
string ts_to_date(int64_t ts);
string get_module_file(Module& M);
bool isNameReserved(Value* V);
bool ensure_dir(const string& path);
bool dumpIR(const string& suffix, Function* F);

#define POLICY_PATH         "policy.json"
class Config {
    bool inited;
    nlohmann::json conf;
private:
    void removeUnusedKey(nlohmann::json& root) {
        if (root.is_object()) {
            vector<string> keys_to_remove;
            for (auto& [key, value] : root.items()) {
                if (!key.empty() && key[0] == '#') {
                    keys_to_remove.push_back(key);
                } else {
                    removeUnusedKey(value);
                }
            }
            for (const auto& key : keys_to_remove) {
                root.erase(key);
            }
        } else if (root.is_array()) {
            for (auto& element : root) {
                removeUnusedKey(element);
            }
        }
    }
    nlohmann::json getPolicyForName(const string& name, nlohmann::json& policy_m) {
        nlohmann::json policy = nlohmann::json::object();
        const nlohmann::json& policy_map = conf["policy_map"];
        policy["name"] = name;
        if (!policy_map.contains(name)) {
            return policy;
        }
        const nlohmann::json& policy_templ = policy_map[name];
        for (auto& it : policy_templ.items()) {
            string key = it.key();
            policy[key] = it.value();
            if ((startswith(key, "enable_") || startswith(key, "enable-")) && policy[key].get<bool>()) {
                policy_m["has_" + key.substr(7)] = true;
                policy_m["name"] = name;
            }
        }
        return policy;
    }
public:
    static Config& inst() {
        static Config instance;
        return instance;
    }
    Config() {
        inited = false;
        filesystem::path fs_pwd = filesystem::current_path();
        filesystem::path fs_json_path = fs_pwd / POLICY_PATH;
        if (!filesystem::exists(fs_json_path)) {
            errs() << "Error: conf not found: " << fs_json_path << "\n";
            return;
        }
        errs() << "Conf:           " << fs_json_path << "\n";
        ifstream ifs(fs_json_path.string());
        try {
            conf = nlohmann::json::parse(ifs);
        } catch (const exception& exc) {
            errs() << "Error: config parse failed: " << exc.what() << "\n";
            return;
        }
        if (!conf.contains("globals")) {
            conf["globals"] = nlohmann::json::object();
        }
        if (!conf.contains("policy_map")) {
            conf["policy_map"] = nlohmann::json::object();
        }
        if (!conf.contains("policies")) {
            conf["policies"] = nlohmann::json::array();
        }
        errs() << "Root:           " << fs_pwd.string() << "\n"; // 配置文件所在路径
        if (!conf.contains("src_root")) {
            conf["src_root"] = fs_pwd.string(); // 源文件路径
        }
        errs() << "SrcRoot:        " << conf["src_root"].get<string>() << "\n";
        removeUnusedKey(conf);
        inited = true;
    }
    nlohmann::json& getGlobalConf() {
        return conf["globals"];
    }
    nlohmann::json getModulePolicy(Module& M) {
        if (!inited) {
            return nullptr;
        }
        nlohmann::json module_config = nlohmann::json::object();
        const nlohmann::json& mod_pol_lst = conf["policies"];
        string src_root = conf["src_root"].get<string>();
        if (mod_pol_lst.empty()) {
            return module_config;
        }
        filesystem::path mod_path = get_module_file(M);
        if (mod_path.is_relative()) {
            mod_path = src_root / mod_path;
        }
        unordered_map<string, string> func_pol_map;
        unordered_set<string> func_name_lst;
        nlohmann::json func_policy_map = nlohmann::json::object();
        nlohmann::json module_policy = nlohmann::json::object();
        for (Function& F : M) {
            if (isNameReserved(&F) || F.isDeclaration() || F.isIntrinsic()) {
                continue;
            }
            string func_name = F.getName().str();
            string escaped_name = GlobalValue::dropLLVMManglingEscape(F.getName()).str();
            func_name_lst.insert(escaped_name);
        }
        for (auto& item : mod_pol_lst) {
            if (!item.contains("policy") || !item.contains("module") || !item.contains("func")) {
                continue;
            }
            filesystem::path match_path = item["module"].get<string>();
            if (match_path.is_relative()) {
                match_path = filesystem::path(src_root) / match_path;
            }
            string match_path_s = match_path.string();
            if (match_path_s.find("./") != string::npos || match_path_s.find("../") != string::npos) {
                match_path = weakly_canonical(match_path);
            }
            if (!regex_match(mod_path.string(), regex(match_path.string()))) { // 后面的规则覆盖前面的
                continue;
            }
            string policy_name = item["policy"];
            if (item.contains("func")) { // Config for function
                string match_func = item["func"];
                for (const string& name: func_name_lst) {
                    if (!regex_match(name, regex(match_func))) { // 后面的规则覆盖前面的
                        continue;
                    }
                    nlohmann::json policy = getPolicyForName(policy_name, module_policy);
                    if (!policy.empty()) {
                        func_policy_map[name] = policy;
                    } else {
                        func_policy_map.erase(name);
                    }
                    if (item.contains("final")) {
                        break;
                    }
                }
            }
        }
        module_config[".mp"] = module_policy;
        module_config[".fp"] = func_policy_map;
        return module_config;
    }
};

#endif

