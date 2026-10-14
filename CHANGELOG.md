# Changelog

## 0.1.1 (2026-10-14)

- Fixed: a Foothold Demo Course placed in a level had no section starts in a packaged (cooked) game, because they were
  built in OnConstruction, which cooked games don't run, and weren't saved. They're saved with the actor now, and a
  course saved before this fix rebuilds itself on BeginPlay. New test: `Foothold.Demo.SectionStartsSurviveSaveAndLoad`.
- Fixed: the course's labels made the cook warn `Saving FText ... initialized from FString`; they're culture-invariant
  text now.
- README: results of packaged-build, dedicated-server and network-condition testing, and two UE 5.8 Mover issues to know
  about.

## 0.1 (2026-10-14)

First public release: the movement half of Foothold, under MIT.

- Three Mover movement modes: **Mantle**, **Ledge hang** (shimmy, climb up, drop) and **Ladder** (rung settle,
  mantle off the top), with their global entry transitions: Air mantle, Ledge grab and Ladder start.
- `UFootholdMoverComponent`, Mover's character component with the modes and transitions already added, and
  `FFootholdDefaults::AddToMover` for an existing Mover component.
- `UFootholdComponent`: drop and crouch inputs, thread-safe animation data for your Anim Blueprint (hand and foot
  targets, mantle progress, ledge reach), `OnFootholdModeChanged`, entry montages.
- `AFootholdLadder` and `UFootholdLadderComponent`; ladders replicate.
- `AFootholdCharacter`, a ready-made Mover pawn with Enhanced Input.
- `AFootholdDemoCourse` and `Foothold.SpawnDemo`: mantle blocks, a ledge wall, a ladder and a 13-step mantle tower.
- 47 automation tests. Verified on UE 5.8, Win64: all tests pass in a clean host project, and the plugin builds with
  strict includes.
- Same module (`Foothold`) and class names as Foothold Pro, so a project can move to Pro without losing references.
