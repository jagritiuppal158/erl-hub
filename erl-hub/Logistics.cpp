// Logistics.cpp
#include "Logistics.h"

#include <algorithm>
#include <iomanip>
#include <stdexcept>

#include "DispatchQueue.h"

namespace ReliefOps::Logistics {

// definitions of the static data members
int ReliefVehicle::operationalCount_ = 0;
bool ReliefVehicle::trace_ = false;

// ============================ ReliefVehicle ================================
ReliefVehicle::ReliefVehicle(const std::string& id, const std::string& name, double payloadLimitKg, double fuelL,
                             double fuelPerKm)
    : id_(id),
      name_(name),
      payloadLimitKg_(payloadLimitKg),
      tankCapacityL_(fuelL),
      fuelL_(fuelL),
      fuelPerKm_(fuelPerKm),
      operational_(true),
      corridorOverride_(false) {
    if (id.empty() || payloadLimitKg <= 0.0 || fuelL < 0.0 || fuelPerKm <= 0.0)
        throw std::invalid_argument("Invalid vehicle parameters");
    ++operationalCount_;
}

ReliefVehicle::~ReliefVehicle() {
    if (operational_) --operationalCount_;
    logLifecycle("  ~ReliefVehicle     " + id_);
}

void ReliefVehicle::logLifecycle(const std::string& msg) {
    if (trace_) std::cout << msg << '\n';
}

void ReliefVehicle::setOperational(bool on) {
    if (on != operational_) {
        operational_ = on;
        operationalCount_ += on ? 1 : -1;
    }
}

void ReliefVehicle::refuel(double litres) {
    if (litres < 0.0) throw std::invalid_argument("Fuel amount cannot be negative");
    fuelL_ = std::min(tankCapacityL_, fuelL_ + litres);
}

void ReliefVehicle::loadFrom(Inventory::DistributionDepot& depot, const Inventory::ReliefCargo& cargo) {
    double newTotal = cargo_.totalWeightKg() + cargo.getWeightKg();
    if (newTotal > payloadLimitKg_ + 1e-9) throw CargoOverweightException(id_, newTotal, payloadLimitKg_);

    depot -= cargo;  // may throw InsufficientStockException -> then nothing has changed yet
    cargo_ += cargo;
}

double ReliefVehicle::evaluateRoute(const Mission& mission) const {
    if (!corridorOverride_ && !canUseCorridor(mission.corridor))  // virtual call
        throw DamagedCorridorException(id_, mission.corridor);

    double needed = fuelNeededL(mission.distanceKm, fuelPerKm_);
    if (needed > fuelL_ + 1e-9) throw FuelDepletionException(id_, needed, fuelL_);

    return mission.distanceKm / speedKmh();  // virtual call -> ETA in hours
}

double ReliefVehicle::dispatch(Mission& mission) {
    if (!operational_) throw ReliefException("Vehicle " + id_ + " is out of service");
    if (cargo_.size() == 0) throw ReliefException("Vehicle " + id_ + " has no cargo loaded");

    double eta = evaluateRoute(mission);  // throws before anything is changed

    double deliveredKg = cargo_.totalWeightKg();
    fuelL_ -= fuelNeededL(mission.distanceKm, fuelPerKm_);
    mission.payloadKg = deliveredKg;  // call by reference: the caller's mission is updated
    Inventory::DistributionDepot::recordDispatch(deliveredKg);
    cargo_.clear();
    return eta;
}

void ReliefVehicle::print(std::ostream& os) const {
    std::ios_base::fmtflags oldFlags = os.flags();
    std::streamsize oldPrecision = os.precision();
    os << std::fixed << std::setprecision(1) << type() << ' ' << id_ << " (" << name_ << ") load "
       << cargo_.totalWeightKg() << '/' << payloadLimitKg_ << " kg, fuel " << fuelL_ << " L, "
       << (operational_ ? "READY" : "OUT-OF-SERVICE");
    os.flags(oldFlags);
    os.precision(oldPrecision);
}

// =========================== IncidentCommander =============================
// Because of "friend class IncidentCommander", the code below can use private members directly.
void IncidentCommander::overridePayloadLimit(ReliefVehicle& v, double newLimitKg) const {
    if (newLimitKg <= 0.0) throw std::invalid_argument("Payload limit must be positive");
    v.payloadLimitKg_ = newLimitKg;
}

void IncidentCommander::authoriseCorridor(ReliefVehicle& v, bool allowed) const { v.corridorOverride_ = allowed; }

void IncidentCommander::forceOperational(ReliefVehicle& v) const {
    if (!v.operational_) {
        v.operational_ = true;
        ++ReliefVehicle::operationalCount_;
    }
}

void IncidentCommander::emergencyRefuel(ReliefVehicle& v, double litres) const {
    if (litres < 0.0) throw std::invalid_argument("Fuel amount cannot be negative");
    v.fuelL_ += litres;  // jerry cans: may go above the tank capacity
}

// ========================== free functions (Unit I) ========================
double allocateSupply(Inventory::DistributionDepot& depot, ReliefVehicle& vehicle,
                      const Inventory::ReliefCargo& cargo, double volumeL) {
    if (volumeL <= 0.0 || cargo.getVolumeL() <= 0.0) throw std::invalid_argument("Volume must be positive");

    double density = cargo.getWeightKg() / cargo.getVolumeL();  // kg per litre of this cargo
    double kg = volumeL * density;
    Inventory::ReliefCargo lot(cargo.getName(), cargo.getCategory(), kg, volumeL, cargo.getPriority());
    vehicle.loadFrom(depot, lot);
    return kg;
}

int allocateSupply(Inventory::DistributionDepot& depot, ReliefVehicle& vehicle, Mission& mission) {
    // 1. collect urgent medical stock into a PriorityDispatchQueue
    PriorityDispatchQueue<Inventory::ReliefCargo> queue;
    const Inventory::Manifest& stock = depot.stock();
    for (int i = 0; i < stock.size(); ++i) {
        const Inventory::ReliefCargo& c = stock.at(i);
        if (c.getCategory() == "Medical" && c.getPriority() >= mission.triagePriority) queue.push(c);
    }
    queue.sortBy(MorePressingNeed());  // 2. most urgent first (functor)

    // 3. load as much as fits, in that order
    int lots = 0;
    while (!queue.empty()) {
        Inventory::ReliefCargo item = queue.pop();
        double room = vehicle.getPayloadLimitKg() - vehicle.loadedKg();
        double take = std::min(item.getWeightKg(), room);
        if (take <= 0.001) break;
        double volume = item.getVolumeL() * (take / item.getWeightKg());
        vehicle.loadFrom(depot, Inventory::ReliefCargo(item.getName(), item.getCategory(), take, volume,
                                                       item.getPriority()));
        ++lots;
    }
    mission.payloadKg = vehicle.loadedKg();  // call by reference
    return lots;
}

void updateMission(Mission& mission, double distanceKm, double payloadKg) {
    mission.distanceKm = distanceKm;
    mission.payloadKg = payloadKg;
}

Mission& escalate(Mission& mission) {
    if (mission.triagePriority < 5) ++mission.triagePriority;
    return mission;
}

Inventory::ReliefCargo& findLot(Inventory::Manifest& manifest, const std::string& name) {
    for (int i = 0; i < manifest.size(); ++i)
        if (manifest[i].getName() == name) return manifest[i];
    throw std::out_of_range("No cargo named " + name + " in this manifest");
}

}  // namespace ReliefOps::Logistics
