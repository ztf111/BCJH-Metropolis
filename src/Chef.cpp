#include "include/json/json.h"
#include "Chef.hpp"
#include <string>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include "../config.hpp"
#include "Calculator.hpp"
#include "utils/json.hpp"
#include "exception.hpp"
// #include "loadToolEquipped.hpp"
#include <memory>

// Add these macros after the includes
#define UnknownWarn                                                            \
    UnknownSkillWarning usw(skillJson["desc"].asString() +                     \
                            " with type: " + type);                            \
    error_msg = &usw
#define AssertUnknownWarn(condition)                                           \
    if (!(condition)) {                                                        \
        UnknownWarn;                                                           \
    }

bool Chef::coinBuffOn = true;
Skill Chef::globalSkill;
Skill Chef::globalSkillMale;
Skill Chef::globalSkillFemale;
ToolFileType Chef::toolFileType = NOT_LOADED;
std::map<int, Skill> Skill::skillList;
std::map<int, Skill> Skill::globalSkillList;
std::map<int, Skill> Skill::globalMaleSkillList;
std::map<int, Skill> Skill::globalFemaleSkillList;
void initBuff(const Json::Value usrBuff) {
    Chef::setGlobalBuff(CookAbility(usrBuff));
    Chef::addGlobalAbilityMale(getInt(usrBuff["Male"]));
    Chef::addGlobalAbilityFemale(getInt(usrBuff["Female"]));
    Chef::setGlobalAbilityAll(getInt(usrBuff["All"]));
}
void splitUltimateSkill(std::map<int, int> &ultimateSkills,
                        const Json::Value &ids) {
    for (auto pair : ids) {
        auto str = pair.asString();
        int id = atoi(str.substr(0, str.find(",")).c_str());
        int skillId = atoi(str.substr(str.find(",") + 1).c_str());
        ultimateSkills[id] = skillId;
    }
}
void loadUltimateSkills(std::map<int, int> &ultimateSkills,
                        const Json::Value &usrBuff) {

    splitUltimateSkill(ultimateSkills, usrBuff["Partial"]["id"]);
    splitUltimateSkill(ultimateSkills, usrBuff["Self"]["id"]);
}
void Chef::loadAppendChef(CList &chefList, int chefRarity,
                          const Json::Value &gameData,
                          const Json::Value &usrData, bool allowTool) {

    const Json::Value &chefs = gameData["chefs"];

    std::map<int, int> ultimateSkills;
    loadUltimateSkills(ultimateSkills, usrData["userUltimate"]);
    CList newChefList;
    auto chefGot = usrData["chefGot"];
    for (auto chef : chefs) {
        int id = chef["chefId"].asInt();
        if (chefGot[std::to_string(id)].asBool()) {
            if (chef["rarity"].asInt() != chefRarity) {
                continue;
            }

            if (ultimateSkills.find(id) != ultimateSkills.end()) {
                newChefList.push_back(Chef(chef, ultimateSkills[id]));
            } else {
                newChefList.push_back(Chef(chef, -1));
            }
        }
    }

    if (allowTool) {
        for (auto &chef : newChefList) {
            chef.modifyTool(NOT_EQUIPPED);
        }
    } else {
        for (auto &chef : newChefList) {
            chef.modifyTool(NO_TOOL);
        }
    }

    chefList.insert(chefList.end(), newChefList.begin(), newChefList.end());
}

void Chef::loadAppendChefInGame(CList &chefList, int chefRarity,
                                const Json::Value &gameData,
                                const Json::Value &usrData, bool allowTool) {

    const Json::Value &chefs = gameData["chefs"];
    CList newChefList;
    std::map<int, Json::Value> chefGot;
    std::map<int, Json::Value> toolsInfo;
    for (auto chef : usrData["chefs"]) {
        chefGot[chef["id"].asInt()] = chef;
    }
    for (auto tool : gameData["equips"]) {
        toolsInfo[tool["equipId"].asInt()] = tool;
    }
    for (auto chef : chefs) {
        int id = chef["chefId"].asInt();
        if (chefGot[id]["got"].asString() == "是") {
            if (chef["rarity"].asInt() != chefRarity) {
                continue;
            }
            if (chefGot[id]["ult"].asString() == "是") {
                newChefList.push_back(
                    Chef(chef, chef["ultimateSkill"].asInt()));
            } else {
                newChefList.push_back(Chef(chef, -1));
            }
            if (chefGot[id].isMember("equip")) {
                int equipID = chefGot[id]["equip"].asInt();
                if (equipID >= 19) {
                    // 前18个厨具都是不值钱的，装作没有
                    auto tool = toolsInfo[equipID];
                    for (auto &skillID : tool["skill"]) {
                        newChefList.back().addSkill(skillID.asInt());
                        // Handles skills that do not exist
                    }
                    newChefList.back().modifyTool(NO_TOOL);
                }
            }
        }
    }
    if (allowTool) {
        for (auto &chef : newChefList) {
            chef.modifyTool(NOT_EQUIPPED);
            // This is safe, as it will automatically skip those with NO_TOOL
        }
    } else {
        for (auto &chef : newChefList) {
            chef.modifyTool(NO_TOOL);
        }
    }
    chefList.insert(chefList.end(), newChefList.begin(), newChefList.end());
}

/**
 * Chef
 * @param ultimateSkillId: -1 means no ultimate skill
 */
std::string Chef::getName(bool wTool) const {
    std::string toolName;
    if (wTool) {
        toolName = getToolName();
    }
    return this->name + toolName;
}
Chef::Chef(Json::Value &chef, int ultimateSkillId)
    : id(chef["chefId"].asInt()), name(chef["name"].asString()) {
    if (chef.isMember("chefId") && chef.isMember("name") &&
        chef.isMember("skill")) {

        this->skill = std::make_shared<Skill>(Skill::SELF);
        this->companyBuff = std::make_shared<Skill>(Skill::PARTIAL);
        this->nextBuff = std::make_shared<Skill>(Skill::NEXT);
        this->tagForCompanyBuff = std::make_shared<Tags>();
        this->skill->ability = CookAbility(chef);
        if (chef.isMember("tags") && chef["tags"].isArray()) {
            auto tags = chef["tags"];
            this->male = false;
            this->female = false;
            for (auto tag : tags) {
                if (tag.asInt() == 1) {
                    this->male = true;
                    *(this->skill) += (globalSkillMale);
                }
                if (tag.asInt() == 2) {
                    this->female = true;
                    *(this->skill) += (globalSkillFemale);
                }
                this->tagForCompanyBuff->insert(tag.asInt());
            }
        }
    } else {
        std::cout << chef << std::endl;
        throw std::logic_error("Chef Json Error");
    }
    *(this->skill) += globalSkill;
    this->addSkill(chef["skill"].asInt());
    if (ultimateSkillId != -1) {
        this->addSkill(ultimateSkillId);
    }
    this->tool.type = NOT_EQUIPPED;
}
std::string Chef::getToolName() const {
    std::string toolName;
    if (tool.value == 0) {
        if (tool.type == NO_TOOL) {
            return "设定厨具";
        } else {
            return "";
        }
    }
    switch (tool.type) {
    case STIRFRY:
        toolName = "炒";
        break;
    case BOIL:
        toolName = "煮";
        break;
    case FRY:
        toolName = "炸";
        break;
    case STEAM:
        toolName = "蒸";
        break;
    case BAKE:
        toolName = "烤";
        break;
    case KNIFE:
        toolName = "切";
        break;
    case NO_TOOL:
        toolName = "设定厨具";
        break;
    default:
        toolName = "无厨具";
        return "";
    }

    toolName = "-" + toolName;
    if (tool.type != NO_TOOL) {
        toolName += "(" + std::to_string(tool.value) + ")";
    }
    return toolName;
}
void Chef::print() const {
    auto toolName = getToolName();

    std::cout << this->id << ": " << this->name << toolName << "\t"
              << (this->male ? "男" : "") << (this->female ? "女" : "")
              << std::endl;
    this->skill->print();
}
CookAbility::CookAbility(const Json::Value &v) {

    if (v.isMember("stirfry") && v.isMember("bake") && v.isMember("boil") &&
        v.isMember("fry") && v.isMember("knife")) {

        this->stirfry = getInt(v["stirfry"]);
        this->bake = getInt(v["bake"]);
        this->boil = getInt(v["boil"]);
        this->steam = getInt(v["steam"]);
        this->fry = getInt(v["fry"]);
        this->knife = getInt(v["knife"]);

    } else if (v.isMember("Stirfry") && v.isMember("Bake") &&
               v.isMember("Boil") && v.isMember("Fry") && v.isMember("Knife")) {

        this->stirfry = getInt(v["Stirfry"]);
        this->bake = getInt(v["Bake"]);
        this->boil = getInt(v["Boil"]);
        this->steam = getInt(v["Steam"]);
        this->fry = getInt(v["Fry"]);
        this->knife = getInt(v["Knife"]);
    } else {
        std::cout << "no" << std::endl;

        throw std::logic_error("CookAbility: Invalid Json");
    }
}
class GlobalTag {
  public:
    enum GlobalTagEnum {
        NO_TAG = -1,
        DEFAULT_ALL = 0,
        MALE = 1,
        FEMALE = 2,
    };

    GlobalTag() : tag(NO_TAG) {}

    GlobalTag &operator=(GlobalTagEnum newTag) {
        if (tag == NO_TAG) {
            tag = newTag;
        } else if (tag != newTag) {
            throw std::logic_error(
                "Cannot reassign GlobalTag to a different value");
        }
        return *this;
    }
    bool operator==(GlobalTagEnum newTag) { return tag == newTag; }

  private:
    GlobalTagEnum tag;
};
void Skill::loadJson(const Json::Value &v) {
    std::map<std::string, std::string> missingSkills;
    static std::set<std::string> nameOfTools = {"Stirfry", "Bake", "Boil",
                                                "Steam",   "Fry",  "Knife"};
    static std::map<std::string, Skill::Type> typeMap = {{"Partial", PARTIAL},
                                                         {"Self", SELF},
                                                         {"Next", NEXT},
                                                         {"Global", GLOBAL}};
    static std::set<std::string> ignoredSkills = {"Sweet",
                                                  "Sour",
                                                  "Salty",
                                                  "Bitter",
                                                  "Spicy",
                                                  "Tasty",
                                                  "Vegetable",
                                                  "Meat",
                                                  "Fish",
                                                  "Creation",
                                                  "ExploreTime",
                                                  "GuestApearRate",
                                                  "GuestDropCount",
                                                  "Material_Gain",
                                                  "OpenTime",
                                                  "Rejuvenation",
                                                  "InvitationApearRate",
                                                  "Material_Creation",
                                                  "Material_Fish",
                                                  "Material_Meat",
                                                  "Material_Vegetable",
                                                  "SpecialInvitationRate",
                                                  "GuestAntiqueDropRate",
                                                  "MaterialReduce",
                                                  "ExperienceTimeRate"};
    for (auto skillJson : v) {
        int id = skillJson["skillId"].asInt();
        GlobalTag globalTag;
        for (auto effect : skillJson["effect"]) {
            UnknownSkillWarning *error_msg = NULL;

            Skill skill;
            std::string conditionStr = effect["condition"].asString();
            std::string type = effect["type"].asString();
            if ((missingSkills.count(type)) || (ignoredSkills.count(type))) {
                continue;
            }
            skill.type = typeMap[conditionStr];
            int value = effect["value"].asInt();
            std::shared_ptr<BuffCondition> conditionPtr;

            bool skillInheritConditional = true;
            if (!effect.isMember("conditionType"))
                skillInheritConditional = false;
            else {
                auto conditionType = effect["conditionType"].asString();
                std::set<std::string> specialConditions = {
                    "PerRank", "SameSkill", "ChefTag"};
                if (specialConditions.contains(conditionType)) {
                    skillInheritConditional = false;
                }
            }

            if (skillInheritConditional) {
                PriceBuffData *buffTarget = NULL;
                auto calString = effect["cal"].asString();
                if (type == "BasicPrice" && calString == "Percent")
                    buffTarget = &skill.priceBasePerc;
                else if (type == "BasicPrice" && calString == "Abs")
                    buffTarget = &skill.priceBaseAbs;
                else if (type == "CookbookPrice") {
                    AssertUnknownWarn(calString == "Percent");
                    buffTarget = &skill.pricePerc;
                }
                auto conditionType = effect["conditionType"].asString();
                int cvalue = 0;
                if (effect.isMember("conditionValue"))
                    cvalue = getInt(effect["conditionValue"]);
                if (conditionType == "CookbookRarity") {
                    AssertUnknownWarn(buffTarget != NULL);
                    for (auto &i : effect["conditionValueList"]) {
                        buffTarget->rarityBuff[getInt(i)] += value;
                    }
                } else if (conditionType == "ExcessCookbookNum") {
                    AssertUnknownWarn(buffTarget != NULL);
                    AssertUnknownWarn(cvalue != 0);
                    buffTarget->amountBuff.gte_add(cvalue, value);
                } else if (conditionType == "FewerCookbookNum") {
                    AssertUnknownWarn(buffTarget != NULL);
                    AssertUnknownWarn(cvalue != 0);
                    buffTarget->amountBuff.lte_add(cvalue, value);
                } else if (conditionType == "Rank") {
                    AssertUnknownWarn(buffTarget != NULL);
                    AssertUnknownWarn(cvalue != 0);
                    buffTarget->gradeBuff.gte_add(cvalue, value);
                } else {
                    missingSkills[conditionType] = skillJson["desc"].asString();
                    UnknownWarn;
                }
            } else {
                if (type == "Gold_Gain") {
                    AssertUnknownWarn(effect["cal"].asString() == "Percent");
                    skill.pricePerc.unconditional = value;
                } else if (type == "UseAll") {
                    globalTag = GlobalTag::DEFAULT_ALL;
                    std::string cal = effect["cal"].asString();
                    int rarity = effect["rarity"].asInt();
                    if (conditionStr == "Global" && cal == "Percent") {
                        skill.pricePerc.rarityBuff[rarity] += value;
                    } else {
                        UnknownWarn;
                    }
                } else if (type == "MaxEquipLimit") {
                    if (conditionStr == "Global") {
                        globalTag = GlobalTag::DEFAULT_ALL;
                    }
                    int rarity = effect["rarity"].asInt();
                    skill.amountAdd[rarity] += value;
                } else if (type == "MutiEquipmentSkill") {
                    // 为啥图鉴网接口时muti而不是multi
                    // assert(value % 100 == 0);
                    // if (value % 100 == 0) {
                    //     skill.multiToolEffect = 1 + value / 100;
                    // } else {
                    UnknownWarn;
                    // }
                } else if (nameOfTools.count(type)) {
                    if (conditionStr == "Global") {
                        if (effect.isMember("tag")) {
                            globalTag =
                                GlobalTag::GlobalTagEnum(effect["tag"].asInt());
                        } else {
                            globalTag = GlobalTag::DEFAULT_ALL;
                        }
                    }
                    std::string cal = effect["cal"].asString();
                    CookAbility *ptr = NULL;
                    if (cal == "Abs")
                        ptr = &skill.ability;
                    else {
                        ptr = &skill.cookAbilityPercentBuff;
                    }
                    *(*ptr)[type] = value;
                } else if (type.starts_with("Use")) {
                    std::string specificType = type.substr(3);
                    AssertUnknownWarn(effect["cal"].asString() == "Percent");
                    auto &perc = skill.pricePerc;
                    if (perc.abilityBuff[specificType] != NULL) {
                        *perc.abilityBuff[specificType] = value;
                    } else if (perc.flavorBuff[specificType] != NULL) {
                        *perc.flavorBuff[specificType] = value;
                    } else if (perc.materialBuff[specificType] != NULL) {
                        *perc.materialBuff[specificType] = value;
                    } else {
                        throw UnknownSkillException(
                            skillJson["desc"].asString() + "（debug代码：" +
                            type + ")");
                    }
                } else if (type == "BasicPrice") {
                    auto calString = effect["cal"].asString();
                    if (calString != "Percent" && calString != "Abs") {
                        throw UnknownSkillException(
                            "cal值不是Percent或Abs，来自" +
                            skillJson["desc"].asString() + "（debug代码：" +
                            type + ")");
                    }
                    if (calString == "Percent")
                        skill.priceBasePerc.unconditional += value;
                    else
                        skill.priceBaseAbs.unconditional += value;
                } else if (type == "CookbookPrice") {
                    AssertUnknownWarn(effect["cal"].asString() == "Percent");
                    skill.pricePerc.unconditional += value;
                } else if (type.starts_with("BasicPriceUse")) {
                    auto calString = effect["cal"].asString();
                    if (calString != "Percent" && calString != "Abs") {
                        throw UnknownSkillException(
                            "cal值不是Percent或Abs，来自" +
                            skillJson["desc"].asString() + "（debug代码：" +
                            type + ")");
                    }

                    PriceBuffData *ptr = NULL;
                    if (calString == "Abs") {
                        ptr = &skill.priceBaseAbs;
                    } else {
                        ptr = &skill.priceBasePerc;
                    }

                    std::string type_name = type.substr(13);
                    if (ptr->abilityBuff[type_name] != NULL) {
                        *ptr->abilityBuff[type_name] = value;
                    } else if (ptr->flavorBuff[type_name] != NULL) {
                        *ptr->flavorBuff[type_name] = value;
                    } else if (ptr->materialBuff[type_name] != NULL) {
                        *ptr->materialBuff[type_name] = value;
                    } else {
                        throw UnknownSkillException(
                            skillJson["desc"].asString() + "（debug代码：" +
                            type + ")");
                    }

                } else {
                    missingSkills[type] = skillJson["desc"].asString();
                    UnknownWarn;
                    continue;
                }
                if (effect.isMember("conditionType")) {
                    auto conditionType = effect["conditionType"].asString();
                    int cvalue = 0;
                    if (effect.isMember("conditionValue"))
                        cvalue = getInt(effect["conditionValue"]);

                    if (conditionType == "PerRank") {
                        conditionPtr =
                            std::make_shared<GradeBuffCondition>(cvalue);
                    } else if (conditionType == "SameSkill") {
                        conditionPtr = std::make_shared<
                            ThreeSameCookAbilityBuffCondition>();
                    } else if (conditionType == "ChefTag") {
                        if (conditionStr == "Partial") {
                            auto conditionValueList =
                                effect["conditionValueList"];
                            for (auto tag : conditionValueList) {
                                skill.chefTagsForPARTIAL.insert(tag.asInt());
                            }
                        } else {
                            UnknownWarn;
                        }
                    }
                }
            }

            if (error_msg) {
                continue;
            }
            if (conditionStr != "Global") {
                if (skillList.count(id) == 0) {
                    skillList[id] = Skill(typeMap[conditionStr]);
                } else {
                    // assert(skillList[id].type == typeMap[conditionStr]);
                    if (skillList[id].type != typeMap[conditionStr]) {
                        throw std::logic_error("Skill type mismatch");
                    }
                }
                if (!conditionPtr) {
                    skillList[id] += skill;
                } else {
                    skillList[id].conditionalEffects.push_back(
                        std::make_shared<ConditionalBuff>(
                            ConditionalBuff(conditionPtr, skill)));
                }
            } else {
                if (globalSkillList.count(id) == 0) {
                    globalSkillList[id] = Skill(GLOBAL);
                    globalMaleSkillList[id] = Skill(GLOBAL);
                    globalFemaleSkillList[id] = Skill(GLOBAL);
                }
                if (conditionPtr) {
                    throw std::logic_error("Global Buffs should not have "
                                           "conditional effects");
                }
                if (globalTag == GlobalTag::DEFAULT_ALL) {
                    globalSkillList[id] += skill;
                } else if (globalTag == GlobalTag::FEMALE) {
                    globalFemaleSkillList[id] += skill;
                } else if (globalTag == GlobalTag::MALE) {
                    globalMaleSkillList[id] += skill;
                } else {
                    throw std::logic_error("GlobalTag not set");
                }
            }
        }
    }
    for (auto pair : missingSkills) {
        UnknownSkillWarning(pair.second + "（debug代码：" + pair.first + "）");
    }
}
void Chef::addSkill(int id) {
    if (Skill::skillList.count(id) == 0)
        return;
    auto skill = Skill::skillList[id];
    if (skill.type == Skill::SELF) {
        *this->skill += skill;
    } else if (skill.type == Skill::PARTIAL) {
        *this->companyBuff += skill;
    } else if (skill.type == Skill::NEXT) {
        *this->nextBuff += skill;
    } else {
        std::cout << "Skill type is unset for skill " << id << std::endl;
        exit(1);
    }
}

int CookAbility::operator*(const AbilityBuff &a) const {
    int buff = 0;
    const int *thisptr = &this->stirfry;
    const int *aptr = &a.stirfry;
    for (int i = 0; i < 6; i++) {
        if (thisptr[i] != 0) {
            buff += aptr[i];
        }
    }
    return buff;
}

// bool Chef::isCapable(Recipe *recipe) {
//     if (this->skill.ability / recipe->cookAbility > 0) {
//         return true;
//     }
//     return false;
// }
// void Chef::loadRecipeCapable(std::vector<Recipe> &recipeList) {
//     if (this->tool == NO_TOOL) {
//         for (auto &recipe : recipeList) {
//             if (this->isCapable(&recipe)) {
//                 this->recipeCapable.push_back(&recipe);
//             }
//         }
//     } else if (this->tool == NOT_EQUIPPED) {
//         std::vector<Recipe *> recipeListCopy;
//         for (auto &recipe : recipeList) {
//             recipeListCopy.push_back(&recipe);
//         }
//         for (int i = 0; i < 6; i++) {
//             this->modifyTool((ToolEnum)i);
//             auto iter = recipeListCopy.begin();
//             while (iter != recipeListCopy.end()) {
//                 if (this->isCapable(*iter)) {
//                     this->recipeCapable.push_back(*iter);
//                     iter = recipeListCopy.erase(iter);
//                 } else {
//                     iter++;
//                 }
//             }
//         }
//         this->modifyTool(NOT_EQUIPPED);
//     }
// }
void Chef::modifyTool(Tool a) {
    if (this->tool.type == NO_TOOL)
        return;
    this->tool = a;
}
void Chef::modifyTool(ToolEnum a) {
    if (this->tool.type == NO_TOOL)
        return;
    this->tool.type = a;
}
