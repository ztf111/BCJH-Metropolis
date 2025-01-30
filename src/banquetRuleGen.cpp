#include "banquetRuleGen.hpp"
#include "utils/json.hpp"
#include "functions.hpp"
#include <cassert>
#include <memory>
double randomRecipeTime = 0;
double randomChefTime = 0;
double banquetRuleTime = 0;
double generateBanquetRuleTime = 0;
double generateBanquetRuleTimeOut = 0;

typedef std::map<int, Json::Value> GDMap;

const Json::Value &getIntentBuffById(const GDMap &GD, int Id) {
    auto it = GD.find(Id);
    if (it == GD.end()) {
        bool isBuff = GD.begin()->second.isMember("buffId");
        if (isBuff) {
            throw std::runtime_error("Buff not found " + std::to_string(Id));
        } else {
            throw std::runtime_error("Intent not found " + std::to_string(Id));
        }
    }
    return it->second;
}
/**
 * @deprecated
 */
// const Json::Value &getIntentById(const Json::Value &intentsGD, int intentId)
// {
//     for (auto &intent : intentsGD) {
//         if (intent["intentId"].asInt() == intentId) {
//             return intent;
//         }
//     }
//     throw std::runtime_error("Intent not found " + std::to_string(intentId));
// }
// const Json::Value &getBuffById(const Json::Value &intentsGD, int intentId) {
//     for (auto &intent : intentsGD) {
//         if (intent["buffId"].asInt() == intentId) {
//             return intent;
//         }
//     }
//     throw std::runtime_error("Buff not found " + std::to_string(intentId));
// }
std::shared_ptr<Rule> getRuleFromJson(const Json::Value &intent, int d,
                                      const GDMap &allIntents,
                                      const GDMap &allBuffs,
                                      int remainingDishes = DISH_PER_CHEF);

std::tuple<int, RuleInfo>
loadBanquetRuleFromJson(const Json::Value &rulesTarget, const GDMap &allBuffs,
                        const GDMap &allIntents) {
    int d = 0;
    int num_guest = 0;
    RuleInfo ruleInfo;
    for (auto &guest : rulesTarget) {
        num_guest++;
        ruleInfo.bestFull.push_back(guest["Satiety"].asInt());
        auto &intents = guest["IntentList"];
        for (auto &phaseIntents : intents) {
            if (guest.isMember("GlobalBuffList")) {
                auto &globalBuffs = guest["GlobalBuffList"];
                // Add global buff to this list
                for (auto &buffId : globalBuffs) {
                    auto &buffContent =
                        getIntentBuffById(allBuffs, buffId.asInt());

                    auto singleRule = getRuleFromJson(buffContent, d,
                                                      allIntents, allBuffs, 1);
                    auto e = std::make_shared<CreatePhaseRulesEffect>(
                        singleRule, DISH_PER_CHEF, true, false);
                    auto rule = std::make_shared<SingleConditionRule>(
                        std::make_shared<AlwaysTrueCondition>(d), e,
                        buffContent["desc"].asString());
                    ruleInfo.rl.push_back(rule);
                }
            }
            for (auto &intent : phaseIntents) {
                int intentId = intent.asInt();
                auto &intentContent = getIntentBuffById(allIntents, intentId);
                ruleInfo.rl.push_back(
                    getRuleFromJson(intentContent, d, allIntents, allBuffs));
            }
            d += DISH_PER_CHEF;
        }
    }
    return {num_guest, std::move(ruleInfo)};
}

/**
 * @todo Error handling.
 */
std::tuple<int, RuleInfo> loadFirstBanquetRule(const Json::Value &gameData,
                                               bool print) {
    auto &buffsGD = gameData["buffs"];
    auto &intentsGD = gameData["intents"];
    auto &rulesGD = gameData["rules"];
    std::map<int, Json::Value> buffsMap;
    for (auto &buff : buffsGD) {
        buffsMap[buff["buffId"].asInt()] = buff;
    }
    std::map<int, Json::Value> intentsMap;
    for (auto &intent : intentsGD) {
        intentsMap[intent["intentId"].asInt()] = intent;
    }
    // find the rule in rulesGD with Id ruleID
    Json::Value ruleGD = rulesGD[0];
    if (print) {
        auto ruleName = ruleGD.isMember("Title") ? ruleGD["Title"].asString()
                                                 : ruleGD["title"].asString();
        std::cout << "请核对规则：" << UNDERLINE << ruleName << NO_FORMAT
                  << "。若规则还是上周的，说明还没有更新，请过段时间再运行。"
                  << std::endl;
    }
    Json::Value rulesTarget;
    if (ruleGD.isMember("Group")) {
        rulesTarget = ruleGD["Group"];
    } else if (ruleGD.isMember("group")) {
        rulesTarget = ruleGD["group"];
    } else {
        rulesTarget = Json::Value(Json::arrayValue);
        rulesTarget.append(ruleGD);
    }
    if (rulesTarget.size() == 0) {
        std::cout << "规则为空。" << std::endl;
        throw std::runtime_error("规则为空。");
    }
    return loadBanquetRuleFromJson(rulesTarget, buffsMap, intentsMap);
}

std::tuple<int, RuleInfo> loadBanquetRuleFromInput(const Json::Value &ruleData,
                                                   bool print) {
    if (print)
        std::cout << "规则: " << ruleData["title"].asString() << std::endl;
    Json::Value rulesTarget;
    if (ruleData.isMember("Group")) {
        rulesTarget = ruleData["Group"];
    } else if (ruleData.isMember("group")) {
        rulesTarget = ruleData["group"];
    } else {
        rulesTarget = Json::Value(Json::arrayValue);
        rulesTarget.append(ruleData);
    }
    if (rulesTarget.size() == 0) {
        std::cout << "规则为空。" << std::endl;
        throw std::runtime_error("规则为空。");
    }
    auto &buffs = ruleData["buffs"];
    auto &intents = ruleData["intents"];
    std::map<int, Json::Value> buffsMap;
    for (auto &buff : buffs) {
        buffsMap[buff["buffId"].asInt()] = buff;
    }
    std::map<int, Json::Value> intentsMap;
    for (auto &intent : intents) {
        intentsMap[intent["intentId"].asInt()] = intent;
    }
    return loadBanquetRuleFromJson(rulesTarget, buffsMap, intentsMap);
}
std::shared_ptr<Rule> getRuleFromJson(const Json::Value &intent, int dish,
                                      const GDMap &allIntents,
                                      const GDMap &allBuffs,
                                      int remainingDishes) {
    std::shared_ptr<Condition> c;
    auto effectType = intent["effectType"].asString();
    auto effectValue = intent["effectValue"].asInt();
    std::string conditionType;
    std::string conditionValue;

    if (intent.isMember("conditionType")) {
        conditionType = intent["conditionType"].asString();
        conditionValue = intent["conditionValue"].asString();
        if (conditionType == "CookSkill") {
            c = std::make_shared<SkillCondition>(dish, conditionValue,
                                                 remainingDishes);
        } else if (conditionType == "CondimentSkill") {
            c = std::make_shared<FlavorCondition>(dish, conditionValue,
                                                  remainingDishes);
        } else if (conditionType == "Order") {
            c = std::make_shared<OrderCondition>(
                dish, getInt(intent["conditionValue"]));
        } else if (conditionType == "Rarity") {
            c = std::make_shared<RarityCondition>(
                dish, getInt(intent["conditionValue"]), remainingDishes);
        } else if (conditionType == "Group") {
            assert(effectType == "CreateBuff");
            c = std::make_shared<GroupCondition>(dish, conditionValue,
                                                 remainingDishes);
        } else if (conditionType == "Rank") {
            c = std::make_shared<RankCondition>(
                dish, getInt(intent["conditionValue"]), remainingDishes);
        } else {
            std::cout << "Unknown condition type: " << conditionType
                      << std::endl;
            throw std::runtime_error("Unknown condition type: " +
                                     conditionType);
        }
    } else {
        c = std::make_shared<AlwaysTrueCondition>(dish);
    }

    std::shared_ptr<Effect> e;
    if (effectType == "BasicPriceChange") {
        e = std::make_shared<BasePriceAddEffect>(effectValue);
    } else if (effectType == "BasicPriceChangePercent") {
        e = std::make_shared<BasePricePercentEffect>(effectValue);
    } else if (effectType == "PriceChangePercent") {
        e = std::make_shared<PricePercentEffect>(effectValue);
    } else if (effectType == "SatietyChange") {
        e = std::make_shared<FullAddEffect>(effectValue);
    } else if (effectType == "SetSatietyValue") {
        e = std::make_shared<FullSetEffect>(effectValue);
    } else if (effectType == "IntentAdd") {
        e = std::make_shared<IntentAddEffect>();
    } else if (effectType == "CreateIntent") {
        auto newIntent = getIntentBuffById(allIntents, effectValue);
        auto newRule =
            getRuleFromJson(newIntent, dish + 1, allIntents, allBuffs, 1);
        e = std::make_shared<NextRuleEffect>(newRule);

    } else if (effectType == "CreateBuff") {
        auto newIntent = getIntentBuffById(allBuffs, effectValue);
        int lastRounds = getInt(newIntent["lastRounds"]);
        auto newRule = getRuleFromJson(newIntent, dish + DISH_PER_CHEF,
                                       allIntents, allBuffs, 1);
        e = std::make_shared<CreatePhaseRulesEffect>(
            newRule, DISH_PER_CHEF * lastRounds, true);
    } else {
        std::cout << "Unknown effect type: " << effectType << std::endl;
        throw std::runtime_error("Unknown effect type: " + effectType);
    }
    return std::make_shared<SingleConditionRule>(c, e,
                                                 intent["desc"].asString());
}
void banquetRuleGenerated(BanquetRuleTogether *brt, States &s,
                          const RuleInfo &allRules) {
#ifdef MEASURE_TIME
    struct timespec start, end;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
#endif
    for (auto &r : allRules.rl) {
        (*r)(brt, s);
    }
#ifdef MEASURE_TIME
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
    generateBanquetRuleTime +=
        (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
#endif
}