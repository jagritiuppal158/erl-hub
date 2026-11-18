// Logistics.h  --  namespace ReliefOps::Logistics
//   UNIT I   : overloaded functions, call/return by reference, inline functions, friend class
//   UNIT II  : static members (operational vehicle count), encapsulation, dynamic Manifest member
//   UNIT III : inheritance, virtual base class (diamond), containership, runtime polymorphism
#ifndef LOGISTICS_H
#define LOGISTICS_H

#include <cmath>
#include <iostream>
#include <string>

#include "Exceptions.h"
#include "Inventory.h"

namespace ReliefOps::Logistics {

// ---------------------------------------------------------------------------
// Mission parameters (plain data that functions update through references)
// ---------------------------------------------------------------------------
struct Mission {
    std::string id;
    double distanceKm = 0.0;
    double payloadKg = 0.0;
    int triagePriority = 1;          // 1 (routine) ... 5 (critical)
    std::string corridor = "clear";  // clear | damaged | flooded
};

// ---------------------------------------------------------------------------
// UNIT I : inline utility functions (the compiler pastes the body at the call site)
// ---------------------------------------------------------------------------
inline double travelDistanceKm(double x1, double y1, double x2, double y2) {
    return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}
inline double fuelNeededL(double distanceKm, double litresPerKm) { return distanceKm * litresPerKm; }
inline double payloadUtilisationPct(double loadKg, double limitKg) {
    return limitKg > 0.0 ? 100.0 * loadKg / limitKg : 0.0;
}

// ---------------------------------------------------------------------------
// UNIT III : two small independent classes used by CONTAINERSHIP (has-a relationship)
// ---------------------------------------------------------------------------
class GPSBeacon {
    double lat_;
    double lon_;
    bool active_;

public:
    explicit GPSBeacon(double lat = 28.6139, double lon = 77.2090) : lat_(lat), lon_(lon), active_(true) {}
    void moveTo(double lat, double lon) {
        lat_ = lat;
        lon_ = lon;
    }
    void setActive(bool on) { active_ = on; }
    bool isActive() const { return active_; }
    double lat() const { return lat_; }
    double lon() const { return lon_; }
};

inline std::ostream& operator<<(std::ostream& os, const GPSBeacon& g) {
    return os << "GPS[" << (g.isActive() ? "ON" : "OFF") << " " << g.lat() << ", " << g.lon() << "]";
}

class EmergencyLifeSupportUnit {
    int oxygenCylinders_;
    int medKits_;

public:
    EmergencyLifeSupportUnit(int oxygen = 2, int medKits = 4) : oxygenCylinders_(oxygen), medKits_(medKits) {}
    int oxygen() const { return oxygenCylinders_; }
    int medKits() const { return medKits_; }
    bool useOxygen() {
        if (oxygenCylinders_ <= 0) return false;
        --oxygenCylinders_;
        return true;
    }
};

inline std::ostream& operator<<(std::ostream& os, const EmergencyLifeSupportUnit& u) {
    return os << "LifeSupport[oxygen " << u.oxygen() << ", med-kits " << u.medKits() << "]";
}

class IncidentCommander;  // forward declaration (needed for the friend declaration below)

// ---------------------------------------------------------------------------
// ReliefVehicle : the abstract base class (this is the "TransportVehicle" ADT of Unit II)
// ---------------------------------------------------------------------------
class ReliefVehicle {
    friend class IncidentCommander;  // UNIT I: the commander may touch private data in emergencies

    // ----- private data (information hiding) -----
    std::string id_;
    std::string name_;
    double payloadLimitKg_;
    double tankCapacityL_;
    double fuelL_;
    double fuelPerKm_;
    bool operational_;
    bool corridorOverride_;  // set only by the IncidentCommander
    Inventory::Manifest cargo_;  // dynamic memory is managed inside Manifest
    GPSBeacon gps_;                      // CONTAINERSHIP: vehicle HAS-A beacon
    EmergencyLifeSupportUnit support_;   // CONTAINERSHIP: vehicle HAS-A life-support unit

    // ----- UNIT II: static data shared by all vehicles -----
    static int operationalCount_;
    static bool trace_;  // prints constructor/destructor messages in the demo

public:
    ReliefVehicle(const std::string& id, const std::string& name, double payloadLimitKg, double fuelL,
                  double fuelPerKm);
    virtual ~ReliefVehicle();  // VIRTUAL destructor -> deleting through a base pointer is safe

    ReliefVehicle(const ReliefVehicle&) = delete;  // polymorphic objects are not copied
    ReliefVehicle& operator=(const ReliefVehicle&) = delete;

    // getters
    const std::string& getId() const { return id_; }
    const std::string& getName() const { return name_; }
    double getPayloadLimitKg() const { return payloadLimitKg_; }
    double getFuelL() const { return fuelL_; }
    double getFuelPerKm() const { return fuelPerKm_; }
    bool isOperational() const { return operational_; }
    double loadedKg() const { return cargo_.totalWeightKg(); }
    const Inventory::Manifest& manifest() const { return cargo_; }
    GPSBeacon& gps() { return gps_; }
    EmergencyLifeSupportUnit& lifeSupport() { return support_; }

    // operations
    void setOperational(bool on);
    void refuel(double litres);  // capped at tank capacity
    void loadFrom(Inventory::DistributionDepot& depot, const Inventory::ReliefCargo& cargo);
    double dispatch(Mission& mission);  // returns ETA in hours; updates mission.payloadKg

    // runtime polymorphism: every vehicle type answers these differently
    virtual std::string type() const = 0;
    virtual double speedKmh() const = 0;
    virtual bool canUseCorridor(const std::string& corridor) const = 0;
    virtual double evaluateRoute(const Mission& mission) const;  // common checks, throws on failure
    virtual void print(std::ostream& os) const;

    // static member functions
    static int totalOperational() { return operationalCount_; }
    static void setTrace(bool on) { trace_ = on; }
    static void logLifecycle(const std::string& msg);
};

inline std::ostream& operator<<(std::ostream& os, const ReliefVehicle& v) {
    v.print(os);
    return os;
}

// ---------------------------------------------------------------------------
// Derived classes.  The two intermediate classes inherit the base VIRTUALLY, so a
// vehicle derived from both (AmphibiousCraft) contains only ONE ReliefVehicle.
// ---------------------------------------------------------------------------
class AerialTransport : virtual public ReliefVehicle {
    double cruiseKmh_;

public:
    AerialTransport(const std::string& id, const std::string& name, double payloadKg, double fuelL,
                    double fuelPerKm, double cruiseKmh = 180.0)
        : ReliefVehicle(id, name, payloadKg, fuelL, fuelPerKm), cruiseKmh_(cruiseKmh) {}
    ~AerialTransport() override { logLifecycle("  ~AerialTransport   " + getId()); }

    std::string type() const override { return "Aerial"; }
    double speedKmh() const override { return cruiseKmh_; }
    bool canUseCorridor(const std::string&) const override { return true; }  // flies over everything
    void print(std::ostream& os) const override {
        ReliefVehicle::print(os);
        os << " | cruise " << cruiseKmh_ << " km/h";
    }
};

class GroundConvoy : virtual public ReliefVehicle {
    double roadKmh_;

public:
    GroundConvoy(const std::string& id, const std::string& name, double payloadKg, double fuelL, double fuelPerKm,
                 double roadKmh = 50.0)
        : ReliefVehicle(id, name, payloadKg, fuelL, fuelPerKm), roadKmh_(roadKmh) {}
    ~GroundConvoy() override { logLifecycle("  ~GroundConvoy      " + getId()); }

    std::string type() const override { return "Ground"; }
    double speedKmh() const override { return roadKmh_; }
    bool canUseCorridor(const std::string& corridor) const override { return corridor == "clear"; }
    void print(std::ostream& os) const override {
        ReliefVehicle::print(os);
        os << " | road " << roadKmh_ << " km/h";
    }
};

// Air-cushion (hovercraft style) craft: moves over land AND water, so it inherits from both.
class AmphibiousCraft : public GroundConvoy, public AerialTransport {
    double wadingDepthM_;

public:
    AmphibiousCraft(const std::string& id, const std::string& name, double payloadKg, double fuelL,
                    double fuelPerKm, double wadingDepthM = 1.5)
        // the MOST DERIVED class must construct the virtual base itself
        : ReliefVehicle(id, name, payloadKg, fuelL, fuelPerKm),
          GroundConvoy(id, name, payloadKg, fuelL, fuelPerKm, 60.0),
          AerialTransport(id, name, payloadKg, fuelL, fuelPerKm, 60.0),
          wadingDepthM_(wadingDepthM) {}
    ~AmphibiousCraft() override { logLifecycle("  ~AmphibiousCraft   " + getId()); }

    // Both parents override these, so the diamond has two final overriders -> we must pick one.
    std::string type() const override { return "Amphibious"; }
    double speedKmh() const override { return 60.0; }
    bool canUseCorridor(const std::string& corridor) const override {
        return corridor == "clear" || corridor == "flooded";  // cannot cross collapsed (damaged) roads
    }
    void print(std::ostream& os) const override {
        ReliefVehicle::print(os);
        os << " | amphibious, wading depth " << wadingDepthM_ << " m";
    }
};

// ---------------------------------------------------------------------------
// UNIT I : friend class - may read/write private members of ReliefVehicle
// ---------------------------------------------------------------------------
class IncidentCommander {
    std::string name_;

public:
    explicit IncidentCommander(const std::string& name) : name_(name) {}
    const std::string& getName() const { return name_; }

    void overridePayloadLimit(ReliefVehicle& v, double newLimitKg) const;  // allow a critical overload
    void authoriseCorridor(ReliefVehicle& v, bool allowed) const;          // allow risky passage
    void forceOperational(ReliefVehicle& v) const;                         // return a vehicle to service
    void emergencyRefuel(ReliefVehicle& v, double litres) const;           // ignores the tank cap
};

// ---------------------------------------------------------------------------
// UNIT I : free functions in the Logistics namespace
// ---------------------------------------------------------------------------

// FUNCTION OVERLOADING: same name, different parameter lists.
// 1) volume-based payload: "load 400 litres of this cargo"
double allocateSupply(Inventory::DistributionDepot& depot, ReliefVehicle& vehicle,
                      const Inventory::ReliefCargo& cargo, double volumeL);
// 2) priority-based emergency medical triage: loads the most urgent medical stock first
int allocateSupply(Inventory::DistributionDepot& depot, ReliefVehicle& vehicle, Mission& mission);

// CALL BY REFERENCE: the Mission passed in is changed, not copied
void updateMission(Mission& mission, double distanceKm, double payloadKg);
// RETURN BY REFERENCE: returns the SAME mission, so calls can be chained / used on the left of =
Mission& escalate(Mission& mission);
// RETURN BY REFERENCE: gives direct access to a lot inside a manifest
Inventory::ReliefCargo& findLot(Inventory::Manifest& manifest, const std::string& name);

}  // namespace ReliefOps::Logistics

#endif
