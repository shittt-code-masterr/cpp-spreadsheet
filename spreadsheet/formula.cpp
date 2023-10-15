#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    output << fe.ToString();
    return output;
}

namespace {
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы:
    explicit Formula(std::string expression) 
        : ast_(ParseFormulaAST(expression)) {
        
    }
    
    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            return ast_.Execute(GetCellTranslator_(sheet));
        } catch (FormulaError& err){
            return err;
        } 
    }
    
    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const {
        const auto list = ast_.GetCells();
        return {list.begin(), list.end()};
    }

private:
    FormulaAST ast_;

    // Формирует лямбда-функцию, которая вовзращает значение ячейки
    // по ее позиции из таблицы sheet
    CellTranclatorLambda GetCellTranslator_(const SheetInterface& sheet) const {
        return [&sheet](const Position* pos){
            if (!pos->IsValid()){
                throw FormulaError(FormulaError::Category::Ref);
            }

            const CellInterface *cell = sheet.GetCell(*pos);
            if (!cell){
                return 0.0;
            }
            const auto cell_value = cell->GetValue();
            if (std::holds_alternative<double>(cell_value)){
                return std::get<double>(cell_value);
            } else if (std::holds_alternative<std::string>(cell_value)){
                try {
                    return std::stod(std::get<std::string>(cell_value));
                } catch (...) {
                    throw FormulaError(FormulaError::Category::Value);
                }
            } else if (std::holds_alternative<FormulaError>(cell_value)) {
                throw std::get<FormulaError>(cell_value);
            }
            return 0.0;
        };
    }
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(expression);
}