# Context/state audit closure checklist

This is the current closure checklist for the broad context/state task in
[the implementation list](aspect-ratio-and-background-overrides-tasks.md).
It supersedes historical “remaining coverage” paragraphs in the audit log;
those paragraphs describe the evidence available at the time they were written.
Completed routes are not reopened merely to enumerate every possible key,
health value, unlock mask or quest flag. A new variant belongs in this gate
when it exercises an unverified installation, source lifetime, foreground layout
or functional/error return path. Exhaustive products remain acceptance work.

There are **29 unchecked implementation-list items** as of this revision:

| Section | Unchecked items |
| --- | ---: |
| Catalog/baseline | 3 |
| Geometry/settings | 0 |
| World rendering | 4 |
| Layout | 5 |
| Margin composition | 7 |
| Artwork loading | 5 |
| Export synchronization | 1 |
| Acceptance/budgets | 4 |
| **Total** | **29** |

The first item contains these seven coverage groups: **five completed, two
remaining** (C7 currently lacks ROM fixtures). They are a breakdown of
one parent item, not seven additional implementation-list items or an estimate
of seven turns. Each group closes with checked native lifecycle/source evidence
and documented fixture limits. The list is deliberately explicit so the audit
cannot expand silently after every completed route.

- [x] **C1 — Fusion interaction lifecycle.** Native B/START/L cancellation,
  mismatched-piece recovery, successful consumption/ownership update, direct
  return and world-event/local-map-hint handoff at three installed decorations.
  The seven native partner-name layouts already have retained installation
  evidence; interaction cases need not multiply every layout by every input.
  Completed by the [18-case native interaction audit](background-context-state-audit.md#native-fusion-cancellation-mismatch-and-success-handoffs-2026-10-26).
- [x] **C2 — File operations that commit changes.** Name/create, copy and erase
  success; refusal/cancel retention; write failure and return to file selection.
  Existing view/options/language/cancellation captures remain valid. Completed
  by the [native file-commit audit](background-context-state-audit.md#file-commit-and-post-credits-save-interactions-2026-10-26).
  The observed failed-erase UI/storage discrepancy was subsequently fixed and
  verified under C5.
- [x] **C3 — Post-credits Save.** Acceptance, refusal and device-write failure /
  retry or refusal, with native title return and generated-black lifetime.
  Existing full credits/page/clear/prompt installation evidence remains valid.
  Completed by the [four-case ending audit](background-context-state-audit.md#file-commit-and-post-credits-save-interactions-2026-10-26).
- [x] **C4 — Map/navigation source boundaries.** Native world-map cursor entry
  into detail maps and return; marker/conditional map paths and local fusion-hint
  variants that install distinct content. Use the existing 22-recipe/scroll,
  six-hint and travel matrices to avoid duplicating proven source identities.
  Completed by the [93-case navigation audit](background-context-state-audit.md#native-map-navigation-and-remaining-error-paths-2026-10-26).
- [x] **C5 — Remaining functional error/return paths.** Save-and-Quit and alternate
  error-dismissal branches not exercised by the initial Game Over matrix;
  production host-save flush failure propagation to the native UI, including
  the failed-erase success-reporting/cache discrepancy observed by C2. Existing
  EEPROM failure and storage persistence tests are separate evidence. Completed
  with checked durable erasure, device/host error recovery and extended Game Over
  returns in the [C4/C5 completion](background-context-state-audit.md#native-map-navigation-and-remaining-error-paths-2026-10-26).
- [ ] **C6 — Remaining EU localization coverage.** Extend pause/fusion, dungeon,
  detail-map, travel, sleep, save, Game Over and ending/file interaction routes
  where translated layout/text is present to French/German/Spanish/Italian.
  Figurine collection/prizes and standalone hints already cover all five EU
  languages and must not be reopened for this group.
- [ ] **C7 — Additional regional fixtures.** USA/JP source/layout and applicable
  interaction checks when verified ROMs are available. The workspace currently
  supplies only an EU ROM; retain this external coverage limitation explicitly.
  Unavailable fixtures are not evidence of regional acceptance.

Effect target resolution, top-left title-frame completion, compositor/PNG work,
profile enablement, packaging, GPU acceptance and physical RG measurements have
their own unchecked parent tasks. They do not generate further context-audit
subtasks by implication. The first parent task stays open until its applicable
coverage groups are resolved; deferred unavailable-region work must be named
explicitly instead of silently treated as passed.
