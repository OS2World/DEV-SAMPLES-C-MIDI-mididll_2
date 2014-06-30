/* ===========================================================================
 * mfvlq.c
 *
 * Converts the variable length quantity typed by the user into a LONG and displays that.
 * =========================================================================
 */

#include <stdio.h>
#include <os2.h>

#include "midifile.h"


/* A buffer to hold user input */
UCHAR buffer[4096];



/* *************************** hex2bin() ******************************
 * Convert an ascii hex numeric char (ie, '0' to '9', 'A' to 'F') to its binary equivalent.
 ******************************************************************* */

USHORT hex2bin(UCHAR c)
{
   if(c < 0x3A)
     return (c - 48);
   else
     return (( c & 0xDF) - 55);
}



/* ********************************** main() ***********************************
 * Program entry point. Calls the MIDIFILE.DLL function MidiVLQToLong.
 *************************************************************************** */

main(int argc, char *argv[], char *envp[])
{
    register USHORT i;
    ULONG conv, final;
    register UCHAR * ptr = &buffer[0];
    register UCHAR * arg;

    /* If no args supplied by user, exit with usage info */
    if ( argc < 2 )
    {
	 printf("This program converts the variable length quantity\r\n");
	 printf("(ie, series of bytes) into a ULONG and displays that.\r\n");
	 printf("It requires MIDIFILE.DLL to run.\r\n");
	 printf("Syntax: MFVLQ.EXE [...bytes...]\r\n");
	 exit(1);
    }

    /* Convert ascii numerals to byte values, and store in buffer */
    *(ptr)++ = 0;
    for (i=1; i<argc; i++)
    {
	 arg = argv[i];
	 if ( *arg == '0' && (*(arg+1) == 'x' || *(arg+1) == 'X') )
	 {
	      arg+=2;
	      conv = (ULONG)hex2bin(*(arg)++);
	      conv = (conv << 4) + (ULONG)hex2bin(*(arg)++);
	 }
	 else
	 {
	      conv = (ULONG)atoi(arg);
	 }
	 *(ptr)++ = (UCHAR)conv | 0x80;
    }
    *(ptr-1) &= 0x7F;

    /* Null terminate the buffer. This ensures that MidiVLQToLong() definitely sees a final
	terminating byte to the variable length quantity. Remember that all bytes except for the
	last byte of the variable length quantity have bit #7 set. */
    *(ptr) = 0;

    /* Call MIDIFILE.DLL function to do the conversion, and print it  */
    final = MidiVLQToLong(&buffer[1], &conv);
    printf("ULONG value = 0x%08lx\r\n%ld bytes converted\r\n", final, conv);

    exit(0);
}
