#pragma once

#include "common.h"
#include "formula.h"

#include <set>
#include <iostream>
#include <optional>

class Cell : public CellInterface {
public:
    Cell(SheetInterface& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;

    std::vector<Position> GetDependentCells() const override;
    void AddDependentCell(Position pos) override;

private:
    class Impl {
    public:
        virtual ~Impl() = default;
        virtual void Set(std::string text) = 0;
        virtual Value GetValue(const SheetInterface& sheet) const = 0;
        virtual std::string GetText() const = 0;
        virtual std::vector<Position> GetReferencedCells() const = 0;
    };
    
    class EmptyImpl : public Impl {
    public:    
        void Set(std::string text) override {
            (void)text;
        }
        
        Value GetValue([[maybe_unused]] const SheetInterface& sheet) const override {
            return 0.0;
        }
        
        std::string GetText() const override {
            return "";
        }

        std::vector<Position> GetReferencedCells() const {
            return {};
        }
        
    };
    
    class TextImpl : public Impl {
    public:
        void Set(std::string text) override {
            text_ = text;
        }
        
        Value GetValue([[maybe_unused]] const SheetInterface& sheet) const override {
            if (text_.size() && text_.at(0) == '\''){
                return text_.substr(1, text_.size() - 1);
            }
            return text_;
        }
        
        std::string GetText() const override {
            return text_;
        }

        std::vector<Position> GetReferencedCells() const {
            return {};
        }
        
    private:
        std::string text_;
        
    };
    
    class FormulaImpl : public Impl {
    public:
        void Set(std::string text) override {
            try {
                formula_ = ParseFormula(text);
            } catch (...) {
               throw FormulaException("Parsing Error");
            }
        }
        
        Value GetValue(const SheetInterface& sheet) const override {
            auto res = formula_.get()->Evaluate(sheet);
            if (std::holds_alternative<double>(res)){
                return std::get<double>(res);
            }
            return std::get<FormulaError>(res);
        }
        
        std::string GetText() const override {
            return "=" + formula_.get()->GetExpression();
        }
        
        std::vector<Position> GetReferencedCells() const {
            return formula_.get()->GetReferencedCells();
        }
        
    private:
        std::unique_ptr<FormulaInterface> formula_;
        
    };
    
    std::unique_ptr<Impl> impl_;
    SheetInterface& sheet_;
    mutable std::optional<Value> cached_value_;
    // Ячейки, которые зависят от текущей ячейка
    std::vector<Position> dependent_cells_;
    // Ячейки, от которых зависит текущая ячейка
    std::vector<Position> referenced_cells_;

    // Очищает кэши в текущей ячейке
    // и во всех зависимых
    void ClearCache_() override;

};