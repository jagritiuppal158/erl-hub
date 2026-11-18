// main.cpp  --  ERL-HUB : Post-Earthquake Multi-Agency Emergency Relief Logistics Hub
//   Menu-driven console program + a guided demo that exercises every syllabus unit.
#include <cstdio>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "DispatchQueue.h"
#include "Exceptions.h"
#include "Inventory.h"
#include "IoManip.h"
#include "Logistics.h"
#include "Persistence.h"

namespace Inv = ReliefOps::Inventory;
namespace Lgs = ReliefOps::Logistics;
namespace Per = ReliefOps::Persistence;

namespace {

const char* kDataFile = "erlhub_data.bin";

struct InputClosed {};  // thrown when stdin ends (Ctrl+D / end of piped input)

// ------------------------------- input helpers -------------------------------
std::string ask(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) throw InputClosed{};
    return line;
}

int askInt(const std::string& prompt) {
    while (true) {
        std::string s = ask(prompt);
        try {
            std::size_t pos = 0;
            int v = std::stoi(s, &pos);
            if (pos == s.size()) return v;
        } catch (const std::invalid_argument&) {
        } catch (const std::out_of_range&) {
        }
        std::cout << "  Please enter a whole number.\n";
    }
}

double askDouble(const std::string& prompt) {
    while (true) {
        std::string s = ask(prompt);
        try {
            std::size_t pos = 0;
            double v = std::stod(s, &pos);
            if (pos == s.size()) return v;
        } catch (const std::invalid_argument&) {
        } catch (const std::out_of_range&) {
        }
        std::cout << "  Please enter a number.\n";
    }
}

void banner(const std::string& title) { std::cout << "\n==== " << title << " ====\n"; }

// hours with 2 decimals, e.g. "0.33 h"
std::string hrs(double hours) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << hours << " h";
    return oss.str();
}

// ----------------------------------- world -----------------------------------
struct World {
    std::vector<Inv::DistributionDepot> depots;
    std::vector<std::unique_ptr<Lgs::ReliefVehicle>> fleet;        // owns the vehicles (base pointers)
    std::unordered_map<std::string, Lgs::ReliefVehicle*> byId;     // fast lookup by id
    std::vector<Per::TransitLog> logs;
    Lgs::IncidentCommander commander{"Cmdr_Rao"};

    void addVehicle(std::unique_ptr<Lgs::ReliefVehicle> v) {
        Lgs::ReliefVehicle* raw = v.get();
        byId[raw->getId()] = raw;
        fleet.push_back(std::move(v));
    }
    Lgs::ReliefVehicle* find(const std::string& id) {
        auto it = byId.find(id);
        return it == byId.end() ? nullptr : it->second;
    }
};

void populate(World& w) {
    w.depots.emplace_back("Hub_A");
    w.depots.emplace_back("Hub_B");
    w.depots[0] += Inv::ReliefCargo("Ration_Kits", "Food", 500, 800, 2);
    w.depots[0] += Inv::ReliefCargo("Trauma_Kits", "Medical", 120, 200, 5);
    w.depots[0] += Inv::ReliefCargo("Clean_Water", "Water", 2000, 2000, 3);
    w.depots[0] += Inv::ReliefCargo("Thermal_Blankets", "Shelter", 150, 600, 2);
    w.depots[1] += Inv::ReliefCargo("Ration_Kits", "Food", 300, 480, 2);
    w.depots[1] += Inv::ReliefCargo("Insulin_Vials", "Medical", 40, 60, 4);

    w.addVehicle(std::make_unique<Lgs::AerialTransport>("H-01", "Rescue_Heli", 800, 400, 2.0, 180));
    w.addVehicle(std::make_unique<Lgs::GroundConvoy>("G-01", "Heavy_Truck", 5000, 300, 0.5, 50));
    w.addVehicle(std::make_unique<Lgs::AmphibiousCraft>("A-01", "Hover_Medic", 1200, 250, 1.2, 1.5));
    w.addVehicle(std::make_unique<Lgs::AerialTransport>("Q-01", "Search_Quad", 30, 10, 0.05, 90));
}

// ---------------------------------- pickers ----------------------------------
Inv::DistributionDepot& pickDepot(World& w) {
    for (std::size_t i = 0; i < w.depots.size(); ++i) std::cout << "  " << i + 1 << ") " << w.depots[i].getName() << '\n';
    int n = askInt("Depot number: ");
    if (n < 1 || n > static_cast<int>(w.depots.size())) throw std::out_of_range("No such depot");
    return w.depots[static_cast<std::size_t>(n - 1)];
}

void showFleet(World& w) {
    for (const auto& v : w.fleet) std::cout << "  " << *v << '\n';  // virtual print through the base reference
    std::cout << "  Operational vehicles: " << Lgs::ReliefVehicle::totalOperational()
              << " | Total dispatched: " << Inv::DistributionDepot::totalDispatchedTonnes() << " t\n";
}

Lgs::ReliefVehicle& pickVehicle(World& w) {
    showFleet(w);
    Lgs::ReliefVehicle* v = w.find(ask("Vehicle id (e.g. H-01): "));
    if (v == nullptr) throw std::invalid_argument("No vehicle with that id");
    return *v;
}

Inv::ReliefCargo pickStockItem(const Inv::DistributionDepot& depot) {
    depot.stock().printTable(std::cout);
    for (int i = 0; i < depot.stock().size(); ++i) std::cout << "  " << i + 1 << ") " << depot.stock().at(i).getName() << '\n';
    int n = askInt("Cargo number: ");
    return depot.stock().at(n - 1);  // throws out_of_range if invalid
}

// --------------------------------- menu actions ---------------------------------
void actShowStock(World& w) {
    for (const auto& d : w.depots) {
        std::cout << "\nDepot " << d.getName() << "  (" << d.stock().totalWeightKg() << " kg in stock)\n";
        d.stock().printTable(std::cout);
    }
}

void actRestock(World& w) {
    Inv::DistributionDepot& depot = pickDepot(w);
    std::string name = ask("Cargo name: ");
    std::string cat = ask("Category (Food/Medical/Water/Shelter/...): ");
    double kg = askDouble("Weight (kg): ");
    double litres = askDouble("Volume (litres): ");
    int pr = askInt("Priority (1-5): ");
    depot += Inv::ReliefCargo(name, cat, kg, litres, pr);  // operator +=
    std::cout << "  Restocked " << depot.getName() << ".\n";
}

void actLoadByVolume(World& w) {
    Inv::DistributionDepot& depot = pickDepot(w);
    Inv::ReliefCargo cargo = pickStockItem(depot);
    Lgs::ReliefVehicle& v = pickVehicle(w);
    double litres = askDouble("Volume to load (litres): ");
    double kg = Lgs::allocateSupply(depot, v, cargo, litres);  // overload 1 (volume based)
    std::cout << "  Loaded " << litres << " L = " << kg << " kg of " << cargo.getName() << " on " << v.getId() << ".\n";
}

void actLoadTriage(World& w) {
    Inv::DistributionDepot& depot = pickDepot(w);
    Lgs::ReliefVehicle& v = pickVehicle(w);
    int minPriority = askInt("Minimum medical priority to load (1-5): ");
    Lgs::Mission m{"TRIAGE", 0.0, 0.0, minPriority, "clear"};
    int lots = Lgs::allocateSupply(depot, v, m);  // overload 2 (priority based)
    std::cout << "  Loaded " << lots << " medical lot(s), " << m.payloadKg << " kg now on " << v.getId() << ".\n";
}

void actDispatch(World& w) {
    Lgs::ReliefVehicle& v = pickVehicle(w);
    double km = askDouble("Distance (km): ");
    int c = askInt("Corridor 1) clear  2) damaged  3) flooded : ");
    const char* names[] = {"clear", "damaged", "flooded"};
    if (c < 1 || c > 3) throw std::invalid_argument("Corridor must be 1, 2 or 3");

    Lgs::Mission m{"M" + std::to_string(w.logs.size() + 1), km, 0.0, 2, names[c - 1]};
    double fuelBefore = v.getFuelL();
    double eta = v.dispatch(m);  // may throw: nothing is changed in that case
    w.logs.push_back(Per::TransitLog{static_cast<int>(w.logs.size()) + 1, v.getId(), v.type(), m.corridor,
                                     m.distanceKm, m.payloadKg, fuelBefore - v.getFuelL()});
    std::cout << "  " << v.getId() << " delivered " << m.payloadKg << " kg, ETA " << hrs(eta) << ".\n";
}

void actCompare(World& w) {
    Lgs::ReliefVehicle& a = pickVehicle(w);
    Lgs::ReliefVehicle& b = pickVehicle(w);
    std::cout << "  Manifest of " << a.getId() << " == manifest of " << b.getId() << " ? "
              << (a.manifest() == b.manifest() ? "YES (identical)" : "NO") << '\n';
    for (int i = 0; i < a.manifest().size(); ++i)
        if (b.manifest().contains(a.manifest().at(i)))
            std::cout << "  Duplicate cargo on both vehicles: " << a.manifest().at(i).getName() << '\n';
    std::cout << "  " << a.getId() << " manifest:\n";
    a.manifest().printTable(std::cout);
}

void actSave(World& w) {
    Inv::Manifest audit;  // audit snapshot = everything currently in stock
    for (const auto& d : w.depots)
        for (int i = 0; i < d.stock().size(); ++i) audit += d.stock().at(i);
    Per::saveAtomic(kDataFile, w.logs, audit);
    std::cout << "  Saved " << w.logs.size() << " trip(s) and " << audit.size() << " audit record(s) to " << kDataFile << ".\n";
}

void actLoad(World& w) {
    Inv::Manifest audit;
    Per::load(kDataFile, w.logs, audit);
    std::cout << "  Restored " << w.logs.size() << " trip(s). Audit snapshot from the file:\n";
    audit.printTable(std::cout);
}

void actCommander(World& w) {
    Lgs::ReliefVehicle& v = pickVehicle(w);
    std::cout << "  1) Override payload limit   2) Authorise any corridor   3) Cancel corridor authorisation\n"
                 "  4) Emergency refuel (ignores tank cap)   5) Take out of service   6) Force back into service\n";
    int c = askInt("Action: ");
    switch (c) {
        case 1: w.commander.overridePayloadLimit(v, askDouble("New payload limit (kg): ")); break;
        case 2: w.commander.authoriseCorridor(v, true); break;
        case 3: w.commander.authoriseCorridor(v, false); break;
        case 4: w.commander.emergencyRefuel(v, askDouble("Litres: ")); break;
        case 5: v.setOperational(false); break;
        case 6: w.commander.forceOperational(v); break;
        default: throw std::invalid_argument("Unknown action");
    }
    std::cout << "  " << v << '\n';
}

// ------------------------------- guided demo -------------------------------
void tryIt(const std::string& label, const std::function<void()>& action) {
    try {
        action();
        std::cout << "  " << label << ": OK\n";
    } catch (const std::exception& e) {  // every custom exception derives from std::exception
        std::cout << "  " << label << " -> caught: " << e.what() << '\n';
    }
}

void runGuidedDemo() {
    std::vector<Per::TransitLog> logs;
    Inv::DistributionDepot depot("Demo_Hub");
    depot += Inv::ReliefCargo("Ration_Kits", "Food", 500, 800, 2);
    depot += Inv::ReliefCargo("Trauma_Kits", "Medical", 600, 900, 5);
    depot += Inv::ReliefCargo("Insulin_Vials", "Medical", 40, 60, 4);
    depot += Inv::ReliefCargo("Clean_Water", "Water", 2000, 2000, 3);

    Lgs::GroundConvoy truck("G-D1", "Demo_Truck", 3000, 300, 0.5);
    Lgs::AerialTransport heli("H-D1", "Demo_Heli", 700, 200, 2.0);
    Lgs::AerialTransport quad("Q-D1", "Demo_Quad", 30, 10, 0.05, 90);

    // ============================= UNIT I =============================
    banner("UNIT I : namespaces, overloading, references, inline, friend");
    std::cout << "Namespaces used: ReliefOps::Inventory (cargo, depots) and ReliefOps::Logistics (vehicles, routing)\n";
    std::cout << "Inline utilities : distance (0,0)->(3,4) = " << Lgs::travelDistanceKm(0, 0, 3, 4)
              << " km, fuel for 120 km at 0.5 L/km = " << Lgs::fuelNeededL(120, 0.5) << " L\n";

    Inv::ReliefCargo rationType = depot.stock().at(0);
    double kg = Lgs::allocateSupply(depot, truck, rationType, 400.0);  // overload 1
    std::cout << "Overload 1 (volume): 400 L of Ration_Kits -> " << kg << " kg loaded on the truck\n";

    Lgs::Mission triage{"M-T1", 0.0, 0.0, 4, "clear"};
    int lots = Lgs::allocateSupply(depot, heli, triage);  // overload 2
    std::cout << "Overload 2 (triage): " << lots << " medical lots loaded on the heli, most urgent first:\n";
    heli.manifest().printTable(std::cout);

    Lgs::Mission mission{"M-1", 0.0, 0.0, 2, "clear"};
    Lgs::updateMission(mission, 120.0, truck.loadedKg());  // call by reference
    Lgs::escalate(Lgs::escalate(mission));                 // return by reference -> chaining
    std::cout << "Call/return by reference: mission now " << mission.distanceKm << " km, " << mission.payloadKg
              << " kg, triage priority " << mission.triagePriority << "\n";

    Inv::Manifest scratch;
    scratch += Inv::ReliefCargo("Ration_Kits", "Food", 10, 16, 2);
    Lgs::findLot(scratch, "Ration_Kits").setPriority(5);  // modifies the element inside the manifest
    std::cout << "findLot() returned a reference: priority of Ration_Kits is now "
              << scratch.at(0).getPriority() << "\n";

    Lgs::IncidentCommander cmdr("Cmdr_Rao");
    std::cout << "Friend class: truck payload limit " << truck.getPayloadLimitKg();
    cmdr.overridePayloadLimit(truck, 3300);
    std::cout << " -> " << truck.getPayloadLimitKg() << " kg (changed by IncidentCommander)\n";

    // ============================= UNIT II =============================
    banner("UNIT II : encapsulation, dynamic memory, static, operators");
    Inv::Manifest a;
    a += Inv::ReliefCargo("Clean_Water", "Water", 1000, 1000, 3);
    a += Inv::ReliefCargo("Blankets", "Shelter", 80, 300, 2);
    Inv::Manifest b(a);  // copy constructor = deep copy
    b += Inv::ReliefCargo("Clean_Water", "Water", 500, 500, 3);  // += merges the duplicate lot
    std::cout << "After b += 500 kg water :  a == b ? " << (a == b ? "yes" : "no") << "\n";
    b -= Inv::ReliefCargo("Clean_Water", "Water", 500, 500, 3);  // -= deducts
    std::cout << "After b -= 500 kg water :  a == b ? " << (a == b ? "yes" : "no") << "   (a was never touched)\n";

    std::stringstream ss;
    ss << a;  // operator<< writes the manifest as text
    std::cout << "Serialised manifest text:\n" << ss.str();
    Inv::Manifest restored;
    ss >> restored;  // operator>> reads it back
    std::cout << "Restored via >> equals original ? " << (restored == a ? "yes" : "no") << "\n";

    double fuelBefore = heli.getFuelL();
    Lgs::Mission run{"M-2", 60.0, 0.0, 4, "damaged"};
    double eta = heli.dispatch(run);
    logs.push_back(Per::TransitLog{1, heli.getId(), heli.type(), run.corridor, run.distanceKm, run.payloadKg,
                                   fuelBefore - heli.getFuelL()});
    std::cout << "Heli dispatched: " << run.payloadKg << " kg, ETA " << hrs(eta) << "\n";
    std::cout << "Static data: total dispatched = " << Inv::DistributionDepot::totalDispatchedTonnes()
              << " t, operational vehicles = " << Lgs::ReliefVehicle::totalOperational() << "\n";

    // ============================= UNIT III ============================
    banner("UNIT III : inheritance, virtual base, containership, polymorphism");
    std::vector<std::unique_ptr<Lgs::ReliefVehicle>> fleet;  // base-class pointers
    fleet.push_back(std::make_unique<Lgs::AerialTransport>("H-1", "Rescue_Heli", 800, 400, 2.0));
    fleet.push_back(std::make_unique<Lgs::GroundConvoy>("G-1", "Heavy_Truck", 5000, 300, 0.5));
    fleet.push_back(std::make_unique<Lgs::AmphibiousCraft>("A-1", "Hover_Medic", 1200, 250, 1.2));
    for (const auto& v : fleet) std::cout << "  " << *v << '\n';

    std::cout << "Route evaluation over 80 km through each corridor (one call, different behaviour per type):\n";
    const char* corridors[] = {"clear", "damaged", "flooded"};
    for (const auto& v : fleet) {
        for (const char* c : corridors) {
            Lgs::Mission m{"M-R", 80.0, 0.0, 3, c};
            try {
                double eta = v->evaluateRoute(m);  // virtual call; may throw
                std::cout << "  " << v->getId() << " via " << c << " : ETA " << hrs(eta) << '\n';
            } catch (const std::exception& e) {
                std::cout << "  " << v->getId() << " via " << c << " : " << e.what() << '\n';
            }
        }
    }
    std::cout << "Containership (AmphibiousCraft has-a beacon and has-a life-support unit):\n  "
              << fleet[2]->gps() << "  " << fleet[2]->lifeSupport() << '\n';
    std::cout << "Diamond: AmphibiousCraft inherits ReliefVehicle through GroundConvoy AND AerialTransport,\n"
                 "         but 'virtual' inheritance keeps a single copy, so getId() = " << fleet[2]->getId() << " is unambiguous.\n";

    // ============================= UNIT IV =============================
    banner("UNIT IV : exceptions, binary files, STL, templates, functors, manipulators");
    tryIt("Overweight load on quad", [&] { quad.loadFrom(depot, Inv::ReliefCargo("Ration_Kits", "Food", 100, 160, 2)); });
    tryIt("Truck asks for 900 kg of rations", [&] { truck.loadFrom(depot, Inv::ReliefCargo("Ration_Kits", "Food", 900, 1440, 2)); });
    tryIt("Truck 2000 km mission", [&] {
        Lgs::Mission far{"M-FAR", 2000.0, 0.0, 2, "clear"};
        truck.dispatch(far);
    });
    tryIt("Truck through damaged corridor", [&] {
        Lgs::Mission m{"M-DMG", 50.0, 0.0, 2, "damaged"};
        truck.dispatch(m);
    });

    Lgs::PriorityDispatchQueue<Inv::ReliefCargo> q;  // class template
    q.push(Inv::ReliefCargo("Ration_Kits", "Food", 500, 800, 2));
    q.push(Inv::ReliefCargo("Trauma_Kits", "Medical", 120, 200, 5));
    q.push(Inv::ReliefCargo("Clean_Water", "Water", 2000, 2000, 3));
    q.push(Inv::ReliefCargo("Insulin_Vials", "Medical", 40, 60, 5));
    q.push(Inv::ReliefCargo("Blankets", "Shelter", 150, 600, 2));
    q.sortBy(Lgs::MorePressingNeed());  // functor + std::sort
    std::cout << "Dispatch queue sorted by MorePressingNeed (tabular output uses custom manipulators):\n";
    Inv::printCargoRows(std::cout, q.begin(), q.end());

    std::unordered_map<std::string, double> kgByCategory;  // STL map
    q.forEach([&](const Inv::ReliefCargo& c) { kgByCategory[c.getCategory()] += c.getWeightKg(); });  // std::for_each
    std::vector<std::pair<std::string, double>> rows(kgByCategory.begin(), kgByCategory.end());
    std::sort(rows.begin(), rows.end(), [](const auto& x, const auto& y) { return x.first < y.first; });
    std::cout << "Weight per category (unordered_map -> vector -> sort):\n";
    for (const auto& r : rows) std::cout << "  " << ReliefOps::Manip::Col(10) << r.first << r.second << " kg\n";

    Inv::Manifest audit;
    for (int i = 0; i < depot.stock().size(); ++i) audit += depot.stock().at(i);
    const std::string demoFile = "erlhub_demo.bin";
    tryIt("Atomic binary save (first)", [&] { Per::saveAtomic(demoFile, logs, audit); });
    logs.push_back(Per::TransitLog{2, "G-D1", "Ground", "clear", 40.0, 250.0, 20.0});
    tryIt("Atomic binary save (overwrites safely)", [&] { Per::saveAtomic(demoFile, logs, audit); });
    std::vector<Per::TransitLog> back;
    Inv::Manifest auditBack;
    tryIt("Binary restore", [&] { Per::load(demoFile, back, auditBack); });
    Per::printLogs(std::cout, back);
    tryIt("Restore of a missing file", [&] { Per::load("no_such_file.bin", back, auditBack); });
    std::remove(demoFile.c_str());

    std::cout << "\nVirtual destructors: deleting the fleet through base pointers (watch the order):\n";
    Lgs::ReliefVehicle::setTrace(true);
    fleet.clear();
    Lgs::ReliefVehicle::setTrace(false);
    std::cout << "\nDemo finished.\n";
}

// ---------------------------------- menu ----------------------------------
void printMenu() {
    std::cout << "\n============== ERL-HUB : Emergency Relief Logistics Hub ==============\n"
                 "  1) Show depot stock                 7) Compare two vehicle manifests\n"
                 "  2) Restock a depot (+=)             8) View transit log\n"
                 "  3) Show fleet                       9) Save log + audit (binary, atomic)\n"
                 "  4) Load vehicle by volume          10) Restore log from file\n"
                 "  5) Load vehicle - medical triage   11) Incident Commander overrides\n"
                 "  6) Dispatch a vehicle              12) Run guided demo (all 4 units)\n"
                 "  0) Exit\n";
}

}  // namespace

int main() {
    World world;
    populate(world);
    std::cout << "ERL-HUB ready: " << world.depots.size() << " depots, " << world.fleet.size() << " vehicles.\n";

    try {
        while (true) {
            printMenu();
            int choice = askInt("Choose: ");
            if (choice == 0) break;
            try {
                switch (choice) {
                    case 1: actShowStock(world); break;
                    case 2: actRestock(world); break;
                    case 3: showFleet(world); break;
                    case 4: actLoadByVolume(world); break;
                    case 5: actLoadTriage(world); break;
                    case 6: actDispatch(world); break;
                    case 7: actCompare(world); break;
                    case 8: Per::printLogs(std::cout, world.logs); break;
                    case 9: actSave(world); break;
                    case 10: actLoad(world); break;
                    case 11: actCommander(world); break;
                    case 12: runGuidedDemo(); break;
                    default: std::cout << "  Unknown option.\n";
                }
            } catch (const ReliefOps::ReliefException& e) {  // our own exception hierarchy
                std::cout << "  ! " << e.what() << '\n';
            } catch (const std::exception& e) {  // invalid_argument, out_of_range, ...
                std::cout << "  ! Error: " << e.what() << '\n';
            }
        }
        std::cout << "Goodbye. Stay safe.\n";
    } catch (const InputClosed&) {
        std::cout << "\nInput closed. Goodbye.\n";
    }
    return 0;
}
