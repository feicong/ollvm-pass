#include <json.hpp>
#include "common.h"
#include "PassUtils.h"

#include "BogusControlFlow.h"
#include "Flattening.h"
#include "GlobalEncryption.h"
#include "IndirectCall.h"
#include "MBAObfuscation.h"
#include "Substitution.h"

#define PLUGIN_NAME PlutoOBF
#define PLUGIN_VER "1.0"

using namespace Pluto;

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
        bool dump = module_policy.value("enable-dump", false);
        if (dump) {
            dumpIR(".pluto.orig.ll", &M);
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
                if (!func_policy.value("enable-bcf", false)) {
                    continue;
                }
                errs() << ">>>> run bcf on " << func_name << "\n";
                bool dump = func_policy.value("dump", false);
                if (dump) {
                    dumpIR(".pluto.bcf.orig.ll", &F);
                }
                bcf.run(F);
                if (dump) {
                    dumpIR(".pluto.icall.ll", &F);
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
                if (!func_policy.value("enable-fla", false)) {
                    continue;
                }
                errs() << ">>>> run fla on " << func_name << "\n";
                bool dump = func_policy.value("dump", false);
                if (dump) {
                    dumpIR(".pluto.fla.orig.ll", &F);
                }
                fla.run(F);
                if (dump) {
                    dumpIR(".pluto.fla.ll", &F);
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
                if (!func_policy.value("enable-sub", false)) {
                    continue;
                }
                errs() << ">>>> run sub on " << func_name << "\n";
                bool dump = func_policy.value("dump", false);
                if (dump) {
                    dumpIR(".pluto.sub.orig.ll", &F);
                }
                sub.run(F);
                if (dump) {
                    dumpIR(".pluto.sub.ll", &F);
                }
            }
        }
        // mba
        if (module_policy.value("has_mba", false)) {
            MbaObfuscation mba;
            for (Function& F : M) {
                string func_name = GlobalValue::dropLLVMManglingEscape(F.getName()).str();
                if (!func_policy_map.contains(func_name)) {
                    continue;
                }
                const nlohmann::json& func_policy = func_policy_map[func_name];
                if (!func_policy.value("enable-mba", false)) {
                    continue;
                }
                errs() << ">>>> run mba on " << func_name << "\n";
                bool dump = func_policy.value("dump", false);
                if (dump) {
                    dumpIR(".pluto.mba.orig.ll", &F);
                }
                mba.run(F);
                if (dump) {
                    dumpIR(".pluto.mba.ll", &F);
                }
            }
        }
        // icall
        if (module_policy.value("enable-idc", false)) {
            IndirectCall icall;
            icall.run(M);
        }
        // enc
        if (module_policy.value("enable-gle", false)) {
            GlobalEncryption enc;
            errs() << ">>>> run gle\n";
            enc.run(M);
        }
        if (dump) {
            dumpIR(".pluto.ll", &M);
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

