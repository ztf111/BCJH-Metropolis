#ifndef BCJH_JS_HPP
#define BCJH_JS_HPP
#include <iostream>
#include <string>
#include "Chef.hpp"
#include "Recipe.hpp"
#include <map>
#include "Calculator.hpp"
#include <vector>
#include "SARunner.hpp"
#include "functions.hpp"
#include "../config.hpp"
#include <stdio.h>
#include <fstream>
#include <time.h>
#include "exception.hpp"
#include <future>
#include <vector>
#ifdef EMSCRIPTEN
#include <emscripten/bind.h>
using namespace emscripten;
#endif
#include "run.hpp"

#ifdef MEASURE_TIME
extern double randomRecipeTime;
extern double randomChefTime;
extern double banquetRuleTime;
extern double getStatesSkillsTime;
extern double generateBanquetRuleTime, generateBanquetRuleTimeOut;
extern double calculatePriceTime, calculatePriceTimeOut;
#endif

class ResultJsonSerializable {
  public:
    int score;
    int seed;
    const States state;
    std::string logs;
    std::string recover_str;
    ResultJsonSerializable(int score, int seed, States state, std::string &logs)
        : score(score), seed(seed), state(state), logs(logs) {}
    ResultJsonSerializable(Result &result)
        : score(result.score), seed(result.seed), state(result.state),
          logs(result.logs) {}
    ResultJsonSerializable(Result &result, std::string &recover_str)
        : score(result.score), seed(result.seed), state(result.state),
          logs(result.logs), recover_str(recover_str) {}
    Json::String toJson() {
        Json::Value result;
        result["score"] = score;
        result["seed"] = seed;
        Json::Value chefsList(Json::arrayValue);
        for (int i = 0; i < NUM_CHEFS; i++) {
            chefsList[i] = state.getChefPtr(i)->getName();
        }
        result["chefs"] = chefsList;
        Json::Value recipesList(Json::arrayValue);
        for (int i = 0; i < NUM_CHEFS * DISH_PER_CHEF; i++) {
            recipesList[i] = state.recipe[i]->name;
        }
        result["recipes"] = recipesList;
        result["logs"] = logs;
        result["recover_str"] = recover_str;
        return result.toStyledString();
    }
};
/**
 * @return  pair<Json::Value gameData, Json::Value userData>
 */
std::pair<Json::Value, Json::Value> loadJson(std::stringstream &userDataSs);

/**
 * @brief This exposes the `run` function to JavaScript
 * @param recover_string
 * `recover_str` from the previous result can be input here, so the next run
 * will continue from the previous state.
 * @return A dict, with keys "chefs", "logs", "recipes", "recover_str", "score",
 * "seed".
 */
std::string
#ifdef EMSCRIPTEN
    EMSCRIPTEN_KEEPALIVE
#endif
    runjs(const std::string &userDataIn, const std::string &ruleDataIn,
          int targetScore, int iterChef = 5000, int iterRecipe = 1000,
          bool allowTool = true
//   , const std::string &recover_string = ""
#ifdef EMSCRIPTEN_PROGRESS
          ,
          emscripten::val postProgress = emscripten::val::null()
#endif
    );

#ifdef EMSCRIPTEN
EMSCRIPTEN_BINDINGS(module) { emscripten::function("run", &runjs); };
#endif
#endif
// PgEAAA4EAAADAwAAiQQAAMECAACSBAAABwAAADwAAAACAAAAZAAAAAEAAAAAAAAABQAAAGQAAAAEAAAAZAAAAAEAAAAAAAAAQwAAAFkAAAB4AgAAiwAAAHcCAAABAgAADQAAAHkCAAAyAAAA9gEAAHoCAAASAAAAIAEAAPwBAABFAAAAXgAAADICAAD4AQAA