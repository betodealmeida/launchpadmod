#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif

#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#define LAUNCHPADMOD_URI "http://dealmeida.net/plugins/launchpadmod"

#define RAPID_LED_UPDATE (0x92)
#define SET_GRID_LED (0x90)

#define OFF (0xc)
#define LOW_RED (0xd)
#define FULL_RED (0xf)
#define LOW_AMBER (0x1d)
#define FULL_AMBER (0x3f)
#define FULL_YELLOW (0x3e)
#define LOW_GREEN (0x1c)
#define FULL_GREEN (0x3c)

#define FLASH_FULL_RED (0xb)
#define FLASH_FULL_AMBER (0x3b)
#define FLASH_FULL_YELLOW (0x3a)
#define FLASH_FULL_GREEN (0x38)

typedef enum {
    MIDI_OUT = 0,
    TRACK_1 = 1,
    TRACK_2 = 2,
    TRACK_3 = 3,
    TRACK_4 = 4,
    TRACK_5 = 5,
    TRACK_6 = 6,
    TRACK_7 = 7,
    TRACK_8 = 8,
} PortIndex;

typedef struct
{
    // Features
    LV2_URID_Map* map;

    struct
    {
        LV2_URID midi_MidiEvent;
    } uris;

    // Ports
    LV2_Atom_Sequence* launchpad_out;
    float* input[8];
} Launchpadmod;

static LV2_Handle
instantiate(const LV2_Descriptor* descriptor,
    double rate,
    const char* bundle_path,
    const LV2_Feature* const* features)
{
    LV2_URID_Map* map = NULL;
    for (int i = 0; features[i]; ++i) {
        if (!strcmp(features[i]->URI, LV2_URID__map)) {
            map = (LV2_URID_Map*)features[i]->data;
            break;
        }
    }
    if (!map) {
        return NULL;
    }

    Launchpadmod* self = (Launchpadmod*)malloc(sizeof(Launchpadmod));
    self->map = map;
    self->uris.midi_MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);

    return (LV2_Handle)self;
}

static void
connect_port(LV2_Handle instance,
    uint32_t port,
    void* data)
{
    Launchpadmod* self = (Launchpadmod*)instance;

    if (port == 0) {
        self->launchpad_out = (LV2_Atom_Sequence*)data;
    } else {
        self->input[port - 1] = (float*)data;
    }
}

static void
draw(Launchpadmod* self, float level[8])
{
    // Struct for a 3 byte MIDI event, used for writing notes
    typedef struct
    {
        LV2_Atom_Event event;
        uint8_t msg[3];
    } MIDINoteEvent;

    // Initially self->launchpad_out contains a Chunk with size set to capacity
    // Get the capacity
    const uint32_t out_capacity = self->launchpad_out->atom.size;

    // Write an empty Sequence header to the output
    lv2_atom_sequence_clear(self->launchpad_out);
    self->launchpad_out->atom.type = self->uris.midi_MidiEvent;

    // Rapid LED updates the 8x8 grid in left-to-right, top-to-bottom order,
    // then the eight scene launch buttons in top-to-bottom order, and finally
    // the eight Automap/Live buttons in left-to-right order.
    int palette[8] = {
        LOW_GREEN,
        FULL_GREEN,
        FULL_YELLOW,
        LOW_AMBER,
        FULL_AMBER,
        LOW_RED,
        FULL_RED,
    };
    int color, z, amplitude, power;
    int messages[80];
    for (int i = 0; i < 8; i++) {
        amplitude = level[i] * sqrt(2);
        power = amplitude * 9 + 1;
        z = log10(power) * 7;

        for (int j = 7; j >= 0; j--) {
            if (z >= j) {
                color = palette[j];
            } else {
                color = OFF;
            }
            messages[j * 8 + i] = color;
        }
    }
    for (int i = 0; i < 80; i += 2) {
        MIDINoteEvent pad;
        pad.event.time.frames = 0;
        pad.event.body.type = self->uris.midi_MidiEvent;
        pad.event.body.size = sizeof(messages);
        pad.msg[0] = RAPID_LED_UPDATE;
        pad.msg[1] = messages[i];
        pad.msg[2] = messages[i + 1];

        lv2_atom_sequence_append_event(
            self->launchpad_out, out_capacity, &pad.event);
    }

    // Exit rapid LED update mode by resending a value with regular status
    MIDINoteEvent pad;
    pad.event.time.frames = 0;
    pad.event.body.type = self->uris.midi_MidiEvent;
    pad.event.body.size = sizeof(messages);
    pad.msg[0] = SET_GRID_LED;
    pad.msg[1] = 0x0;
    pad.msg[2] = messages[0];

    lv2_atom_sequence_append_event(
        self->launchpad_out, out_capacity, &pad.event);
}

static void
run(LV2_Handle instance, uint32_t n_samples)
{
    Launchpadmod* self = (Launchpadmod*)instance;

    float level[8];
    for (int i = 0; i < 8; i++) {
        level[i] = 0;
        for (uint32_t pos = 0; pos < n_samples; pos++) {
            level[i] += abs(self->input[i][pos]);
        }
        level[i] /= n_samples;
    }
    draw(self, level);
}

static void
cleanup(LV2_Handle instance)
{
    free(instance);
}

static const void*
extension_data(const char* uri)
{
    return NULL;
}

static const LV2_Descriptor descriptor = {
    LAUNCHPADMOD_URI,
    instantiate,
    connect_port,
    NULL, // activate
    run,
    NULL, // deactivate
    cleanup,
    extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
    switch (index) {
    case 0:
        return &descriptor;
    default:
        return NULL;
    }
}
