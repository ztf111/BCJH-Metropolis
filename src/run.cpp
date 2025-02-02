#include "run.hpp"
Result run(const RuleInfo &rl, const CList &chefList, RList &recipeList,
           int log, bool silent, int seed
#ifdef EMSCRIPTEN_PROGRESS
           ,
           emscripten::val postProgress
#endif
           ,
           int threadId, States *state_resumed) {
    auto chefListPtr = new CList(chefList);

    for (auto &chef : *chefListPtr) {
        chef.recipeLearned = new std::vector<Recipe *>();
    }

    srand(seed);
    SARunner saRunner(&rl, chefListPtr, &recipeList, true, f::t_dist_slow,
                      threadId);

    auto s = saRunner.run(state_resumed,
#ifdef EMSCRIPTEN_PROGRESS
                          postProgress,
#endif
                          silent);
    // *s = perfectChef(rl, *s, chefListPtr);
    int score = sumPrice(rl, s, log);
    exactChefTool(rl, s);
    debugIntegrity(s);
    for (auto &chef : *chefListPtr) {
        delete chef.recipeLearned;
    }
    delete chefListPtr;
    return Result{score, seed, s, ""};
}

std::tuple<Skill, Skill, Skill>
_reconstructUserUltimate(const Json::Value &gameData,
                         const Json::Value &directUserData) {
    try {
        Skill::loadJson(gameData["skills"]);
    } catch (const Json::RuntimeError &) {
        std::cout << "json文件格式不正确。如果文件内容是手动复制的，确认文件已"
                     "经复制完整。\n";
    } catch (const Json::LogicError &) {
        std::cout << "json文件格式不正确。请确认文件来自白菜菊花而非图鉴网。\n";
    } catch (const UnknownSkillException &e) {
        std::cout << e.what() << std::endl;
    }
    // uc=[c["id"] for c in directUserData["chefs"] if c["ult"]=="是"]
    Skill globalBuff, globalBuffMale, globalBuffFemale;
    std::map<int, Json::Value> chefInfo;
    for (const auto &c : gameData["chefs"]) {
        chefInfo[c["chefId"].asInt()] = c;
    }
    for (const auto &c : directUserData["chefs"]) {
        if (c["ult"].asString() == "是") {
            int chefID = c["id"].asInt();
            int id = chefInfo[chefID]["ultimateSkill"].asInt();
            if (Skill::globalSkillList.contains(id)) {
                globalBuff += Skill::globalSkillList[id];
                // globalBuff.print(true);
                globalBuffMale += Skill::globalMaleSkillList[id];
                globalBuffFemale += Skill::globalFemaleSkillList[id];
            }
        }
    }
    return {std::move(globalBuff), std::move(globalBuffMale),
            std::move(globalBuffFemale)};
}
std::tuple<RList, CList> _loadJsonInGame(const Json::Value &gameData,
                                         const Json::Value &userData,
                                         bool allowTool = true) {
    auto [globalBuff, globalBuffMale, globalBuffFemale] =
        _reconstructUserUltimate(gameData, userData);
    Chef::setGlobalBuff(globalBuff, globalBuffMale, globalBuffFemale);
    CList chefList;
    RList recipeList;
    Chef::loadAppendChefInGame(chefList, 5, gameData, userData, allowTool);
    int chefRarity = 4;
    do {
        Chef::loadAppendChefInGame(chefList, chefRarity, gameData, userData,
                                   allowTool);
        chefRarity--;
    } while (chefList.size() <= NUM_CHEFS && chefRarity >= 0);
    recipeList = loadRecipe(gameData, userData);
    if (chefList.size() <= NUM_CHEFS) {
        std::cout << NoChefException(chefList.size()).what() << std::endl;
        exit(1);
    }

    std::cout << "读取文件成功。" << std::endl;
    recipeList.initIDMapping();
    chefList.initIDMapping();
    return {std::move(recipeList), std::move(chefList)};
}
std::tuple<RList, CList> _loadJsonBCJH(const Json::Value &gameData,
                                       const Json::Value &userData,
                                       bool allowTool

) {
    RList recipeList;
    CList chefList;
    std::cout << "正在读取厨师与菜谱..." << std::endl;

    try {
        Skill::loadJson(gameData["skills"]);
        Chef::initBuff(userData["userUltimate"]);
        Chef::loadAppendChef(chefList, 5, gameData, userData, allowTool);
        int chefRarity = 4;
        do {
            Chef::loadAppendChef(chefList, chefRarity, gameData, userData,
                                 allowTool);
            chefRarity--;
        } while (chefList.size() <= NUM_CHEFS && chefRarity >= 0);
        recipeList = loadRecipe(gameData, userData);
        if (chefList.size() <= NUM_CHEFS) {

            std::cout << NoChefException(chefList.size()).what() << std::endl;
            exit(1);
        }
    } catch (const Json::RuntimeError &) {
        std::cout << "json文件格式不正确。如果文件内容是手动复制的，确认文件已"
                     "经复制完整。\n";
    } catch (const Json::LogicError &) {
        std::cout << "json文件格式不正确。请确认文件来自白菜菊花而非图鉴网。\n";
    };
    std::cout << "读取文件成功。" << std::endl;
    recipeList.initIDMapping();
    chefList.initIDMapping();
    return {std::move(recipeList), std::move(chefList)};
}

std::tuple<RList, CList> loadJson(const Json::Value &gameData,
                                  const Json::Value &userData, bool allowTool) {
    if (userData["type"] == "bcjh") {
        return _loadJsonBCJH(gameData, userData, allowTool);
    } else {
        return _loadJsonInGame(gameData, userData, allowTool);
    }
}
