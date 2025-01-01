#ifndef EXCEPTION_HPP
#define EXCEPTION_HPP
#include <exception>

class NoChefException : public std::exception {
  private:
    std::size_t numChef;

  public:
    virtual const char *what() const throw() {
        std::cout << "拥有的厨师太少，当前拥有" << numChef << "个厨师。";
        return "拥有的厨师太少，得到的结果可能不理想，建议手动计算。";
    }
    NoChefException(std::size_t numChef = 0) throw() : numChef(numChef) {}
};
class NoRecipeException : public std::exception {
  public:
    std::size_t numRecipe;
    virtual const char *what() const throw() {
        std::cout << "拥有的菜谱太少，当前拥有" << numRecipe << "个菜谱。";
        return "拥有的菜谱太少，得到的结果可能不理想，建议手动计算。";

    }
    NoRecipeException(std::size_t numRecipe) throw() : numRecipe(numRecipe) {}
};
class FileNotExistException : public std::exception {
  public:
    virtual const char *what() const throw() {
        return "json文件缺失。如果在网页端，请确认已经上传了文件；如果在"
               "本地，请确认已经下载了文件。";
    }
    FileNotExistException() throw() {}
};
class UnknownSkillException : public std::exception {
  public:
    std::string description;
    virtual const char *what() const throw() {
        std::cout << "未知技能  " << RED << description << NO_FORMAT
                  << std::endl;
        return "未知技能。";
    }
    UnknownSkillException() throw() {}
    UnknownSkillException(std::string error_description) throw()
        : description(error_description) {}
};
class UnknownSkillWarning {
  public:
    std::string description;
    UnknownSkillWarning() {}
    UnknownSkillWarning(std::string error_description)
        : description(error_description) {
        std::cerr << RED << "未知技能：   " << description << NO_FORMAT
                  << std::endl;
    }
};
#define RANDOM_SEARCH_TIMEOUT 100
#endif