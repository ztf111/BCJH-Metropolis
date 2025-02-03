#include "Randomizer.hpp"
#include "functions.hpp"
#include "SARunner.hpp"
#include "exception.hpp"
const double bestToolProb = 0.9;
ToolEnum toolHeuristic(States s, int chefId) {
    auto chef = s.getChefPtr(chefId);
    Recipe **recipes = &s.recipe[chefId * DISH_PER_CHEF];
    if (!chef->allowsTool())
        return NO_TOOL;
    Tool best = s.getTool(chefId);
    s.modifyTool(chefId, NOT_EQUIPPED);
    int max = 0;
    for (int i = 0; i < DISH_PER_CHEF; i++) {
        max += (chef->skill->ability + best) / recipes[i]->cookAbility;
    }
    for (int t = ABILITY_ENUM_START; t < ABILITY_ENUM_END; t++) {
        Tool tool{(ToolEnum)t, 100};
        s.modifyTool(chefId, tool);
        if (!s.capable()) {
            continue;
        }
        int value = 0;
        for (int i = 0; i < DISH_PER_CHEF; i++) {
            auto skill = (chef->skill->ability + tool);
            auto thisgrade = skill / recipes[i]->cookAbility;
            value += thisgrade;
        }
        if (value > max) {
            max = value;
            best = tool;
        }
    }
    return best.type;
}

bool ChefRandomizer::randomChef(States &s) const {
    debugIntegrity(s);
    auto &chefList = this->c;
    int chefNum = rand() % NUM_CHEFS;
    Chef pChef = s.getChef(chefNum);
    int count = 0;
    auto learned = pChef.recipeLearned;
    learned->clear();
    int dishNum = chefNum * DISH_PER_CHEF;
    int totalDishNum = NUM_CHEFS * DISH_PER_CHEF;
    for (int i = 0; i < DISH_PER_CHEF; i++) {
        learned->push_back(s.recipe[dishNum + i]);
    }
    do {
        pChef = chefList->at(rand() % chefList->size());
        count++;
    } while (s.repeatedChef(&pChef, -1) && count < RANDOM_SEARCH_TIMEOUT);
    if (count >= RANDOM_SEARCH_TIMEOUT) {
        return false;
    }
    s.setChef(chefNum, pChef);
    if (pChef.recipeLearned->size() == DISH_PER_CHEF) {
        for (int i = 0; i < DISH_PER_CHEF; i++) {
            s.recipe[dishNum + i] = (*pChef.recipeLearned)[i];
        }
    }
    bool changed = true;
    auto oldS = s;
    const Skill *skills = s.getCookAbilities();
    int i = dishNum;
    do {
        auto skill = skills[i / DISH_PER_CHEF];
        if ((skill.ability / s.recipe[i]->cookAbility == 0) ||
            inArray(s.recipe, NUM_CHEFS * DISH_PER_CHEF, s.recipe[i])) {
            bool thisChanged = this->unrepeatedRandomRecipe(
                skill, s.recipe.data(), totalDishNum, i,
                RANDOM_SEARCH_TIMEOUT * RANDOM_SEARCH_TIMEOUT);
            changed = changed && thisChanged;
        }
        i++;
    } while (i % (DISH_PER_CHEF * CHEFS_PER_GUEST) != 0);
    if (changed) {
        debugIntegrity(s);
        return true;
    } else {
        s = oldS;
        debugIntegrity(s);
        return false;
    }
}
bool Randomizer::swapRecipe(States &s) const {
    double random = rand() / RAND_MAX;
    bool toolChanged = false;
    for (int i = 1; i < RANDOM_SEARCH_TIMEOUT; i++) {
        int recipeNum1 = rand() % (NUM_CHEFS * DISH_PER_CHEF);
        int recipeNum2 = rand() % (NUM_CHEFS * DISH_PER_CHEF);
        int chefNum1 = recipeNum1 / DISH_PER_CHEF;
        int chefNum2 = recipeNum2 / DISH_PER_CHEF;
        const Chef *chef1 = s.getChefPtr(chefNum1);
        const Chef *chef2 = s.getChefPtr(chefNum2);
        if (!toolChanged && random < bestToolProb) {
            toolChanged = true;
            s.modifyTool(chefNum1, toolHeuristic(s, chefNum1));
        }
        if (chef1 == chef2) {
            swap(s.recipe[recipeNum1], s.recipe[recipeNum2]);
            return true;
        } else {
            bool chef1CanCook = s.getCookAbilities()[chefNum1].ability /
                                    s.recipe[recipeNum2]->cookAbility >
                                0;
            bool chef2CanCook = s.getCookAbilities()[chefNum2].ability /
                                    s.recipe[recipeNum1]->cookAbility >
                                0;
            if (chef1CanCook && chef2CanCook) {
                swap(s.recipe[recipeNum1], s.recipe[recipeNum2]);
                return true;
            }
        }
    }
    return false;
}

bool RecipeRandomizer::randomRecipe(States &s) const {
    for (int tries = 0; tries < RANDOM_SEARCH_TIMEOUT; tries++) {
        int recipeNum = rand() % (NUM_CHEFS * DISH_PER_CHEF);
        const Skill &skill = s.getCookAbilities()[recipeNum / DISH_PER_CHEF];
        bool changed = this->unrepeatedRandomRecipe(
            skill, s.recipe.data(), NUM_CHEFS * DISH_PER_CHEF, recipeNum);
        if (changed) {
            return true;
        }
    }
    return false;
}

bool ChefRandomizer::swapChefTool(States &s) const {
    States saveS = s;
    int n;
    for (n = 0; n < RANDOM_SEARCH_TIMEOUT; n++) {
        s = saveS;
        int chefNum = rand() % NUM_CHEFS;
        int orig_tool = s.getToolType(chefNum);
        int tool;
        do {
            tool = rand() % 6 + ABILITY_ENUM_START;
        } while (tool == orig_tool);
        s.modifyTool(chefNum, (ToolEnum)tool);
        auto &skill = s.getCookAbilities()[chefNum];
        auto &ability = skill.ability;
        for (int i = chefNum * DISH_PER_CHEF;
             i < chefNum * DISH_PER_CHEF + DISH_PER_CHEF; i++) {
            if (ability / s.recipe[i]->cookAbility == 0) {
                bool thisChanged = this->unrepeatedRandomRecipe(
                    skill, s.recipe.data(), NUM_CHEFS * DISH_PER_CHEF, i);
                if (!thisChanged) {
                    break;
                }
            }
        }
    }
    if (n >= RANDOM_SEARCH_TIMEOUT) {
        s = saveS;
        return false;
    } else {
        return true;
    }
}
States RecipeRandomizer::iterPropose(States s, int i) {
    if (iter == r->end()) {
        iter = r->begin();
        throw IterStopException();
    }
    while ((iter->rarity != s.recipe[i]->rarity) ||
           (inArray(s.recipe, NUM_DISHES, &(*iter)))) {
        iter++;
        if (iter == r->end()) {
            iter = r->begin();
            throw IterStopException();
        }
    }
    s.recipe[i] = &(*iter);
    iter++;
    return s;
}
States RecipeRandomizer::operator()(States s) {
#ifdef MEASURE_TIME
    struct timespec start, finish;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
#endif
    debugIntegrity(s);
    double r = (double)rand() / RAND_MAX;
    double p_randomRecipe = 1;
    p_randomRecipe = 0.9;
    if (r > p_randomRecipe) {
        success += swapRecipe(s);
    } else {
        success += randomRecipe(s);
    }
    calls++;

#ifdef MEASURE_TIME
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &finish);
    randomRecipeTime +=
        finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) * 1e-9;
#endif
    debugIntegrity(s);
    return s;
}
/**
 * @todo swap chef may cause grade=0.
 */
States ChefRandomizer::operator()(States s) {
#ifdef MEASURE_TIME
    struct timespec start, finish;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
#endif
    debugIntegrity(s);
    double random = (double)rand() / RAND_MAX;
    double p_randomChef = 0.9;
    calls++;
    p_randomChef = 0.85;
    if (random < 1 - p_randomChef) {
        success += swapChefTool(s);

    } else if (random >= 1 - p_randomChef) {
        success += randomChef(s);
    }

    debugIntegrity(s);
    SARunner saRunner(rl, c, r, false, f::t_dist_slow);
#ifdef MEASURE_TIME
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &finish);
    randomChefTime +=
        finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) * 1e-9;
#endif
    return saRunner.run(&s);
}
Randomizer::Randomizer(const CList *c, RList *r, const RuleInfo *rl)
    : c(c), r(r), rl(rl), success(0), calls(0) {
    for (int rarity = 1; rarity <= 5; rarity++) {
        rarityRecipeMap[rarity] = std::vector<int>();
    }
    for (auto &recipe : *r) {
        rarityRecipeMap[recipe.rarity].push_back(recipe.id);
    }
}
bool Randomizer::unrepeatedRandomRecipe(const Skill &skill, Recipe **recipes,
                                        int size, int index,
                                        int repeats) const {
    int count = 0;
    Recipe *r = NULL;
    auto &rl = this->r;
    double p_sameRarity = 0.4;
    do {
        if (rand() / RAND_MAX < p_sameRarity) {
            int rarity = recipes[index]->rarity;
            if (rarityRecipeMap.at(rarity).size() == 1) {
                p_sameRarity = 0;
            } else {
                int radomRecipeId = rarityRecipeMap.at(
                    rarity)[rand() % rarityRecipeMap.at(rarity).size()];
                r = rl->byId(radomRecipeId);
            }
        } else {
            r = &rl->at(rand() % rl->size());
        }
        count++;
    } while (
        ((skill.ability / r->cookAbility == 0) || inArray(recipes, size, r)) &&
        count < repeats);
    if (count >= repeats) {
        return false;
    }
    recipes[index] = r;

    return true;
}
