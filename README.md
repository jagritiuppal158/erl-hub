# ERL-HUB — Post-Earthquake Multi-Agency Emergency Relief Logistics Hub

Object-Oriented Programming (CIC-211) project, written in **C++17**.
Relief coordinators manage depots of supplies (ration kits, trauma medicine, water, blankets) and a fleet of
heterogeneous vehicles (helicopters, trucks, amphibious craft), and dispatch them over clear, damaged or flooded corridors.

## Build and run

**Linux / macOS / MinGW with make**
```
make
./erlhub
```

**Any platform with g++ (no make needed)**
```
g++ -std=c++17 -Wall -Wextra -o erlhub main.cpp Inventory.cpp Logistics.cpp Persistence.cpp
./erlhub            (Windows: erlhub.exe)
```

**Code::Blocks / Visual Studio / VS Code:** create a project, add the four `.cpp` files and the `.h` files, and set the
language standard to C++17.

Start with **option 12** in the menu: it runs a guided demo that shows every syllabus requirement, unit by unit.

## Files

| File | Contents |
|---|---|
| `Exceptions.h` | Exception framework (derived from `std::exception`) |
| `BinaryIO.h` | Helpers for binary `fstream` read and write |
| `IoManip.h` | User-defined I/O manipulators (`Col(n)`, `tableDivider`, `cargoHeader`) |
| `Inventory.h/.cpp` | Namespace `ReliefOps::Inventory`: `ReliefCargo`, `Manifest`, `DistributionDepot` |
| `Logistics.h/.cpp` | Namespace `ReliefOps::Logistics`: vehicles, `IncidentCommander`, overloaded and reference functions |
| `DispatchQueue.h` | Class template `PriorityDispatchQueue<T>` and the functor `MorePressingNeed` |
| `Persistence.h/.cpp` | Atomic binary save, all-or-nothing restore, log table printing |
| `main.cpp` | Menu program and guided demo |

## Where each syllabus requirement lives

### Unit I — Core foundations and call semantics
| Requirement | Where |
|---|---|
| Logical namespaces `ReliefOps::Logistics` and `ReliefOps::Inventory` | All code is wrapped in them; `main.cpp` uses the aliases `Inv::` and `Lgs::` |
| Function overloading (volume-based vs. priority-based) | `allocateSupply(depot, vehicle, cargo, volumeL)` and `allocateSupply(depot, vehicle, mission)` in `Logistics.h/.cpp` |
| Call by reference | `updateMission(Mission&, ...)`, and the depot and vehicle passed by reference into `allocateSupply` |
| Return by reference | `escalate(Mission&)` returns `Mission&`; `findLot(...)` returns `ReliefCargo&`; `Manifest::operator[]` |
| Inline utility methods | `travelDistanceKm`, `fuelNeededL`, `payloadUtilisationPct` in `Logistics.h` |
| Friend class overriding access barriers | `IncidentCommander` is a friend of `ReliefVehicle` and changes private payload limit, fuel, corridor authorisation and status |

### Unit II — Encapsulation, dynamic memory and operator overloading
| Requirement | Where |
|---|---|
| `ReliefCargo`, `DistributionDepot`, `TransportVehicle` as abstract data types | Private data plus getters and validating setters. `ReliefVehicle` is the TransportVehicle class |
| Static variables and functions | `DistributionDepot::totalDispatchedKg_` with `recordDispatch()` and `totalDispatchedTonnes()`; `ReliefVehicle::operationalCount_` with `totalOperational()` |
| Copy constructor, dynamic constructor, destructor | `Manifest(const Manifest&)` is a deep copy, `Manifest(int capacity)` allocates, `~Manifest()` runs `delete[]`; `operator=` completes the rule of three |
| `+=` restock | `Manifest::operator+=` and `DistributionDepot::operator+=` (merges duplicate lots) |
| `-=` deduct cargo | `Manifest::operator-=` and `DistributionDepot::operator-=` (throws if stock is short) |
| `==` verify manifest identity / duplicate cargo | `Manifest::operator==` (same lots and weights, any order); `ReliefCargo::operator==` (same name and category) |
| `<<` and `>>` | Text serialisation of `ReliefCargo` and `Manifest`; `ReliefVehicle` has `<<` for printing |

### Unit III — Inheritance, containership and polymorphism
| Requirement | Where |
|---|---|
| `ReliefVehicle` base with `AerialTransport`, `GroundConvoy`, `AmphibiousCraft` | `Logistics.h` |
| Virtual base class to solve the diamond | `AerialTransport` and `GroundConvoy` use `virtual public ReliefVehicle`; `AmphibiousCraft` derives from both and builds the shared base itself |
| Containership with `GPSBeacon` and `EmergencyLifeSupportUnit` | They are **members** of `ReliefVehicle`, so every vehicle HAS-A beacon and life-support unit |
| Runtime polymorphism through base pointers | `std::vector<std::unique_ptr<ReliefVehicle>>`; `evaluateRoute()`, `speedKmh()`, `canUseCorridor()`, `print()` are virtual |
| Virtual destructor | `virtual ~ReliefVehicle()`; the demo prints the destructor chain when the fleet is deleted |

### Unit IV — Streams, persistence, exception safety and STL
| Requirement | Where |
|---|---|
| Exception framework from `std::exception` | `ReliefException` and `CargoOverweightException`, `FuelDepletionException`, `DamagedCorridorException`, `InsufficientStockException`, `PersistenceException` |
| Binary `fstream` save and restore | `Persistence.cpp`, with `BinaryIO.h` helpers (`write` and `read`) |
| Atomic file updates | `saveAtomic` writes `file.tmp` and then renames it over the real file |
| `PriorityDispatchQueue<T>` template | `DispatchQueue.h` (uses `std::vector`, `std::sort`, `std::for_each`) |
| `std::unordered_map` | `World::byId` fleet lookup in `main.cpp`; category totals in the demo |
| Functor for sorting urgent medical needs | `MorePressingNeed` |
| User-defined I/O manipulators for tabular output | `IoManip.h`; used by `printCargoRows` and `printLogs` |

## Exception safety
Every risky operation checks first and changes state last, so a failed operation leaves everything as it was:
- `loadFrom` checks payload, then deducts from the depot (which may throw), then adds to the vehicle.
- `dispatch` evaluates the route (which may throw) before burning fuel or clearing the cargo.
- `Manifest::operator=` allocates the new array before deleting the old one.
- `Persistence::load` fills temporaries and commits only when the whole file was valid.

## Push to GitHub
```
git init
git add .
git commit -m "ERL-HUB: OOP project (CIC-211)"
git branch -M main
git remote add origin https://github.com/<your-username>/erl-hub.git
git push -u origin main
```
