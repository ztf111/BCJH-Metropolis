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
#include "banquetRuleGen.hpp"
#include "StatesRecorder.hpp"
#include "utils/ProgressBar.hpp"
#include "run.hpp"

size_t NUM_GUESTS; 

const int targetScore = TARGET_SCORE_APPROXIMATE;
const int T_MAX_CHEF = T_MAX_CHEF_orig;
const int T_MAX_RECIPE = T_MAX_RECIPE_orig;

std::tuple<Json::Value, Json::Value, Json::Value, Json::Value, std::size_t>
loadJsonFiles();

std::tuple<bool, int, bool, int, int, int, std::string>
parseArgs(int argc, char *argv[]) {
    int iterChef = 5000;
    int iterRecipe = 1000;
    bool silent = false;
    int log = 0;
    int seed = (int)(time(NULL) * 100);
    bool mp = true;
    int seed_orig = seed;

    std::string recover_str;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-s") {
            silent = true;
        } else if (arg == "-v") {
            log += VERBOSE;
        } else if (arg == "-h") {
            std::cout << "-s: 无进度条，默认有进度条" << std::endl;
            std::cout << "-v: 输出详细信息，默认不输出" << std::endl;
            std::cout << "-C: 厨师迭代次数，默认5000" << std::endl;
            std::cout << "-R: 菜谱迭代次数，默认1000" << std::endl;
            std::cout << "--no-mp: 禁用多线程" << std::endl;
            std::cout << "--recover-str: 恢复代码" << std::endl;
        } else if (arg == "--no-mp") {
            mp = false;
        } else if (arg == "--seed") {
            seed = atoi(argv[++i]);
        } else if (arg == "-C") {
            iterChef = atoi(argv[++i]);
        } else if (arg == "-R") {
            iterRecipe = atoi(argv[++i]);
        } else if (arg == "--recover-str") {
            recover_str = argv[++i];
        } else {
            std::cout << "未知参数：" << arg << std::endl;
            exit(1);
        }
    }
    if (seed_orig != seed) {
        if (mp) {
            mp = false;
            std::cout << "Seed set to " << seed << ", mp disabled" << std::endl;
        } else {
            std::cout << "Seed set to " << seed << std::endl;
        }
    }
    if (recover_str != "") {
        assert(iterChef == 0 && iterRecipe == 0 && !mp);
        // In debug mode, whenever recover_str is set, we disable
        // multi-threading and make sure that iters are 0 to avoid recovered
        // string from being overwritten.
    }
    return {silent, log, mp, seed, iterChef, iterRecipe, recover_str};
}

int main(int argc, char *argv[]) {
    auto [silent, log, mp, seed, iterChef, iterRecipe, recover_str] =
        parseArgs(argc, argv);
    SARunner::init(T_MAX_CHEF, T_MAX_RECIPE, iterChef, iterRecipe, targetScore);
    auto [directUserData, usrData, gameData, ruleData, fileHash] =
        loadJsonFiles();
    if (directUserData.size() != 0) {
        usrData = directUserData;
        std::cout << GREEN << "读取到游戏中直接导出的数据，不使用白菜菊花数据。"
                  << NO_FORMAT << std::endl;
    }
    testJsonUpdate(gameData, usrData);
    auto [num_guests,rl] = loadFirstBanquetRule( ruleData, true);
    if (num_guests == -1) {
        std::cout << "读取规则失败。" << std::endl;
        exit(1);
    } else {
        NUM_GUESTS = num_guests;
    }
    auto [recipeList, chefList] = loadJson(gameData, usrData);

    clock_t start, end;
    start = clock();
    int totalScore = 0;
    size_t num_threads = std::thread::hardware_concurrency();

    if (!mp) {
        num_threads = 1;
    }
    // num_threads = 1;
    // seed = 10;
    MultiThreadProgressBar::getInstance(num_threads);
    std::cout << "启用" << num_threads
              << "线程，建议期间不要离开窗口，否则可能影响速度。" << std::endl;

    std::vector<std::future<Result>> futures;
    StatesRecorderBase *statesRecorder = NULL;
    if (recover_str == "") {
        statesRecorder = new StatesRecorderFile("states.txt", fileHash,
                                                &chefList, &recipeList);
    } else {
        statesRecorder =
            new StatesRecorderString(recover_str, &chefList, &recipeList);
    }
    auto statesRecord = statesRecorder->get_states(num_threads);
    for (size_t i = 0; i < num_threads; i++) {
        futures.push_back(std::async(std::launch::async, run, std::ref(rl),
                                     std::ref(chefList), std::ref(recipeList),
                                     0, !silent, seed++, (int)i, statesRecord[i]));
    }
    int max_score = 0;
    Result result;
    for (auto &future : futures) {
        Result tmp = future.get();
        statesRecorder->add_state(&tmp.state);
        totalScore += tmp.score;
        if (tmp.score > max_score) {
            result = tmp;
            max_score = result.score;
        }
    }
    std::cout << "\n最佳结果：" << std::endl;

    log += ORDINARY;
    // log += VERBOSE;
    // result.state.updateNameFromTool();
    std::cout << "随机种子：" << result.seed << std::endl;
    sumPrice(rl, result.state, log);
    std::cout << "**************\n总分: " << result.score << "\n***************"
              << std::endl;

    end = clock();
    std::cout << "用时：" << (double)(end - start) / CLOCKS_PER_SEC << "秒"
              << std::endl;
#ifdef VIS_HISTORY
    std::cout << "均分：" << totalScore / num_threads << std::endl;
#endif

    delete statesRecorder;
    MultiThreadProgressBar::destroy();
}

/**
 * @return directUserData, userData, gameData, ruleData
 */
std::tuple<Json::Value, Json::Value, Json::Value, Json::Value, std::size_t>
loadJsonFiles() {
    Json::Value usrData;
    Json::Value gameData;
    Json::Value ruleData;
    Json::Value directUsrData;

    auto dirs = {"./", "../data/", "../../data/", "../../../data/"};

    std::ifstream gameDataF, usrDataF, ruleDataF, directUsrDataF;
    std::map<std::string, std::ifstream &> files = {
        {"data.min", gameDataF},
        {"userData", usrDataF},
        {"ruleData", ruleDataF},
        {"directUserData", directUsrDataF}};
    for (auto &file : files) {
        for (const std::string &dir : dirs) {
            file.second.open(dir + file.first + ".json", std::ifstream::binary);
            if (file.second.good()) {
                break;
            }
            file.second.close();
        }
    }

    if (!gameDataF.good() || !ruleDataF.good() ||
        (!usrDataF.good() && !directUsrDataF.good())) {
        std::cout
            << "json文件有缺失。如果在网页端，请确认已经上传了文件；如果在"
               "本地，请确认已经data.min.json和ruleData."
               "json文件已经在工作目录下，而且directUserData.json或userData."
               "json文件已经在工作目录下。\n";
        exit(1);
    }

    try {
        if (usrDataF.good()) {
            usrDataF >> usrData;
            usrDataF.close();
        }
        if (directUsrDataF.good()) {
            directUsrDataF >> directUsrData;
            directUsrDataF.close();
        }
        gameDataF >> gameData;
        gameDataF.close();
        ruleDataF >> ruleData;
        ruleDataF.close();
    } catch (Json::RuntimeError &) {
        std::cout
            << "json文件格式不正确。如果文件内容是手动复制的，确认文件已"
               "经复制完整。如果文件是从powershell下载的，请确认编码是utf-8\n";
        exit(1);
    } catch (Json::LogicError &) {
        std::cout << "json文件格式不正确。请确认文件来自白菜菊花而非图鉴网。\n";
        exit(1);
    }

    std::hash<std::string> hasher;
    size_t fileHash = hasher(gameData.toStyledString()) ^
                      hasher(usrData.toStyledString()) ^
                      hasher(ruleData.toStyledString()) ^
                      hasher(directUsrData.toStyledString());

    if (usrData.isMember("user")) {
        std::cout << GREEN "用户名：" << usrData["user"].asString()
                  << "；创建时间：" << usrData["create_time"].asString()
                  << NO_FORMAT << std::endl;
        std::stringstream data(usrData["data"].asCString());
        data >> usrData;
        // Convert the escaped string to an object
        usrData["type"] = "bcjh";
    }
    if (directUsrData["ret"].asString() == "E") {
        std::cout << "游戏数据错误，请检查游戏中导出的代码是否填写正确。"
                  << std::endl;
        directUsrData = Json::Value();
    } else {
        directUsrData = directUsrData["msg"];
        directUsrData["type"] = "in-game";
    }

    return {std::move(directUsrData), std::move(usrData), std::move(gameData),
            std::move(ruleData), fileHash};
}
