/* ===========================================================================
 * mfvlq.c
 *
 * Converts ULONGs entered by the user into the variable length quantity equivalent and
 * displays that.
 * =========================================================================
 */

#include <stdio.h>
#include <os2.h>

#include "midifile.h"



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
    ULONG conv, len;
    UCHAR buffer[6];
    UCHAR convbuf[32];
    register UCHAR * arg;

    /* If no args supplied by user, exit with usage info */
    if ( argc < 2 )
    {
	 printf("This program converts ULONGs into the variable length\r\n");
	 printf("quantity (ie, series of bytes) equivalent and displays that.\r\n");
	 printf("It requires MIDIFILE.DLL to run.\r\n");
	 printf("Syntax: MFTOVLQ.EXE [ULONG1 ULONG2...]\r\n");
	 exit(1);
    }

    /* Convert ascii numerals to byte values, and store in buffer */
    for (i=1; i<argc; i++)
    {
	 arg = argv[i];
	 if ( *arg == '0' && (*(arg+1) == 'x' || *(arg+1) == 'X') )
	 {
	      arg+=2;
	      conv=0;
	      while (*arg)
	      {
		 conv = (conv << 4) + (ULONG)hex2bin(*(arg)++);
	      }
	 }
	 else
	 {
	      conv = (ULONG)atoi(arg);
	 }

	 /* Call MIDIFILE.DLL function to do the conversion */
	 len = MidiLongToVLQ(conv, &buffer[0]);

	 /* Format the variable length bytes as ascii and print out */
	 printf("%10s = ", argv[i]);
	 arg = &buffer[0];
	 for (; len; len--)
	 {
	      printf("0x%02x ", (ULONG)*(arg)++);
	 }
	 printf("\r\n");
    }

    exit(0);
}
