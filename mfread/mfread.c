/* ===========================================================================
 * mfread.c
 *
 * Reads in and displays information about a MIDI file, including all events in an Mtrk chunk.
 * =========================================================================
 */

#include <os2.h>
#include <stdio.h>
#include "midifile.h"

/* Uncomment this define if you want standard C buffered file I/O */
/* #define BUFFERED_IO 1 */

/* function definitions */
VOID initfuncs(CHAR * fn);

/* Need a CALLBACK structure for the DLL */
CALLBACK cb;

/* Need a MIDIFILE structure for the DLL */
MIDIFILE mfs;

#ifdef BUFFERED_IO
CHAR * fname;
#endif

/* A text description of the various text-based Meta-Events (types 0x01 to 0x0F) */
CHAR types[8][14] = {
    "Unknown Text",
    "Text event  ",
    "Copyright   ",
    "Track Name  ",
    "Instrument  ",
    "Lyric       ",
    "Marker      ",
    "Cue Point   ",
};

UCHAR keys[15][3] = { "Cb", "Gb", "Db", "Ab", "Eb", "Bb", " F", " C", " G", " D", " A", " E", " B", "F#", "C#" };

ULONG packet;

UCHAR compress=0;

ULONG counts[23];

UCHAR * strptrs[] = { "Note Off", "Note On", "Aftertouch", "Controller", "Program", "Channel Pressure",
			"Pitch Wheel", "System Exclusive", "Escaped", "Unknown Text",
			"Text", "Copyright", "Track Name", "Instrument Name", "Lyric",
			"Marker", "Cue Point", "Proprietary", "Unknown Meta", "Key Signature",
			"Tempo", "Time Signature", "SMPTE" };




/*********************************** main() ***********************************
 * Program entry point. Calls the MIDIFILE.DLL function to read in a MIDI file. The callbacks do
 * all of the real work of displaying info about the file.
 ****************************************************************************/

main(int argc, char *argv[], char *envp[])
{
    LONG result;
    UCHAR buf[60];

    /* If no filename arg supplied by user, exit with usage info */
    if ( argc < 2 )
    {
	 printf("This program displays the contents of a MIDI (sequencer)\n");
	 printf("file. It requires MIDIFILE.DLL to run.\n\n");
	 printf("Syntax: MFREAD.EXE [filename] /I\n");
	 printf("    where /I means list info about the MTrk but not each event\r\n");
	 exit(1);
    }

    /* See if he wants compressed listing */
    if (argc > 2)
    {
	 if (strncmp(argv[2],"/I",2) ||strncmp(argv[2],"/i",2) )
	 {
	      compress=1;
	 }
    }

    /* Initialize the pointers to our callback functions (ie, for the MIDIFILE.DLL to call) */
    initfuncs(argv[1]);

    /* Set the Flags */
    mfs.Flags = MIDIDENOM;

    /* Tell MIDIFILE.DLL to read in the file, calling my callback functions */
    result = MidiReadFile(&mfs);

    /* Print out error message. NOTE: For 0, DLL returns a "Successful MIDI file load" message.
	Also note that DLL returns the length even though we ignore it. */
    MidiGetErr(&mfs, result, &buf[0]);
    printf(&buf[0]);

    exit(0);
}




/********************************** prtime() *********************************
 * Prints out the time that the event occurs upon. This time is referenced from 0 (ie, as opposed
 * to the previous event in the track as is done with delta-times in the MIDI file). The MIDIFILE
 * DLL automatically maintains the MIDIFILE's Time field, updating it for the current event.
 **************************************************************************/

VOID prtime(MIDIFILE * mf)
{
    if (!compress)
	printf("%8ld |",mf->Time);
}




/******************************** startMThd() ********************************
 * This is called by MIDIFILE.DLL when it encounters the start of an MThd chunk (ie, at the head
 * of the MIDI file). We simply print out loaded info from that header. At this point, the valid
 * MIDIFILE fields set by the DLL include Handle, FileSize, Format, NumTracks, Division,
 * ChunkSize (minus the 6 loaded bytes of a standard MThd), and TrackNum=-1 (ie, we haven't
 * encountered an MTrk yet).
 **************************************************************************/

LONG EXPENTRY startMThd(MIDIFILE * mf)
{
    /* Print the MThd info. */
    printf("MThd Format=%d, # of Tracks=%d, Division=%d\r\n",mf->Format,mf->NumTracks,mf->Division);

    /* Return 0 to indicate no error */
    return(0);
}




/********************************* startMTrk() ********************************
 * This is called by MIDIFILE.DLL when it encounters the start of an Mtrk chunk. We simply print
 * out a message that we encountered an Mtrk start, and the track number. We return 0 to let the
 * DLL load/process the MTrk 1 event at a time.
 **************************************************************************/

LONG EXPENTRY startMTrk(MIDIFILE * mf)
{
    USHORT i;

    /* Print heading */
    printf("\r\n==================== Track #%d ====================\r\n   %s      Event\r\n", mf->TrackNum, (compress) ? "Total" : "Time" );

    /* If compressing info on display, init array for this track */
    if (compress)
    {
	 for (i=0; i<23; i++)
	 {
	      counts[i]=0;
	 }
    }

    return(0);
}




/******************************* standardEvt() *********************************
 * This is called by MIDIFILE.DLL when it encounters a MIDI message of Status < 0xF0 within
 * an Mtrk chunk. We simply print out info about that event.
 ***************************************************************************/

LONG EXPENTRY standardEvt(MIDIFILE * mf)
{
    /* Get MIDI channel for this event */
    UCHAR chan = mf->Status & 0x0F;

    if (!compress)
    {
	 /* Print out the event's time */
	 prtime((MIDIFILE *)mf);

	 switch ( mf->Status & 0xF0 )
	 {
	      case 0x80:
		   printf("Note off    | chan=%2d   | pitch= %-3d | vol=%d\r\n", chan, mf->Data[0], mf->Data[1]);
		   break;

	      case 0x90:
		   printf("Note on     | chan=%2d   | pitch= %-3d | vol=%d\r\n", chan, mf->Data[0], mf->Data[1]);
		   break;

	      case 0xA0:
		   printf("Aftertouch  | chan=%2d   | pitch= %-3d | press=%d\r\n", chan, mf->Data[0], mf->Data[1]);
		   break;

	      case 0xB0:
		   printf("Controller  | chan=%2d   | contr #%-3d | value=%d\r\n", chan, mf->Data[0], mf->Data[1]);
		   break;

	      case 0xC0:
		   printf("Program     | chan=%2d   | pgm #=%d\r\n", chan, mf->Data[0]);
		   break;

	      case 0xD0:
		   printf("Poly Press  | chan=%2d   | press= %-3d\r\n", chan, mf->Data[0]);
		   break;

	      case 0xE0:
		   printf("Pitch Wheel | chan=%2d   | LSB=%d MSB=%d\r\n", chan, mf->Data[0], mf->Data[1]);
	 }
    }
    else
    {
	 switch ( mf->Status & 0xF0 )
	 {
	      case 0x80:
		   counts[0]+=1;
		   break;

	      case 0x90:
		   counts[1]+=1;
		   break;

	      case 0xA0:
		   counts[2]+=1;
		   break;

	      case 0xB0:
		   counts[3]+=1;
		   break;

	      case 0xC0:
		   counts[4]+=1;
		   break;

	      case 0xD0:
		   counts[5]+=1;
		   break;

	      case 0xE0:
		   counts[6]+=1;
	 }
    }
    return(0);
}




/********************************* sysexEvt() ********************************
 * This is called by MIDIFILE.DLL when it encounters a MIDI SYSEX or ESCAPE (ie, usually
 * REALTIME or COMMON) event within an Mtrk chunk. We simply print out info about that event.
 * Note that 0xF7 is used both as SYSEX CONTINUE and ESCAPE. Which one it is depends upon
 * what preceded it. When 0xF7 is used as a SYSEX CONTINUE, it must follow an 0xF0 event
 * that doesn't end with 0xF7, and there can't be intervening Meta-Events or MIDI messages with
 * Status < 0xF0 inbetween. Of course, it IS legal to have an ESCAPE event, (ie, REALTIME)
 * inbetween the 0xF0 and SYSEX CONTINUE events. The DLL helps differentiate between SYSEX
 * CONTINUE and ESCAPE by setting the MIDISYSEX flag when a 0xF0 is encountered. If this
 * flag is not set when you receive a 0xF7 event, then it is definitely an ESCAPE. If MIDISYSEX
 * flag is set, then a 0xF7 could be either SYSEX CONTINUE or an ESCAPE. You can check the
 * first loaded byte; if it is < 0x80 or it's 0xF7, then you've got a SYSEX CONTINUE. Whenever
 * you have an event that ends in 0xF7 while the MIDISYSEX is set, then you should clear the
 * MIDISYSEX flag this is the end of a stream of SYSEX packets. NOTE: The DLL will skip any
 * bytes that we don't read in of this event.
 ***************************************************************************/

LONG EXPENTRY sysexEvt(MIDIFILE * mf)
{
    UCHAR chr;
    LONG result;

    prtime((MIDIFILE *)mf);

    /* Load the first byte. */
    if ( (result =MidiReadBytes(mf, &chr, 1)) ) return(result);

    /* Did we encounter a SYSEX (0xF0)? If not, then this can't possibly be a SYSEX event.
	It must be an ESCAPE. */
    if ( mf->Flags & MIDISYSEX )
    {
	 /* If the first byte > 0x7F but not 0xF7, then we've really got an ESCAPE */
	 if (chr > 0x7F && chr != 0xF7) goto escd;

	 /* NOTE: Normally, we would allocate some memory to contain the SYSEX message, and
	     read it in via MidiReadBytes. We'd check the last loaded byte of this event, and if
	     a 0xF7, then clear MIDISYSEX. Note that, by simply returning, the DLL will skip any
	     bytes that we haven't read of this event. */

	 /* Seek to and read the last byte */
	 chr=0;
	 if (mf->EventSize)
	 {
	      MidiSeek(mf, mf->EventSize-1);
	      if ( (result =MidiReadBytes(mf, &chr, 1)) ) return(result);
	 }

	 /* 0xF7 (SYSEX CONTINUE) */
	 if (mf->Status == 0xF7)
	 {
	      /* If last char = 0xF7, then this is the end of the SYSEX stream (ie, last packet) */
	      if (chr == 0xF7)
	      {
		   if (!compress)
			printf("Last Packet | len=%-6ld|\r\n",(mf->EventSize)+2);
	      }
	      else
	      {
		   if (!compress)
			printf("Packet %-5ld| len=%-6ld|\r\n", packet, (mf->EventSize)+2);
	      }
	 }

	 /* 0xF0 */
	 else
	 {
	      if (!compress)
	      {
		   /* If last char = 0xF7, then this is a full SYSEX message, rather than the first of
		       a series of packets (ie, more SYSEX CONTINUE events to follow) */
		   if (chr == 0xF7)
		   {
			printf("Sysex 0xF0  | len=%-6ld|\r\n",(mf->EventSize)+3); /* +3 to also include the 0xF0 status which
										      is normally considered part of the SYSEX */
		   }
		   else
		   {
			packet=1;
			printf("First Packet| len=%-6ld|\r\n",(mf->EventSize)+3);
		   }
	      }
	      else
		   counts[7]+=1;
	 }
    }

    /* Must be an escaped event such as MIDI REALTIME or COMMON. */
    else
    {
escd:
	 if (!compress)
	      printf("Escape 0x%02x | len=%-6ld|\r\n", chr, mf->EventSize+1);
	 else
	      counts[8]+=1;
	 /* NOTE: The DLL will skip any bytes of this event that we don't read in, so let's just
	     ignore the rest of this event even if there are more bytes to it. */
    }

    return(0);
}




/********************************* metatext() ********************************
 * This is called by MIDIFILE.DLL when it encounters a variable length Meta-Event within an Mtrk
 * chunk. We figure out which of the several "type of events" it is, and simply print out info
 * about that event. NOTE: The DLL will skip any bytes that we don't read in of this event.
 **************************************************************************/

LONG EXPENTRY metatext(MIDIFILE * mf)
{
    USHORT i, p;
    UCHAR chr[60];
    LONG result;

    prtime((MIDIFILE *)mf);

    /* If a text-based event, print what kind of event */
    if ( mf->Status < 0x10 )
    {
	 if (mf->Status > 7) mf->Status=0;	/* I only know about 7 of the possible 15 */
	 if (!compress)
	      printf("%s| len=%-6ld|", &types[mf->Status][0], mf->EventSize);
	 else
	      counts[9+mf->Status]+=1;
    }

    /* A Proprietary event, or some Meta event that we don't know about */
    else
    {
	 switch (mf->Status)
	 {
	      case 0x7F:
		   if (!compress)
			printf("Proprietary | len=%-6ld|", mf->EventSize);
		   else
			counts[17]+=1;
		   break;

	      default:
		   if (!compress)
			printf("Unknown Meta| Type = 0x%02x, len=%ld", mf->Status, mf->EventSize);
		   else
			counts[18]+=1;
	 }
    }

    /* Print out the actual text, one byte at a time, 15 chars to a line */
    if (!compress)
    {
	 i=0;
	 while (mf->EventSize)
	 {
	      if (!i) printf("\r\n         <");

	      /* NOTE: MidiReadBytes() decrements EventSize by the number of bytes read */
	      if ( (result =MidiReadBytes(mf, &chr[i], 1)) ) return(result);

	      printf( (isprint(chr[i])||isspace(chr[i])) ? "%c" : "." , chr[i]);

	      if (i++ == 15)
	      {
		   printf(">    ");
		   for (i=0; i<15; i++)
		   {
			 printf("%02x ", chr[i]);
		   }
		   i=0;
	      }
	 }

	 if (i)
	 {
	      printf(">");
	      for (p=20-i; p; p--)
	      {
		   printf(" ");
	      }
	      for (p=0; p<i; p++)
	      {
		   printf("%02x ", chr[p]);
	      }
	      printf("\r\n");
	 }
    }

    return(0);
}




/******************************** metaseq() *********************************
 * This is called by MIDIFILE.DLL when it encounters a Sequence Number Meta-Event within an
 * Mtrk chunk. We simply print out info about that event.
 **************************************************************************/

LONG EXPENTRY metaseq(METASEQ * mf)
{
    prtime((MIDIFILE *)mf);
    printf("Seq # = %-4d|\r\n", mf->SeqNum);
    return(0);
}




/********************************* metaend() ********************************
 * This is called by MIDIFILE.DLL when it encounters an End of Track Meta-Event within an Mtrk
 * chunk. We simply print out info about that event.
 **************************************************************************/

LONG EXPENTRY metaend(METAEND * mf)
{
    USHORT i;

    if (!compress)
    {
	 prtime((MIDIFILE *)mf);
	 printf("End of track\r\n");
    }

    /* If compressing info on display, print out the counts now that we're at the end of track */
    else
    {
	 for (i=0; i<23; i++)
	 {
	      printf("%-10ld %s events.\r\n", counts[i], strptrs[i]);
	 }
    }

    return(0);
}




/********************************** metakey() ********************************
 * This is called by MIDIFILE.DLL when it encounters a Key Signature Meta-Event within an Mtrk
 * chunk. We simply print out info about that event.
 ***************************************************************************/

LONG EXPENTRY metakey(METAKEY * mf)
{
    UCHAR * ptr;

    prtime((MIDIFILE *)mf);

    if (!compress)
	 printf("Key sig     | %s %-7s|\r\n", &keys[mf->Key+7][0], mf->Minor ? "Minor" : "Major");
    else
	 counts[19]+=1;

    return(0);
}




/******************************** metatempo() ********************************
 * This is called by MIDIFILE.DLL when it encounters a Tempo Meta-Event within an Mtrk
 * chunk. We simply print out info about that event.
 ***************************************************************************/

LONG EXPENTRY metatempo(METATEMPO * mf)
{
    prtime((MIDIFILE *)mf);

    if (!compress)
	 printf("Tempo       | BPM=%-6d| micros/quarter=%ld \r\n", mf->TempoBPM, mf->Tempo);
    else
	 counts[20]+=1;

    return(0);
}




/******************************** metatime() *********************************
 * This is called by MIDIFILE.DLL when it encounters a Time Signature Meta-Event within an
 * Mtrk chunk. We simply print out info about that event.
 **************************************************************************/

LONG EXPENTRY metatime(METATIME * mf)
{
    prtime((MIDIFILE *)mf);

    if (!compress)
	 printf("Time sig    | %2d/%-7d| MIDI-clocks/click=%d | 32nds/quarter=%d\r\n",
		mf->Nom, mf->Denom, mf->Clocks, mf->_32nds);
    else
	 counts[21]+=1;

    return(0);
}




/********************************* metasmpte() ********************************
 * This is called by MIDIFILE.DLL when it encounters a SMPTE Meta-Event within an Mtrk
 * chunk. We simply print out info about that event.
 ****************************************************************************/

LONG EXPENTRY metasmpte(METASMPTE * mf)
{
    prtime((MIDIFILE *)mf);

    if (!compress)
	 printf("SMPTE       | hour=%d | min=%d | sec=%d | frame=%d | subs=%d\r\n",
		mf->Hours, mf->Minutes, mf->Seconds, mf->Frames, mf->SubFrames);
    else
	 counts[22]+=1;

    return(0);
}




/********************************* unknown() *********************************
 * This is called by MIDIFILE.DLL when it encounters a chunk that isn't an Mtrk or MThd.
 * We simply print out info about this. NOTES: The ID and ChunkSize are in the MIDIFILE. If
 * TrackNum is -1, then this was encountered after the MThd, but before any MTrk chunks.
 * Otherwise, TrackNum is the MTrk number that this chunk follows.
 ***************************************************************************/

LONG EXPENTRY unknown(MIDIFILE * mf)
{
    register UCHAR * ptr = (UCHAR *)&mf->ID;
    register USHORT i;
    UCHAR str[6];

    /* Normally, we would inspect the ID to see if it's something that we recognize. If so, we
	would read in the chunk with 1 or more calls to MidiReadBytes until ChunkSize is 0.
	(ie, We could read in the entire chunk with 1 call, or many calls). Note that MidiReadBytes
	automatically decrements ChunkSize by the number of bytes that we ask it to read. We
	should never try to read more bytes than ChunkSize. On the other hand, we could read less
	bytes than ChunkSize. Upon return, the DLL would skip any bytes that we didn't read from
	that chunk. */

    /* Copy ID to str[] */
    for (i=0; i<4; i++)
    {
	str[i] = *(ptr)++;
    }
    str[4] = 0;

    printf("\r\n******************** Unknown ********************\r\n");
    printf("     ID = %s    ChunkSize=%ld\r\n", &str[0], mf->ChunkSize);

    return(0);
}




#ifdef BUFFERED_IO

/******************************* midi_open() *********************************
 * Here's an example of providing a MidiOpen callback, so that I can use standard C buffered I/O,
 * instead of letting the DLL use unbuffered I/O.
 **************************************************************************/

LONG EXPENTRY midi_open(MIDIFILE * mf)
{
    /* Open the file. Store the handle in the MIDIFILE's Handle field, although this isn't required,
	but it's a handy place to keep it. */
    if ( !(mf->Handle=(ULONG)fopen(fname, "rb")) )
    {
       printf("Can't open %s\r\n", fname);
       return(MIDIERRFILE);
    }

    /* Set the # of bytes to parse */
    mf->FileSize = _filelength(_fileno((struct __file *)mf->Handle));
    return(0);
}




/******************************* midi_close() *********************************
 * Here's an example of providing a MidiClose callback, so that I can use standard C buffered I/O,
 * instead of letting the DLL use unbuffered I/O.
 ***************************************************************************/

LONG EXPENTRY midi_close(MIDIFILE * mf)
{
    /* Close the file. Note that this never gets called if my MidiOpen callback returned an error.
	But, if any other subsequent errors occur, this gets called before the read operation
	aborts. So, I can always be assured that the file handle gets closed, even if an error
	occurs. */
    fclose((struct __file *)mf->Handle);

    return(0);
}




/******************************* midi_seek() *********************************
 * Here's an example of providing a MidiSeek callback, so that I can use standard C buffered I/O,
 * instead of letting the DLL use unbuffered I/O.
 **************************************************************************/

LONG EXPENTRY midi_seek(MIDIFILE * mf, LONG amt, ULONG type)
{
    /* NOTE: The DLL only ever passes a seek type of DosSeek's FILE_CURRENT, which
	translates to SEEK_CUR for C/Set2 fseek(). */
    if( fseek((struct __file *)mf->Handle, amt, SEEK_CUR) )
    {
       printf("Seek error");
    }

    /* Don't bother with error return. The DLL ignores it, assuming that you'll encounter an
       error with a subsequent read */
}




/******************************* midi_read() *********************************
 * Here's an example of providing a MidiReadWrite callback, so that I can use standard C buffered I/O,
 * instead of letting the DLL use unbuffered I/O.
 **************************************************************************/

LONG EXPENTRY midi_read(MIDIFILE * mf, UCHAR * buffer, ULONG count)
{
     if( fread(buffer, 1L, count, (struct __file *)mf->Handle) ) return(0);
     return(MIDIERRREAD);
}

#endif




/********************************* initfuncs() ********************************
 * This initializes the CALLBACK structure containing pointers to functions. These are used by
 * MIDIFILE.DLL to call our callback routines while reading the MIDI file. It also initializes a few
 * fields of MIDIFILE structure.
 **************************************************************************/

VOID initfuncs(CHAR * fn)
{
    /* Place CALLBACK into MIDIFILE */
    mfs.Callbacks = &cb;

#ifdef BUFFERED_IO
    /* I'll do my own buffered Open, Read, Seek, and Close */
    cb.OpenMidi = midi_open;
    cb.ReadWriteMidi = midi_read;
    cb.SeekMidi = midi_seek;
    cb.CloseMidi = midi_close;
    mfs.Handle = 0;
    fname = fn;
#else
    /* Let the DLL Open, Read, Seek, and Close the MIDI file */
    cb.OpenMidi = 0;
    cb.ReadWriteMidi = 0;
    cb.SeekMidi = 0;
    cb.CloseMidi = 0;
    mfs.Handle = (ULONG)fn;
#endif

    cb.UnknownChunk = unknown;
    cb.StartMThd = startMThd;
    cb.StartMTrk = startMTrk;
    cb.StandardEvt = standardEvt;
    cb.SysexEvt = sysexEvt;
    cb.MetaSMPTE = metasmpte;
    cb.MetaTimeSig = metatime;
    cb.MetaTempo = metatempo;
    cb.MetaKeySig = metakey;
    cb.MetaSeqNum = metaseq;
    cb.MetaText = metatext;
    cb.MetaEOT = metaend;
}
