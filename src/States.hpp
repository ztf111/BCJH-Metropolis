#ifndef STATES_HPP
#define STATES_HPP

#include <vector>
#include "Chef.hpp"
#include "Recipe.hpp"
#include "../config.hpp"

class States {
    bool cookAbilitiesValid = false;
    uint32_t chefHasStrangeSkills = 0;
    std::vector<Skill> cookAbilitiesCache;
    std::vector<Chef> chefs;

  public:
    std::vector<Recipe *> recipe;
    States()
        : cookAbilitiesCache(NUM_CHEFS), chefs(NUM_CHEFS),
          recipe(DISH_PER_CHEF * NUM_CHEFS, nullptr) {}

    enum FLAG_getCookAbilities { FORCE_UPDATE = -2, DEFAULT = -1 };

    void getSkills(Skill *skills, FLAG_getCookAbilities flag = DEFAULT);
    const Skill *getCookAbilities(FLAG_getCookAbilities flag = DEFAULT);
    Chef getChef(int i) const { return chefs[i]; }
    const Chef *getChefPtr(int i) const { return &chefs[i]; }
    void setChef(int i, const Chef &chef) {
        chefs[i] = chef;
        cookAbilitiesValid = false;
        size_t numStrangeSkills = chef.skill->conditionalEffects.size() +
                                  chef.companyBuff->conditionalEffects.size() +
                                  chef.nextBuff->conditionalEffects.size();
        chefHasStrangeSkills =
            (chefHasStrangeSkills & ~(1 << i)) | ((numStrangeSkills > 0) << i);
    }
    void modifyTool(int i, Tool tool) {
        chefs[i].modifyTool(tool);
        cookAbilitiesValid = false;
    }
    void modifyTool(int i, ToolEnum tool) {
        chefs[i].modifyTool(tool);
        cookAbilitiesValid = false;
    }
    Tool getTool(int i) { return chefs[i].getTool(); }
    ToolEnum getToolType(int i) { return chefs[i].getToolType(); }
    bool allowsTool(int i) { return chefs[i].allowsTool(); }
    bool repeatedChef(const Chef *const chef = NULL, int except = -1,
                      int maxChef = NUM_CHEFS) const {
        if (chef != NULL) {
            for (int i = 0; i < maxChef; i++) {
                if (except != i && chef->id == chefs[i].id) {
                    return true;
                }
            }
            return false;
        } else {
            for (int i = 0; i < maxChef; i++) {
                for (int j = i + 1; j < maxChef; j++) {
                    if (chefs[i].id == chefs[j].id) {
                        return true;
                    }
                }
            }
            return false;
        }
    }
    bool repeatedRecipe(Recipe *recipe = NULL, int except = -1,
                        int maxRecipe = NUM_CHEFS * DISH_PER_CHEF) const {
        if (recipe == NULL) {
            for (int i = 0; i < maxRecipe; i++) {
                for (int j = i + 1; j < maxRecipe; j++) {
                    if (this->recipe[i] == this->recipe[j]) {
                        return true;
                    }
                }
            }
            return false;
        } else {
            for (int i = 0; i < maxRecipe; i++) {
                if (except != i && recipe == this->recipe[i]) {
                    return true;
                }
            }
            return false;
        }
    }
    bool capable() {
        auto skills = this->getCookAbilities();
        for (size_t i = 0; i < NUM_DISHES; i++) {
            if (skills[i / DISH_PER_CHEF].ability /
                    this->recipe[i]->cookAbility ==
                0) {
                return false;
            }
        }
        return true;
    }
};

#endif