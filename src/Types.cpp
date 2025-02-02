#include "Types.hpp"
#include "Recipe.hpp"
#include <cassert>
#include "include/cereal/archives/portable_binary.hpp"
#include "include/cereal/types/memory.hpp"
#include "include/cereal/types/vector.hpp"
#include "include/cereal/types/map.hpp"
#include "include/cereal/types/string.hpp"
int MaterialCategoryBuff::operator*(const Materials &m) const {
    int sum = 0;
    sum += this->vegetable * m.vegetable;
    sum += this->meat * m.meat;
    sum += this->fish * m.fish;
    sum += this->creation * m.creation;
    return sum;
}
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
    this->pricePerc += s.pricePerc;
    this->priceBasePerc += s.priceBasePerc;
    this->priceBaseAbs += s.priceBaseAbs;
    this->amountAdd.add(s.amountAdd);
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
    if (this->type == PARTIAL || this->type == NEXT || this->type == GLOBAL) {
        assert(this->type == s.type);
        // Only Unset and Self can merge with different types.
    }
}

void PriceBuffData::operator+=(const PriceBuffData &p) {
    this->unconditional += p.unconditional;
    this->abilityBuff.add(p.abilityBuff);
    this->flavorBuff.add(p.flavorBuff);
    this->materialBuff.add(p.materialBuff);
    this->rarityBuff.add(p.rarityBuff);
    this->gradeBuff += p.gradeBuff;
    this->amountBuff += p.amountBuff;
}

PriceBuffData PriceBuffData::operator+(const PriceBuffData &p) const {
    PriceBuffData tmp = *this;
    tmp += p;
    return tmp;
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

void DiscretizedBuff::print(const std::string &name,
                            bool use_percentage_instead_of_plus) const {
    int sum = 0;
    for (int i = 0; i < 5; i++) {
        sum += data[i];
    }
    if (sum > 0) {
        std::cout << name << ": ";
        if (use_percentage_instead_of_plus) {
            for (int i = 0; i < 5; i++) {

                std::cout << data[i] << "% ";
            }
        } else {
            for (int i = 0; i < 5; i++) {

                std::cout << "+" << data[i] << " ";
            }
        }
    }
}
void GradeBuff::gte_add(int threshold, int value) {
    for (int i = threshold - 1; i < 5; i++) {
        data[i] += value;
    }
}
void GradeBuff::operator+=(const GradeBuff &g) {
    for (int i = 0; i < 5; i++) {
        data[i] += g.data[i];
    }
}
void GradeBuff::print(const std::string &word, bool perc) const {
    int sum = data[4];
    if (sum <= 0) {
        return;
    }
    int diff[5];
    diff[0] = data[0];
    for (int i = 1; i < 5; i++) {
        diff[i] = data[i] - data[i - 1];
    }
    if (perc) {
        std::cout << word << ": ";
        for (int i = 0; i < 5; i++) {
            std::cout << diff[i] << "% ";
        }
    } else {
        std::cout << word << ": ";
        for (int i = 0; i < 5; i++) {
            std::cout << "+" << diff[i] << " ";
        }
    }
}
int AbilityBuff::operator*(const CookAbility &a) const {
    int sum = 0;
    const int *ptr = &this->stirfry;
    const int *aptr = &a.stirfry;
    for (int i = 0; i < 6; i++) {
        if (aptr[i] != 0) {
            sum += ptr[i];
        }
    }
    return sum;
}
BuffSummary Skill::getBuffs(const Recipe *r) const {
    BuffSummary s;
    int grade = this->ability / r->cookAbility;
    auto &perc = this->pricePerc;
    auto &basePerc = this->priceBasePerc;
    auto &baseAbs = this->priceBaseAbs;
    s.grade = {perc.gradeBuff[grade], basePerc.gradeBuff[grade],
               baseAbs.gradeBuff[grade]};
    s.flavor = {perc.flavorBuff * r->flavor, basePerc.flavorBuff * r->flavor,
                baseAbs.flavorBuff * r->flavor};
    s.material = {perc.materialBuff * r->materialCategories,
                  basePerc.materialBuff * r->materialCategories,
                  baseAbs.materialBuff * r->materialCategories};
    s.rarity = {perc.rarityBuff[r->rarity], basePerc.rarityBuff[r->rarity],
                baseAbs.rarityBuff[r->rarity]};
    int amount = Recipe::dishNum[r->rarity] + this->amountAdd[r->rarity];
    s.amount = {perc.amountBuff[amount], basePerc.amountBuff[amount],
                baseAbs.amountBuff[amount]};
    s.ability = {perc.abilityBuff * r->cookAbility,
                 basePerc.abilityBuff * r->cookAbility,
                 baseAbs.abilityBuff * r->cookAbility};
    s.unconditional = {perc.unconditional, basePerc.unconditional,
                       baseAbs.unconditional};
    return s;
}
void PriceBuffData::print(const char *title, bool perc) const {
    Printer p(title);
    p.noValue();
    p.add("无条件加成", this->unconditional, perc);
    p.add(this->abilityBuff.getPrinters(true));
    p.add(this->flavorBuff.getPrinters());
    p.add(this->materialBuff.getPrinters());
    this->rarityBuff.print("菜品火数加成", perc);
    this->gradeBuff.print("菜品品级加成", perc);
    this->amountBuff.print("菜品数量加成", perc);
}
void Skill::print(bool printNum) const {
    this->ability.print("\t");
    if (printNum) {
        this->amountAdd.print("菜品数量增加", false);
    }
    this->pricePerc.print("\n百分比加成", true);
    this->priceBasePerc.print("\n基础售价百分比加成", true);
    this->priceBaseAbs.print("\n基础售价绝对值加成", false);
    // Printer p("\n基础加成");
    // p.noValue();
    // p.add(abilityBuff.getPrinters(true));
    // p.add(flavorBuff.getPrinters());
    // p.add(materialBuff.getPrinters());
    // p.add("金币", this->pricePercentBuff, true);
    // p.add("基础售价", this->baseAddBuff, true);
    // p.print("", "  ", "\t");
    // if (!(this->abilityBaseBuff == 0)) {
    //     Printer p("基础技法加成");
    //     p.noValue();
    //     p.add(abilityBaseBuff.getPrinters(true));
    //     p.add(flavorBaseBuff.getPrinters());
    //     p.add(materialBaseBuff.getPrinters());
    //     p.print("", "  ", "\t");
    // }
    // this->rarityBuff.print("菜品火数加成");
    // this->gradeBuff.print("菜品品级加成");
    // this->amountBuff.print("菜品数量加成");
    // this->amountBaseBuff.print("菜品数量基础加成");

    std::cout << std::endl;
}

BuffSum BuffSum::operator+(const BuffSum &b) const {
    BuffSum tmp;
    tmp.perc = this->perc + b.perc;
    tmp.basePerc = this->basePerc + b.basePerc;
    tmp.baseAbs = this->baseAbs + b.baseAbs;
    return tmp;
}

BuffSum BuffSummary::sum() const {
    return flavor + material + rarity + grade + amount + ability +
           unconditional;
}

bool Tags::intersectsWith(const Tags &t) const {
    for (auto i : t) {
        if (this->find(i) != this->end())
            return true;
    }
    return false;
}

bool RarityBuff::print() const {
    if (data[0] || data[1] || data[2] || data[3] || data[4]) {
        std::cout << "稀有度加成: ";
        for (int i = 0; i < 5; i++) {
            std::cout << data[i] << "% ";
        }
        return true;
    } else {
        return false;
    }
}

void AmountBuff::gte_add(int threshold, int value) {
    int sum = 0;
    for (int i = threshold - 1; i < 64; i++) {
        sum += buff[i];
    }
}

void AmountBuff::lte_add(int threshold, int value) {
    int sum = 0;
    for (int i = 0; i < threshold - 1; i++) {
        sum += buff[i];
    }
}

void AmountBuff::operator+=(const AmountBuff &a) {
    for (int i = 0; i < 64; i++) {
        buff[i] += a.buff[i];
    }
}

void AmountBuff::print(const std::string &word, bool perc) const {
    bool allZero = true;
    bool allSame = true;
    int firstValue = buff[0];

    for (int i = 0; i < 64; ++i) {
        if (buff[i] != 0) {
            allZero = false;
        }
        if (buff[i] != firstValue) {
            allSame = false;
        }
    }

    if (allZero) {
        return;
    }
    std::cout << word << ": ";
    if (allSame) {
        std::cout << (perc ? std::to_string(firstValue) + "%"
                           : "+" + std::to_string(firstValue))
                  << std::endl;
        return;
    }

    int start = 0;
    while (start < 64) {
        int value = buff[start];
        int end = start;
        while (end < 64 && buff[end] == value) {
            ++end;
        }

        if (start == 0) {
            std::cout << (perc ? std::to_string(value) + "%"
                               : "+" + std::to_string(value))
                      << " (<= " << end << ")";
        } else if (end == 64) {
            std::cout << ", "
                      << (perc ? std::to_string(value) + "%"
                               : "+" + std::to_string(value))
                      << " (>= " << (start + 1) << ")";
        } else {
            std::cout << ", "
                      << (perc ? std::to_string(value) + "%"
                               : "+" + std::to_string(value))
                      << " (" << (start + 1) << "~" << end << ")";
        }

        start = end;
    }

    std::cout << std::endl;
}
CEREAL_REGISTER_DYNAMIC_INIT(Types)
// Register polymorphic types with cereal
CEREAL_REGISTER_TYPE(GradeBuffCondition)
CEREAL_REGISTER_TYPE(ThreeSameCookAbilityBuffCondition)