#include "Calculator.hpp"
#include "Chef.hpp"
#include "utils/Printer.hpp"
#include "utils/math.hpp"
std::string gradeName(int i) {
    switch (i) {
    case 1:
        return "可级";
    case 2:
        return "优级";
    case 3:
        return "特级";
    case 5:
        return "传级";
    case 4:
        return "神级";
    default:
        return "未知，是BUG，菜谱等级为" + std::to_string(i);
    }
}

BanquetInfo getPrice(const Skill &skill, Recipe *recipe, BanquetRuleTogether &r,
                     bool verbose) {

    int grade = skill.ability / recipe->cookAbility;
    int gradebuff = 0;
    switch (grade) {
    case 0: {
        if (verbose)
            std::cout << "Grade 0" << std::endl;
        {
            BanquetInfo b = {0, 500};
            // Need this when exacting chef tool.
            return b;
        }
    }
    case 1:
        gradebuff = 0;
        break;
    case 2:
        gradebuff = 10;
        break;
    case 3:
        gradebuff = 30;
        break;
    case 4:
        gradebuff = 50;
        break;
    default:
        gradebuff = 100;
    }
    int amount =
        Recipe::dishNum[recipe->rarity] + skill.amountAdd[recipe->rarity];
    const BanquetRule &rule = r.merge();
    int intentionAddBuff = rule.addRule.buff;
    int intentionBaseBuff = rule.baseRule.buff;
    int intentionBaseAdd = rule.baseRule.directAdd;
    BuffSummary skillBuff = skill.getBuffs(recipe);
    auto buffSum = skillBuff.sum();
    int perc = gradebuff + intentionAddBuff + buffSum.perc;
    int singlePrice =
        int_ceil((recipe->price + intentionBaseAdd + buffSum.baseAbs) *
                 (1.0 + (intentionBaseBuff + buffSum.basePerc) / 100.0) *
                 (1.0 + (perc) / 100.0));
    int totalPrice = singlePrice * amount;
    int full;
    if (rule.addRule.fullAdd) {
        full = recipe->rarity + rule.addRule.full;
    } else {
        full = rule.addRule.full;
    }
    BanquetInfo b = {totalPrice, full};
    if (verbose) {
        Printer skillPercentPrinter("售价", true);
        skillPercentPrinter.add("味道", skillBuff.flavor.perc);
        skillPercentPrinter.add("技法", skillBuff.ability.perc);
        skillPercentPrinter.add("食材", skillBuff.material.perc);
        skillPercentPrinter.add("火数", skillBuff.rarity.perc);
        skillPercentPrinter.add("数量", skillBuff.amount.perc);
        skillPercentPrinter.add(
            "金币", (Chef::coinBuffOn ? skillBuff.unconditional.perc : 0));

        Printer skillBasePrinter("基础", true);
        skillBasePrinter.noValue();
        skillBasePrinter.add("", buffSum.basePerc);
        skillBasePrinter.add("", buffSum.baseAbs, false);

        Printer skillPrinter("技能");
        skillPrinter.noValue();
        skillPrinter.add(skillBasePrinter);
        skillPrinter.add(skillPercentPrinter);

        Printer intentionPrinter("意图");
        intentionPrinter.noValue();
        intentionPrinter.add("基础售价", rule.baseRule.directAdd, false);
        intentionPrinter.add("基础售价", intentionBaseBuff, true);
        intentionPrinter.add("售价", intentionAddBuff, true);

        std::cout << "╭─> ";
        recipe->print(amount, "│ ");
        std::cout << "" << gradeName(grade) << " +" << gradebuff << "%"
                  << std::endl;
        skillPrinter.print("│ ", " + ", "\n");
        intentionPrinter.print("│ ", "; ", "\n");

        std::cout << "│ 售价总计百分比Buff: " << int2signed_str(perc) << "%"
                  << std::endl;
        std::cout << "╰─> 饱腹度: " << full << "\t总价: " << totalPrice
                  << std::endl;
    }
    return b;
}
const BanquetRule &BanquetRuleTogether::merge() {
    this->lenientRule.execOneMore();
    this->lenientRule.addRule.add(this->strictRule.addRule);
    this->lenientRule.baseRule.add(this->strictRule.baseRule);
    return this->lenientRule;
}