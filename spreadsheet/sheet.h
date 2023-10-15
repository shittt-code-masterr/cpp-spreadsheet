#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <iostream>
#include <map>

typedef std::function<void(std::ostream&, CellInterface*)> PrintFunc;

class Sheet : public SheetInterface {
public:
    ~Sheet();
    
    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(const Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    std::map<Position, std::unique_ptr<CellInterface>> sheet_;
    Position top_left_ = Position::NONE;
    Position bottom_right_ = Position::NONE;

    // Проверка на циклическую зависимость.
    // В случае ошибки выбросится исключение
    void CheckCircularDependence_(CellInterface *tmp_cell, Position pos);

    // Обновление зависимостей
    void UpdateDependencies_(CellInterface* tmp_cell, Position pos);

    // Общая функция для печати значений и
    // текстовых вариаций ячейки
    void Print_(std::ostream &output, PrintFunc print_func) const;

    // Проверка позиции на принадлежность таблице.
    // Выбрасывает исключение, если позиция
    // превышает предельные допустимые значения
    bool IsPositionValid_(Position pos) const;

    // Находит правый нижний угол печатаемой области
    Position FindMaxPosition_();
    
    // Находит верхний левый угол печатаемой области
    Position FindMinPosition_();
    
    // Обновляет левый верхний и правый нижний
    // углы печатаемой области текущей таблицы
    void UpdateCorners_(Position pos);
    
};