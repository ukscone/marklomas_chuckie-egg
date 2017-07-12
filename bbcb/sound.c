/*
    sound.c: Approximation of BBC model B SOUND and ENVELOPE commands
    Copyright (C) 2007  Mark Lomas

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/*
    Notes: Doesn't block the program if it issues a sound 'request' on
           a channel with a full queue. To enable this functionality,
           define a macro called 'BLOCK_ON_QUEUE_FULL'.

    TODO: Finish noise implementation for channel 0 i.e. the pulse-wave stuff.
    TODO: Handle cancellation of infinite duration indicated by negative duration param
*/

#include <math.h>
#include <assert.h>
#include "SDL.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Uncomment this line to enable blocking when a channels request queue becomes full. */
/*#define BLOCK_ON_QUEUE_FULL*/

/* Uncomment this line to generate unit tests. */
/*#define UNIT_TEST*/


#define FREQUENCY 16000 //11025 /* The playback sample frequency*/
#define MAX_CHANNELS 4
#define MAX_ENVELOPES 16 /* Only the full 16 were available on the BBC if the RS432 port was disabled. */

/* The BASIC program generates each sound by initiating one or more ‘requests’
   each of which may take the form of a musical note or a single effect and is
   directed to a specific channel. If the destination channel is idle when a request
   requires it, the sound starts playing immediately. If a previous request is still
   being handled the new one is placed on a queue, where it waits until the current
   event is over (or past a critical stage – see ENVELOPE). If the queue is full, the
   program waits. Separate queues are provided for the four channels, each of which
   can hold up to four requests, not counting the one currently being executed. The
   program can look at the state of any queue and flush any queue, but cannot find
   out or alter the state of the current event, except for flushing the whole queue.
*/
#ifdef BLOCK_ON_QUEUE_FULL
#define MAX_QUEUELENGTH 4
#else
/* I have permitted a much larger queue. This has made waiting for a sequence of
   notes to play simpler because we don't have waits inside the sound code as well
   as in the game code. */
#define MAX_QUEUELENGTH 64
#endif


/* Sound command queue */
typedef union
{
    unsigned char array[8];
    struct
    {
        unsigned int channel;
        char amplitude;
        unsigned char pitch;
        unsigned char duration;
    } params;

} Sound;

typedef union
{
    unsigned char array[14];
    struct
    {
        unsigned char N;        /* 1 to 16      Envelope number */
        unsigned char T;        /* bits 0-6    1 to 127  length of each step in hundredth of a second i.e. the length of a step in 100ths of a second
                                   bit 7       0 or 1    0 = auto repeat pitch envelope,
                                                         1 = don’t repeat pitch envelope */
        char PI1;               /* –128 to 127 change of pitch per step in section 1 */
        char PI2;               /* –128 to 127 change of pitch per step in section 2 */
        char PI3;               /* –128 to 127 change of pitch per step in section 3 */
        unsigned char PN1;      /* 0 to 255    Number of steps in section l */
        unsigned char PN2;      /* 0 to 255    Number of steps in section 2 */
        unsigned char PN3;      /* 0 to 255    Number of steps in section 3 */
        char AA;                /* –127 to 127 Change of amplitude per step during attack phase */
        char AD;                /* –127 to 127 Change of amplitude per step during decay phase */
        char AS;                /* –127 to 0   Change of amplitude per step during sustain phase */
        char AR;                /* –127 to 0   Change of amplitude per step during release phase */
        unsigned char ALA;      /* 0 to 126    Target level at end of attack phase */
        unsigned char ALD;      /* 0 to 126    Target level at end of decay phase */

    } params;

} Envelope;

typedef enum
{
    ATTACK,
    DECAY,
    SUSTAIN,
    RELEASE
} Phase;

typedef struct
{
    unsigned char pitch;
    unsigned char amplitude;
    unsigned char envelope;
    int duration;

    /* State data */
    unsigned int noise; /* Last chosen noise value */
    unsigned int angle;

    /* Envelope state*/
    int pitchDelta;
    unsigned int steps;
    Phase adsr;

} Channel;

/* Audio specification */
SDL_AudioSpec wanted;

/* Converts between 1/14 semitone increments and actual pitch of notes given that middle A is 89. */
float pitchLookup[256];

Channel channels[MAX_CHANNELS];

Envelope envelopes[MAX_ENVELOPES];

Sound soundQueue[MAX_QUEUELENGTH][MAX_CHANNELS];
unsigned int queueBegin[MAX_CHANNELS], queueEnd[MAX_CHANNELS], queueSize[MAX_CHANNELS];


/* Initialise channel data */
void ChannelsInit(void)
{
    unsigned int i;
    for(i = 0; i < MAX_CHANNELS; ++i)
    {
        channels[i].amplitude = 0;
        channels[i].duration = 0;
        channels[i].envelope = 0;
        channels[i].angle = 0;
        channels[i].noise = 0;
        channels[i].pitch = 0;
        channels[i].pitchDelta = 0;
        channels[i].steps = 0;
        channels[i].adsr = ATTACK;
    }
}

void QueueInit(void)
{
    unsigned int i;
    for(i = 0; i < MAX_CHANNELS; ++i)
    {
        queueBegin[i] = 0;
        queueEnd[i] = 0;
        queueSize[i] = 0;
    }
}

/*  The pitch P selects notes in quarter semi-tones intervals.
    Middle A is 440Hz and is selected when P is set to 89
    There are 48 steps between octaves (12 semitones * 4 incrments per semitone)

    A musical octave spans a factor of two in frequency and there are twelve notes per octave.
    Notes are separated by the factor 2^1/12 or 1.059463.
    (1.059463 x 1.059463 x 1.059463...continued for a total of twelve = 2; try it!)

    Starting at any note the frequency to other notes may be calculated from its frequency by:

    Freq = note x 2^(N/12),

    where N is the number of notes away from the starting note. N may be positive, negative or zero.

    For example, starting at D (146.84 Hz), the frequency to the next higher F is:

    146.84 x 2^(3/12) = 174.62,

    since F is three notes above. The frequency of A in the next lower octave is:

    146.84 x 2^(-17/12) = 55,

    since there are 17 notes from D down to the lower A.

    The equation will work starting at any frequency but remember that the N value for the
    starting frequency is zero.

    Octave number
    Note 1   2   3   4   5   6   7
    B    1   49  97  145 193 241
    A #  0   45  93  141 189 237
    A        41  89  137 185 233
    G #      37  85  133 181 229
    G        33  81  129 177 225
    F #      29  77  125 173 221
    F        25  73  121 169 217
    E        21  69  117 165 213
    D #      17  65  113 161 209
    D        13  61  109 157 205 253
    C #      9   57  105 153 201 249
    C        5   53  101 149 197 245
*/
void PitchInit(void)
{
    int middleA = 89;
    float middleAFrequency = 440.0f;
    int i;

    // TODO: Doesn't really need to be floating point. Might be a problem on CPUs with no floating point unit.
    for(i = 0; i < sizeof(pitchLookup)/sizeof(pitchLookup[0]); ++i)
    {
        pitchLookup[i] = middleAFrequency * (float)pow(2, (i - middleA)/(4.0f*12.0f));
    }
}

/*
    The channel C is 0-3, where 0 is a noise channel.
    The amplitude, A can be varied between 0 (off) and – 15(loud).
    A positive number selects a pre-defined envelope. Four envelopes are normally permitted and they are numbered 1 to 4.
    The pitch P selects notes in quarter semi-tones intervals. Middle C is produced when P is set at 53.
    The duration D, determines the length of the note and is given in twentieths of a second.

    SOUND C, A, P, D
    C is the channel number 0 to 3
    A is the amplitude or loudness  0 to – 15
    P is the pitch  0 to 255
    D is the duration   1 to 235

    As was mentioned earlier, channel number 0 produces ‘noises’ rather than notes
    and the value of P in the statement
    SOUND 0,A,P,D
    has a different effect from that described for channels 1, 2 and 3. Here is a
    summary of the effects of different values of P on the noise channel:

    P Effect
    0 High frequency periodic noise.
    1 Medium frequency periodic noise.
    2 Low frequency periodic noise.
    3 Periodic noise of frequency determined by the pitch setting of channel 1.
    4 High frequency ‘white’ noise.
    5 Medium frequency ‘white’ noise.
    6 Low frequency ‘white’ noise.
    7 Noise of frequency determined (continuously) by the pitch setting of channel 1.

    Values of P between 0 and 3 produce a rasping, harsh note. With P set to 4 the
    noise is like that produced by a radio when it is not tuned to a station – a sort of
    ‘shssh’ effect. P=6 sounds like the interference found on bad telephone calls.
    When P is set to 3 or 7 then the frequency of the noise is controlled by the pitch
    setting of sound channel number 1. If the pitch of channel 1 is changed while
    channel 0 is generating noise then the pitch of the noise will also change. The
    program below generates a noise on channel 0 and varies the pitch of the noise by
    changing the pitch of channel 1. Notice that the amplitude of channel 1 is very
    low (-1) so you will hardly hear it – but you will hear the noise on channel 0.

    The first parameter in the above example has the value &1213. The ampersand
    (&) indicates to the computer that the number is to be treated as a hexadecimal
    number. The four figures which follow the ampersand each control one feature.
    In this new expanded form the SOUND statement looks like

    SOUND &HSFC,A,P,D

    and the functions H, S, F and C will be explained in turn. In essence these
    numbers enable you to synchronise notes so that you can play chords effectively.

    The first number (H) can have the value 0 or 1. If H=1 then instead of playing a
    new note on the selected channel, the previous note on that channel is allowed
    to continue. If a note were gently dying away then it might be abruptly
    terminated when its time was up. Setting H=1 allows the note to continue
    instead of playing a new note. If H=1 then the note defined by the rest of the
    SOUND statement is ignored.
*/
void basicsound(unsigned int chan, char amplitude, unsigned char pitch, unsigned char duration)
{
    Sound* sound = 0;
    unsigned int pos;
    unsigned int flush= ((chan>>4) & 1);
    unsigned int C = chan & 3;

    /* Flush sound queue for this channel to allow this note to play immediately */
    if(flush)
        queueSize[C] = queueBegin[C] = queueEnd[C] = 0;

    /* Handle the queue full case. */
    if(queueSize[C] == MAX_QUEUELENGTH)
#ifdef BLOCK_ON_QUEUE_FULL
    {
        /* If queue is full, block until a slot is available. */
        while(queueSize[C] == MAX_QUEUELENGTH)
        {
        }
    }
#else
        return; /* If queue is full, discard */
#endif

    pos = queueEnd[C];
    ++queueEnd[C];
    queueEnd[C] %= MAX_QUEUELENGTH;

    sound = &soundQueue[pos][C];

    /* Queue up the sound. */
    sound->params.channel = chan;
    sound->params.amplitude = amplitude;
    sound->params.pitch = pitch;
    sound->params.duration = duration;

    ++queueSize[C];
}

void sound(unsigned char *array)
{
    basicsound((unsigned int)(array[0]|(array[1]<<8)), (char)array[2], array[4], array[6]);
}

void playSound(Sound* sound)
{
    char amplitude = sound->params.amplitude;
    unsigned char pitch = sound->params.pitch;
    unsigned char duration = sound->params.duration;
    unsigned int chan = sound->params.channel;
    Channel* channel = &channels[(chan&3)];
    unsigned char H = (chan >> 12) & 1; /* Continuation control */

    /* Continue existing sound */
    if(H) /* Creates a dummy note causing the release phase of the existing note to be played out. */
    {
        /* Reset angle to restart duration, but keep waveform continuous */
        channel->angle %= (int)(FREQUENCY/pitchLookup[(unsigned int)(channel->pitch + channel->pitchDelta) % 256]);

        /* Ignore the rest of the paramters to this sound command. */
        return;
    }

    /*  Negative values indicate amplitude 0 .. -15*/
    if(amplitude <= 0)
    {
        /* Clamp amplitude */
        amplitude = (-amplitude) << 3;
        if(amplitude > 127)
            amplitude = 127;

        /* Convert 0..-15 into range 0..127 */
        channel->amplitude = amplitude;
        channel->envelope = 0;
    }
    else
    {
        /* A positive number selects a pre-defined envelope.
           Envelopes are numbered 1 to 16.
        */
        channel->envelope = amplitude % 17;
        channel->amplitude = 0; /* Amplitude starts at zero for envelopes. */
    }

    /* Duration is in 20ths of a second (1..235) - convert into milliseconds */
    if(duration == 255) /* -1 indicates that the sound continues forever */
        channel->duration = -1;
    else
        channel->duration = duration * (1000 / 20);

    channel->pitch = pitch;
    channel->angle = 0;    /* Init sound */
    channel->noise = 0;     /* First noise sample choice */
    channel->pitchDelta = 0;   /* Init envelop state */
    channel->steps = 0;
    channel->adsr = ATTACK;
}

/* Issue next sound for this channel if there is one. */
void issueSound(unsigned char chan)
{
    Sound* sound = 0;
    unsigned char playGroup, syncCtrl, i;

    /* Are there any queued sounds for this channel? */
    if(queueSize[chan] == 0)
        return;

    /* Peek at next sound in queue */
    sound = &soundQueue[queueBegin[chan]][chan];

    /* Perform sync control
       The value initially determines the number of other channels that must receive requests with the
       same value of S before the group will play.
    */
    syncCtrl = (sound->params.channel >> 8) & 3; /* Synchronisation control */
    if(syncCtrl)
    {
        playGroup = syncCtrl;

        /* Look for sounds on other channels that belong to the same sync group and count them. */
        for(i = 0; i < MAX_CHANNELS; ++i)
        {
            if(i != chan)
            {
                if(((soundQueue[queueBegin[i]][i].params.channel >> 8) & 3) == syncCtrl)
                    --playGroup;
            }
        }

        /* If we still have to wait for other members of the sync group then exit and play sound later. */
        if(playGroup)
            return;
    }


    /* Pop sound off queue */
    ++queueBegin[chan];
    queueBegin[chan] %= MAX_QUEUELENGTH;
    --queueSize[chan];
    playSound(sound);
}


/* ENVELOPE N, T, PI1, PI2, PI3, PN1, PN2, PN3, AA, AD, AS, AR, ALA, ALD */
void basicenvelope(unsigned char N,
                   unsigned char T,
                   char PI1,
                   char PI2,
                   char PI3,
                   unsigned char PN1,
                   unsigned char PN2,
                   unsigned char PN3,
                   char AA,
                   char AD,
                   char AS,
                   char AR,
                   unsigned char ALA,
                   unsigned char ALD)
{
    Envelope* envelope;

    if(N < 1 && N > 16)
        return;

    envelope = &envelopes[N-1];
    envelope->params.N = N;
    envelope->params.T = T;
    envelope->params.PI1 = PI1;
    envelope->params.PI2 = PI2;
    envelope->params.PI3 = PI3;
    envelope->params.PN1 = PN1;
    envelope->params.PN2 = PN2;
    envelope->params.PN3 = PN3;
    envelope->params.AA = AA;
    envelope->params.AD = AD;
    envelope->params.AS = AS;
    envelope->params.AR = AR;
    envelope->params.ALA = ALA;
    envelope->params.ALD = ALD;
}

void envelope(unsigned char *array)
{
    Envelope* envelope;
    unsigned char N = array[0];

    if(N < 1 && N > 16)
        return;

    envelope = &envelopes[N-1];
    memcpy(envelope->array, array, sizeof(unsigned char)*14);
}


    /*
http://central.kaserver5.org/Kasoft/Typeset/BBC/KeyE.html

    ENVELOPE

Purpose

The envelope statement is used with the SOUND statement to control the volume and pitch of a sound while it is playing.
All natural sounds change in volume (loudness or amplitude);
for example, the sounds from a piano start off loudly and then fade away.
An aircraft flying overhead starts off softly, gets louder and then fades away.

The variation of amplitude (loudness) for the aircraft, as it flew overhead, might well look something like this:

This variation of amplitude with time is described as an "amplitude envelope".

Some sounds change in pitch. For example, a wailing police siren

This variation of pitch with time is called a "pitch envelope".

The BBC computer can use both pitch and amplitude envelopes and these are set up with the ENVELOPE statement.
Example

10 ENVELOPE l,l,4,-4,4,10,20,10,

127,0,0,-5,126,126

20 SOUND 1,1,100,200
Description

The ENVELOPE statement is followed by 14 parameters.

ENVELOPE N,T,PI1,PI2,PI3,PNl,PN2,

PN3,AA,AD,AS,AR,ALA,ALD

Parameter   Range   Function
N   1 to 4  Envelope Number
T bits 0-6  0 to 127    Length of each step in hundredths of a second
bit 7   0 or 1  0 = auto-repeat the pitch envelope
1 = don't auto-repeat
PI1     -128 to 127     Change of pitch per step in
section 1
PI2     -128 to 127     Change of pitch per step in
section 2
    -128 to 127     Change of pitch per step in
section 3
PN1     0 to 255    Number of steps in section 1
PN2     0 to 255    Number of steps in section 2
PN3     0 to 255    Number of steps in section 3
AA  -127 to 127     Change of amplitude per step during attack phase
AD  -127 to 127     Change of amplitude per step during decay phase
AS  -127 to 0   Change of amplitude per step during sustain phase
AR  -127 to 0   Change of amplitude per step during release phase
ALA     0 to 126    Target of level at end of attack phase
ALD     0 to 126    Target of level at end of decay phase

The N parameter specifies the envelope number that is to be defined.
It normally has a value in the range 1 to 4.
If the BASIC statement BPUT# is not being used then envelope numbers up to and including 16 may be used.

The T parameter determines the length in centi-seconds of each step of the pitch and amplitude envelopes.
The pitch envelope normally auto-repeats but this can be suppressed by setting the top bit of T -
i.e. using values of T greater than 127.

The six parameters PI1,PI2,PI3,PN1,PN2 and PN3 determine the pitch envelope.
The pitch envelope has 3 sections and each section is specified with two parameters;
the increment which may be positive or negative, and the number of times the increment is to be
applied during that section, that is the number of steps. A typical pitch envelope might look like

In the above example

T =1 centi-second

PI1=+10 PN1=12

PI2= - 5 PN2=27

PI3=+50 PN3=3

The pitch envelope is added to the pitch parameter (P) given in the SOUND statement.
In the above example it must have been 40 since the pitch starts at 40.
If bit 7 of the T parameter is zero then at the end of the pitch envelope; at a time given by the equation.

time=(PN1 + PN2 + PN3)*T centi-seconds

the pitch envelope will be set to zero and will repeat automatically.
Note that the pitch can only take on values in the range 0 to 255 and values outside this range "fold over", that
is the value used is MOD 256 of the value calculated.

The six parameters AA,AD,AS,AR,ALA and ALD determine the amplitude envelope.
Although the current internal sound generator has only 16 amplitude levels the software is upward compatible with
a generator having 128 levels.

The shape of the amplitude envelope is defined in terms of rates (increments) between levels, and is an extended
form of the standard ADSR system of envelope control.
The envelope starts at zero and then climbs at a rate set by AA (the attack rate) until it reaches the level set
by ALA. It then climbs or falls at the rate set by AD (the decay rate) until it reaches the level set by ALD.
However, if AD is zero the amplitude will stay at the level set by ALA for the duration (D) of the sound.

The envelope then enters the sustain phase which lasts for the remaining duration (D) of the sound.
The duration, D., is set by the SOUND statement. During the sustain phase the amplitude will remain the same or
fall at a rate set by "AS".

At the end of the sustain phase the note will be terminated if there is another note waiting to be played on
the selected channel. If no note is waiting then the amplitude will decay at a rate set by AR until the amplitude
reaches zero. If AR is zero then the note will continue indefinitely, with the pitch envelope auto-repeating if
bit 7 of parameter T is zero.

A typical amplitude envelope might look like

In the above example

T= 1 centi-second

ALA= 120

ADA= 80

AA = 30 (120 in 4 centi-seconds)

AD = - 4 (- 40 in 10 centi-seconds)

AS = 0

AR = - 5 (- 80 in 16 centi-seconds)

Note that the amplitude cannot be moved outside the range 0 to 126.
Syntax

ENVELOPE <numeric>, <numeric>, <numeric>,<numeric>, <numeric>, <numeric>,<numeric>, <numeric>, <numeric>,<numeric>, <numeric>, <numeric>,<numeric>, <numeric>
    */



/* Called every n centiseconds as determined by bits 0..7 of the T parameter of the envelope command. */
void processPitchEnvelope(Channel* channel, Envelope* envelope)
{
    unsigned int totalEnvelopeSteps, steps;

    /* Only apply pitch envelope if we have a valid duration. */
    /* TODO: Precompute totalEnvelopeSteps */
    totalEnvelopeSteps = (envelope->params.PN1+envelope->params.PN2+envelope->params.PN3);
    if(totalEnvelopeSteps > 0)
    {
        steps = channel->steps;

        /* Compute number of steps based on length of a step. */
        if(envelope->params.T & 0x80)
        {
            /* No repeat */
            if(steps > totalEnvelopeSteps)
            {
                channel->pitchDelta = 0; /* Reset pitch delta */
                return;
            }
        }
        else
        {
            /* Auto repeat */
            steps %= totalEnvelopeSteps;
        }


        /* Compute pitch envelope */
        if(steps < envelope->params.PN1)
            channel->pitchDelta += envelope->params.PI1;
        else if(steps < envelope->params.PN2)
            channel->pitchDelta += envelope->params.PI2;
        else
            channel->pitchDelta += envelope->params.PI3;
    }
}

/* Called every n centiseconds as determined by bits 0..7 of the T parameter of the envelope command. */
void processAmplitudeEnvelope(unsigned char chan, Envelope* envelope)
{
    Channel* channel = &channels[chan];
    switch(channel->adsr)
    {
    case ATTACK:
        /* If AA is negative, this will wrap around. */
        channel->amplitude += envelope->params.AA;
        if(channel->amplitude >= envelope->params.ALA)
            channel->adsr = DECAY;
        break;
    case DECAY:
        /* If AA is negative, this will wrap around. */
        channel->amplitude += envelope->params.AD;
        if(channel->amplitude <= envelope->params.ALD)
            channel->adsr = SUSTAIN;
        break;
    case SUSTAIN:
        /* The sustain phase can be terminated either by the amplitude reaching zero or the time set
        by the duration of the SOUND statement running out. The duration has to be set with care to
        ensure that it doesn’t cut the note off at the wrong moment. */
        channel->amplitude += envelope->params.AS;
        if(channel->amplitude == 0)
            channel->adsr = RELEASE;
        break;
    case RELEASE: /* This phase can be entered if the duration of the sound has ended. */
        /* If AR us zero, the amplitude will not change and the note will continue forever. */
        channel->amplitude += envelope->params.AR;

        /* If the amplitude reaches zero or there is another sound to queue, force termination of the sound. */
        if(channel->amplitude == 0 || (queueSize[chan]>0))
            channel->duration = 0; /* Force termination of sound by setting duration to zero. */
        break;
    };
}

void processEnvelope(unsigned char chan, unsigned int tcs)
{
    unsigned char stepCentiseconds;
    unsigned int numSteps, step;
    Channel* channel = &channels[chan];
    Envelope* envelope = &envelopes[channel->envelope - 1];
    assert(channel->envelope > 0);

    stepCentiseconds = (envelope->params.T & 0x7f);
    if(stepCentiseconds == 0)
        return; /* Must be 1..127 */

    /* Detect whether elasped time has caused a step to occur. */
    numSteps = (tcs/stepCentiseconds);
    step = (numSteps != channels[chan].steps);
    channel->steps = numSteps;
    if(!step)
        return;

    /* Process envelopes. */
    processPitchEnvelope(channel, envelope);
    processAmplitudeEnvelope(chan, envelope);
}

int finishedPlaying(unsigned char chan)
{
    /* Negative duration indicates to continue to sound the note forever. */
    return ((channels[chan].duration - (int)(channels[chan].angle*1000)/FREQUENCY) <= 0) && (channels[chan].duration>=0);
}

/* The audio function callback takes the following parameters:
    stream:  A pointer to the audio buffer to be filled
    len:     The length (in bytes) of the audio buffer
*/
void fill_audio(void *udata, Uint8 *stream, int len)
{
    unsigned int numSamplesPerWave;
    unsigned char chan;
    int i, val;
    unsigned int pitch;
    for(i = 0; i < len; ++i)
    {
        val = 0;

        /* Mix each channel by adding samples together*/
        for(chan = 0; chan < MAX_CHANNELS; ++chan)
        {
            if(finishedPlaying(chan))
            {
                /* Enter release phase for the existing sound. */
                channels[chan].adsr = RELEASE;

                /* Channel has finished playing so issue the next sound command from the queue. */
                issueSound(chan);
                continue;
            }


            channels[chan].angle++;

            /* Generate noise for channel 0 */
            if(chan == 0)
            {
                switch(channels[chan].pitch)
                {
                    case 0: /* 0 High frequency periodic noise. */
                        /* TODO: pulsewave
                            Channel 0 can produce noise (unpitched sound of pseudo-random
                            structure) or a pulse waveform. The frequency of the pulsewave or period of the
                            noise can be set to one of the three fixed options, or to the frequency of channel 1.
                        */
                        break;
                    case 1: /* 1 Medium frequency periodic noise. */
                        break;
                    case 2: /* 2 Low frequency periodic noise. */
                        break;
                    case 3: /* 3 Periodic noise of frequency determined by the pitch setting of channel 1. */
                        break;
                    case 4: /* 4 High frequency ‘white’ noise. */
                        if((channels[chan].angle%((int)(FREQUENCY/10000))) == 0)
                        {
                            channels[chan].noise = (((rand()&1)*2) - 1) * channels[chan].amplitude;
                        }
                        val -= channels[chan].noise;
                        break;
                    case 5: /* 5 Medium frequency ‘white’ noise. */
                        if((channels[chan].angle%((int)(FREQUENCY/5000))) == 0)
                        {
                            channels[chan].noise = (((rand()&1)*2) - 1) * channels[chan].amplitude;
                        }
                        val -= channels[chan].noise;
                        break;
                    case 6: /* 6 Low frequency ‘white’ noise. */
                        if((channels[chan].angle%((int)(FREQUENCY/2500))) == 0)
                        {
                            channels[chan].noise = (((rand()&1)*2) - 1) * channels[chan].amplitude;
                        }
                        val -= channels[chan].noise;
                        break;
                    case 7: /* 7 Noise of frequency determined (continuously) by the pitch setting of channel 1. */
                        {
                            /* Apply pitch envelope. Since this is unsigned, negative numbers wrap round to positive numbers */
                            pitch = (unsigned int)(channels[1].pitch + channels[1].pitchDelta) % 256;

                            /* Convert pitch into frequency and compute number of samples per wave at given playback frequency */
                            numSamplesPerWave = (int)(FREQUENCY/pitchLookup[pitch]);

                            if((channels[chan].angle%numSamplesPerWave) == 0)
                            {
                                channels[chan].noise = (((rand()&1)*2) - 1) * channels[chan].amplitude;
                            }
                            val -= channels[chan].noise;
                        }
                        break;
                }
            }
            else /* Channels 1..4 */
            {
                /* Apply pitch envelope. Since this is unsigned, negative numbers wrap round to positive numbers */
                pitch = (unsigned int)(channels[chan].pitch + channels[chan].pitchDelta) % 256;

                /* Convert pitch into frequency and compute number of samples per wave at given playback frequency */
                numSamplesPerWave = (int)(FREQUENCY/pitchLookup[pitch]);

                /* Generate square wave */
                if((channels[chan].angle%numSamplesPerWave) > (numSamplesPerWave>>1))
                    val -= channels[chan].amplitude;
                else
                    val += channels[chan].amplitude;
            }

            /* Process envelope */
            if(channels[chan].envelope > 0)
                processEnvelope(chan, (channels[chan].angle*100)/FREQUENCY);
        }

        /* Saturate after all samples have been mixed/added together. */
        if(val > 127)
            val = 127;
        else if(val < -127)
            val = -127;

        /* Scale down volume a bit on PC because it is really loud! Divide by 8 seems good. */
        stream[i] = val/2;
    } /* Next sample */
}

int AudioInit(void)
{
    if(SDL_InitSubSystem( SDL_INIT_AUDIO )==-1)
        return 0;

    ChannelsInit();
    PitchInit();
    QueueInit();

    /* Set the audio format */
    wanted.freq = FREQUENCY;
    wanted.format = AUDIO_S8; /* BBC only has 16 levels of amplitude, so 8 bit is more than sufficient. */
    wanted.channels = 1;    /* 1 = mono, 2 = stereo */
    wanted.samples = 512;  /* Good low-latency value for callback */
    wanted.callback = fill_audio;
    wanted.userdata = NULL;

    /* Open the audio device, forcing the desired format */
    if ( SDL_OpenAudio(&wanted, NULL) < 0 ) {
        //fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
        return 0;
    }

    /* Let the callback function play the audio chunk */
    SDL_PauseAudio(0);

    return 1;
}

void AudioQuit(void)
{
    SDL_LockAudio();
    SDL_PauseAudio(1);
    SDL_UnlockAudio();
    SDL_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

#ifdef UNIT_TEST

/* Wait function to invoke between unit tests */
void WaitForCompletion(void)
{
    int finished, chan;

    /* Wait for sound to complete */
    finished = 0;
    while ( !finished )
    {
        /* Find a channel that hasn't finished. */
        finished = 1;
        for(chan = 0; chan < MAX_CHANNELS; ++chan)
        {
            if(! finishedPlaying(chan) || queueSize[chan] > 0)
                finished = 0;
        }
        SDL_Delay(100);         /* Sleep 1/10 second */
    }
}

int main(int argc, char* argv[])
{
    if(!AudioInit())
        return 1;

    /* Test #0 */
    basicsound(0,-15,4,50);
    basicsound(0,-15,5,50);
    basicsound(0,-15,6,50);
    WaitForCompletion();

    /* Test #1 */
    basicenvelope(1,1,-26,-36,-45,255,255,255,1,0,0,0,0,0);
    basicsound(1,1,100,50);
    basicsound(0,-15,7,50);
    WaitForCompletion();

    /* Test #2 */
    basicenvelope(1,1,4,-4,4,10,20,10,127,0,0,-5,126,126);
    basicsound(1,1,100,200);
    WaitForCompletion();

    /* Test #3 */
    basicenvelope(1,1,-26,-36,-45,255,255,255,127,0,0,0,126,0);
    basicsound(1,1,1,100);
    basicsound(1, -4, 53, 200);
    WaitForCompletion();

    /* Test #4 */
    basicenvelope(3,25,16,12,8,1,1,1,10,-10,0,-10,100,50);
    basicsound(3,3,100,100);
    WaitForCompletion();

    /* Test #5 */
    basicenvelope(1,1,0,0,0,0,0,0,2,0,-10,-5,120,0);
    basicsound(1,1,100,100);
    WaitForCompletion();

    /* Test #6 */
    basicenvelope(3,7,2,1,1,1,1,1,121,-10,-5,-2,120,120);
    basicsound(3,3,100,100);
    WaitForCompletion();

    /* Test #7 */
    basicenvelope(1,8,1,-1,1,1,1,1,121,-10,-5,-2,120,1);
    basicsound(1,1,100,100);
    WaitForCompletion();

    /* Test #8 */
    basicsound(1, -1, 53, 200);
    basicsound(2, -1, 69, 200);
    basicsound(3, -1, 81, 200);
    WaitForCompletion();

    AudioQuit();

    return 0;
}
#endif

#ifdef __cplusplus
}
#endif
