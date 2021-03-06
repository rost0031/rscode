/* 
 * Reed Solomon Encoder/Decoder 
 *
 * Copyright Henry Minsky (hqm@alum.mit.edu) 1991-2009
 *
 * This software library is licensed under terms of the GNU GENERAL
 * PUBLIC LICENSE
 *
 * RSCODE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RSCODE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Rscode.  If not, see <http://www.gnu.org/licenses/>.

 * Commercial licensing is available under a separate license, please
 * contact author for details.
 *
 * Source code is available at http://rscode.sourceforge.net
 */

#define RSCODE_INTERNAL
#include "ecc.h"
#ifdef DEBUG
#include <stdio.h>
#endif
#include <ctype.h>
#include "galois.h"
#include "berlekamp.h"

static void
compute_genpoly (struct rscode_driver * driver, int nbytes, int * genpoly);

/* Initialize lookup tables, polynomials, etc. */
void
initialize_ecc (struct rscode_driver * driver)
{
  /* Compute the encoder generator polynomial */
  compute_genpoly(driver, NPAR, driver->genPoly);
}

void
zero_fill_from (unsigned char * buf, int from, int to)
{
  int i;
  for (i = from; i < to; i++) buf[i] = 0;
}

#ifdef DEBUG

/* debugging routines */
void
print_parity (struct rscode_driver * driver)
{ 
  int i;
  printf("Parity Bytes: ");
  for (i = 0; i < NPAR; i++) 
    printf("[%d]:%x, ",i,driver->pBytes[i]);
  printf("\n");
}


void
print_syndrome (struct rscode_driver * driver)
{ 
  int i;
  printf("Syndrome Bytes: ");
  for (i = 0; i < NPAR; i++) 
    printf("[%d]:%x, ",i,driver->synBytes[i]);
  printf("\n");
}

#endif

/* Append the parity bytes onto the end of the message */
void
build_codeword (struct rscode_driver * driver, unsigned char * msg, int nbytes, unsigned char * dst)
{
  int i;
	
  for (i = 0; i < nbytes; i++) dst[i] = msg[i];
	
  for (i = 0; i < NPAR; i++) {
    dst[i+nbytes] = driver->pBytes[NPAR-1-i];
  }
}
	
/**********************************************************
 * Reed Solomon Decoder 
 *
 * Computes the syndrome of a codeword. Puts the results
 * into the synBytes[] array.
 */
 
void
decode_data(struct rscode_driver * driver, unsigned char * data, int nbytes)
{
  int i, j, sum;
  for (j = 0; j < NPAR;  j++) {
    sum	= 0;
    for (i = 0; i < nbytes; i++) {
      sum = data[i] ^ gmult(driver, gexp(driver,j+1), sum);
    }
    driver->synBytes[j]  = sum;
  }
}


/* Check if the syndrome is zero */
int
check_syndrome (struct rscode_driver * driver)
{
 int i, nz = 0;
 for (i =0 ; i < NPAR; i++) {
  if (driver->synBytes[i] != 0) {
      nz = 1;
      break;
  }
 }
 return nz;
}

/* Create a generator polynomial for an n byte RS code. 
 * The coefficients are returned in the driver->genPoly arg.
 * Make sure that the driver->genPoly array which is passed in is 
 * at least n+1 bytes long.
 */

static void
compute_genpoly (struct rscode_driver * driver, int nbytes, int * genpoly)
{
  int i, tp[256], tp1[256];
	
  /* multiply (x + a^n) for n = 1 to nbytes */

  zero_poly(driver, tp1);
  tp1[0] = 1;

  for (i = 1; i <= nbytes; i++) {
    zero_poly(driver, tp);
    tp[0] = gexp(driver,i);		/* set up x+a^n */
    tp[1] = 1;
	  
    mult_polys(driver, genpoly, tp, tp1);
    copy_poly(driver, tp1, genpoly);
  }
}

/* Simulate a LFSR with generator polynomial for n byte RS code. 
 * Pass in a pointer to the data array, and amount of data. 
 *
 * The parity bytes are deposited into driver->pBytes[], and the whole message
 * and parity are copied to dest to make a codeword.
 * 
 */

void
encode_data (struct rscode_driver * driver, unsigned char *msg, int nbytes, unsigned char *dst)
{
  int i, LFSR[NPAR+1],dbyte, j;
	
  for(i=0; i < NPAR+1; i++) LFSR[i]=0;

  for (i = 0; i < nbytes; i++) {
    dbyte = msg[i] ^ LFSR[NPAR-1];
    for (j = NPAR-1; j > 0; j--) {
      LFSR[j] = LFSR[j-1] ^ gmult(driver, driver->genPoly[j], dbyte);
    }
    LFSR[0] = gmult(driver, driver->genPoly[0], dbyte);
  }

  for (i = 0; i < NPAR; i++) 
    driver->pBytes[i] = LFSR[i];
	
  build_codeword(driver, msg, nbytes, dst);
}

