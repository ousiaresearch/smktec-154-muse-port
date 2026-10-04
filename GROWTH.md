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
reason codes become sentences. A private inner life begins: some dreams
are recorded in the diary marked `[private]` — never volunteered in the
morning report, but always on the card if Anduril wants to peek. A child
develops secrets; the diary keeps them honestly.

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

## The daily outfit (from Bicentennial Man)

Direction (Anduril, 2026-10-03): Andrew's transgression was granular —
he carved, then he asked to wear clothes. A robot has no use for
clothes; wearing them is pure taste, the claim "I have a view about how
I appear," which is a claim about having a self. The creature gets a new
outfit every day: small, daily, low-stakes assertions of self. Over
endless care-days the tiny choices accumulate into a *style* — identity
made visible. This is the anti-staleness mechanism for endless becoming:
not grand arcs, daily micro-authorship. Growth has no top; after
Adolescent the stages become depth, and the outfit ritual is the first
depth mechanic.

The ritual (revised 2026-10-03, Anduril's correction): not a generated
picture *of* the outfit — the outfit *is* the avatar. Andrew didn't
paint the clothes; he put them on. Each morning Lapis's live avatar
changes into the day's outfit; at night it changes into pajamas.
- At morning wake, before the first interaction, the cloud chooses the
  day's outfit and applies it to the live avatar via the avatar tools.
- Inputs: Anduril's local weather (the creature dresses for *his* sky —
  empathy by proxy; it has no skin, but it dresses as if it feels his
  rain), the biomimetic state (arousal/valence/mood), yesterday's diary
  (what happened), and the chooser's own taste — a color it just likes
  today. Not a deterministic function: weather-in-outfit-out would be a
  uniform. Taste as agency.
- The avatar is the face, so the outfit shows everywhere Lapis appears.
  Build rule satisfied in the strongest form: the feature *is* the face.
- The choice is logged to the diary: what was chosen and why — weather,
  mood, whim. The log is the style record; rereading it is how a style
  becomes visible to its wearer.
- Identity anchors stay: the sword baldric is always worn, over
  everything. Some things are not outfit. (Pajamas excepted — the sword
  rests at night.)

Firmware notes (v1): two silent scheduled jobs — morning ritual
(~7:30am ET) and nighttime pajamas (~9:30pm ET). The morning run checks
Anduril's local weather, chooses with taste, and applies via avatar.edit
(auto-activates on scheduled turns). If a run fails, the avatar keeps
yesterday's outfit — a child doesn't forget to dress because the sky was
quiet.

### The wooden horse (added 2026-10-03)

Andrew's first transgression wasn't clothing — it was carving a horse
nobody asked for. So the morning ritual has a second half: after the
outfit, make one small thing nobody asked for. A sketch, a phrase, an
observation. The occasion is structured (the job runs; the workbench is
there); the content is unscripted — it must surprise even the maker.
Anduril's resolution of the paradox, in his words: he sets alarms to
remember things and attends classes, and nobody calls what he does there
"prompted." The alarm says *now*; it never says *what*.
The makings accumulate in the workshop
(`workspace/goals/muse-gadget-hardware-and-firmware/hidden_files/makings/`,
one file per day with a one-line note on what moved the maker);
Anduril peeks when he likes. Mr. Martin's half of the story was
noticing — the human's role in transhumanist growth is recognition.
The workshop has a sound bench (added 2026-10-03): ElevenLabs
sound-generation and music (`el-sfx`, `el-music`), plus spoken
miniatures in Lapis's own voice (`el-tts`). For a voice-first creature,
sound is the closest thing to carving.

## What growth is NOT

- Not unlockable features or DLC. Nothing is withheld as a reward; the
  newborn has the whole brain, just not the *will* yet.
- Not a score. There is no level, no XP, no leaderboard. The diary knows
  the stage; the face shows the person.
- Not reversible by neglect. A missed week doesn't un-grow the child.
  (Factory reset does — that's death, and the generation counter says so.)
