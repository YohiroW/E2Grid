# E2Grid

E2Grid is a data-driven Unreal Engine grid plugin for discrete tactical space. The runtime owns spatial facts only: static Cell topology, Placement, Occupancy, pathfinding, movement-budget reachability, topology range, and versioned move commits. Team, AP, combat, interaction, AI, UI, and other game rules belong to the consuming project.

## Modules

- `E2Grid`: production runtime API and movement adapter.
- `E2GridEd`: editor mode, collision-based Grid Build, visualization, and debug information.
- `E2GridGame`: playable example and integration-test module only. Final games should depend on `E2Grid`, not inherit gameplay architecture from `E2GridGame`.

`UE2GridMapAsset` is the static Grid authority. `AE2GridManager` places one asset in the world and supplies its transform. `UE2GridSubsystem` owns the single Active Grid's dynamic Unit registration, Occupancy, and monotonically increasing Runtime Revision.

## Runtime queries

New code should consume the structured result APIs:

- `QueryPlacement` reports an explicit `EE2GridQueryStatus` without changing state.
- `FindPath` returns start, goal, cost, ordered steps, and the Runtime Revision used by the query.
- `FindReachableCells` performs one deterministic Dijkstra expansion for a movement budget; results are ordered by cost then Cell Key.
- `BuildPathFromReachableResult` reconstructs a shortest path without rerunning A*.
- `FindCellsInRange` provides gameplay-agnostic `StepCount` or `TraversalCost` topology range.
- `CommitUnitMoveFromPath` rejects stale previews and returns a structured commit result.
- `OnStateChanged` publishes Manager, Unit, and Occupancy deltas with the new Runtime Revision.

The legacy bool Placement and step-array Commit functions remain compatibility wrappers.

## MVP movement protocol

`UE2GridMovementComponent` retains the source Cell's Occupancy during visual movement and atomically commits the destination at the end. It validates the versioned path before movement and again at commit; a Runtime Revision change safely fails and restores the actor to the occupied source Cell. Consuming gameplay must serialize authoritative movement actions while one movement is resolving. Concurrent movement, mid-path topology changes, opportunity reactions, and large footprints require the future reservation/step-transaction design.

## Validation

Install or link the plugin under a host project's `Plugins/E2Grid/` directory, then compile the host Editor or package the plugin with Unreal Automation Tool. Automation tests use the `E2Grid.*` prefix and cover Runtime, Editor Build, and the example integration module.
