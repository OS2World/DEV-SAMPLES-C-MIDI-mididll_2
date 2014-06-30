/* ============================== MIDIFILE.H =================================
 *  C Include file that contains definitions of various structures/constants found within a MIDI
 *  file, and structures/constants used by the MIDIFILE.DLL to read/write MIDI files.
 ========================================================================== */


/* ===========================================================================
    CALLBACK structure -- allocated and initialized by an app, and passed to the MIDIFILE.DLL
    within the MIDIFILE structure. This contains pointers to various functions within the app to
    handle processing of parts of a MIDI file. The MIDIFILE.DLL calls one of those functions when
    handling its respective part of a MIDI file.
 */

typedef long ( EXPENTRY _CALL) ();
typedef _CALL *CALL;

typedef struct _CALLBACK
{
  CALL OpenMidi,   /* Opens MIDI file for reading or writing. If 0, then it's assumed that the
			 MIDIFILE.DLL will take care of opening the file, and the MIDIFILE
			 structure's Handle field contains a pointer to the filename to open. */
  ReadWriteMidi,   /* If reading, this reads the specified number of bytes into the specified
			 buffer. If writing, this writes the specified number of bytes within the
			 specified buffer. If 0, MIDIFILE.DLL takes care of reading/writing bytes.
		      */
  SeekMidi,	    /* Seeks forward/backward the specified number of bytes from current position */
  CloseMidi,	    /* Closes MIDI file. If 0, MIDIFILE.DLL takes care of closing the file. */
  StartMThd,	    /* If reading, called upon load of an Mthd header. If writing, initializes the
			Format, NumTracks, and Division fields of MIDIFILE struct (after which
			MIDIFILE.DLL then writes out the Mthd chunk).
		      */
  StartMTrk,	    /* If reading, called upon load of an Mtrk header. If writing, the app can use
			this to write app-specific chunks before each MTrk, and set up global
			variables for a particular track. Alternately, the app can use this to have
			the DLL write out a pre-formatted MTrk chunk.
		      */
  UnknownChunk,   /* If reading, called upon load of an unknown header. If writing, the app
			 should write out the desired app-specific chunk(s) using the DLL's
			 MidiWriteHeader(), 1 or more calls to MidiWriteBytes(), and finally
			 MidiCloseChunk(). Such chunks appear after all of the MTrks.
		      */
  MetaText,	    /* If reading, loads in and processes text-based Meta-Event via
			 MidiReadBytes(). If writing, writes out the remainder of the Meta-event.
			 NOTE: Proprietary Meta-Events (type=0x7F) as well as unknown
			 Meta-events are also read/written via this function.
		      */
  SysexEvt,	    /* For reading, loads in and processes the remainder of the SYSEX event via
			 MidiReadBytes(). For writing, writes out the remainder of the SYSEX
			 event via MidiWriteBytes(). */
  StandardEvt,	    /* If reading, processes loaded MIDI event with status of 0x80 to 0xEF. If
			 writing 1 event at a time, reports back to the DLL what event to write.
			 First, it sets the MIDIFILE struct's Time field to this event's time (from
			 0). Then, for a MIDI event with status of 0x80 to 0xEF, 0xF1, 0xF2,
			 0xF3, 0xF6, 0xF8, 0xFA, 0xFB, 0xFC, or 0xFE, stores that status in
			 MIDIFILE's Status and the remaining 1 or 2 MIDI data bytes in the Data[0]
			 and Data[1] fields. For fixed length Meta-Events (ie, meta type of 0x00,
			 0x2F, 0x51, 0x54, 0x58, and 0x59), the Status must be 0xFF, and Data[0]
			 must be the meta type. The characters starting at Data[1] must be the rest
			 of the message. For example, with the Key Signature event, Data[1] would
			 be the length (0x02), Data[2] would be the Key, and Data[3] would be the
			 Minor setting. Note that the DLL always sets the length (ie, Data[1]) for
			 fixed length Meta-Events. For a Tempo Meta-Event, you have an option.
			 Instead of placing the 3 micros bytes in Data[2], Data[3], and Data[4], you
			 can alternately place the tempo BPM in Data[2] and set the MIDIFILE's
			 Flags MIDIBPM bit. The DLL will format the micros bytes for you.
			 If the event to be written is a SYSEX, set Status to 0xF0 or 0xF7, set
			 EventSize to the bytes to write (not counting 0xF0), set Time to event's
			 time (from 0), and then set a pointer to the data buffer to write in
			 Data[2] (ie, ULONG). If the ULONG starting at Data[2] is 0, then the DLL
			 will instead call the SysexEvt callback to write out the remaining bytes.
			 If the event to be written is a variable length Meta-Event, set Status to
			 0xFF, Data[0] is the desired meta type, EventSize to the bytes to
			 write (not counting 0xFF and Type), set Time to event's time (from 0),
			 then set a pointer to the data buffer to write in Data[2] (ie, ULONG).
			 If the ULONG starting at Data[2] is 0, then the DLL will instead call the
			 MetaText callback to write out the remaining bytes.
		     */
  MetaSeqNum,	   /* If reading, processes loaded Sequence Number Meta-Event. If writing,
			  returns the METASEQ formatted for a Sequence Number event. NOTE:
			  This is the first "event" processing function called when writing out an
			  Mtrk chunk, 1 event at a time (ie, it's called one time; before any calls
			  to StandardEvt). If NamePtr field is not 0, then this is assumed to be
			  a null-terminated string which will be written out as a Track name
			  Meta-Event.
		      */
  MetaTimeSig,	   /* Read only. Processes loaded Time Signature Meta-Event */
  MetaKeySig,	    /* Read only. Processes loaded Key Signature Meta-Event */
  MetaTempo,	   /* Read only. Processes loaded Tempo Meta-Event */
  MetaSMPTE,	  /* Read only. Processes loaded SMPTE Meta-Event */
  MetaEOT;	    /* Read only. Processes loaded End Of Track Meta-Event */
} CALLBACK;


/* ============================================================================
   MIDIFILE structure -- allocated by an app, and passed to the MIDIFILE.DLL in order for the DLL
   to help the app read/write MIDI files). This is also passed to several callbacks.
 */

typedef struct _MIDIFILE
{
 CALLBACK * Callbacks; /* Pointer to the Callbacks structure */
 ULONG	 Handle;      /* File Handle of the open MIDI file */
 LONG	 FileSize;    /* Size of the file. Initially, when the Mthd header is read in, this reflects
			    the number of data bytes in the remainder of the file (ie, after the 8
			    byte Mthd header -- starting at the Mthd's Format). As bytes are read
			    in, this is decremented to reflect how many bytes remain in the file.
			    For writes, this reflects how many bytes the DLL has written out (ie,
			    size of the MIDI file written by the DLL).
			 */
 ULONG	 ID;	      /* ID of the chunk. */
 LONG	 ChunkSize;  /* Size of the chunk (Mtrk or Mthd) currently being read. Initially, when
			    the chunk header is read in, this would be the size as specified in the
			    header. As bytes are read in, this is decremented. For writes, this is
			    used for other purposes by the DLL.
			 */
 USHORT Format;     /* From Mthd */
 USHORT NumTracks; /* From Mthd */
 USHORT Division;    /* From Mthd */
 USHORT Flags;	     /* Flag bits */
 LONG	 EventSize;  /* The current event's size. This will be non-zero only if the event is
			    a SYSEX or Meta-Event of variable length. As bytes are read in, this is
			    decremented. For writes, this is used to specify the length of variable
			    length Meta-Events and SYSEX. */
 ULONG	PrevTime;   /* Maintained by DLL. */
 ULONG	Time;	     /* The current event's time, referenced from 0 (instead of from the previous
			    event's time as is done with delta-times in the MIDI file) unless
			    MIDIDELTA flag is set.
			 */
 UCHAR	TrackNum;   /* The track # that the current event belongs to */
 UCHAR	Status;       /* The event's status. For MIDI events, this is the actual MIDI Status.
			     0xFF for Meta-Event. 0xF7 was sysex continuation.
			 */
 UCHAR	Data[7];      /* For MIDI events (other than SYSEX), this will contain the 2 subsequent
			    MIDI data bytes. If there's only 1 data byte, the second byte will be
			    0xFF.
			    For Meta-Events, the first byte will be the Type. For Meta-Events that
			    have a fixed length (ie, Sequence Number, End of Track, Tempo. SMPTE,
			    Time Signature, and Key Signature), the rest of the bytes will be the
			    remainder of that Meta-Event.
			    In other words, the DLL automatically loads MIDI events (except SYSEX)
			    and fixed length Meta-Events directly into the MIDIFILE structure. For
			    Meta-Events of variable length, and SYSEX messages, the DLL loads the
			    Status (0xFF if Meta-Event) and Type (if a Meta-Event), and then loads
			    the variable length quantity bytes that form the Length of the event,
			    setting EventSize to this value. It's up to the app to then load or skip
			    the rest of the event's bytes.
			  */
  UCHAR RunStatus;     /* Maintained by DLL */
} MIDIFILE;


/* MIDIFILE Flags */
#define MIDIWRITE 0x8000 /* Set if callback was called during MidiWriteFile() instead of MidiReadFile() */
#define MIDIBPM   0x4000 /* Set if writing a Tempo Meta-Event, where Data[2] is BPM and
				  DLL calculates micros per quarter */
#define MIDISYSEX 0x2000 /* Set by the DLL when a 0xF0 event is encountered. The DLL doesn't
				  clear this until it encounters a MIDI event with status of 0x80
				  to 0xEF or SYSTEM COMMON. Use this when reading via your SysexEvt
				  callback to help you distinguish SYSEX continuation events from
				  ESCAPED events, both of which have a 0xF7 Status. If an ESCAPED
				  event, the first data byte that you MidiReadBytes() in your
				  SysexEvt callback should be a MIDI REALTIME or SYSTEM
				  COMMON status (bit #7 set). A continuation event can ONLY occur
				  when this flag is set, and it's very unlikely that the first data byte
				  will have bit #7 set (unless it happens to also be 0xF7, unlikely). */
#define MIDIDENOM 0x1000 /* When reading/writing a Time Signature Meta-Event, the
				   denominator isn't expressed as a power of 2, but rather, as the
				   real time sig denominator. In other words for 4/4, instead of a
				   Denom of 2, it's 4. */
#define MIDIDELTA 0x0800 /* Time field is delta instead of referenced from 0 */
#define MIDIREALTIME 0x0400 /* Set this if you want MIDI REALTIME to not cancel running
				      status. This will yield more compression if you're saving
				      REALTIME events (ie, via an ESCAPE event) intermixed with
				      regular MIDI messages. But, some software doesn't follow the
				      MIDI spec with regard to REALTIME not affecting running
				      status, so this is made optional. */
#define MIDIDIRTY 0x0200 /* Don't alter this if not using your own ReadWriteMidi callback.
				     The DLL uses it for an intelligent File I/O buffering */

/* ============================================================================
   METATEMPO structure -- Passed by DLL to the app's MetaTempo callback. Most of the fields
   are the same as the app's MIDIFILE structure with a few, noted exceptions. This is just a
   redefinition of the MIDIFILE specifically for Tempo Meta-Events in order to fool the compiler
   into seeing certain fields that were defined as UCHAR in MIDIFILE as ULONG in METATEMPO.
   Used for both reading and writing.
 */

typedef struct _METATEMPO
{
 CALLBACK * Callbacks;
 ULONG	 Handle;
 LONG	 FileSize;
 ULONG	 ID;
 LONG	 ChunkSize;
 USHORT Format;
 USHORT NumTracks;
 USHORT Division;
 USHORT Flags;
 ULONG	 UnUsed1;   /* NOTE: EventSize not used for read/write of Tempo events */
 ULONG	PrevTime;
 ULONG	Time;
 UCHAR	TrackNum;
 UCHAR	Type;	    /* Meta Type (ie, 0x51) for reads. 0xFF for writes */
 UCHAR	WriteType;  /* Length (3) for reads. 0x51 for writes. */
 UCHAR	Length;      /* Don't use */
 ULONG	Tempo;	    /* Tempo in micros per quarter note */
 UCHAR	TempoBPM; /* Tempo in Beats Per Minute */
 UCHAR	RunStatus;
} METATEMPO;



/* ============================================================================
   METASEQ structure -- Passed by DLL to the app's MetaSeqNum callback. This is used by both
   reading and writing.
 */

typedef struct _METASEQ
{
 CALLBACK * Callbacks;
 ULONG	 Handle;
 LONG	 FileSize;
 ULONG	 ID;
 LONG	 ChunkSize;
 USHORT Format;
 USHORT NumTracks;
 USHORT Division;
 USHORT Flags;
 UCHAR * NamePtr;   /* For writing, ptr to the null-terminated Track Name to write as a
			    Meta-Event following the Sequence Number Meta-Event, or 0 if none */
 ULONG	PrevTime;
 ULONG	Time;
 UCHAR	TrackNum;
 UCHAR	Type;	     /* Meta type (0x00) for reads. 0xFF for writes */
 UCHAR	WriteType;  /* Length (2) for reads. Meta type (0x00) for writes */
 UCHAR	Length;      /* Don't use */
 USHORT SeqNum;    /* Sequence number */
 UCHAR	UnUsed2, UnUsed3, UnUsed4;
 UCHAR	RunStatus;
} METASEQ;



/* ============================================================================
   METASMPTE structure -- Passed by DLL to the app's MetaSMPTE callback.
 */

typedef struct _METASMPTE
{
 CALLBACK * Callbacks;
 ULONG	 Handle;
 LONG	 FileSize;
 ULONG	 ID;
 LONG	 ChunkSize;
 USHORT Format;
 USHORT NumTracks;
 USHORT Division;
 USHORT Flags;
 ULONG	 UnUsed1;
 ULONG	 PrevTime;
 ULONG	Time;
 UCHAR	TrackNum;
 UCHAR	Type;	    /* Meta Type 0x54 for reads. 0xFF for writes. */
 UCHAR	WriteType; /* Length (5) for reads. Meta type 0x54 for writes */
 UCHAR	Length;     /* Don't use */
 UCHAR	Hours;	    /* SMPTE Hours */
 UCHAR	Minutes;    /* SMPTE Minutes */
 UCHAR	Seconds;   /* SMPTE Secs */
 UCHAR	Frames;    /* SMPTE Frames */
 UCHAR	SubFrames; /* SMPTE SubFrames */
 UCHAR	RunStatus;
} METASMPTE;



/* ============================================================================
   METATIME structure -- Passed by DLL to the app's MetaTimeSig callback.
 */

typedef struct _METATIME
{
 CALLBACK * Callbacks;
 ULONG	 Handle;
 LONG	 FileSize;
 ULONG	 ID;
 LONG	 ChunkSize;
 USHORT Format;
 USHORT NumTracks;
 USHORT Division;
 USHORT Flags;
 LONG	 UnUsed1;
 ULONG	PrevTime;
 ULONG	Time;
 UCHAR	TrackNum;
 UCHAR	Type;	    /* Meta Type 0x58 for reads. 0xFF for writes */
 UCHAR	WriteType;  /* Length (4) for reads. Meta Type 0x58 for writes */
 UCHAR	Length;      /* Don't use */
 UCHAR	Nom;	    /* Time sig nominator */
 UCHAR	Denom;	   /* Time sig denominator */
 UCHAR	Clocks;     /* MIDI clocks in metronome click */
 UCHAR	_32nds;    /* number of 32nd notes in 24 MIDI clocks */
 UCHAR	UnUsed2;
 UCHAR	RunStatus;
} METATIME;



/* ============================================================================
   METAKEY structure -- Passed by DLL to the app's MetaKeySig callback.
 */

typedef struct _METAKEY
{
 CALLBACK * Callbacks;
 ULONG	 Handle;
 LONG	 FileSize;
 ULONG	 ID;
 LONG	 ChunkSize;
 USHORT Format;
 USHORT NumTracks;
 USHORT Division;
 USHORT Flags;
 LONG	 UnUsed1;
 ULONG	PrevTime;
 ULONG	Time;
 UCHAR	TrackNum;
 UCHAR	Type;	    /* For reads, Meta Type 0x59. 0xFF for writes */
 UCHAR	WriteType; /* Length (2) for reads. meta type 0x59 for writes */
 UCHAR	Length;     /* Don't use */
 CHAR	 Key;	     /* -7 for 7 flats... 0 for C... 7 for 7 sharps */
 UCHAR	Minor;	    /* 0=Major, 1=Minor */
 UCHAR	UnUsed2, UnUsed3, UnUsed4;
 UCHAR	RunStatus;
} METAKEY;



/* ============================================================================
   METAEND structure -- Passed by DLL to the app's MetaEOT callback.
 */

typedef struct _METAEND
{
 CALLBACK * Callbacks;
 ULONG	 Handle;
 LONG	 FileSize;
 ULONG	 ID;
 LONG	 ChunkSize;
 USHORT Format;
 USHORT NumTracks;
 USHORT Division;
 USHORT Flags;
 LONG	 UnUsed1;
 ULONG	PrevTime;
 ULONG	Time;
 UCHAR	TrackNum;
 UCHAR	Type;	    /* For reads, Meta Type 0x2F. 0xFF for writes */
 UCHAR	WriteType; /* Length (0) for reads. Meta type 0x2F for writes */
 UCHAR	Length;     /* Don't use */
 UCHAR	UnUsed2, UnUsed3, UnUsed4, UnUsed5, UnUsed6;
 UCHAR	RunStatus;
} METAEND;


/* ============================================================================
   METATXT structure -- Used by app when formatting SYSEX and MetaText events for writing.
 */

typedef struct _METATXT
{
 CALLBACK * Callbacks;
 ULONG	 Handle;
 LONG	 FileSize;
 ULONG	 ID;
 LONG	 ChunkSize;
 USHORT Format;
 USHORT NumTracks;
 USHORT Division;
 USHORT Flags;
 ULONG	EventSize;  /* Size of buffer to write out */
 ULONG	PrevTime;
 ULONG	Time;
 UCHAR	TrackNum;
 UCHAR	Type;	    /* 0xFF for MetaText, 0xF0 or 0xF7 for SYSEX */
 UCHAR	WriteType;  /* MetaType for MetaText events. */
 UCHAR	Length;    /* Don't use */
 UCHAR * Ptr;	     /* Pointer to buffer to write out */
 UCHAR	UnUsed2;
 UCHAR	RunStatus;
} METATXT;



/* =========================================================================
 * Errors returned by the DLL's MidiReadFile() and MidiWriteFile()
 */

#define MIDIERRFILE    1  /* Can't open the MIDI file for reading/writing */
#define MIDIERRINFO    2  /* Can't determine the FileSize for reading */
#define MIDIERRNOMIDI 3  /* Tried to read a file that didn't contain a required MThd */
#define MIDIERRREAD   4  /* An error while reading bytes from the file */
#define MIDIERRWRITE  5  /* An error while writing bytes to the file */
#define MIDIERRBAD     6  /* A mal-formed MIDI file -- it's garbage */
#define MIDIERRSTATUS 7  /* Encountered running status where it shouldn't be. (mal-formed MTrk) */
#define MIDIERREVENT   8  /* Encountered an unknown Status while reading in an MTrk.
				   Not a Meta-Event, but not a MIDI event either. */
#define MIDIAPPERR     9   /* App defined errors start with this if positive, or can be
				    negative numbers (except for -1 for ReadWriteMidi callback) */


/* ==========================================================================
 * The MIDIFILE.DLL functions
 */

 /* reading */
extern LONG EXPENTRY MidiReadFile(MIDIFILE * mf);
extern LONG EXPENTRY MidiReadBytes(MIDIFILE * mf, UCHAR * buf, ULONG count);
extern VOID EXPENTRY MidiSkipChunk(MIDIFILE * mf);
extern VOID EXPENTRY MidiSkipEvent(MIDIFILE * mf);
extern LONG EXPENTRY MidiReadVLQ(MIDIFILE * mf);
extern LONG EXPENTRY MidiReadHeader(MIDIFILE * mf);

 /* writing */
extern LONG EXPENTRY MidiWriteBytes(MIDIFILE * mf, UCHAR * buf, ULONG count);
extern LONG EXPENTRY MidiWriteVLQ(MIDIFILE * mf, ULONG val);
extern LONG EXPENTRY MidiWriteHeader(MIDIFILE * mf);
extern LONG EXPENTRY MidiWriteFile(MIDIFILE * mf);
extern LONG EXPENTRY MidiCloseChunk(MIDIFILE * mf);
extern LONG EXPENTRY MidiWriteEvt(MIDIFILE * mf);

 /* misc */
extern VOID EXPENTRY MidiSeek(MIDIFILE * mf, LONG amt);
extern LONG EXPENTRY MidiFileSize(MIDIFILE * mf);
extern VOID EXPENTRY MidiFlipLong(UCHAR * ptr);
extern VOID EXPENTRY MidiFlipShort(UCHAR * ptr);
extern BOOL EXPENTRY MidiCompareID(UCHAR * id, UCHAR * ptr);
extern VOID EXPENTRY MidiCloseFile(MIDIFILE * mf);
extern LONG EXPENTRY MidiVLQToLong(UCHAR * ptr, ULONG * len);
extern ULONG EXPENTRY MidiLongToVLQ(ULONG val, UCHAR * ptr);
extern ULONG EXPENTRY MidiGetErr(MIDIFILE * mf, LONG err, UCHAR * buf);

