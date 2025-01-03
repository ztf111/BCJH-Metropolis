#include "Types.hpp"
#include "Recipe.hpp"
#include <cassert>
#include "include/cereal/archives/portable_binary.hpp"
#include "include/cereal/types/memory.hpp"
#include "include/cereal/types/vector.hpp"
#include "include/cereal/types/map.hpp"
#include "include/cereal/types/string.hpp"

bool Tool::allowTool = true;

int GradeBuffCondition::test(const Skill *s, Recipe **r) {
    if (grade == -1) {
        throw std::runtime_error("GradeBuffCondition::test: grade not set");
    }
    int count = 0;
    for (int i = 0; i < DISH_PER_CHEF; i++) {
        count += (s->ability / r[i]->cookAbility >= grade);
    }
    return count;
}

int ThreeSameCookAbilityBuffCondition::test(const Skill *s, Recipe **r) {
    Ability tmp(1, 1, 1, 1, 1, 1);
    for (int i = 0; i < DISH_PER_CHEF; i++) {
        tmp = tmp && r[i]->cookAbility;
    }
    return (tmp != 0);
}

void Skill::operator+=(const Skill &s) {
    this->ability.add(s.ability);
    this->cookAbilityPercentBuff.add(s.cookAbilityPercentBuff);
    this->abilityBuff.add(s.abilityBuff);
    this->abilityBaseBuff.add(s.abilityBaseBuff);
    this->flavorBuff.add(s.flavorBuff);
    this->flavorBaseBuff.add(s.flavorBaseBuff);
    this->materialBuff.add(s.materialBuff);
    this->materialBaseBuff.add(s.materialBaseBuff);
    this->rarityBuff.add(s.rarityBuff);
    this->rarityBaseBuff.add(s.rarityBaseBuff);
    this->gradeBuff.add(s.gradeBuff);
    this->pricePercentBuff += s.pricePercentBuff;
    this->baseAddBuff += s.baseAddBuff;
    this->conditionalEffects.insert(this->conditionalEffects.end(),
                                    s.conditionalEffects.begin(),
                                    s.conditionalEffects.end());
    if (this->type == PARTIAL) {
        // PARTIAL: loading from json
        // It's possible that in data.min.json, a skill has multiple effects
        // pointing at the same Tag.
        if (this->chefTagsForPARTIAL.size()) {
            assert(s.chefTagsForPARTIAL == this->chefTagsForPARTIAL);
        }
        this->chefTagsForPARTIAL = s.chefTagsForPARTIAL;
    }
    if (this->type == UNSET) {
        // UNSET: load to empty skill in chef
        assert(this->chefTagsForPARTIAL.size() == 0);
        this->chefTagsForPARTIAL = s.chefTagsForPARTIAL;
        this->type = s.type;
    }
    if (this->type == PARTIAL || this->type == NEXT) {
        assert(this->type == s.type);
        // Only Unset and Self can merge with different types.
    }
}

void MaterialCategoryBuff::add(const MaterialCategoryBuff &m) {
    this->vegetable += m.vegetable;
    this->meat += m.meat;
    this->fish += m.fish;
    this->creation += m.creation;
}

std::vector<Printer> MaterialCategoryBuff::getPrinters() const {
    std::vector<Printer> p;
    p.push_back(Printer("菜", vegetable));
    p.push_back(Printer("肉", meat));
    p.push_back(Printer("鱼", fish));
    p.push_back(Printer("面", creation));
    return p;
}

int *MaterialCategoryBuff::operator[](std::string name) {
    if (name == "Vegetable") {
        return &this->vegetable;
    }
    if (name == "Meat") {
        return &this->meat;
    }
    if (name == "Fish") {
        return &this->fish;
    }
    if (name == "Creation") {
        return &this->creation;
    }
    return NULL;
}

void FlavorBuff::add(const FlavorBuff &f) {
    this->sweet += f.sweet;
    this->salty += f.salty;
    this->sour += f.sour;
    this->bitter += f.bitter;
    this->spicy += f.spicy;
    this->tasty += f.tasty;
}

std::vector<Printer> FlavorBuff::getPrinters() const {
    std::vector<Printer> p;
    p.push_back(Printer("甜", sweet));
    p.push_back(Printer("咸", salty));
    p.push_back(Printer("酸", sour));
    p.push_back(Printer("苦", bitter));
    p.push_back(Printer("辣", spicy));
    p.push_back(Printer("鲜", tasty));
    return p;
}

int FlavorBuff::operator*(const FlavorEnum f) const {
    const int *ptr = &this->sweet;
    return ptr[f - FLAVOR_ENUM_START - 1];
}

int *FlavorBuff::operator[](std::string name) {
    if (name == "Sweet") {
        return &this->sweet;
    }
    if (name == "Salty") {
        return &this->salty;
    }
    if (name == "Sour") {
        return &this->sour;
    }
    if (name == "Bitter") {
        return &this->bitter;
    }
    if (name == "Spicy") {
        return &this->spicy;
    }
    if (name == "Tasty") {
        return &this->tasty;
    }
    return NULL;
}

void Ability::multiply(double a) {
    int *ptr = &this->stirfry;
    for (int i = 0; i < 6; i++) {
        ptr[i] = int(ptr[i] * a);
    }
}

int Ability::operator/(const Ability &a) const {
    int grade = 5;
    const int *thisptr = &this->stirfry;
    const int *aptr = &a.stirfry;
    for (int i = 0; i < 6; i++) {
        if (aptr[i] != 0) {
            grade =
                grade < (thisptr[i] / aptr[i]) ? grade : (thisptr[i] / aptr[i]);
        }
    }
    grade = grade < 0 ? 0 : grade;
    return grade;
}

Ability Ability::operator+(Tool t) const {
    Ability tmp(t);
    tmp.add(*this);
    return tmp;
}

Ability Ability::operator&&(const Ability &a) const {
    Ability tmp;
    const int *thisptr = &this->stirfry;
    const int *aptr = &a.stirfry;
    int *tmpptr = &tmp.stirfry;
    for (int i = 0; i < 6; i++) {
        tmpptr[i] = thisptr[i] && aptr[i];
    }
    return tmp;
}

int Ability::operator!=(int a) const {
    const int *ptr = &this->stirfry;
    int count = 0;
    for (int i = 0; i < 6; i++) {
        count += (ptr[i] != a);
    }
    return count;
}

bool Ability::operator==(int a) const {
    const int *ptr = &this->stirfry;
    for (int i = 0; i < 6; i++) {
        if (ptr[i] != a)
            return false;
    }
    return true;
}

void Ability::add(const Ability &a) {
    int *thisptr = &this->stirfry;
    const int *aptr = &a.stirfry;
    for (int i = 0; i < 6; i++) {
        thisptr[i] += aptr[i];
    }
}

void Ability::add(int a) {
    int *ptr = &this->stirfry;
    for (int i = 0; i < 6; i++) {
        ptr[i] += a;
    }
}

std::vector<Printer> Ability::getPrinters(bool percent) const {
    std::vector<Printer> p;
    p.push_back(Printer("炒", stirfry, percent));
    p.push_back(Printer("烤", bake, percent));
    p.push_back(Printer("煮", boil, percent));
    p.push_back(Printer("蒸", steam, percent));
    p.push_back(Printer("炸", fry, percent));
    p.push_back(Printer("切", knife, percent));
    return p;
}

int Ability::operator[](int name) {
    if (name == KNIFE) {
        return this->knife;
    }
    if (name == STIRFRY) {
        return this->stirfry;
    }
    if (name == BAKE) {
        return this->bake;
    }
    if (name == BOIL) {
        return this->boil;
    }
    if (name == STEAM) {
        return this->steam;
    }
    if (name == FRY) {
        return this->fry;
    }
    std::cout << "Ability::operator[]: Error: " << name << std::endl;
    exit(1);
    return -1;
}

int *Ability::operator[](std::string name) {
    if (name == "Knife") {
        return &this->knife;
    }
    if (name == "Stirfry") {
        return &this->stirfry;
    }
    if (name == "Bake") {
        return &this->bake;
    }
    if (name == "Boil") {
        return &this->boil;
    }
    if (name == "Steam") {
        return &this->steam;
    }
    if (name == "Fry") {
        return &this->fry;
    }
    return NULL;
}

void DiscretizedBuff::add(const DiscretizedBuff &r) {
    for (int i = 0; i < 5; i++)
        this->data[i] += r.data[i];
}

void DiscretizedBuff::print(const std::string &name) const {
    int sum = 0;
    for (int i = 0; i < 5; i++) {
        sum += data[i];
    }
    if (sum > 0) {
        int data_print = 0;
        std::cout << name << ": ";
        for (int i = 0; i < 5; i++) {
            data_print += data[i];
            std::cout << data_print << "% ";
        }
    }
}

void DiscretizedBuff::masked_add(Mask m, int value) {
    bool mask[5] = {std::get<0>(m), std::get<1>(m), std::get<2>(m),
                    std::get<3>(m), std::get<4>(m)};
    for (int i = 0; i < 5; i++) {
        if (mask[i]) {
            this->data[i] += value;
        }
    }
}

void Skill::print() const {
    this->ability.print("\t");
    Printer p("\n加成");
    p.noValue();
    p.add(abilityBuff.getPrinters(true));
    p.add(flavorBuff.getPrinters());
    p.add(materialBuff.getPrinters());
    p.add("金币", this->pricePercentBuff, true);
    p.add("基础售价", this->baseAddBuff, true);
    p.print("", "  ", "\t");
    if (!(this->abilityBaseBuff == 0)) {
        Printer p("基础技法加成");
        p.noValue();
        p.add(abilityBaseBuff.getPrinters(true));
        p.add(flavorBaseBuff.getPrinters());
        p.add(materialBaseBuff.getPrinters());
        p.print("", "  ", "\t");
    }
    this->rarityBuff.print("菜品火数加成");
    this->gradeBuff.print("菜品品级加成");

    std::cout << std::endl;
}

CEREAL_REGISTER_DYNAMIC_INIT(Types)
// Register polymorphic types with cereal
CEREAL_REGISTER_TYPE(GradeBuffCondition)
CEREAL_REGISTER_TYPE(ThreeSameCookAbilityBuffCondition)