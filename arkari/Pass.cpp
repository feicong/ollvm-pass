#include <json.hpp>
#include "common.h"
#include "PassUtils.h"

#include "Flattening.h"
#include "IndirectBranch.h"
#include "IndirectCall.h"
#include "IndirectGlobalVariable.h"
#include "StringEncryption.h"

#define PLUGIN_NAME Arkari
#define PLUGIN_VER "1.0"

namespace llvm {
int EnableABIBreakingChecks;
int DisableABIBreakingChecks;
__attribute__((visibility("default"))) void Value::dump() const { 
    print(dbgs(), /*IsForDebug=*/true); dbgs() << '\n'; 
}
__attribute__((visibility("default"))) void Module::dump() const {
    print(llvm::errs(), 0, true);
}
};


static uint64_t ts_init = 0;

class PLUGIN_NAME : public PassInfoMixin<PLUGIN_NAME> {
public:
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
        Config& conf = Config::inst();
        nlohmann::json policy = conf.getModulePolicy(M);
        if (policy == nullptr) {
            return PreservedAnalyses::none();
        }
        const nlohmann::json& module_policy = policy[".mp"];
        const nlohmann::json& func_policy_map = policy[".fp"];
        bool dump = module_policy.value("has_dump", false);
        if (dump) {
            dumpIR(".arkari.orig.ll", &M);
        }
        // strenc
        if (conf.getGlobalConf().value("enable-cse", false)) { // cse是模块级选项
            StringEncryption cse;
            errs() << ">>>> run cse\n";
            cse.runOnModule(M);
        }
        // fla
        if (module_policy.value("has_fla", false)) {
            Flattening fla;
            for (Function& F : M) {
                string func_name = GlobalValue::dropLLVMManglingEscape(F.getName()).str();
                if (!func_policy_map.contains(func_name)) {
                    continue;
                }
                const nlohmann::json& func_policy = func_policy_map[func_name];
                if (!func_policy.value("enable-cff", false)) {
                    continue;
                }
                errs() << ">>>> run fla on " << func_name << "\n";
                bool dump = func_policy.value("dump", false);
                if (dump) {
                    dumpIR(".arkari.fla.orig.ll", &F);
                }
                fla.runOnFunction(F);
                if (dump) {
                    dumpIR(".arkari.fla.ll", &F);
                }
            }
        }
        // ibr
        if (module_policy.value("has_indbr", false)) {
            IndirectBranch indbr;
            for (Function& F : M) {
                string func_name = GlobalValue::dropLLVMManglingEscape(F.getName()).str();
                if (!func_policy_map.contains(func_name)) {
                    continue;
                }
                const nlohmann::json& func_policy = func_policy_map[func_name];
                if (!func_policy.value("enable-indbr", false)) {
                    continue;
                }
                errs() << ">>>> run indbr on " << func_name << "\n";
                bool dump = func_policy.value("dump", false);
                if (dump) {
                    dumpIR(".arkari.indbr.orig.ll", &F);
                }
                indbr.runOnFunction(F);
                if (dump) {
                    dumpIR(".arkari.indbr.ll", &F);
                }
            }
        }
        // icall
        if (module_policy.value("has_icall", false)) {
            IndirectCall icall;
            for (Function& F : M) {
                string func_name = GlobalValue::dropLLVMManglingEscape(F.getName()).str();
                if (!func_policy_map.contains(func_name)) {
                    continue;
                }
                const nlohmann::json& func_policy = func_policy_map[func_name];
                if (!func_policy.value("enable-icall", false)) {
                    continue;
                }
                errs() << ">>>> run icall on " << func_name << "\n";
                bool dump = func_policy.value("dump", false);
                if (dump) {
                    dumpIR(".arkari.icall.orig.ll", &F);
                }
                icall.runOnFunction(F);
                if (dump) {
                    dumpIR(".arkari.icall.ll", &F);
                }
            }
        }
        // igv
        if (module_policy.value("has_indgv", false)) {
            IndirectGlobalVariable indgv;
            for (Function& F : M) {
                string func_name = GlobalValue::dropLLVMManglingEscape(F.getName()).str();
                if (!func_policy_map.contains(func_name)) {
                    continue;
                }
                const nlohmann::json& func_policy = func_policy_map[func_name];
                if (!func_policy.value("enable-indgv", false)) {
                    continue;
                }
                errs() << ">>>> run indgv on " << func_name << "\n";
                bool dump = func_policy.value("dump", false);
                if (dump) {
                    dumpIR(".arkari.indgv.orig.ll", &F);
                }
                indgv.runOnFunction(F);
                if (dump) {
                    dumpIR(".arkari.indgv.ll", &F);
                }
            }
        }
        if (dump) {
            dumpIR(".arkari.ll", &M);
        }
        return PreservedAnalyses::all();
    };
    static bool isRequired() { return true; }
};

extern "C" __attribute__((visibility("default")))
LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = STR(PLUGIN_NAME),
        .PluginVersion = PLUGIN_VER,
        .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager& MPM
#if LLVM_VERSION_MAJOR >= 12
                , OptimizationLevel Level
#endif
                ) {
                    ts_init = get_ts_ms();
                    MPM.addPass(PLUGIN_NAME());
            });
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager& MPM, ArrayRef<PassBuilder::PipelineElement>) {
                    ts_init = get_ts_ms();
                    MPM.addPass(PLUGIN_NAME());
                    return true;
            });
        }
    };
}

