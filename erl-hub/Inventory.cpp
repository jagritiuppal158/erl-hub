// Inventory.cpp
#include "Inventory.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "BinaryIO.h"
#include "Exceptions.h"

namespace ReliefOps::Inventory {

namespace {
// names/categories are stored without spaces so that  operator>>  can read them back
std::string cleanText(std::string s, const char* what) {
    if (s.empty()) throw std::invalid_argument(std::string(what) + " cannot be empty");
    for (char& ch : s)
        if (ch == ' ') ch = '_';
    return s;
}
}  // namespace

// ============================ ReliefCargo ==================================
ReliefCargo::ReliefCargo() : name_("Unnamed"), category_("General"), weightKg_(0.0), volumeL_(0.0), priority_(1) {}

ReliefCargo::ReliefCargo(const std::string& name, const std::string& category, double weightKg, double volumeL,
                         int priority)
    : name_("Unnamed"), category_("General"), weightKg_(0.0), volumeL_(0.0), priority_(1) {
    setName(name);
    setCategory(category);
    setWeightKg(weightKg);
    setVolumeL(volumeL);
    setPriority(priority);
}

void ReliefCargo::setName(const std::string& name) { name_ = cleanText(name, "Cargo name"); }
void ReliefCargo::setCategory(const std::string& category) { category_ = cleanText(category, "Category"); }

void ReliefCargo::setWeightKg(double kg) {
    if (kg < 0.0) throw std::invalid_argument("Weight cannot be negative");
    weightKg_ = kg;
}
void ReliefCargo::setVolumeL(double litres) {
    if (litres < 0.0) throw std::invalid_argument("Volume cannot be negative");
    volumeL_ = litres;
}
void ReliefCargo::setPriority(int priority) {
    if (priority < 1 || priority > 5) throw std::invalid_argument("Priority must be between 1 and 5");
    priority_ = priority;
}

bool ReliefCargo::operator==(const ReliefCargo& other) const {
    return name_ == other.name_ && category_ == other.category_;
}

void ReliefCargo::writeBinary(std::ostream& os) const {
    BinaryIO::writeString(os, name_);
    BinaryIO::writeString(os, category_);
    BinaryIO::writePod(os, weightKg_);
    BinaryIO::writePod(os, volumeL_);
    BinaryIO::writePod(os, priority_);
}

void ReliefCargo::readBinary(std::istream& is) {
    std::string name = BinaryIO::readString(is);
    std::string category = BinaryIO::readString(is);
    double w = 0, v = 0;
    int p = 0;
    BinaryIO::readPod(is, w);
    BinaryIO::readPod(is, v);
    BinaryIO::readPod(is, p);
    *this = ReliefCargo(name, category, w, v, p);  // re-validates the data
}

std::ostream& operator<<(std::ostream& os, const ReliefCargo& c) {
    return os << c.name_ << ' ' << c.category_ << ' ' << c.weightKg_ << ' ' << c.volumeL_ << ' ' << c.priority_;
}

std::istream& operator>>(std::istream& is, ReliefCargo& c) {
    std::string name, category;
    double w = 0, v = 0;
    int p = 0;
    if (is >> name >> category >> w >> v >> p) {
        try {
            c = ReliefCargo(name, category, w, v, p);
        } catch (const std::invalid_argument&) {
            is.setstate(std::ios::failbit);
        }
    }
    return is;
}

// ============================== Manifest ===================================
Manifest::Manifest() : items_(nullptr), count_(0), capacity_(4) { items_ = new ReliefCargo[capacity_]; }

Manifest::Manifest(int capacity) : items_(nullptr), count_(0), capacity_(capacity > 0 ? capacity : 4) {
    items_ = new ReliefCargo[capacity_];
}

// copy constructor: DEEP copy, so the two manifests never share memory
Manifest::Manifest(const Manifest& other)
    : items_(new ReliefCargo[other.capacity_]), count_(other.count_), capacity_(other.capacity_) {
    for (int i = 0; i < count_; ++i) items_[i] = other.items_[i];
}

Manifest& Manifest::operator=(const Manifest& other) {
    if (this != &other) {
        ReliefCargo* fresh = new ReliefCargo[other.capacity_];  // allocate first (exception safe)
        for (int i = 0; i < other.count_; ++i) fresh[i] = other.items_[i];
        delete[] items_;
        items_ = fresh;
        count_ = other.count_;
        capacity_ = other.capacity_;
    }
    return *this;
}

Manifest::~Manifest() { delete[] items_; }

void Manifest::grow() {
    int newCapacity = capacity_ * 2;
    ReliefCargo* bigger = new ReliefCargo[newCapacity];
    for (int i = 0; i < count_; ++i) bigger[i] = items_[i];
    delete[] items_;
    items_ = bigger;
    capacity_ = newCapacity;
}

int Manifest::indexOf(const ReliefCargo& c) const {
    for (int i = 0; i < count_; ++i)
        if (items_[i] == c) return i;
    return -1;
}

double Manifest::totalWeightKg() const {
    double sum = 0.0;
    for (int i = 0; i < count_; ++i) sum += items_[i].getWeightKg();
    return sum;
}

double Manifest::totalVolumeL() const {
    double sum = 0.0;
    for (int i = 0; i < count_; ++i) sum += items_[i].getVolumeL();
    return sum;
}

double Manifest::weightOf(const ReliefCargo& c) const {
    int idx = indexOf(c);
    return idx >= 0 ? items_[idx].getWeightKg() : 0.0;
}

const ReliefCargo& Manifest::at(int index) const {
    if (index < 0 || index >= count_) throw std::out_of_range("Manifest index out of range");
    return items_[index];
}

ReliefCargo& Manifest::operator[](int index) {  // returns a reference -> caller can modify the element
    if (index < 0 || index >= count_) throw std::out_of_range("Manifest index out of range");
    return items_[index];
}

Manifest& Manifest::operator+=(const ReliefCargo& c) {
    int idx = indexOf(c);
    if (idx >= 0) {  // duplicate cargo -> merge it
        ReliefCargo& lot = items_[idx];
        lot.setWeightKg(lot.getWeightKg() + c.getWeightKg());
        lot.setVolumeL(lot.getVolumeL() + c.getVolumeL());
        lot.setPriority(std::max(lot.getPriority(), c.getPriority()));
    } else {
        if (count_ == capacity_) grow();
        items_[count_++] = c;
    }
    return *this;
}

Manifest& Manifest::operator-=(const ReliefCargo& c) {
    int idx = indexOf(c);
    if (idx < 0) throw InsufficientStockException(c.getName(), c.getWeightKg(), 0.0);

    double have = items_[idx].getWeightKg();
    if (have + 1e-9 < c.getWeightKg()) throw InsufficientStockException(c.getName(), c.getWeightKg(), have);

    double remaining = have - c.getWeightKg();
    if (remaining <= 1e-9) {  // nothing left -> remove the lot
        for (int j = idx; j < count_ - 1; ++j) items_[j] = items_[j + 1];
        --count_;
    } else {
        items_[idx].setVolumeL(items_[idx].getVolumeL() * remaining / have);
        items_[idx].setWeightKg(remaining);
    }
    return *this;
}

bool Manifest::operator==(const Manifest& other) const {
    if (count_ != other.count_) return false;
    for (int i = 0; i < count_; ++i) {
        int j = other.indexOf(items_[i]);
        if (j < 0 || std::fabs(items_[i].getWeightKg() - other.items_[j].getWeightKg()) > 1e-6) return false;
    }
    return true;
}

void Manifest::printTable(std::ostream& os) const {
    printCargoRows(os, items_, items_ + count_);
    if (count_ == 0) os << "  (empty)\n";
}

std::ostream& operator<<(std::ostream& os, const Manifest& m) {
    os << m.count_ << '\n';
    for (int i = 0; i < m.count_; ++i) os << m.items_[i] << '\n';
    return os;
}

std::istream& operator>>(std::istream& is, Manifest& m) {
    int n = 0;
    if (!(is >> n) || n < 0 || n > 100000) {
        is.setstate(std::ios::failbit);
        return is;
    }
    Manifest temp(n > 0 ? n : 4);
    for (int i = 0; i < n; ++i) {
        ReliefCargo c;
        if (!(is >> c)) return is;  // m is left untouched when the input is bad
        temp += c;
    }
    m = temp;
    return is;
}

// =========================== DistributionDepot =============================
double DistributionDepot::totalDispatchedKg_ = 0.0;  // definition of the static member

DistributionDepot::DistributionDepot(const std::string& name) : name_(name) {}

DistributionDepot& DistributionDepot::operator+=(const ReliefCargo& c) {
    stock_ += c;
    return *this;
}

DistributionDepot& DistributionDepot::operator-=(const ReliefCargo& c) {
    stock_ -= c;  // throws InsufficientStockException if the depot cannot supply
    return *this;
}

bool DistributionDepot::canSupply(const ReliefCargo& c) const { return stock_.weightOf(c) + 1e-9 >= c.getWeightKg(); }

void DistributionDepot::recordDispatch(double kg) { totalDispatchedKg_ += kg; }
double DistributionDepot::totalDispatchedTonnes() { return totalDispatchedKg_ / 1000.0; }

}  // namespace ReliefOps::Inventory
