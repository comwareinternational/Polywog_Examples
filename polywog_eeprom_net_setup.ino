// Arduino Network EEPROM Setup utility - eeprom_net_setup
// D. Retz, March 2017

#include "Arduino.h"
#include "EEPROM.h"
#include "string.h"

#include "polywog_eeprom.h"    // contains EEPROM structure


// eeprom_crc returns a 32-bit unsigned value

unsigned long eeprom_crc(unsigned char *p, int length) {

  const unsigned long crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };

  unsigned long crc = ~0L;

  for (int index = 0 ; index < length ; ++index) {
    crc = crc_table[(crc ^ p[index]) & 0x0F] ^ (crc >> 4);
    crc = crc_table[(crc ^ (p[index] >> 4)) & 0x0F] ^ (crc >> 4);
    crc = ~crc;
  }
  return crc;
}

/*----------------------------------------------------------------------*
 *	This code loads the values into EEPROM structure and returns	*
 *	TRUE if EEPROM valid.						*
 *									* 
 *	Assumes pointer to POLYWOG_EEPROM structure, which gets filled	*
 *	in IF the EEPROM is valid.					*
 *	Returns TRUE if EEPROM is valid, FALSE otherwise.		*
 *----------------------------------------------------------------------*/

boolean verify_eeprom(POLYWOG_EEPROM_PTR eep)
  {
  unsigned long tcrc; // temp crc for comparison
  int i; 
  unsigned char *xcp;

  for (i=POLYWOG_EEPROM_BASE, xcp=(unsigned char *) eep; i<sizeof(POLYWOG_EEPROM); i++)
     *xcp++ = EEPROM.read(i);  // copy byte-for-byte into structure    
  tcrc = eeprom_crc((unsigned char *) eep, sizeof(POLYWOG_EEPROM)-sizeof(eep->crc) );  // compute CRC on stored data
  return (tcrc == eep->crc);
  }









// The following code is only used for initializing the EEPROM


// readline - reads a line of text ending with NL character

String readline(void)
  {
  String a;
  
  do {
  a = Serial.readStringUntil('\n'); 
  } while (a.length()==0);
  return(a);
  }

// hexVal - convert nul-terminated string of form 0xFF to int (hex)

int hexVal(char *hs)
  {
  int ih,ij;
  char c;
  
  ih = 0;         // initialize accumulated hex value

  for (ij=2; ij<8; ij++)
    {
    c = toupper(hs[ij]);               // get next hex character (ascii)
    if (c == 0) return(ih);  // if nul, return accumulated hex value  
    if (c >= 0x30 && c <= 0x39) // numeric
       {ih <<= 4; ih |= (c & 0x0F); }
    if (c >= 'A' && c <= 'F')  // characters A-F ?
       {ih <<= 4; ih |= (c-'A'+0x0A); } 
    }
  return (0);               // if loop ran out, return 0
  }

// getIntVal - converts string to decimal or hex value and return the value as an int

int getIntVal(String x)
  {
  char data[16];

  if (x.length() < 1) return(0);
  x.toCharArray(data, sizeof(data)-1); // copy bytes to array
  if (memcmp(data, "0x", 2) != 0)
    return(atoi(data));
  else
    return(hexVal(data));  
  }

// confirm, return 0 if cancel, 1 if ok

unsigned int confirm(void)
  {
    String b;
  
  Serial.print("Confirm [Y/N]: ");
  b = readline();
  if (toupper(b[0]) == 'Y') return (1); else return(0);
  }


void setup() {
  // put your setup code here, to run once:

Serial.begin(38400);
Serial.setTimeout(15000);

Serial.println("EEPROM NET Address Setup. Enter character command:");
Serial.println("I - Initialize; C - Clear; V - Verify");
Serial.println();
}

 
void loop()
  {
  unsigned char xc;
  unsigned char *xcp;
  int i;
  unsigned long tcrc; // temp crc for comparison
  POLYWOG_EEPROM eepval;
  
  // put your main code here, to run repeatedly:

Serial.print("$# ");   // Issue command prompt
String b = readline();

xc = toupper(b[0]);
switch(xc)
  {
  case 'I':  
   Serial.println("Initializing.");

   Serial.print("Net number: ");
   i = getIntVal( readline() );     // get a decimal or hex value as int
   eepval.net_number=i;
   
//   Serial.print("Entered decimal net number: ");
//   Serial.println(eepval.net_number);

   Serial.println();
   Serial.print("Node number: ");
   i = getIntVal( readline() );
   eepval.node_number = i;

//   Serial.println("Entered decimal node number: ");
//   Serial.println(eepval.node_number);

   Serial.println();
   Serial.print("Node name: "); b = readline();
   if (b.length() > sizeof(eepval.node_name)-1)
     {
     Serial.println("Node name is limited in size (input aborted).");
     break; // abort the input
     }
   b.toCharArray(&eepval.node_name[0], sizeof(eepval.node_name)-1);

// echo back the parameters before asking to confirm

   Serial.print("Net number: 0x"); Serial.print(eepval.net_number, HEX); Serial.print(" ("); Serial.print(eepval.net_number); Serial.println(")");
   Serial.print("Node number: 0x"); Serial.print(eepval.node_number, HEX); Serial.print(" ("); Serial.print(eepval.node_number); Serial.println(")");
   Serial.print("Node name: "); Serial.println(eepval.node_name);  // ok, now confirm that below.

   
   if (confirm())
     {
     tcrc = eeprom_crc((unsigned char *)&eepval, sizeof(eepval)-sizeof(eepval.crc)); // generate CRC
     eepval.crc = tcrc;   // store the computed CRC into structure
     Serial.print("Generated CRC = "); Serial.println(tcrc, HEX);
 // now write the structure to EEPROM
     for (i=POLYWOG_EEPROM_BASE, xcp=(unsigned char *)&eepval; i<sizeof(eepval); i++)
       EEPROM.write(i, *xcp++);  // copy byte-for-byte to EEPROM    

     Serial.println("OK");
     }
   else
     {
     Serial.println("Cancelled initialization.");
     }
  
   
   break;


// C - CLEAR EEPROM to 0's if confirm with Y.

  case 'C':
    if ( !confirm()) break;
    Serial.println("Zapping EEPROM (or at least our section) !");
    
    for (i=POLYWOG_EEPROM_BASE; i<sizeof(eepval); i++) EEPROM.write(i, 0x00); // zap the eeprom
    break;


// V - Verify (or S, Status)

   case 'S':
   case 'V':
     Serial.println("EEPROM Status.");
     // eepval.crc = 0;
     for (i=POLYWOG_EEPROM_BASE, xcp=(unsigned char *)&eepval; i<sizeof(eepval); i++)
       *xcp++ = EEPROM.read(i);  // copy byte-for-byte into structure    
     tcrc = eeprom_crc((unsigned char *)&eepval, sizeof(eepval)-sizeof(eepval.crc) );  // compute CRC on stored data
     Serial.print("Stored CRC=");
     Serial.print(eepval.crc, HEX);
     Serial.print(", computed CRC=");
     Serial.print(tcrc, HEX);
     Serial.println();  
     Serial.print("Net number: 0x"); Serial.print(eepval.net_number, HEX); Serial.print(" ("); Serial.print(eepval.net_number); Serial.println(")");
     Serial.print("Node number: 0x"); Serial.print(eepval.node_number, HEX); Serial.print(" ("); Serial.print(eepval.node_number); Serial.println(")");
     
     Serial.print("Node name: "); Serial.println(eepval.node_name);
     
     if (tcrc != eepval.crc)
       {
       Serial.println("EEPROM CRC Error - Run Initialize to setup.");
       }
     else
       Serial.println("EEPROM CRC is valid.");
     
     // show binary image of eeprom, in bytes
     Serial.println("EEPROM Image (little-endian):");
     for (i=POLYWOG_EEPROM_BASE; i<sizeof(eepval); i++)
        {
        Serial.print(EEPROM.read(i), HEX);
        Serial.print(" ");
        }
     Serial.println(); 

     if (verify_eeprom(&eepval)) Serial.println("Verified."); else Serial.println("EEPROM Error - run Setup.");

     break;

   case '?':
   case 'H':
    Serial.println("Commands are:");
    Serial.println("S or V - Status or Verify, prints configuration and verifies EEPROM.");
    Serial.println("C - Clears EEPROM (just the polywog portion)");
    Serial.println("I - Initialize, set network, node number, and name for this node. Write EEPROM after confirm.");
    break; 


   default:
     Serial.println("Unknown command.");
  
  }


}
