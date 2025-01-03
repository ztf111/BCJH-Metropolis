#ifndef RECIPE_HPP
#define RECIPE_HPP

#include "include/json/json.h"
#include <string>
#include <fstream>
#include <iostream>
#include <exception>
#include "Chef.hpp"
#include "Values.hpp"
#include "Types.hpp"
#include <vector>
#include <map>
#include "include/cereal/archives/portable_binary.hpp"
#include "include/cereal/types/memory.hpp"
#include "include/cereal/types/vector.hpp"
#include "include/cereal/types/map.hpp"

struct DishBuff {
    int dishNum;
    int dishBuff = 0;
};
class Materials {
  public:
    bool vegetable;
    bool meat;
    bool fish;
    bool creation;
    Materials() : vegetable(false), meat(false), fish(false), creation(false) {}
    void print(std::string end = "\n") const {
        std::cout << (this->vegetable ? "菜 " : "") << (this->meat ? "肉 " : "")
                  << (this->fish ? "鱼 " : "") << (this->creation ? "面 " : "")
                  << end;
    }
    int operator*(const MaterialCategoryBuff &buff) {
        int sum = 0;
        if (this->vegetable) {
            sum += buff.vegetable;
        }
        if (this->meat) {
            sum += buff.meat;
        }
        if (this->fish) {
            sum += buff.fish;
        }
        if (this->creation) {
            sum += buff.creation;
        }
        return sum;
    }
};

class Recipe {
  private:
    FlavorEnum getFlavor(Json::Value &flavorJson);
    void getMaterials(Json::Value &materialsJson);
    void printFlavor(std::string end = "\n") const {
        if (this->flavor == SWEET) {
            std::cout << "甜";
        } else if (this->flavor == SALTY) {
            std::cout << "咸";
        } else if (this->flavor == SOUR) {
            std::cout << "酸";
        } else if (this->flavor == BITTER) {
            std::cout << "苦";
        } else if (this->flavor == SPICY) {
            std::cout << "辣";
        } else if (this->flavor == TASTY) {
            std::cout << "鲜";
        } else {
            std::cout << "未知";
        }
        std::cout << end;
    }

  public:
    static DishBuff rarityBuff[5];
    /**
     * @return 小于等于返回值火数的都可以做到这么多份
     */
    static DiscretizedBuff::Mask moreThan(int dishNumTargetMin) {
        if (rarityBuff[0].dishNum == 0) {
            std::cout << "需要先load Recipes (init rarityBuff)再loadSkills"
                      << std::endl;
            exit(1);
        }
        bool mask[5] = {false};
        for (int i = 0; i < 5; i++) {
            mask[i] = rarityBuff[i].dishNum >= dishNumTargetMin;
        }
        return std::make_tuple(mask[0], mask[1], mask[2], mask[3], mask[4]);
    }
    static DiscretizedBuff::Mask lessThan(int dishNumTargetMax) {
        if (rarityBuff[0].dishNum == 0) {
            std::cout << "需要先load Recipes (init rarityBuff)再loadSkills"
                      << std::endl;
            exit(1);
        }
        bool mask[5] = {false};
        for (int i = 0; i < 5; i++) {
            mask[i] = rarityBuff[i].dishNum <= dishNumTargetMax;
        }
        return std::make_tuple(mask[0], mask[1], mask[2], mask[3], mask[4]);
    }
    std::string name;
    int id;
    int rarity;
    int price;
    // std::map<int, int> materials;
    Tags tagsForSkills;
    Materials materialCategories;
    CookAbility cookAbility;
    FlavorEnum flavor;
    Recipe(Json::Value &recipe);
    Recipe() : flavor(UNIDENTIFIED_FLAVOR) {}
    void print(const std::string &startLine = "", int priceDirectAdd = 0,
               int priceBuffAdd = 0) const;
    static void initRarityBuff(const Json::Value &usrBuff);
};
class RList : public std::vector<Recipe> {
    std::map<int, Recipe *> id2index;

  public:
    RList() : std::vector<Recipe>(){};
    void initIDMapping() {
        for (auto &recipe : *this) {
            id2index[recipe.id] = &recipe;
        }
    }
    Recipe *byId(int id) { return id2index[id]; }
};
RList loadRecipe(const Json::Value &gameData, const Json::Value &usrData);
void testJsonUpdate(const Json::Value &gameData, const Json::Value &usrData);
#endif