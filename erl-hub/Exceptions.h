// Exceptions.h  --  UNIT IV: custom exception framework (derived from std::exception)
#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <exception>
#include <iomanip>
#include <sstream>
#include <string>

namespace ReliefOps {

// small helper: number -> text with one decimal place
inline std::string fmt(double value) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << value;
    return oss.str();
}

// Base of the whole framework. Derives from std::exception and overrides what().
class ReliefException : public std::exception {
protected:
    std::string message_;

public:
    explicit ReliefException(const std::string& msg) : message_(msg) {}
    const char* what() const noexcept override { return message_.c_str(); }
};

class CargoOverweightException : public ReliefException {
public:
    CargoOverweightException(const std::string& vehicleId, double attemptedKg, double limitKg)
        : ReliefException("[CargoOverweight] Vehicle " + vehicleId + " cannot carry " + fmt(attemptedKg) +
                          " kg (payload limit " + fmt(limitKg) + " kg)") {}
};

class FuelDepletionException : public ReliefException {
public:
    FuelDepletionException(const std::string& vehicleId, double neededL, double availableL)
        : ReliefException("[FuelDepletion] Vehicle " + vehicleId + " needs " + fmt(neededL) +
                          " L of fuel but only has " + fmt(availableL) + " L") {}
};

class DamagedCorridorException : public ReliefException {
public:
    DamagedCorridorException(const std::string& vehicleId, const std::string& corridor)
        : ReliefException("[DamagedCorridor] Vehicle " + vehicleId + " cannot use a " + corridor + " corridor") {}
};

class InsufficientStockException : public ReliefException {
public:
    InsufficientStockException(const std::string& item, double requestedKg, double availableKg)
        : ReliefException("[InsufficientStock] " + item + ": requested " + fmt(requestedKg) +
                          " kg but only " + fmt(availableKg) + " kg available") {}
};

class PersistenceException : public ReliefException {
public:
    explicit PersistenceException(const std::string& msg) : ReliefException("[Persistence] " + msg) {}
};

}  // namespace ReliefOps

#endif
