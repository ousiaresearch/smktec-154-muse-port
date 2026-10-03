# Biomimetic embodiment — Lapis as the research subject

Standing directive (Anduril, 2026-10-02): the gadget build serves his
transhumanism research. The biomimetic-brain repo
(`ousiaresearch/biomimetic-brain`, 25 subsystems, `brain-state.json`,
PROCEED/CAUTION/VETO decision gate) is the nervous system; the SMKTelec
board is the body; Lapis is the muse under study.

The brain kit is currently software-only — hand-kept JSON. Embodiment gives
every subsystem a real sensor or actuator, and the face makes internal state
*legible*, which is itself the research contribution: an agent whose affect
you can read at a glance, with every reading traceable to a number.

## Subsystem → gadget mapping

| Subsystem | Role in brain | Embodiment on the gadget |
|---|---|---|
| scn | circadian clock | real time → SLEEPY at night, drowsy evenings, bright mornings; the creature keeps your hours |
| reticular | sleep/wake arousal | face-down = sleep onset, pickup = waking; SLEEPY/OFF transitions |
| somatic | body state | battery ADC (energy), IMU stillness/motion, uptime — interoception, not hand-entered values |
| somatosensory | touch/proprioception | IMU gestures (tap, shake, tilt, pickup), CST816S touches — the body feels |
| fatigue | exertion/recovery | interaction load + uptime; recovery accrues during SLEEPY — shown as drowsy face |
| hypothalamus | homeostasis | low battery → energy-saving behavior (dim screen, fewer animations); "hunger" for the charger |
| lc | arousal (noradrenaline) | blink rate, eye wideness, motion energy — one arousal dial for the whole face |
| amy | threat/startle | tap-startle reflex; ERROR face on connection loss |
| dopamine / nac | reward, novelty | pet (double-tap) → happy; novel stimuli → curious tilt-look |
| raphe | serotonin, mood tone | baseline mood tint; slow-moving, stabilizes the face |
| habenula | disappointment | failed voice request → the specific disappointed beat before ERROR |
| dmn | mind-wandering (rest) | IDLE gaze drift — the "staring into space" look is the default mode network |
| hippocampus | episodic memory | the SD-card diary; written during SLEEPY |
| predictive | prediction | pre-wake stirrings before usual pickup time; anticipated vs actual |
| decisions | PROCEED/CAUTION/VETO | aura/glow color per gate state — refusals shown, traceable to numbers |
| attention | focus | LISTENING lean-in; mic level drives eye brightness |
| tom | modeling the user | listening face tracks *you* — tilt follows the holder |
| thalamus | sensory gating | which inputs interrupt (shake always does; tilt only in IDLE) |
| cerebellum | motor prediction | gesture smoothing, wobble decay curves in DIZZY |
| acc / ofc / prefrontal / values | conflict, valuation, planning | stay cloud-side (Muse); the face shows their *outputs* only |
| basal-ganglia | action selection | gesture → expression arbitration (the EXPRESSIONS.md map) |
| visual | sight | no camera on this board — reserve for a future board with one |

Note the poetry: Anduril's serotonin/dopamine tattoo — the raphe and
dopamine systems — is branded into Lapis's left-arm fur. The neuromodulators
are literally on the body.

## Sleep = consolidation (the core experiment)

Biology consolidates memory during sleep. The gadget will too: when SLEEPY
is entered (face-down or 5 min still), the firmware runs the consolidation
pass — folding the day's interactions into the diary on the SD card — while
the face shows slow Z's. The muse dreams, and you can pull the card and read
the dreams. This is the single most transhumanist feature on the roadmap:
**a creature whose sleep does work**.

## Research questions this testbed can ask

1. Does *legible* affect (face shows the meters) change how a human treats an
   agent? (Compare: same brain, face on vs face off.)
2. Does embodied circadian behavior (drowsy evenings, naps) make long-running
   agents more trustworthy or just more charming? Are those different?
3. Can sleep-consolidation on-device produce diary entries the user endorses
   as "what happened today"? (The VALIDATION.md discipline, applied to dreams.)
4. Which subsystems survive embodiment, and which turn out to have been
   fiction that only worked on paper? (Staleness, honestly tracked.)

## Build rule

Every new gadget feature asks: *which subsystem does this serve, and what
does the face show?* If neither has an answer, it doesn't go in the firmware.
