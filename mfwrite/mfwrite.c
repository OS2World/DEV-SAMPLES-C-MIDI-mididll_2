/* ===========================================================================
 * mfwrite.c
 *
 * Demonstrates how to use MIDIFILE.DLL to write out a dummy MIDI file. We have statically
 * defined MIDI data in this program's global data which we'll write out to a MIDI file.
 * =========================================================================
 */

#include <stdio.h>
#include <os2.h>

#include "midifile.h"

/* function definitions */
VOID initfuncs(CHAR * fn);

/* Let's define some "event" structures for the internal use of this program. Each will hold one
    "event". The first ULONG will be the event's time (referenced from 0). Then, there will be 4
    more bytes. The first of those 4 will be the event's Status, and the remaining 3 will be
    interpreted differently depending upon the Status. By having all of the events the same size
    (ie, 8 bytes), with the Time and Status first, this makes it easy to sort, insert, and delete
    events from a block of memory. The only drawback is that 8 bytes is a little cramped to store
    lots of info, so you could go with a larger structure, but 8 bytes is perfect for the majority of
    events.
	For fixed length MIDI events (ie, Status is 0x80 to 0xEF, 0xF1, 0xF2, 0xF3, 0xF6, 0xF8,
    0xFA, 0xFB, 0xFC, and 0xFE), the Status is simply the MIDI status, and the subsequent Data
    bytes are any subsequent MIDI data bytes. For example, for a Note-On, Status would be 0x90
    to 0x9F, Data1 would be the note number, Data2 would be the velocity, and Data3 wouldn't be
    used.
	For Meta-Events, the Status will be the meta Type (ie, instead of 0xFF). Note that all
    Meta-Event types are less than 0x80, so it's easy to distinguish them from MIDI status
    bytes. For fixed length Meta-Events (ie, meta types of 0x00, 0x2F, 0x51, 0x54, 0x58, and
    0x59), the remaining 3 bytes will be interpreted as follows:
	       TYPE		  DATA BYTES 1, 2, and 3
	    Sequence Number  = Not Used, SeqNumLSB, SeqNumMSB
	    End Of Track      = Not Used...
	     Tempo	      = BPM, Not Used, Not Used
	      SMPTE	     = There's too much data to store in only 3 bytes. We could define
				  the bytes to be an index into a list of larger structures, but I'll
				  just ignore SMPTE for simplicity.
	    Time Signature   =	Numerator, Denominator, MIDI clocks per metronome click.
	     Key Signature    = Key, Minor, Not Used
    For variable length Meta-Events (ie, type of 0x01 to 0x0F, or 0x7F), since these have data
    that can be any length, we'll set Data1 to be the length of the data (ie, we'll set a limit of
    255 bytes), and Data2 and Data3 will together form a USHORT index into a larger array
    (ie, we'll set a limit of 65,535 such Meta-Events).
	 For SYSEX events (0xF0 or 0xF7), these also can be any length, so we'll set Data1 to
    be an index into a larger array (ie, set a limit of 256 such events), and Data2 and Data3
    will form a USHORT length of the SYSEX (not counting the Status).
*/

typedef struct _EVENT  /* general form of an "event" */
{
    ULONG Time;
    UCHAR Status;
    UCHAR Data1, Data2, Data3;
} EVENT;

typedef struct _XEVENT
{
    ULONG  Time;
    UCHAR  Status;
    UCHAR  Index;
    USHORT Length;
} XEVENT;

typedef struct _SEQEVENT
{
    ULONG  Time;
    UCHAR  Status;
    UCHAR  UnUsed1;
    USHORT SeqNum;
} SEQEVENT;

typedef struct _TXTEVENT
{
    ULONG  Time;
    UCHAR  Status;
    UCHAR  Length;
    USHORT Index;
} TXTEVENT;

typedef struct _TEMPOEVENT
{
    ULONG  Time;
    UCHAR  Status;
    UCHAR  BPM, Unused1, UnUsed2;
} TEMPOEVENT;

typedef struct _TIMEEVENT
{
    ULONG Time;
    UCHAR Status;
    UCHAR Nom, Denom, Clocks;
} TIMEEVENT;

typedef struct _KEYEVENT
{
    ULONG Time;
    UCHAR Status;
    UCHAR Key, Minor, UnUsed1;
} KEYEVENT;




/* Need a CALLBACK structure for the DLL */
CALLBACK cb;



/* Need a MIDIFILE structure for the DLL */
MIDIFILE mfs;



/* OK, here's an artificially created block of EVENTS. We mix it up with a variety of Status,
    to show you how you might store such. This is for writing a Format 0. In your own app, you
    may choose to store data in a different way.
*/

EVENT evts[20] = {
     {0, 0x04, 5, 0x01, 0x00}, /* Instrument Name Meta-Event. Status = 0x04. Note that Data2
				      and Data3 form a USHORT index. Because Intel CPU uses
				      reverse byte order, the seq number here is really 0x0001 */
     {0, 0x01, 11, 0x00, 0x00}, /* A Text Meta-Event. Status = 0x01. */
     {0, 0x58, 5, 4, 24},	  /* A Time Signature Meta-Event. Status = 0x58. 5/4 */
     {0, 0x59, 1, 0, 0},	  /* A Key Signature Meta-Event. Status = 0x59. G Major */
     {0, 0x51, 100, 0, 0},	  /* A Tempo Meta-Event. Status = 0x51. */
     {96, 0x90, 64, 127, 0},	 /* A Note-on (ie, fixed length MIDI message) */
     {96, 0xC0, 1, 0xFF, 0},	 /* A Program Change (ie, also a fixed length MIDI message) */
     {96, 0xF0, 0, 0x02, 0x00}, /* SYSEX (ie, 0xF0 type) */
     {96, 0x90, 71, 127, 0},	 /* A Note-on (ie, fixed length MIDI message) */
     {192, 0xFC, 0, 0, 0},	 /* MIDI REALTIME "Stop" */
     {192, 0x90, 68, 127, 0},	/* A Note-on (ie, fixed length MIDI message) */
     {288, 0x90, 64, 0, 0},	 /* A Note-off (ie, Note-on with 0 velocity) */
     {288, 0x51, 120, 0, 0},	 /* A Tempo Meta-Event. Status = 0x51. */
     {288, 0x90, 68, 0, 0},	 /* A Note-off */
     {384, 0xF0, 1, 0x01, 0x00}, /* SYSEX (ie, 0xF0 type, but the start of a series of packets,
				       so it doesn't end with 0xF7) */
     {384, 0xF7, 2, 0x01, 0x00}, /* SYSEX (ie, 0xF7 type, the next packet). */
     {384, 0xF7, 3, 0x02, 0x00}, /* SYSEX (ie, 0xF7 type, the last packet, so it ends with 0xF7). */
     {480, 0x7F, 4, 0x02, 0x00}, /* Proprietary Meta-Event. */
     {480, 0x90, 71, 0, 0},	  /* A Note-off */
     {768, 0x2F, 0, 0, 0},	   /* An End Of Track Meta-Event. Status = 0x2F. */
};



/* Here's 2 artificially created blocks of EVENTS. These 2 are for writing a Format 1. We simply
    separate the events in the above track into 2 tracks. With a format 1, the first track is
    considered the "tempo map", so we put appropriate events in that (ie, all tempo and time
    signature events should go in only this one track).
*/

EVENT trk1[6] = {
     {0, 0x58, 5, 4, 24},
     {0, 0x59, 1, 0, 0},
     {0, 0x51, 100, 0, 0},
     {288, 0x51, 120, 0, 0},
     {480, 0x7F, 4, 0x02, 0x00},
     {768, 0x2F, 0, 0, 0},
};

EVENT trk2[15] = {
     {0, 0x04, 5, 0x01, 0x00},
     {0, 0x01, 11, 0x00, 0x00},
     {96, 0x90, 64, 127, 0},
     {96, 0xC0, 1, 0xFF, 0},
     {96, 0xF0, 0, 0x02, 0x00},
     {96, 0x90, 71, 127, 0},
     {192, 0xFC, 0, 0, 0},
     {192, 0x90, 68, 127, 0},
     {288, 0x90, 64, 0, 0},
     {288, 0x90, 68, 0, 0},
     {384, 0xF0, 1, 0x01, 0x00},
     {384, 0xF7, 2, 0x01, 0x00},
     {384, 0xF7, 3, 0x02, 0x00},
     {480, 0x90, 71, 0, 0},
     {768, 0x2F, 0, 0, 0},
};



/* Pointers to our tracks. Assume Format 0. In your own app, you may choose to do something
    different.
 */
EVENT * trk_ptrs[2] = { &evts[0], &trk2[0] };



/* An array of text for Meta-Text events. In your own app, you may choose to do something
    different. */
UCHAR text[3][20] = {
     { "Here's text" },  /* For Text */
     { "Piano" },       /* For Instrument Name */
     { 0, 1, 2, 3 },	 /* For Proprietary */
};



/* An array of track names. In your own app, you may choose to do something different. */
UCHAR names[2][20] = {
     { "Dummy" },
     { "Blort" },
};



/* An array of SYSEX data. In your own app, you may choose to do something
    different. */
UCHAR sysex[4][2] = {
     { 0x01, 0xF7 },   /* an 0xF0 type always ends with 0xF7 unless it has an 0xF7 type to follow */
     { 0x02 },
     { 0x03 },		/* an 0xF7 type doesn't always end with 0xF7 unless it's the last packet */
     { 0x04, 0xF7 },
};



/* To point to the next event to write out */
EVENT * TrkPtr;



/* To look up the address of sysex data */
UCHAR ex_index;



/* ********************************** main() ***********************************
 * Program entry point. Calls the MIDIFILE.DLL function to write out a MIDI file. The callbacks do
 * all of the real work of feeding data to the DLL to write out.
 *************************************************************************** */

main(int argc, char *argv[], char *envp[])
{
    LONG result;
    UCHAR buf[60];

    /* If no filename arg supplied by user, exit with usage info */
    if ( argc < 2 )
    {
	 printf("This program writes out a 'dummy' MIDI (sequencer) file.\r\n");
	 printf("It requires MIDIFILE.DLL to run.\r\n");
	 printf("Syntax: MFWRITE.EXE filename [0, 1, or 2 (for Format)]\r\n");
	 exit(1);
    }

    printf("Writing %s...\r\n", argv[1]);

    /* Initialize the pointers to our callback functions (ie, for the MIDIFILE.DLL to call) */
    initfuncs(argv[1]);

    /* NOTE: We can initialize the MThd before the call to MidiWriteFile(), or we can do it in
	our StartMThd callback. We'll do it now since we don't need to do anything else right
	before the MThd is written out (ie, and so don't need a StartMThd callback) */
    mfs.Format = 0;
    mfs.NumTracks = 1;
    if (argc>2)
    {
	 /* User wants us to write out a particular Format. If not 0, then write 2 MTrk chunks */
	 if ( (mfs.Format = atoi(argv[2])) )
	 {
	      mfs.NumTracks = 2;
	      trk_ptrs[0] = &trk1[0];
	 }
    }
    mfs.Division = 96;	 /* Arbitrarily chose 96 PPQN for my time-stamps */

    /* Make it easier for me to specify Tempo and Time Signature events */
    mfs.Flags = MIDIBPM|MIDIDENOM;

    /* Tell MIDIFILE.DLL to write out the file, calling my callback functions */
    result = MidiWriteFile(&mfs);

    /* Print out error message */
    MidiGetErr(&mfs, result, &buf[0]);
    printf(&buf[0]);

    exit(0);
}



/* ******************************** startMTrk() *****************************
 * This is called by MIDIFILE.DLL before it writes an MTrk chunk's header (ie, before it starts
 * writing out an MTrk). The MIDIFILE's TrackNum field is maintained by the DLL, and reflects the
 * MTrk number (ie, the first MTrk is 0, the second MTrk is 1, etc). We could use this to help us
 * "look up" data and structures associated with that track number. (It's best not to alter this
 * MIDIFILE field). At this point, we should NOT do any MidiReadBytes() because the header
 * hasn't been written yet. What we perhaps can do is write out some chunk of our own creation
 * before the MTrk gets written. We could do this with MidiWriteHeader(), MidiWriteBytes(), and
 * then MidiCloseChunk(). But, that's probably not a good idea. There are many programs that have
 * a very inflexible way of reading MIDI files. Such software might barf on a chunk that isn't an
 * MTrk, even though it's perfectly valid to insert such chunks. The DLL correctly handles such
 * situations. In fact, the CALLBACK allows you to specify a function for chunks that aren't MTrks.
 *    This should return 0 to let the DLL write the MTrk header, after which the DLL starts calling
 * our StandardEvt() callback (below) for each event that we wish to write to that MTrk chunk (up
 * to an End Of Track Meta-Event). Alternately, if we already had all of the MTrk data in a buffer
 * (minus the MTrk header), formatted and ready to be written out as an MTrk chunk, we could
 * place the size and a pointer to that buffer in the MIDIFILE's EventSize and Time fields
 * respectively, and return 0. In this case, the DLL will write out the MTrk chunk completely and
 * then call our StandardEvt() callback once.
 *	If this function returns non-zero, that will cause the write to abort and we'll return to
 * main() after the call to MidiWriteFile(). The non-zero value that we pass back here will be
 * returned from MidiWriteFile(). The only exception is a -1 which causes this MTrk to be
 * skipped, but not the write to be aborted (ie, the DLL will move on to the next MTrk number).
 ************************************************************************** */

LONG EXPENTRY startMTrk(MIDIFILE * mf)
{
    /* Display which MTrk chunk we're writing */
    printf("Writing track #%d...\r\n", mf->TrackNum);

    /* Initialize a global ptr to the start of the EVENTs that we're going to write in this MTrk */
    TrkPtr = trk_ptrs[mf->TrackNum];

    /* Here, we could write out some non-standard chunk of our own creation. It will end up
	in the MIDI file ahead of the MTrk that we're about to write. Nah! */

    /* Return 0, but we didn't place our own buffer ptr into MIDIFILE's Time field, so the DLL
	will call standardEvt() for each event. */
    return(0);
}



/* ****************************** store_seq() *******************************
 * Called by standardEvt() to format the MIDIFILE for writing out a SEQUENCE NUMBER (and
 * SEQUENCE NAME) Meta-Events
 ************************************************************************ */

VOID store_seq(SEQEVENT * trk, METASEQ * mf)
{
    mf->NamePtr = &names[mf->TrackNum][0];
    mf->SeqNum = trk->SeqNum;
}



/* ****************************** store_tempo() *******************************
 * Called by standardEvt() to format the MIDIFILE for writing out a TEMPO Meta-Event
 ************************************************************************ */

VOID store_tempo(TEMPOEVENT * trk, METATEMPO * mf)
{

    /* NOTE: We set MIDIBPM Flag so the DLL will calculate micros from this BPM.
	Without MIDIBPM, we'd have to set mf->Tempo to the micros, and ignore
	mf->TempoBPM. */
    mf->TempoBPM = trk->BPM;
}



/* ****************************** store_time() *******************************
 * Called by standardEvt() to format the MIDIFILE for writing out a TIME SIGNATURE Meta-Event
 ************************************************************************ */

VOID store_time(TIMEEVENT * trk, METATIME * mf)
{
    mf->Nom = trk->Nom;
    mf->Denom = trk->Denom;
    mf->Clocks = trk->Clocks;
    mf->_32nds = 8;   /* hard-wire this to 8 for my app. You might do it differently */
}



/* ****************************** store_key() *******************************
 * Called by standardEvt() to format the MIDIFILE for writing out a KEY SIGNATURE Meta-Event
 ************************************************************************ */

VOID store_key(KEYEVENT * trk, METAKEY * mf)
{
    mf->Key = trk->Key;
    mf->Minor = trk->Minor;
}



/* ****************************** store_meta() *******************************
 * Called by standardEvt() to format the MIDIFILE for writing out one of the variable length
 * Meta-Events (ie, types 0x01 to 0x0F, or 0x7F).
 ************************************************************************ */

VOID store_meta(TXTEVENT * trk, METATXT * mf)
{
    /* If a variable length Meta-Event (ie, type is 0x01 to 0x0F, or 0x7F), then we set
	the EventSize to the number of bytes that we expect to output. We also set the
	Ptr (ie, ULONG at Data[2]) either to a pointer to a buffer containing the data bytes
	to write, or 0. If we set a pointer, then when we return, the DLL writes the data,
	and then calls StandardEvt for the next event. If we set a 0 instead, when we return,
	the DLL will call our MetaText callback next, which is expected to MidiWriteBytes() that
	data. Here, we'll supply the pointer and let the DLL do all of the work of writing out
	the data. */

    /* Look up where I've stored the data for this meta-event (ie, in my text[] array), and
       store this pointer in the MIDIFILE. */
    mf->Ptr = &text[trk->Index][0];

    /* Set the event length. NOTE: If we set this to 0, then that tells the DLL that
	we're passing a null-terminated string, and the DLL uses strlen() to get the length.
	We could have done that here to really simplify our the structure of our TXTEVENT
	since all of our strings happen to be null-terminated. But, if you ever need to write
	out data strings with imbedded NULLs, you'll need to something like... */
    mf->EventSize = (ULONG)trk->Length;
}



/* ****************************** store_sysex() *******************************
 * Called by standardEvt() to format the MIDIFILE for writing out a SYSEX Meta-Events
 ************************************************************************ */

VOID store_sysex(XEVENT * trk, METATXT * mf)
{

    /* For SYSEX, just store the status, and the length of the message. We also set the
	the ULONG at Data[2], just like with variable length Meta-Events. (See comment
	above). Upon return, the DLL will write the supplied buffer, or if none supplied,
	call our SysexEvt callback which will MidiWriteBytes() the rest of the message.
	Here, let's do the opposite of what we did in store_meta(), just for the sake
	of illustration. The easier way is to let the DLL write out the SYSEX data, if that
	data happens to be in one buffer. */

    /* Set the length */
    mf->EventSize = (ULONG)trk->Length;

    /* Store index in a global for our sysexEvt callback. Alternately, we could put it
	into the UnUsed2 field of the METATXT (since it's a UCHAR), and retrieve the
	value in sysexEvt() */
    ex_index = trk->Index;

    /* Set Ptr (ie, ULONG at Data[2]) to 0. This ensures that the DLL calls sysexEvt instead
	of writing out a buffer for us */
    mf->Ptr = 0;
}



/* ****************************** standardEvt() *******************************
 * This is called by MIDIFILE.DLL for each event to be written to an MTrk chunk (or it's only
 * called once if we supplied a preformatted buffer in our startMTrk callback). This does the real
 * work of feeding each event to the DLL to write out within an MTrk.
 *     The MIDIFILE's TrackNum field is maintained by the DLL, and reflects the MTrk number.
 * We could use this to help us "look up" data associated with that track number.
 *     We need to place the event's time (referenced from 0 as opposed to the previous event
 * unless we set the MIDIDELTA Flag) in the MIDIFILE's Time field. When we return from here,
 * the DLL will write it out as a variable length quantity.
 *	Next, we need to place the Status in the MIDIFILE's Status field. For fixed length MIDI
 * events (ie, Status < 0xEF), then this is simply the MIDI Status byte. No running status. We
 * must provide the proper status if it's a fixed length MIDI message. Then, we place the 1 or 2
 * subsequent MIDI data bytes for this event in the MIDIFILE's Data[0] and Data[1]. Upon return,
 * the DLL will completely write out the event, and then call this function again for the next
 * event.
 *	For fixed length Meta-Events (ie, meta types of 0x00, 0x2F, 0x51, 0x54, 0x58, and
 * 0x59), the Status must be 0xFF, and Data[0] must be the meta type. The characters starting at
 * Data[1] (ie, length byte) must be the rest of the message. Upon return, the DLL will completely
 * write out the event, and then call this function again for the next event. Note that the DLL
 * automatically set the length for this fixed length Meta-Events, so you need not bother setting
 * Data[1]. For a Tempo Meta-Event, you have an option. Although Status=0xFF and Data[0]=0x51
 * always, instead of placing the micros per quarter as a ULONG starting at Data[2] (ie, Data[2],
 * Data[3], Data[4], and Data[5]), you can alternately place the tempo BPM in Data[6] and set the
 * MIDIFILE's Flags MIDIBPM bit. Note that if you recast the MIDIFILE as a METATEMPO, it
 * makes it easier to store the micros as a ULONG in the Tempo field, and BPM in the Tempo
 * field. The DLL will format the micros bytes for you. NOTE: When you return with an End Of
 * Track event (ie, Status=0xFF and Data[0]=0x2f), then the DLL will write out this event and
 * close the MTrk chunk. It then moves on to the next MTrk if there is another, calling startMTrk
 * again with the next TrackNum.
 *	For SYSEX events (ie, Status = 0xF0 and 0xF7), you set the status byte and then set
 * EventSize to how many bytes are in the message (not counting the status). You then can set
 * the ULONG at Data[2] (ie, Data[2], Data[3], Data[4], and Data[5]) to point to a buffer
 * containing the remaining bytes (after 0xF0 or 0xF7) to output. Alternately, you can set this
 * ULONG to 0. The DLL will write out the status and length as a variable length quantity. Then,
 * if you supplied the buffer pointer, it will write out the data. If you set the pointer to 0, then
 * the DLL will call your SysexEvt callback which is expected to MidiWriteBytes() the actual data
 * for this SYSEX message. Note that the SysexEvt could make numerous calls to MidiWriteBytes,
 * before it returns. The number of bytes written must be the same as the previously specified
 * EventSize.
 *	For variable length Meta-Events (ie, types of 0x01 through 0x0F, and 0x7F), you only set
 * the Status=0xFF, Data[0] = the meta type, and then set EventSize to how many bytes are in
 * the message (not counting the status and type). You can then set the ULONG at Data[2]. (See
 * the notes on SYSEX above). The DLL will write out the status, type, and length as a variable
 * length quantity, and then either the supplied buffer, or if no supplied buffer, the DLL will call
 * your MetaText callback which is expected to MidiWriteBytes() the actual data for this
 * Meta-Event.
 *	For MIDI REALTIME and SYSTEM COMMON messages (ie, 0xF1, 0xF2, 0xF3, 0xF6, 0xF8,
 * 0xFA, 0xFB, 0xFC, and 0xFE), simply set the MIDIFILE's Status to the appropriate MIDI Status,
 * and then store any subsequent data bytes (ie, 1 or 2) in Data[0] and Data[1]. Upon return, the
 * DLL will completely write out the event, and then call this function again for the next event.
 *	If this function returns non-zero, that will cause the write to abort and we'll return to
 * main() after the call to MidiWriteFile(). The non-zero value that we pass back here will be
 * returned from MidiWriteFile().
 ************************************************************************** */

LONG EXPENTRY standardEvt(MIDIFILE * mf)
{
    register UCHAR chr;
    register USHORT val;

    /* Store the Time of this event in MIDIFILE's Time field */
    mf->Time = TrkPtr->Time;

    /* Get the Status for this event */
    chr = mf->Status = TrkPtr->Status;

    /* Is this a Meta-Event? (ie, we use meta's Type for the status, and these values are always
	less than 0x80) */
    if ( !(chr & 0x80) )
    {
	 /* Set Status to 0xFF */
	 mf->Status = 0xFF;
	 /* Set Data[0] = Type */
	 mf->Data[0] = chr;
	 switch (chr)
	 {
	      /* NOTE: MIDIFILE.DLL sets Data[1] to the proper length for the fixed length
		  meta types (ie, 0x00, 0x2F, 0x51, 0x54, 0x58, and 0x59). */

	      /* ------- Sequence # ------- */
	      /* NOTE: If we use a MetaSeqNum callback, we wouldn't write out a Meta-Event
		 here, and so this case wouldn't be needed. */
	      case 0x00:
		   /* Note the recasting of the EVENT and the MIDIFILE structures to the versions
		       appropriate for a SEQUENCE NUMBER Meta-Event. */
		   store_seq( (SEQEVENT *)TrkPtr, (METASEQ *)mf );
		   break;

	      /* ------- Set Tempo -------- */
	      case 0x51:
		   store_tempo( (TEMPOEVENT *)TrkPtr, (METATEMPO *)mf );
		   break;

	      /* --------- SMPTE --------- */
	      case 0x54:
		   /* Right now, let's ignore SMPTE events. This was too much of a hassle
		       since there are too many data bytes to fit into the 8 bytes that I use to
		       express events internally in this program. */
		   break;

	      /* ------- End of Track ------ */
	      case 0x2F:
		   printf("Closing track #%ld...\r\n", mf->TrackNum);
		   break;

	      /* ------ Time Signature ----- */
	      case 0x58:
		   store_time( (TIMEEVENT *)TrkPtr, (METATIME *)mf );
		   break;

	      /* ------ Key Signature ------ */
	      case 0x59:
		   store_key( (KEYEVENT *)TrkPtr, (METAKEY *)mf );
		   break;

	      /* Must be a variable length Meta-Event (ie, type is 0x01 to 0x0F, or 0x7F) */
	      default:
		   store_meta( (TXTEVENT *)TrkPtr, (METATXT *)mf );
	 }
    }

    /* Must be a real MIDI event (as opposed to a MetaEvent) */
    else switch ( chr )
    {
	 /* SYSEX (0xF0) or SYSEX CONTINUATION (0xF7) */
	 case 0xF0:
	 case 0xF7:
	      store_sysex( (XEVENT *)TrkPtr, (METATXT *)mf );
	      break;

	 /* For other MIDI messages, they're all fixed length, and will fit into the MIDIFILE
	     structure, so we copy 2 more data bytes to the MIDIFILE (whether those data bytes
	     are valid or not -- the DLL takes care of writing the message out properly). */
	 default:
	      mf->Data[0] = TrkPtr->Data1;
	      mf->Data[1] = TrkPtr->Data2;
    }

    /* Advance the ptr to the next event (ie, for the next call to this function) */
    TrkPtr++;

    return(0);
}



/* ******************************** sysexEvt() ********************************
 * This is called by MIDIFILE.DLL if it needs me to write out the data bytes for a SYSEX
 * message being written to an MTrk chunk. If the DLL called me, then standardEvt must have
 * initiated writing a SYSEX event to an MTrk, but didn't supply a pointer to a buffer filled with
 * data. Now, I need to write those data bytes here. NOTE: EventSize still contains the length of
 * data to write.
 ************************************************************************** */

LONG EXPENTRY sysexEvt(MIDIFILE * mf)
{
    register ULONG i;
    register UCHAR * ptr;
    register LONG result;

    /* Look up where I put the data for the SYSEX event that we're writing right now */
    ptr = &sysex[ex_index][0];

    /* Here's an example of writing the data one byte at a time. This is slow, but it shows that
	you can make multiple calls to MidiWriteBytes(). NOTE: commented out; just for
	illustration. Also note that MidiWriteBytes() decrements EventSize, so we could have
	done: while( mf->EventSize )  */
/*    for (i = mf->EventSize; i; i--)
       {
	      if ( (result = MidiWriteBytes(mf, ptr, 1)) ) return(result);
	      ptr++;
       }
       return(0);
*/

    /* Since we happen to have all of the bytes in one buffer, the fast way is to make one call
	to MidiWriteBytes() for that block. */
    return( MidiWriteBytes(mf, ptr, mf->EventSize) );
}



/* ******************************* metaseq() *********************************
 * This is called by MIDIFILE.DLL after it has called startMTrk (ie, the MTrk header has been
 * written) but before standardEvt gets called. In other words, this gives us a chance to write
 * the first event in an MTrk. Usually, this is the Sequence Number event, which MUST be first.
 * If we had a Sequence Number event at the start of this example's internal data, we wouldn't
 * need this, since standardEvt would write that out. But, this gives us an opportunity to
 * illustrate how to write such an event now. We simply format the METASEQ for the DLL to write
 * a Seq Number event followed by a Track Name event.
 *     If we wished to write other events at the head of the MTrk, but which aren't actually
 * events in our example track, we would need to make calls to MidiWriteEvt(). We would have to
 * do this for the sequence number since that event must come first in an MTrk. Then, we would
 * do MidiWriteEvt for all of our other desired "contrived events", except for the last one. We
 * must always return at least one event in the MIDIFILE structure.
 *     NOTE: MIDIFILE's Time field is 0 upon entry. Do not write out any events with a non-zero
 * Time here. Do that in standardEvt().
 ************************************************************************** */

LONG EXPENTRY metaseq(METASEQ * mf)
{
    LONG val;
    MIDIFILE * mf2;

    mf->Type = 0xFF;
    mf->WriteType = 0x00;

    /* Arbitrarily set Seq # to 10 + TrackNum */
    mf->SeqNum = 10+mf->TrackNum;

    /* Set NamePtr to point to the null-terminated Track Name to write as a Meta-Event.
	Alternately, we could set this to 0 if we don't want a Name event to be written (or if
	we already have an explicit Name event within our track data, to be written out in standardEvt) */
    mf->NamePtr = &names[mf->TrackNum][0];

    /* This code shows how we might write additional events. Commented out. */

    /* Write the sequence num and name events */
/*    if ( (val = MidiWriteEvt((MIDIFILE *)mf)) ) return(val); */

    /* Fool compiler into regarding the METASEQ as a MIDIFILE, which it really is */
/*    mf2 = (MIDIFILE *)mf; */

    /* We could write out more events here */

    /* Let DLL write the last of these events at Time of 0. In this case, a text event */
/*    mf->Status = 0xFF; */
/*    mf2->Data[0] = 0x01; */
/*    mf2->EventSize = 0; */
/*    set_ptr((ULONG *)&mf2->Data[2], "More text"); */

    /* Success */
    return(0);
}



/* ******************************* make_extra() *********************************
 * This is called by MIDIFILE.DLL after all of the MTrk chunks have been successfully written out.
 * We could use this to write a proprietary chunk not defined by the MIDI File spec (although a
 * better way to store proprietary info is within a MTrk with the Proprietary Meta-Event since
 * badly written MIDI file readers might barf on an undefined chunk -- MIDIFILE.DLL doesn't do
 * that). Just for the sake of illustration, we'll write out a chunk with the ID of "XTRA" and
 * it will contain 10 bytes, numbers 0 to 9.
 ************************************************************************** */

LONG EXPENTRY make_extra(MIDIFILE * mf)
{
    LONG val;
    USHORT i;
    CHAR chr;

    /* Set the ID. Note that the ULONG contains the 4 ascii chars of 'X', 'T', 'R', and 'A',
	except that the order has been reversed to 'A', 'R', 'T', and 'X'. This reversal is due to
	the backwards byte order of the Intel CPU when storing longs. */
    mf->ID = 0x41525458;

    /* Set the ChunkSize to 0 initially. We write an empty header, then after we write out all
	of the bytes, MidiCloseChunk will automatically set the proper ChunkSize. */
    mf->ChunkSize=0;

    /* Write the header */
    if ( (val = MidiWriteHeader(mf)) ) return(val);

    /* Write the data bytes */
    for (i=0; i<10; i++)
    {
	 chr = i+'0';
	 if ( (val = MidiWriteBytes(mf, &chr, 1)) ) return(val);
    }

    /* Close the chunk */
    MidiCloseChunk(mf);

    /* Of course, we could write out more chunks here */

    /* Success */
    return(0);
}



/* ******************************** initfuncs() ********************************
 * This initializes the CALLBACK structure containing pointers to functions. These are used by
 * MIDIFILE.DLL to call our callback routines while reading the MIDI file. It also initializes a few
 * fields of MIDIFILE structure.
 ************************************************************************** */

VOID initfuncs(CHAR * fn)
{
    /* Place CALLBACK into MIDIFILE */
    mfs.Callbacks = &cb;

    /* Let the DLL Open, Read, Seek, and Close the MIDI file */
    cb.OpenMidi = 0;
    cb.ReadWriteMidi = 0;
    cb.SeekMidi = 0;
    cb.CloseMidi = 0;
    mfs.Handle = (ULONG)fn;

    cb.UnknownChunk = make_extra;
    cb.StartMThd = 0;	 /* Don't need this since I initialized the MThd before MidiWriteFile() */
    cb.StartMTrk = startMTrk;
    cb.StandardEvt = standardEvt;
    cb.SysexEvt = sysexEvt;
    cb.MetaSeqNum = metaseq;
    cb.MetaText = 0;	/* Not needed since we supply the data buffer to the DLL in standardEvt() */
}
