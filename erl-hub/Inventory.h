// Inventory.h  --  namespace ReliefOps::Inventory
//   UNIT I  : nested namespace
//   UNIT II : ReliefCargo, Manifest (dynamic memory), DistributionDepot
//             encapsulation, static members, copy constructor, destructor,
//             operator overloading  +=  -=  ==  <<  >>
#ifndef INVENTORY_H
#define INVENTORY_H

#include <iomanip>
#include <iostream>
#include <string>

#include "IoManip.h"

namespace ReliefOps::Inventory {

// ---------------------------------------------------------------------------
// ReliefCargo : one lot of relief supplies (all data private -> information hiding)
// ---------------------------------------------------------------------------
class ReliefCargo {
    std::string name_;      // use underscores instead of spaces, e.g. Ration_Kits
    std::string category_;  // Food, Medical, Water, Shelter ...
    double weightKg_;
    double volumeL_;
    int priority_;  // 1 (low) ... 5 (critical)

public:
    ReliefCargo();
    ReliefCargo(const std::string& name, const std::string& category, double weightKg, double volumeL, int priority);

    // getters
    const std::string& getName() const { return name_; }
    const std::string& getCategory() const { return category_; }
    double getWeightKg() const { return weightKg_; }
    double getVolumeL() const { return volumeL_; }
    int getPriority() const { return priority_; }

    // setters with validation
    void setName(const std::string& name);
    void setCategory(const std::string& category);
    void setWeightKg(double kg);
    void setVolumeL(double litres);
    void setPriority(int priority);

    // == : "duplicate cargo" check -> same name AND same category
    bool operator==(const ReliefCargo& other) const;
    bool operator!=(const ReliefCargo& other) const { return !(*this == other); }

    // binary serialisation (Unit IV)
    void writeBinary(std::ostream& os) const;
    void readBinary(std::istream& is);

    // text serialisation:  name category weightKg volumeL priority
    friend std::ostream& operator<<(std::ostream& os, const ReliefCargo& c);
    friend std::istream& operator>>(std::istream& is, ReliefCargo& c);
};

// ---------------------------------------------------------------------------
// Manifest : dynamically allocated list of cargo (rule of three)
// ---------------------------------------------------------------------------
class Manifest {
    ReliefCargo* items_;  // dynamic array
    int count_;
    int capacity_;

    void grow();
    int indexOf(const ReliefCargo& c) const;  // -1 if not found

public:
    Manifest();                       // default capacity 4
    explicit Manifest(int capacity);  // dynamic constructor
    Manifest(const Manifest& other);  // copy constructor  (deep copy)
    Manifest& operator=(const Manifest& other);
    ~Manifest();                      // destructor frees the array

    int size() const { return count_; }
    double totalWeightKg() const;
    double totalVolumeL() const;
    bool contains(const ReliefCargo& c) const { return indexOf(c) >= 0; }  // duplicate check
    double weightOf(const ReliefCargo& c) const;
    void clear() { count_ = 0; }

    const ReliefCargo& at(int index) const;
    ReliefCargo& operator[](int index);  // return by reference

    Manifest& operator+=(const ReliefCargo& c);  // restock (merges duplicates)
    Manifest& operator-=(const ReliefCargo& c);  // deduct weight (throws if not enough)
    bool operator==(const Manifest& other) const;  // manifest identity
    bool operator!=(const Manifest& other) const { return !(*this == other); }

    void printTable(std::ostream& os) const;

    // serialisation:  first the count, then one cargo per line
    friend std::ostream& operator<<(std::ostream& os, const Manifest& m);
    friend std::istream& operator>>(std::istream& is, Manifest& m);
};

// ---------------------------------------------------------------------------
// DistributionDepot : holds stock; static variable tracks total dispatched tonnage
// ---------------------------------------------------------------------------
class DistributionDepot {
    std::string name_;
    Manifest stock_;
    static double totalDispatchedKg_;  // shared by every depot

public:
    explicit DistributionDepot(const std::string& name);

    const std::string& getName() const { return name_; }
    const Manifest& stock() const { return stock_; }

    DistributionDepot& operator+=(const ReliefCargo& c);  // restock at the depot
    DistributionDepot& operator-=(const ReliefCargo& c);  // deduct on vehicle departure
    bool canSupply(const ReliefCargo& c) const;
    double availableKg(const ReliefCargo& c) const { return stock_.weightOf(c); }
    void replaceStock(const Manifest& m) { stock_ = m; }

    static void recordDispatch(double kg);   // static member functions
    static double totalDispatchedTonnes();
};

// ---------------------------------------------------------------------------
// Prints any range of cargo as a table using the user-defined manipulators.
// (a function template, so it works for the Manifest array and for a std::vector)
// ---------------------------------------------------------------------------
template <typename Iter>
void printCargoRows(std::ostream& os, Iter first, Iter last) {
    std::ios_base::fmtflags oldFlags = os.flags();
    std::streamsize oldPrecision = os.precision();
    os << Manip::tableDivider << Manip::cargoHeader << Manip::tableDivider;
    os << std::fixed << std::setprecision(1);
    for (Iter it = first; it != last; ++it) {
        // long text is shortened with "~" so the table columns always stay aligned
        std::string name = it->getName();
        if (name.size() > 18) name = name.substr(0, 17) + "~";
        std::string category = it->getCategory();
        if (category.size() > 12) category = category.substr(0, 11) + "~";
        os << "| " << Manip::Col(18) << name << " | " << Manip::Col(12) << category << " | "
           << Manip::Col(10) << it->getWeightKg() << " | " << Manip::Col(10) << it->getVolumeL() << " | "
           << Manip::Col(8) << it->getPriority() << " |\n";
    }
    os << Manip::tableDivider;
    os.flags(oldFlags);
    os.precision(oldPrecision);
}

}  // namespace ReliefOps::Inventory

#endif
