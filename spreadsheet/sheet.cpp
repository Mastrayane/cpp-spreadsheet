#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}


void Sheet::ValidatePosition(const Position& pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
}

void Sheet::SetCell(Position pos, std::string text) {
    ValidatePosition(pos);
    const auto& cell = cells_.find(pos);

    if (cell == cells_.end()) {
        cells_.emplace(pos, std::make_unique<Cell>(*this));
    }
    cells_.at(pos)->Set(std::move(text));
}


const CellInterface* Sheet::GetCell(Position pos) const {
    return GetCellPtr(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
    return GetCellPtr(pos);
}

void Sheet::ClearCell(Position pos) {
    ValidatePosition(pos);

    const auto& cell = cells_.find(pos);
    if (cell != cells_.end() && cell->second != nullptr) {
        cell->second->Clear();
        if (!cell->second->IsReferenced()) {
            cell->second.reset();
        }
    }
}

Size Sheet::GetPrintableSize() const {
    Size result{ 0, 0 };

    for (const auto& [pos, cell] : cells_) {
        if (cell != nullptr && !cell->GetText().empty()) {
            result.rows = std::max(result.rows, pos.row + 1);
            result.cols = std::max(result.cols, pos.col + 1);
        }
    }

    return result;
}

void Sheet::PrintValues(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) output << "\t";
            PrintCellValue(output, row, col);
        }
        output << "\n";
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) output << "\t";
            const auto& it = cells_.find({ row, col });
            if (it != cells_.end() && it->second != nullptr) {
                output << it->second->GetText();
            }
        }
        output << "\n";
    }
}

const Cell* Sheet::GetCellPtr(Position pos) const {
    ValidatePosition(pos);

    const auto cell = cells_.find(pos);
    if (cell == cells_.end()) {
        return nullptr;
    }

    return cells_.at(pos).get();
}

Cell* Sheet::GetCellPtr(Position pos) {
    return const_cast<Cell*>(
        static_cast<const Sheet&>(*this).GetCellPtr(pos));
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

void Sheet::PrintCellValue(std::ostream& output, int row, int col) const {
    const auto& it = cells_.find({ row, col });
    if (it != cells_.end() && it->second != nullptr) {
        std::visit([&](const auto& value) {
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>, FormulaError>) {
                output << value.ToString();
            }
            else {
                output << value;
            }
            }, it->second->GetValue());
    }
}