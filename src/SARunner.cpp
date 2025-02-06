#include <iostream>
#include "functions.hpp"
#include "SARunner.hpp"
#include <map>
#include <vector>
#include "Chef.hpp"
#include "Recipe.hpp"
#include <cmath>
#include <random>
#include <fstream>
#include "exception.hpp"
// #include "activityRule.hpp"
#include <limits.h>
#include "Randomizer.hpp"
#ifndef EMSCRIPTEN
#include "utils/ProgressBar.hpp"
#endif

int SARunner::T_MAX_CHEF, SARunner::T_MAX_RECIPE, SARunner::iterChef,
    SARunner::iterRecipe, SARunner::targetScore;
SARunner::SARunner(const RuleInfo *rl, const CList *chefList, RList *recipeList,
                   bool randomizeChef, f::CoolingSchedule coolingScheduleFunc,
                   int threadId)
    : threadId(threadId), rl(rl), chefList(chefList), recipeList(recipeList),
      coolingScheduleFunc(coolingScheduleFunc) {
    if (randomizeChef) {
        this->randomMoveFunc =
            new ChefRandomizer(chefList, recipeList, rl, targetScore);
        this->tMax = T_MAX_CHEF;
        this->tMin = T_MAX_CHEF / 10;
        this->stepMax = iterChef;
    } else {
        this->randomMoveFunc = new RecipeRandomizer(chefList, recipeList, rl);
        this->tMax = T_MAX_RECIPE;
        this->tMin = T_MAX_RECIPE / 10;
        this->stepMax = iterRecipe;
    }
#ifdef VIS_HISTORY
    this->history = new History[stepMax];
#endif
}

States SARunner::generateStates(States *initState, const CList *chefList) {
    States s;

    auto allChefsId = std::set<int>();
    for (size_t i = 0; i < chefList->size(); i++) {
        allChefsId.insert(chefList->at(i).id);
    }

    for (size_t j = 0; j < NUM_CHEFS; j++) {
        if (initState != NULL &&
            allChefsId.contains(initState->getChefPtr(j)->id) &&
            !s.repeatedChef(initState->getChefPtr(j), j, j)) {
            Chef chef = chefList->byId(initState->getChefPtr(j)->id);
            if (chef.getToolType() != NO_TOOL) {
                chef.modifyTool(initState->getToolType(j));
            }
            s.setChef(j, chef);
        } else {
            int count = 0;
            const Chef *randomChef;
            do {
                randomChef = &chefList->at(rand() % chefList->size());
                s.setChef(j, *randomChef);
                count++;

            } while (s.repeatedChef(randomChef, j) &&
                     count < RANDOM_SEARCH_TIMEOUT);
            if (count >= RANDOM_SEARCH_TIMEOUT) {
                std::cout << NoChefException().what() << std::endl;
                exit(1);
            }
        }
    }

    int r = 0;
    RList *recipeList = this->randomMoveFunc->r;
    auto allRecipesId = std::set<int>();
    for (size_t i = 0; i < recipeList->size(); i++) {
        allRecipesId.insert(recipeList->at(i).id);
    }

    for (size_t j = 0; j < NUM_CHEFS; j++) {
        auto &skill = s.getCookAbilities()[j];
        for (int i = 0; i < DISH_PER_CHEF; i++) {
            if (initState != NULL &&
                allRecipesId.contains(initState->recipe[r]->id) &&
                !s.repeatedRecipe(initState->recipe[r], r) &&
                (skill.ability / initState->recipe[r]->cookAbility != 0)) {
                s.recipe[r] = initState->recipe[r];
            } else {
                int count = 0;
                Recipe *newRecipe;
                do {
                    newRecipe = &recipeList->at(rand() % recipeList->size());
                    count++;
                } while (((skill.ability / newRecipe->cookAbility == 0) ||
                          inArray(s.recipe, r, newRecipe)) &&
                         count < RANDOM_SEARCH_TIMEOUT * RANDOM_SEARCH_TIMEOUT);
                s.recipe[r] = newRecipe;
                if (count >= RANDOM_SEARCH_TIMEOUT * RANDOM_SEARCH_TIMEOUT) {
                    std::cout << NoRecipeException(recipeList->size()).what()
                              << std::endl;
                    exit(1);
                }
            }
            r++;
        }
    }
    return s;
}

States SARunner::run(States *s0,
#ifdef EMSCRIPTEN_PROGRESS
                     emscripten::val postProgress,
#endif
#ifndef EMSCRIPTEN
                     bool progress,
#endif
                     bool silent, const char *filename) {
    if (this->stepMax == 0 && s0 != NULL) {
        return *s0;
        // Skip validity check of the initial state
    }
    States s;
    s = generateStates(s0, this->chefList);
    debugIntegrity(s);
    int energy = sumPrice(*rl, s);

    this->bestState = s;
    this->bestEnergy = energy;
    int step = 0;
    double t = this->tMax;
    int progressPercent = 0;
    int changed = 0;
    while (step < this->stepMax) {

#ifdef EMSCRIPTEN_PROGRESS
        if (postProgress != emscripten::val::null()) {
            int newProgressPercent = (int)(step * 100.0 / this->stepMax);
            if (newProgressPercent > progressPercent) {
                progressPercent = newProgressPercent;
                postProgress(progressPercent);
            }
        }

#endif

        if (progress && !silent) {
            if (step * 10 / stepMax > progressPercent) {
                progressPercent = step * 10 / stepMax;
                MultiThreadProgressBar::getInstance(threadId)->print(
                    threadId, progressPercent * 10,
                    "当前最高分数：" + std::to_string(this->bestEnergy));
            }
        }

        States newS;
        newS = (*randomMoveFunc)(s);
        debugIntegrity(newS);
        // std::cin >> step;
        // print(newS);
        int newEnergy = sumPrice(*rl, newS);
        double prob = 0;
        int delta = energy - newEnergy;
        if (delta / t < -30) {
            prob = 1.01;
        } else {
            // prob = 1.0 / (1 + std::exp(delta / (3 * t + 0.0)));
            prob = std::exp(-delta / t);
        }
        if (prob > (double)rand() / RAND_MAX) {
            s = newS;
            // print(s);

            energy = newEnergy;
        }
        if (energy > this->bestEnergy) {
            this->bestEnergy = energy;
            this->bestState = s;
            changed += 1;
        }
        t = coolingScheduleFunc(this->stepMax, step, this->tMax, this->tMin);
        if (t <= this->tMin) {
            break;
        }
#ifdef VIS_HISTORY
        this->history[step].energy = energy;
        this->history[step].t = t;
        this->history[step].states = s;
#endif
        if (energy >= this->targetScore) {
            break;
        }
        step++;
    }
    // Special case: if this is the last step for randomizing the recipe, we go
    // over all recipes of the same rarity
    // if (RecipeRandomizer *rr =
    //         dynamic_cast<RecipeRandomizer *>(this->randomMoveFunc)) {
    //     States newS;
    //     int changedFinal = 0;
    //     for (size_t i = 0; i < NUM_DISHES; i++) {
    //         while (true) {
    //             try {
    //                 newS = rr->iterPropose(s, i);
    //                 int score = sumPrice(*rl, newS);
    //                 if (score > this->bestEnergy) {
    //                     this->bestEnergy = score;
    //                     this->bestState = newS;
    //                     s = newS;
    //                     changedFinal += 1;
    //                 }
    //             } catch (IterStopException &e) {
    //                 break;
    //             }
    //         }
    //     }
    //     if (changedFinal > 0) {
    //         iterRecipe *= 1.001;
    //     }
    //     std::cout << changedFinal << "/" << iterRecipe << " ";
    // }
    if (progress && !silent) {
        MultiThreadProgressBar::getInstance(threadId)->print(
            threadId, 100, "最高分数：" + std::to_string(this->bestEnergy));
    }
#ifdef EMSCRIPTEN_PROGRESS
    if (postProgress != emscripten::val::null()) {
        postProgress(100);
    }
#endif
#ifdef VIS_HISTORY
    if (progress && !silent) {
        std::fstream file;
        file.open("../out/history.csv", std::ios::out);
        for (int i = 0; i < step; i++) {
            file << this->history[i].energy << "," << this->history[i].t
                 << std::endl;
        }
        file.close();
        // std::cout << "Saved to ../out/history.csv!" <<std::endl;
        system("python ../src/plot.py &");
    }

    if (filename) {

        std::fstream file;
        std::string fn(filename);
        // std::cout << "Saving to file: " << fn + ".csv" << std::endl;
        file.open(fn + ".csv", std::ios::out);
        for (int i = 0; i < step; i++) {
            file << this->history[i].energy << "," << this->history[i].t
                 << std::endl;
        }
        file.close();
        std::string cmd = "python ../src/plot.py -f " + fn + " &";
        system(cmd.c_str());
    }
#endif
    return this->bestState;
}

void SARunner::print(States s, bool verbose) const {
    int r = 0;
    for (size_t i = 0; i < NUM_CHEFS; i++) {

        std::cout << "Chef: " << s.getChefPtr(i)->getName() << std::endl
                  << "Recipe ";
        for (int j = 0; j < DISH_PER_CHEF; j++) {
            std::cout << j << ": " << s.recipe[r++]->name;
        }
        std::cout << std::endl;
    }
    if (verbose) {
        std::cout << "------verbose------" << std::endl;

        // r = 0;
        // for (int i = 0; i < NUM_CHEFS; i++) {
        //     for (int j = 0; j < DISH_PER_CHEF; i++) {
        //         getPrice(*s.chef[i], *s.recipe[r++], true);
        //     }
        // }
        sumPrice(*rl, s, ORDINARY);
    }
}
