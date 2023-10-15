#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <variant>


Cell::Cell(SheetInterface& sheet)
    : sheet_(sheet) {
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    if (!text.size()){
        impl_ = std::make_unique<EmptyImpl>();
        return;
    }
    if (text.size() > 1 && text.at(0) == '='){
        std::unique_ptr<FormulaImpl> tmp_ptr = std::make_unique<FormulaImpl>();
        tmp_ptr.get()->Set(text.substr(1, text.size() - 1));
        impl_ = std::move(tmp_ptr);
    } else {
        impl_= std::make_unique<TextImpl>();
        impl_.get()->Set(text);
    }
    referenced_cells_ = impl_.get()->GetReferencedCells();
    ClearCache_();
    cached_value_ = impl_.get()->GetValue(sheet_);
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
    if (cached_value_ == std::nullopt){
        cached_value_ = impl_.get()->GetValue(sheet_);
    }
    return cached_value_.value();
}

std::string Cell::GetText() const {
    return impl_.get()->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return referenced_cells_;
}

std::vector<Position> Cell::GetDependentCells() const {
    return dependent_cells_;
}

void Cell::AddDependentCell(Position pos){
    if (pos.IsValid()){
        dependent_cells_.push_back(pos);
    }
}

void Cell::ClearCache_(){
    cached_value_ = std::nullopt;

    for (const auto& ref_pos : dependent_cells_){
        sheet_.GetCell(ref_pos)->ClearCache_();
    }
}