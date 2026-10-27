# Decisions

This file records durable architectural decisions for Libraries.

## ADR-001 — Common owns only product-neutral contracts

**Decision.** A capability enters Common only when its semantics are genuinely reusable across products.

**Rationale.** Centralising domain policy would couple unrelated applications and weaken ownership.

**Consequence.** Filesystem semantics, calendar rules, manufacturer knowledge and product UI policy remain in their owning repositories.

## ADR-002 — Shared code must be at least as strong as the best caller

**Decision.** A consumer is not required to replace a superior local implementation with a weaker generic helper.

**Rationale.** Deduplication is useful only when it preserves or improves technical quality.

**Consequence.** When local generic code is better, Common is improved first; the duplicate is removed only after the shared contract reaches reference quality.

## ADR-003 — C and C++ are both preferred first-party languages

**Decision.** Across the project family, C and C++ are preferred over other language ecosystems for first-party native/shared implementation; Common's current portable ABI remains C.

**Rationale.** The language choice should serve correctness, control, portability and interoperability rather than ideology.

**Consequence.** Common may expose stable C contracts even when a consumer or tooling component uses C++.

## ADR-004 — Published Common APIs are durable contracts

**Decision.** Once an API is admitted and published, loss of a current caller is not sufficient reason to remove it.

**Rationale.** Shared-library consumers need stable contracts independent of the current repository graph.

**Consequence.** Incompatible semantic/layout changes require explicit ABI/version transitions.

## ADR-005 — Exact pinning is the normal consumer model

**Decision.** Consumers use exact reviewed Common releases/commits.

**Rationale.** A later Common change must not silently redefine a previously reviewed application.

**Consequence.** Consumer builds verify the intended Common identity and update deliberately.

## ADR-006 — External dependencies are mechanisms, not semantic owners

**Decision.** Common may use an operating-system API or external library when its documented contract is stronger than reimplementation, but the Common contract remains explicit.

**Rationale.** Avoiding every dependency is not the goal; owning observable semantics is.

**Consequence.** Dependencies sit behind narrow boundaries and are replaceable without changing consumer meaning where practical.

## ADR-007 — Temporal capabilities use executable filesystem resources

**Decision.** Additional calendars and explicit clocks are independently
compiled resources discovered under the two case-sensitive system directories.
The first provider with valid metadata **and executable ABI** in deterministic
filename order owns a duplicate ID. Invalid earlier files cannot mask it.

**Rationale.** Installation must be independent of applications and metadata
alone cannot demonstrate an executable clock or calendar. No compiled optional
capability list is authoritative for available modules.

**Consequence.** Common owns validated loading and catalogue snapshots;
individual resources own historical semantics. The host Gregorian and
OS-following time selectors remain base capabilities. Any future bundle
format requires a justified ABI revision. This replaces the original
provisional discovery migration.

## ADR-008 — Common owns shared system temporal implementations

**Decision.** The complete calendar/clock resource implementation source, tests,
standalone build, Debian packaging and releases belong to Common. Calendar, Files
and System Settings are consumers. They use normal platform capabilities plus
validated modules discovered in the system resource folders. Custom fallback
implementations in applications are prohibited.

**Consequence.** `resources/temporal` builds the optional `.so` capability pack;
these sources are excluded from the consumer library build. Common publishes
its `.deb` through the existing APT distribution. Calendar release versions and
publication no longer control shared system capabilities. This supersedes the
earlier provisional ownership and empty-base-catalogue statements for temporal
resources in ADR-001 and ADR-007.

## ADR-009 — Internal validation may share an open module handle

**Decision.** Installed catalogue construction validates module metadata and its
calendar/clock executable ABI during one directory-scan load. Public generic
runtime scans continue to validate metadata only. Typed direct module lookup
continues to rescan the live directory, rather than trusting a potentially
stale path from the process-lifetime metadata catalogue.

**Rationale.** Re-opening every accepted catalogue provider duplicates dynamic
loader work, but caching module paths changes the established live lookup
semantics. A single private provider-validity contract avoids validator drift.

**Consequence.** The first executable ABI-valid provider still wins in
filesystem order, public APIs and resource/module identities are unchanged, and
no cross-module runtime dependency is introduced. Standalone legacy and
extended font bootstrap scripts remain self-contained for downstream consumers
that package only one of them; merging those scripts without migrating their
installers would break existing bootstrap contracts.
