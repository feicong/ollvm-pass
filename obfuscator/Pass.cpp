#include <json.hpp>
#include "common.h"
#include "PassUtils.h"

#include "BogusControlFlow.h"
#include "Flattening.h"
#include "Split.h"
#include "Substitution.h"

#define PLUGIN_NAME Obfuscator
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
            dumpIR(".ollvm.orig.ll", &M);
        }
        // split
        if (module_policy.value("has_split", false)) {
            SplitBasicBlock split;
            for (Function& F : M) {
                string func_name = GlobalValue::dropLLVMManglingEscape(F.getName()).str();
                if (!func_policy_map.contains(func_name)) {
                    continue;
                }
                const nlohmann::json& func_policy = func_policy_map[func_name];
                if (!func_policy.value("enable_split", false)) {
                    continue;
                }
                int split_n = func_policy.value("split_n", 2);
                if (split_n <= 1 || split_n > 10) {
                    errs() << "split_n must be 1 < x <= 10\n";
                    continue;
                }
                errs() << ">>>> run split on " << func_name << "\n";
                bool dump = func_policy.value("dump", false);
                if (dump) {
                    dumpIR(".ollvm.split.orig.ll", &F);
                }
                split.SplitNum = split_n;
                split.runOnFunction(F);
                if (dump) {
                    dumpIR(".ollvm.split.ll", &F);
                }
            }
        }
        // bcf
        if (module_policy.value("has_bcf", false)) {
            BogusControlFlow bcf;
            for (Function& F : M) {
                string func_name = GlobalValue::dropLLVMManglingEscape(F.getName()).str();
                if (!func_policy_map.contains(func_name)) {
                    continue;
                }
                const nlohmann::json& func_policy = func_policy_map[func_name];
                if (!func_policy.value("enable_bcf", false)) {
                    continue;
                }
                int bcf_prob = func_policy.value("bcf_prob", 30);
                int bcf_times = func_policy.value("bcf_times", 1);
                if (bcf_times <= 0) {
                    errs() << "bcf_times must be x > 0\n";
                    continue;
                }
                if (bcf_prob <= 0 || bcf_prob > 100) {
                    errs() << "bcf_prob must be 0 < x <= 100\n";
                    continue;
                }
                errs() << ">>>> run bcf on " << func_name << "\n";
                bool dump = func_policy.value("dump", false);
                if (dump) {
                    dumpIR(".ollvm.bcf.orig.ll", &F);
                }
                bcf.ObfProbRate = bcf_prob;
                bcf.ObfTimes = bcf_times;
                bcf.runOnFunction(F);
                if (dump) {
                    dumpIR(".ollvm.bcf.ll", &F);
                }
            }
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
                if (!func_policy.value("enable_fla", false)) {
                    continue;
                }
                errs() << ">>>> run fla on " << func_name << "\n";
                bool dump = func_policy.value("dump", false);
                if (dump) {
                    dumpIR(".ollvm.fla.orig.ll", &F);
                }
                fla.runOnFunction(F);
                if (dump) {
                    dumpIR(".ollvm.fla.ll", &F);
                }
            }
        }
        // sub
        if (module_policy.value("has_sub", false)) {
            Substitution sub;
            for (Function& F : M) {
                string func_name = GlobalValue::dropLLVMManglingEscape(F.getName()).str();
                if (!func_policy_map.contains(func_name)) {
                    continue;
                }
                const nlohmann::json& func_policy = func_policy_map[func_name];
                if (!func_policy.value("enable_sub", false)) {
                    continue;
                }
                int sub_times = func_policy.value("sub_times", 1);
                if (sub_times <= 0) {
                    errs() << "sub_times must be x > 0\n";
                    continue;
                }
                errs() << ">>>> run sub on " << func_name << "\n";
                bool dump = func_policy.value("dump", false);
                if (dump) {
                    dumpIR(".ollvm.sub.orig.ll", &F);
                }
                sub.ObfTimes = sub_times;
                sub.runOnFunction(F);
                if (dump) {
                    dumpIR(".ollvm.sub.ll", &F);
                }
            }
        }
        if (dump) {
            dumpIR(".ollvm.ll", &M);
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

