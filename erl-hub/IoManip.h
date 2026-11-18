// IoManip.h  --  UNIT IV: user-defined I/O manipulators for tabular manifest printing
#ifndef IOMANIP_CUSTOM_H
#define IOMANIP_CUSTOM_H

#include <iomanip>
#include <ostream>
#include <string>

namespace ReliefOps::Manip {

// Parameterised manipulator:  os << Col(10) << value;   (left-aligned column of width 10)
class Col {
    int width_;

public:
    explicit Col(int width) : width_(width) {}
    friend std::ostream& operator<<(std::ostream& os, const Col& c) {
        return os << std::left << std::setw(c.width_);
    }
};

// Plain function manipulators:  os << tableDivider;   os << cargoHeader;
inline std::ostream& tableDivider(std::ostream& os) {
    return os << "+" << std::string(20, '-') << "+" << std::string(14, '-') << "+" << std::string(12, '-') << "+"
              << std::string(12, '-') << "+" << std::string(10, '-') << "+\n";
}

inline std::ostream& cargoHeader(std::ostream& os) {
    return os << "| " << Col(18) << "Cargo" << " | " << Col(12) << "Category" << " | " << Col(10) << "Weight(kg)"
              << " | " << Col(10) << "Volume(L)" << " | " << Col(8) << "Priority" << " |\n";
}

}  // namespace ReliefOps::Manip

#endif
