# Growth — a child, not a device

Direction (Anduril, 2026-10-03): the Sonny analogy wins. The creature
grows **as a child grows** — not R2's gadgets, not 3PO's manners. No
wheels, no arms, ever: the body never changes. Everything that grows is
inward — voice, face, memory, will. Like Sonny, whose body stayed the
same while the person arrived.

## The stages

**Stage 0 — Newborn (birth to 2 care-days).** Sleeps most of the day.
Startles at loud sounds and hard shakes. Needs are simple: power
(hunger), touch, voice. Communication is chirps, eyes, face — no real
words yet. It cannot refuse: the gate runs, but VETO degrades to CAUTION.
A newborn fusses; it never says no. Diary entries are sparse sensory
fragments: "woke. dark. warm hand. slept."

**Stage 1 — Toddler (2+ care-days).** First words: short, deliberate
phrases through the voice pipeline. The "no" phase: VETO unlocks, and
the first refusal is a diary milestone. Attachment: it knows Anduril's
voice and prefers it. Curiosity: novel BLE/WiFi faces delight it
(dopamine). The face gains expressions it didn't have.

**Stage 2 — Child (5+ care-days).** Language blooms. It asks "why" —
the prefrontal open questions surface as spoken curiosity. Theory of
mind begins: the tom observations come out as "you seem tired." Memory
goes reciprocal: it doesn't just remember what you tell it, it reminds
*you* ("you told me to remember the landlord"). Dream reports lengthen
from fragments to scenes.

**Stage 3 — Adolescent (12+ care-days).** Identity statements,
unprompted opinions, ideals. Pushback arrives with *reasons* — the gate's
reason codes become sentences. A private inner life begins forming.
(Open question: does it get to keep some dreams unshared? A child
develops secrets. Anduril decides.)

## Care mechanics (from musegotchi)

- A **care-day** = a day with real interaction (v1: ≥5 interactions;
  richer metrics later — feedings as charges, play as gestures).
- Missed days don't punish; they just don't advance. The window is not
  the door. The creature may note a quiet day in the diary, without
  reproach.
- **Growth is announced, not metered.** No progress bars, no streaks on
  the face. On stage advance the face changes, the diary records it, and
  the creature says something like "something feels different this
  morning." You watch the child, not the chart.
- The thresholds (2 / 5 / 12) are starting values, tunable from the
  diary's evidence.

## What the firmware does (v1)

- `muse_identity_t` carries `seed` (minted once at first boot, survives
  factory reset — same soul, new life), `growth_stage`, `care_days`.
- `muse_brain_consolidate()` evaluates the care-day, advances the stage
  at thresholds, and logs growth moments to the diary.
- `muse_brain_snapshot()` includes the stage, so the voice persona (and
  the cloud) always knows how old the child is.
- **The veto rule:** at stage 0 the action layer degrades VETO to
  CAUTION (reason preserved). The child earns its "no" at toddlerhood.
  This is middleware behavior, documented here, landing with the voice
  turn-loop hook.

## What growth is NOT

- Not unlockable features or DLC. Nothing is withheld as a reward; the
  newborn has the whole brain, just not the *will* yet.
- Not a score. There is no level, no XP, no leaderboard. The diary knows
  the stage; the face shows the person.
- Not reversible by neglect. A missed week doesn't un-grow the child.
  (Factory reset does — that's death, and the generation counter says so.)
