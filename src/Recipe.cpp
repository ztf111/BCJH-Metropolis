#include "include/json/json.h"
#include <string>
#include <fstream>
#include <iostream>
#include <exception>
#include "Chef.hpp"
#include "Recipe.hpp"
#include <vector>
#include "utils/json.hpp"
#include "../config.hpp"
#include "exception.hpp"
#include "utils/math.hpp"
const DishNum Recipe::dishNum;
Recipe::Recipe(Json::Value &recipe, bool isExpert) {
    this->name = recipe["name"].asString();
    this->id = recipe["recipeId"].asInt();
    this->rarity = recipe["rarity"].asInt();
    if (isExpert) {
        this->price = recipe["exPrice"].asInt();
    } else {
        this->price = recipe["price"].asInt();
    }
    this->cookAbility.stirfry = recipe["stirfry"].asInt();
    this->cookAbility.bake = recipe["bake"].asInt();
    this->cookAbility.boil = recipe["boil"].asInt();
    this->cookAbility.steam = recipe["steam"].asInt();
    this->cookAbility.fry = recipe["fry"].asInt();
    this->cookAbility.knife = recipe["knife"].asInt();
    this->getMaterials(recipe["materials"]);
    this->flavor = this->getFlavor(recipe["condiment"]);
    for (auto &m : recipe["tags"]) {
        this->tagsForSkills.insert(m.asInt());
    }
}
// Flavor Recipe::getFlavor(Json::Value &flavorJson) {
//     std::string flavorStr = flavorJson.asString();
//     Flavor flavor;
//     flavor.sweet = flavorStr.find("Sweet") != std::string::npos;
//     flavor.salty = flavorStr.find("Salty") != std::string::npos;
//     flavor.sour = flavorStr.find("Sour") != std::string::npos;
//     flavor.bitter = flavorStr.find("Bitter") != std::string::npos;
//     flavor.spicy = flavorStr.find("Spicy") != std::string::npos;
//     flavor.tasty = flavorStr.find("Tasty") != std::string::npos;
//     return flavor;
// }
FlavorEnum Recipe::getFlavor(Json::Value &flavorJson) {
    std::string flavorStr = flavorJson.asString();
    FlavorEnum f;
    if (flavorStr.find("Sweet") != std::string::npos)
        f = SWEET;
    else if (flavorStr.find("Salty") != std::string::npos)
        f = SALTY;
    else if (flavorStr.find("Sour") != std::string::npos)
        f = SOUR;
    else if (flavorStr.find("Bitter") != std::string::npos)
        f = BITTER;
    else if (flavorStr.find("Spicy") != std::string::npos)
        f = SPICY;
    else if (flavorStr.find("Tasty") != std::string::npos)
        f = TASTY;
    else
        std::abort();
    return f;
}
const struct MaterialList {
    int meat[16] = {1, 3, 4, 5, 7, 8, 9, 12, 26, 27, 28, 38, 39, 40, 43, 44};
    int vegetable[18] = {6,  10, 13, 14, 15, 16, 17, 18, 19,
                         22, 23, 25, 30, 31, 33, 36, 45, 46};
    int fish[7] = {2, 12, 24, 32, 37, 41, 42};
    int creation[6] = {11, 20, 21, 29, 34, 35};
} materialList;

void Recipe::print(int totalAmount, const std::string &startLine,
                   int priceDirectAdd, int priceBuffAdd) const {
    std::stringstream priceDirectAddStr, priceBuffAddStr, origPriceStr,
        finalPriceStr;
    if (priceBuffAdd != 0)
        priceBuffAddStr << " + " << priceBuffAdd << "%";
    if (priceDirectAdd != 0)
    priceDirectAddStr << " + " << priceDirectAdd;
    int finalPrice =
        int_ceil((this->price + priceDirectAdd) * (100 + priceBuffAdd) / 100);
    origPriceStr << (finalPrice == this->price ? "原价" : "") << this->price;
    finalPriceStr << (finalPrice == this->price ? "" : " = "+std::to_string(finalPrice));

    std::cout << this->name << "（" << origPriceStr.str()
              << priceDirectAddStr.str() << priceBuffAddStr.str()
              << finalPriceStr.str() << "）"
              << ", " << this->rarity << "火 * " << totalAmount << std::endl;
    std::cout << startLine;
    this->cookAbility.print();
    this->materialCategories.print("；");
    this->printFlavor("；");
}
RList loadRecipe(const Json::Value &gameData, const Json::Value &usrData) {
    RList recipeList;
    auto recipes = gameData["recipes"];
    if (usrData["type"].asString() == "in-game") {
        auto recipeGot = usrData["recipes"];
        std::map<int, Json::Value> recipeGotMap;
        for (auto recipe : recipeGot) {
            recipeGotMap[recipe["id"].asInt()] = recipe;
        }
        for (auto recipe : recipes) {
            int id = recipe["recipeId"].asInt();
            if (recipeGotMap[id]["got"].asString() == "是") {
                bool isExpert = recipeGotMap[id]["ex"].asString() == "是";
                recipeList.push_back(Recipe(recipe, isExpert));
            }
        }
    } else {
        assert(usrData["type"].asString() == "bcjh");
        auto recipeGot = usrData["repGot"];
        for (auto recipe : recipes) {
            int id = recipe["recipeId"].asInt();
            if (recipeGot[std::to_string(id)].asBool()) {
                recipeList.push_back(Recipe(recipe));
            }
        }
    }
    return recipeList;
}

// // #define jsonStr2Int(v) atoi(v.asCString())

void Recipe::getMaterials(Json::Value &materialsJson) {
    // this->materials.clear();
    this->materialCategories = Materials();
    for (auto m : materialsJson) {
        int materialId = m["material"].asInt();
        // int quantity = m["quantity"].asInt();
        // this->materials[materialId] = quantity;

        for (auto meat : materialList.meat) {
            if (materialId == meat) {
                materialCategories.meat = true;
                break;
            }
        }
        for (auto vegetable : materialList.vegetable) {
            if (materialId == vegetable) {
                materialCategories.vegetable = true;
                break;
            }
        }
        for (auto fish : materialList.fish) {
            if (materialId == fish) {
                materialCategories.fish = true;
                break;
            }
        }
        for (auto creation : materialList.creation) {
            if (materialId == creation) {
                materialCategories.creation = true;
                break;
            }
        }
    }
}

void testJsonUpdate(const Json::Value &gameData, const Json::Value &usrData) {
    int recipeNumUsr, chefNumUsr;
    if (usrData["type"].asString() == "in-game") {
        recipeNumUsr = usrData["recipes"].size();
        chefNumUsr = usrData["chefs"].size();
    } else {
        assert(usrData["type"].asString() == "bcjh");
        recipeNumUsr = usrData["repGot"].size();
        chefNumUsr = usrData["chefGot"].size();
    }

    int recipeNumGame = gameData["recipes"].size();
    int chefNumGame = gameData["chefs"].size();

    if (recipeNumUsr > recipeNumGame) {
        std::cout << "游戏数据更新了，当前文件仅收录了" << recipeNumGame
                  << "个菜谱，"
                  << "但是你的存档记录了" << recipeNumUsr
                  << "个菜谱。缺失的菜谱不会纳入计算。" << std::endl;
    }
    if (chefNumUsr > chefNumGame) {
        std::cout << "游戏数据更新了，当前文件仅收录了" << chefNumGame
                  << "个厨师，"
                  << "但是你的存档记录了" << chefNumUsr << "个厨师。"
                  << "缺失的厨师不会纳入计算。" << std::endl;
    }
}