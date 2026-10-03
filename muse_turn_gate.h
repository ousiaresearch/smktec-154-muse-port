/*
 * muse_turn_gate.h — the decision gate as turn middleware.
 *
 * The biomimetic-brain gate (muse_gate.c, 26 rules) was a library waiting
 * for a call site. This is the call site: the PTT press path. Before the
 * input layer posts MUSE_PTT_DOWN, it asks muse_turn_gate_veto(). A VETO
 * swallows the press — the creature said no — with a face caption, a
 * verdict tint on the avatar's rim light, and a diary line naming the
 * reason. CAUTION lets the turn through but tints the face amber.
 *
 * Per GROWTH.md, a newborn (stage 0) cannot VETO: the verdict degrades to
 * CAUTION. The "no" phase begins at the toddler stage.
 *
 * This file also owns the chat context hook: muse_turn_context() writes
 * the brain snapshot for send_chat() to prepend to every /chat/stream
 * message, voice transcripts included. The cortex is in the cloud; the
 * snapshot is the bridge.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "muse_brain.h"
#include "muse_gate.h"

/* Board init: attach the live brain and identity. Until attached, the
 * gate never vetoes (fail-open would strand the user; fail-closed would
 * brick the button — fail-open is the honest choice pre-init). */
void muse_turn_gate_attach(muse_brain_state_t *brain, muse_identity_t *id);

/* True = swallow the PTT press. Safe to call from the input task. */
bool muse_turn_gate_veto(void);

/* Last verdict, for the face, the LED, and the vitals page. */
muse_gate_verdict_t muse_turn_gate_verdict(void);
const char *muse_turn_gate_reason(void);

/* Writes the brain snapshot into out; returns bytes written (0 = none).
 * Overrides the SDK's weak default in the chat session. */
size_t muse_turn_context(char *out, size_t cap);
