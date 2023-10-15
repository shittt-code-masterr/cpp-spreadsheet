#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <vector>
#include <set>

using namespace std::literals;

Sheet::~Sheet() = default;

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()){
        throw InvalidPositionException("Invalid position");
    }

    std::unique_ptr<CellInterface> tmp_cell = std::make_unique<Cell>(*this);
    tmp_cell.get()->Set(text);
    CheckCircularDependence_(tmp_cell.get(), pos);
    UpdateDependencies_(tmp_cell.get(), pos);

    if (!sheet_.count(pos)){
        sheet_[pos] = std::move(tmp_cell);
        UpdateCorners_(pos);
    } else {
        sheet_[pos] = std::move(tmp_cell);
    }
}

const CellInterface* Sheet::GetCell(const Position pos) const {
    if (!IsPositionValid_(pos)){
        return nullptr;
    }
    return sheet_.at(pos).get();
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!IsPositionValid_(pos)){
        return nullptr;
    }
    return sheet_.at(pos).get();
}

void Sheet::ClearCell(Position pos) {
    if (!IsPositionValid_(pos)){
        return;
    }
    sheet_.erase(pos);
    
    if (!sheet_.size()){
        top_left_ = bottom_right_ = Position::NONE;
        return;
    }
    
    if (pos.row == bottom_right_.row || pos.col == bottom_right_.col){
        bottom_right_ = FindMaxPosition_();
    } 
    if (pos.row == top_left_.row || pos.col == top_left_.col){
        top_left_ = FindMinPosition_();
    }
}

Size Sheet::GetPrintableSize() const {
    if (top_left_ == Position::NONE){
        return {0, 0};
    }
    return {bottom_right_.row - top_left_.row + 1, bottom_right_.col - top_left_.col + 1};
}

void Sheet::PrintValues(std::ostream& output) const {
    PrintFunc print_func = [](std::ostream& output, CellInterface *ptr){
        const auto& value = ptr->GetValue();
        if (std::holds_alternative<double>(value)){
            output << std::get<double>(value);
        } else if (std::holds_alternative<std::string>(value)){
            output << std::get<std::string>(value);
        } else {
            output << std::get<FormulaError>(value);
        }
    };
    Print_(output, print_func);
}

void Sheet::PrintTexts(std::ostream& output) const {
    PrintFunc print_func = [](std::ostream& output, CellInterface *ptr){
        output << ptr->GetText();
    };
    Print_(output, print_func);
}

void Sheet::UpdateDependencies_(CellInterface* tmp_cell, Position pos){
    for (const auto& ref_pos : tmp_cell->GetReferencedCells()){
        try{
            CellInterface *referenced_cell = GetCell(ref_pos);
            if (!referenced_cell){
                sheet_[ref_pos] = std::move(std::make_unique<Cell>(*this));
                sheet_.at(ref_pos).get()->Set("");
                referenced_cell = sheet_.at(ref_pos).get();
            }
            referenced_cell->AddDependentCell(pos);
        } catch (InvalidPositionException&){

        }
    }
}

void Sheet::CheckCircularDependence_(CellInterface* tmp_cell, Position pos) {
    std::vector<Position> positions_to_check;
    std::set<Position> checked_positions;
    for (size_t i = 0; i < tmp_cell->GetReferencedCells().size(); ++i){
        positions_to_check.push_back(tmp_cell->GetReferencedCells()[i]);
    }
    while (!positions_to_check.empty()){
        Position current_pos = positions_to_check[0];
        if (current_pos == pos){
            throw CircularDependencyException("");
        }
        if (checked_positions.count(current_pos)){
            positions_to_check.erase(positions_to_check.begin());
            continue;
        }
        checked_positions.insert(current_pos);
        CellInterface* cell;
        try {
            cell = GetCell(current_pos);
        } catch (InvalidPositionException&){
            cell = nullptr;
        }
        if (cell){       
            for (size_t i = 0; i < cell->GetReferencedCells().size(); ++i){
                positions_to_check.push_back(cell->GetReferencedCells()[i]);
            }
        }
        positions_to_check.erase(positions_to_check.begin());
    }
}

void Sheet::Print_(std::ostream &output, PrintFunc print_func) const
{
    if (top_left_ == Position::NONE){
        return;
    }
    for (int y = top_left_.row; y <= bottom_right_.row; ++y){
        for (int x = top_left_.col; x <= bottom_right_.col; ++x){
            Position pos;
            pos.row = y;
            pos.col = x;
            if (sheet_.count(pos)){
                print_func(output, sheet_.at(pos).get());
            }
            if (x != bottom_right_.col){
                output << '\t';
            } 
        }
        output << '\n';
    }
}

bool Sheet::IsPositionValid_(Position pos) const {
    if (!pos.IsValid()){
        throw InvalidPositionException("Invalid position");
    }
    if (!sheet_.count(pos)){
        return false;
    }
    return true;
}

Position Sheet::FindMaxPosition_(){
    Position pos;
    pos.col = pos.row = 0;
    
    for (const auto& [position, cell] : sheet_){
        if (position.col > pos.col){
            pos.col = position.col;
        }
        if (position.row > pos.row){
            pos.row = position.row;
        }
    }
    
    return pos;
}

Position Sheet::FindMinPosition_(){
    Position pos;
    pos.col = (*sheet_.begin()).first.col;
    pos.row = (*sheet_.begin()).first.row;
    
    for (const auto& [position, cell] : sheet_){
        if (position.col > pos.col){
            pos.col = position.col;
        }
        if (position.row > pos.row){
            pos.row = position.row;
        }
    }
    
    return pos;
}

void Sheet::UpdateCorners_(Position pos){
    if (top_left_ == Position::NONE){
        top_left_ = bottom_right_ = pos;
        return;
    }

    if (pos.row < top_left_.row){
        top_left_.row = pos.row;
    } else if (pos.row > bottom_right_.row){
        bottom_right_.row = pos.row;
    }

    if (pos.col < top_left_.col){
        top_left_.col = pos.col;
    } else if (pos.col > bottom_right_.col){
        bottom_right_.col = pos.col;
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}