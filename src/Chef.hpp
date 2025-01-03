#ifndef CHEF_HPP
#define CHEF_HPP

#include "include/json/json.h"
#include <string>
#include <fstream>
#include <iostream>
#include <exception>
#include "Recipe.hpp"
#include "Values.hpp"
#include "Types.hpp"
#include <vector>
#include "../config.hpp"
#include "utils/json.hpp"
#include <map>
#include <memory> // Include for shared_ptr
#include "include/cereal/archives/portable_binary.hpp"
#include "include/cereal/types/memory.hpp"
#include "include/cereal/types/vector.hpp"
#include "include/cereal/types/map.hpp"
#include "include/cereal/types/string.hpp"

class Recipe;
class CList;

class Chef {
    // This class may be copied intensively, so use pointers within.
  private:
    static CookAbility globalAbilityBuff;
    static int globalAbilityMale;
    static int globalAbilityFemale;
    void addSkill(int id);
    Tool tool;
    static ToolFileType toolFileType;
    std::string name;
    // friend struct GlobalAbilityBuff;
    friend class StatesSerializer;

  public:
    static bool coinBuffOn;
    bool male;
    bool female;
    int id;
    std::shared_ptr<Skill> skill;
    std::shared_ptr<Skill> companyBuff;
    std::shared_ptr<Skill> nextBuff;
    std::shared_ptr<Tags> tagForCompanyBuff;
    Tool getTool() const { return this->tool; }
    ToolEnum getToolType() const { return this->tool.type; }
    bool allowsTool() const { return this->tool.type != NO_TOOL; }
    void setNoTool() { this->tool.type = NO_TOOL; }
    std::vector<Recipe *> *recipeLearned = NULL;

    static void setGlobalBuff(CookAbility buff) { globalAbilityBuff = buff; }
    static void setGlobalAbilityMale(int ability) {
        globalAbilityMale = ability;
    }
    static void setGlobalAbilityFemale(int ability) {
        globalAbilityFemale = ability;
        }
    static void setGlobalAbilityAll(int ability) {
        globalAbilityBuff.add(ability);
    }
    static void initBuff(const Json::Value &usrBuff) {

        coinBuffOn = true;

        setGlobalBuff(CookAbility(usrBuff));
        setGlobalAbilityMale(getInt(usrBuff["Male"]));
        setGlobalAbilityFemale(getInt(usrBuff["Female"]));
        setGlobalAbilityAll(getInt(usrBuff["All"]));
    }
    static void loadAppendChef(CList &chefList, int chefRarity,
                               const Json::Value &gameData,
                               const Json::Value &usrData
#ifndef _WIN32
                               ,
                               bool allowTool
#endif
    );

    std::string getName(bool wTool = true) const;
    Chef(Json::Value &v, int ultimateSkillId);
    Chef() { id = -1; }
    void print() const;
    void modifyTool(Tool);
    void modifyTool(ToolEnum);
    void deletePointers() {
        skill.reset();
        companyBuff.reset();
        nextBuff.reset();
        tagForCompanyBuff.reset();
    }
    std::string getToolName() const;
    template <class Archive> void serialize(Archive &archive) {
        archive(male, female, id, skill, companyBuff, nextBuff,
                tagForCompanyBuff, tool, name);
    }
};
class CList : public std::vector<Chef> {
    std::map<int, int> id2index;

  public:
    CList() : std::vector<Chef>() {}
    void initIDMapping() {
        int i = 0;
        for (auto &chef : *this) {
            id2index[chef.id] = i++;
        }
    }
    Chef byId(int id) const {
        int index = id2index.at(id);
        return (*this)[index];
    }
};

#endif