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
#include "../toolEquipped.hpp"
#include "exception.hpp"
#include "loadToolEquipped.hpp"
#include <memory>

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
                          const Json::Value &usrData
#ifndef _WIN32
                          ,
                          bool allowTool
#endif
) {

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

#ifdef _WIN32
    if (toolFileType == NOT_LOADED) {
        toolFileType = loadToolFile();

        if (toolFileType == EMPTY_FILE__NOT_EQUIPPED) {
            std::cout << "toolEquipped.csv没有设定规则。允许所有厨师装备厨具。"
                      << std::endl;
        } else if (toolFileType == NO_FILE__NO_TOOL) {
            std::cout << "未找到toolEquipped.csv文件。不会装配任何厨具。"
                      << std::endl;
        } else {
            std::cout << "toolEquipped.csv文件已加载。" << std::endl;
        }
    }
    CSVWarning w;
    for (auto &chef : newChefList) {
        for (int i = chef.skill->multiToolEffect; i > 0; i--)
            w += loadToolFromFile(&chef, toolFileType);
    }
    if (w.missingRarity3) {
        std::cout
            << "提示：当前版本toolEquipped."
               "csv已支持“制作三火料理售价加成”，详见样例，但读取到的toolEquipp"
               "ed.csv中没有这一项。默认“制作三火料理售价加成”均为0。"
            << std::endl;
    }
#else
    if (allowTool) {
        for (auto &chef : newChefList) {
            chef.modifyTool(NOT_EQUIPPED);
        }
    } else {
        for (auto &chef : newChefList) {
            chef.modifyTool(NO_TOOL);
        }
    }
#endif
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

            Skill skill;
            std::string conditionStr = effect["condition"].asString();
            std::string type = effect["type"].asString();
            if ((missingSkills.count(type)) || (ignoredSkills.count(type))) {
                continue;
            }
            skill.type = typeMap[conditionStr];
            int value = effect["value"].asInt();
            if (type == "Gold_Gain") {
                skill.pricePercentBuff = value;
            } else if (type == "UseAll") {
                globalTag = GlobalTag::DEFAULT_ALL;
                std::string cal = effect["cal"].asString();
                assert(conditionStr == "Global");
                int rarity = effect["rarity"].asInt();
                assert(cal == "Percent");
                skill.rarityBuff[rarity] += value;
            } else if (type == "MaxEquipLimit") {
                if (conditionStr == "Global") {
                    globalTag = GlobalTag::DEFAULT_ALL;
                }
                int rarity = effect["rarity"].asInt();
                skill.amountAdd[rarity] += value;
            } else if (type == "MutiEquipmentSkill") {
                // 为啥图鉴网接口时muti而不是multi
                assert(value % 100 == 0);
                skill.multiToolEffect = 1 + value / 100;
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
                if (skill.abilityBuff[specificType] != NULL) {
                    *skill.abilityBuff[specificType] = value;
                } else if (skill.flavorBuff[specificType] != NULL) {
                    *skill.flavorBuff[specificType] = value;
                } else if (skill.materialBuff[specificType] != NULL) {
                    *skill.materialBuff[specificType] = value;
                } else {
                    throw UnknownSkillException(skillJson["desc"].asString() +
                                                "（debug代码：" + type + ")");
                }
            } else if (type == "BasicPrice") {
                assert(effect["cal"].asString() == "Percent");
                if (!effect.isMember("conditionType")) {
                    // 菜谱基础售价
                    // 下位上场厨师基础售价
                    skill.baseAddBuff = value;
                } else {
                    std::string conditionType =
                        effect["conditionType"].asString();
                    if (conditionType == "SameSkill" ||
                        conditionType == "PerRank" ||
                        conditionType == "FewerCookbookNum" ||
                        conditionType == "ExcessCookbookNum" ||
                        conditionType == "CookbookRarity") {

                        // Conditional Buff:
                        // 每制作一种神级料理菜谱基础售价
                        // 制作三种同技法料理在场基础售价

                        // Field in Skill designated:
                        // 制作小于22份的料理该料理基础售价 FewerCookbookNum
                        // 制作一二火料理基础售价

                        // Save for later
                    } else if (conditionType == "ChefTag") {
                        skill.baseAddBuff = value;
                    } else {
                        UnknownSkillWarning(skillJson["desc"].asString() +
                                            "conditionType: " + conditionType +
                                            " with type: " + type);
                        // Not implemented:
                        // 使用水果的料理基础售价 CookbookTag
                    }
                }

            } else if (type == "CookbookPrice") {
                if (!effect.isMember("conditionType")) {
                    skill.pricePercentBuff = value;
                } else {
                    std::string conditionType =
                        effect["conditionType"].asString();
                    if (conditionType == "CookbookRarity" ||
                        conditionType == "ExcessCookbookNum" ||
                        conditionType == "FewerCookbookNum" ||
                        conditionType == "Rank") {

                        // Conditional Buff:
                        // 每制作一种神级料理菜谱基础售价
                        // 制作三种同技法料理在场基础售价

                        // Field in Skill designated:
                        // 制作一星和二星料理售价+20% CookBookRarity
                        // 制作小于22份的料理该料理基础售价 FewerCookbookNum
                        // 制作大于17份的料理时售价 + 40 % ExcessCookbookNum
                        // 制作神级料理售价+70% Rank

                        // Save for later
                    } else if (conditionType == "ChefTag") {
                        skill.pricePercentBuff = value;
                    } else {
                        UnknownSkillWarning(skillJson["desc"].asString() +
                                            "conditionType: " + conditionType +
                                            " with type: " + type);
                        // Not implemented:
                        // 使用水果的料理基础售价 CookbookTag
                    }
                }
            } else if (type.starts_with("BasicPriceUse")) {
                // e.g.: BasePriceUseStirfry
                std::string type_name = type.substr(13);
                if (skill.abilityBaseBuff[type_name] != NULL) {
                    *skill.abilityBaseBuff[type_name] = value;
                } else if (skill.flavorBaseBuff[type_name] != NULL) {
                    *skill.flavorBaseBuff[type_name] = value;
                } else if (skill.materialBaseBuff[type_name] != NULL) {
                    *skill.materialBaseBuff[type_name] = value;
                } else {
                    throw UnknownSkillException(skillJson["desc"].asString() +
                                                "（debug代码：" + type + ")");
                }
                assert(effect["cal"].asString() == "Percent");
            } else {
                missingSkills[type] = skillJson["desc"].asString();
                continue;
            }
            std::shared_ptr<BuffCondition> conditionPtr;
            if (effect.isMember("conditionType")) {
                auto conditionType = effect["conditionType"].asString();
                int cvalue = 0;
                if (effect.isMember("conditionValue"))
                    cvalue = getInt(effect["conditionValue"]);
                if (conditionType == "CookbookRarity") {
                    if (type == "BasicPrice" || type == "CookbookPrice") {
                        DiscretizedBuff *targetBuff = NULL;
                        if (type == "BasicPrice") {
                            targetBuff = &skill.rarityBaseBuff;
                        } else if (type == "CookbookPrice") {
                            targetBuff = &skill.rarityBuff;
                        }
                        for (auto &i : effect["conditionValueList"]) {
                            (*targetBuff)[getInt(i)] = value;
                        }
                    } else {
                        UnknownSkillWarning(skillJson["desc"].asString() +
                                            "conditionType: " + conditionType +
                                            " with type: " + type);
                    }
                } else if (conditionType == "PerRank") {
                    conditionPtr = std::make_shared<GradeBuffCondition>(cvalue);
                } else if (conditionType == "ExcessCookbookNum") {
                    assert(type == "CookbookPrice" || type == "BasicPrice");
                    if (type == "BasicPrice") {
                        skill.amountBaseBuff.gte(cvalue, value);
                    } else if (type == "CookbookPrice") {
                        skill.amountBuff.gte(cvalue, value);
                    }
                } else if (conditionType == "FewerCookbookNum") {
                    assert(type == "CookbookPrice" || type == "BasicPrice");
                    if (type == "BasicPrice") {
                        skill.amountBaseBuff.lte(cvalue, value);
                    } else if (type == "CookbookPrice") {
                        skill.amountBuff.lte(cvalue, value);
                    }
                } else if (conditionType == "SameSkill") {
                    conditionPtr =
                        std::make_shared<ThreeSameCookAbilityBuffCondition>();
                } else if (conditionType == "Rank") {
                    assert(type == "CookbookPrice");
                    skill.gradeBuff[cvalue] = value;
                } else if (conditionType == "ChefTag") {
                    auto conditionValueList = effect["conditionValueList"];
                    for (auto tag : conditionValueList) {
                        skill.chefTagsForPARTIAL.insert(tag.asInt());
                    }
                    assert(conditionStr == "Partial");
                } else {
                    missingSkills[conditionType] = skillJson["desc"].asString();
                }
            }

            if (conditionStr != "Global") {
                if (skillList.count(id) == 0) {
                    skillList[id] = Skill(typeMap[conditionStr]);
                } else {
                    assert(skillList[id].type == typeMap[conditionStr]);
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
                assert(!conditionPtr);
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
