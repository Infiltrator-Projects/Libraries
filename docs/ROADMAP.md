# Roadmap

This roadmap describes direction rather than a promise of dates. The exact released capability is defined by the tagged source and tests.

## Current foundation

- maintain the published portable ABI with exact pinned consumer revisions;
- keep Common's shared primitives at least as strong as the best consumer;
- qualify Linux, Windows and Apple and Common's interfaces to independently
  published provider packages;
- keep implementation source, model tests, translations and releases in the
  31 calendar and 23 clock repositories, with no copies in Common;
- enumerate optional capabilities through validated executable provider ABIs
  installed in the two native system resource directories.

## Remaining work

- consume independently qualified provider releases; historical and astronomical
  regression evidence belongs in the provider repositories described by
  [Historical Fidelity](TEMPORAL_HISTORICAL_FIDELITY.md);
- add adversarial duplicate, malformed and concurrent discovery fixtures
  across supported platforms, retaining deterministic fail-closed behavior;
- admit further provider identities only when their owning repositories define
  justified historical contracts, evidence and distinct stable IDs;
- keep calendar/time domain rules owned by their individual resources and
  resist extra runtime abstraction without demonstrated value.

## Admission rule

New work enters Common only when it has a stable product-neutral contract and centralising it is at least as correct and maintainable as the best existing implementation. Project-specific behaviour stays with its owner.

## Completion rule

A roadmap item is complete only when implementation, automated tests, documentation and consumer integration agree. Publication of a Common release is not used to conceal unqualified behaviour.
