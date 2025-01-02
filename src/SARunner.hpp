#ifndef SARUNNER_HPP
#define SARUNNER_HPP
#include <iostream>
#include "functions.hpp"
#include <map>
#include <vector>
#include "Chef.hpp"
#include "Recipe.hpp"
#include <cmath>
#include <random>
#include <fstream>
#include "Randomizer.hpp"
#ifdef EMSCRIPTEN
#include <emscripten/bind.h>
#endif

struct History {
    States states;
    int energy;
    double t;
};

class SARunner {
    Randomizer *randomMoveFunc;
    f::CoolingSchedule coolingScheduleFunc;
    const int threadId;
    int stepMax;
    int tMax;
    int tMin;
    const CList *chefList;
    RList *recipeList;
    static int targetScore;
    const RuleInfo *rl;
    States bestState;
#ifdef VIS_HISTORY
    History *history;
#endif
    States generateStates(States *initState, const CList *chefList);
    static int T_MAX_CHEF;
    static int T_MAX_RECIPE;
    static int iterChef;
    static int iterRecipe;

  public:
    static void init(int T_MAX_CHEF, int T_MAX_RECIPE, int iterChef,
                     int iterRecipe, int targetScore) {
        SARunner::T_MAX_CHEF = T_MAX_CHEF;
        SARunner::T_MAX_RECIPE = T_MAX_RECIPE;
        SARunner::iterChef = iterChef;
        SARunner::iterRecipe = iterRecipe;
        SARunner::targetScore = targetScore;
    }
    int bestEnergy = 0;
    void print(States s, bool verbose = false) const;
    /**
     * @brief Construct a new SARunner object
     * @param chefList
     * @param recipeList
     * @param randomizeChef If false, randomize recipe.
     * @param getEnergyFunc
     * @param coolingScheduleFunc
     */
    SARunner(
        const RuleInfo *rl, const CList *chefList, RList *recipeList,
        bool randomizeChef,
        f::CoolingSchedule coolingScheduleFunc = f::exponential_multiplicative,
        int threadId = -1);
    States run(States *s = NULL,
#ifdef EMSCRIPTEN_PROGRESS
               emscripten::val postProgress = emscripten::val::null(),
#endif
#ifndef EMSCRIPTEN
               bool progress = false,
#endif
               bool silent = false, const char *fn = NULL);

    ~SARunner() {
        delete this->randomMoveFunc;
#ifdef VIS_HISTORY
        delete[] this->history;
#endif
    }
};
#endif