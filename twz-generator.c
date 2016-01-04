//  twz.generator.c
//  Original Author: Peter Meyer
//  Calculate the value of the timewave at a point.
//  Last mod.: 1998-01-05
// http://www.fractal-timewave.com/

// Ported to Linux
// 4 Oct 2009
// John A Phelps
// kl4yfd@gmail.com


/*

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>

09 Dec 2012
*/


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>


#define FALSE 0
#define TRUE  1
#define NUM_POWERS 128
#define PREC 16
#define NUM_SETS 4
#define NUM_DATA_POINTS 384
#define CALC_PREC       100000  /*  precision in calculation  */
/*  of wave values            */ 

double powers[NUM_POWERS];

//  Powers of (normally) 64.
//  Due to the limitations of double precision
//  floating point arithmetic these values are
//  exact only up to powers[8] for powers of 64.

int wave_factor = 2;		//  default wave factor 
int number_set, stringchar;


char *usage = "\nUsage: twz [dtz] [step] [wf]." 
"\n dtz = days to zero (12/21/2012)" 
"\n step = steps in which to decrement time (in minutes)" 
"\n wf = wave factor (default 2, range 2-10000)" 
"\n\nThis program calculates the value of the timewave at a given point relative to the zero point.\n";



char *set_name[NUM_SETS] =
{ "Kelley", "Watkins", "Sheliak", "Huang Ti" };


char *title = "Days to Zero (DTZ), Kelley, Watkins, Sheliak, Huang Ti";


//  The number sets.
int w[NUM_SETS][NUM_DATA_POINTS] = 
{ 
  {
    #include "DATA/DATA.TW1"		//  half-twist
  }, 
  {
    #include "DATA/DATA.TW2"		//  no half-twist
  }, 
  {
    #include "DATA/DATA.TW3"		//  Sheliak 
  }, 
  {
    #include "DATA/DATA.TW4"		//  HuangTi (no half-twist)
  } 
};



void inputerror (void);

void get_dtzp (void);

void get_step (void);

void get_wave_factor (void);

void set_powers (void);

double f (double x, int number_set);

double fONE (double x);

double fTWO (double x);

double fTHREE (double x);

double fFOUR (double x);



double v (double y, int number_set);

double mult_power (double x, int i);

double div_power (double x, int i);


double dtzp, step;


/*-----------------------------*/ 
int
main (int argc, char *argv[]) 
{
  
  
  int i, j, ch;
  
  
  
  if (argc != 4 && argc != 1)
    
  {
    
    printf ("%s", usage);
    
    inputerror ();
    
  }
  
  
  if (argc == 4)
    
  {
    
    dtzp = atof (&argv[1][0]);
    
    
    step = atof (&argv[2][0]);
    
    step /= 60;		// Convert to 60 minute hours 
    step /= 24;			// Convert to 24 hour days 
    
    wave_factor = atoi (&argv[3][0]);
    
    
    if (wave_factor < 2 || wave_factor > 10000)
      
    {
      
      printf ("%s", usage);
      
      inputerror ();
      
    }
    
  }
  
  
  if (argc == 1)		// If no commandline inputs
  {
    
    get_dtzp ();
    
    get_step ();
    
    get_wave_factor ();
    
    
  }
  
  
  set_powers ();
  
  
  
  
  
  
  
  //printf("\n\ndtzp: %lfstep: %lfwave_factor: %d",dtzp, step, wave_factor);
  printf ("\n%s\n", title);
  
  
  while (dtzp >= 0)
    
  {
    
    
    printf ("%.*f ,", PREC, dtzp);
    
    for (number_set = 0; number_set < NUM_SETS; number_set++)
      
    {
      
      printf ("%.*e ,", PREC, f (dtzp, number_set));
      
    }
    
    
    printf ("\n");
    
    dtzp -= step;
    
    
  }
  
  
}



//  wave_factor is a global variable
/*-----------------*/ 
void
set_powers (void) 
{
  
  unsigned int j;
  
  
  /*  put powers[j] = wave_factor^j  */ 
  
  powers[0] = (double) 1;
  
  for (j = 1; j < NUM_POWERS; j++)
    
    powers[j] = wave_factor * powers[j - 1];
  
}



/*  x is number of days to zero date  */ 
/*--------------*/ 
double
f (double x, 
  int number_set) 
{
  
  unsigned int i;
  
  double sum = 0.0, last_sum = 0.0;
  
  
  if (x)
    
  {
    
    for (i = 0; x >= powers[i]; i++)
      
      sum += mult_power (v (div_power (x, i), number_set), i);
    
    
    i = 0;
    
    do
      
    {
      
      if (++i > CALC_PREC + 2)
	
	break;
      
      last_sum = sum;
      
      sum += div_power (v (mult_power (x, i), number_set), i);
      
    }
    while ((sum == 0.0) || (sum > last_sum));
    
  }
  
  
  /*  dividing by 64^3 gives values consistent with the Apple // version
  *  and provides more convenient y-axis labels
  */ 
  sum = div_power (sum, 3);
  
  
  return (sum);
  
}




/*--------------*/ 
double
v (double y, 
  int number_set) 
{
  
  int i = (int) (fmod (y, (double) NUM_DATA_POINTS));
  
  int j = (i + 1) % NUM_DATA_POINTS;
  
  double z = y - floor (y);
  
  
  return (z == 0.0 ? (double) w[number_set][i] : 
  (w[number_set][j] - w[number_set][i]) * z + w[number_set][i]);
  
} 


/*  in order to speed up the calculation, if wave factor = 64
*  then instead of using multiplication or division operation
*  we act directly on the floating point representation;
*  multiplying by 64^i is accomplished by adding i*0x60
*  to the exponent (the last 2 bytes of the 8-byte representation);
*  dividing by 64^i is accomplished by subtracting i*0x60
*  from the exponent
*/ 

/*-----------------------*/ 
double
mult_power (double x, 
	    int i) 
{
  
  int *exponent = (int *) &x + 3;
  
  
  if (wave_factor == 64)
    
    *exponent += i * 0x60;	/*  measurably faster  */
    
    else
      
      x *= powers[i];
    
    
    return (x);
  
}



/*----------------------*/ 
double
div_power (double x, 
	  int i) 
{
  
  int *exponent = (int *) &x + 3;
  
  
  if ((wave_factor == 64) && (*exponent > i * 0x60))
    
    *exponent -= i * 0x60;
  
  else
    
    x /= powers[i];
  
  
  return (x);
  
}



void
get_dtzp (void) 
{
  
  printf ("Enter the number of days before zero point:  ");
  
  scanf ("%lf", &dtzp);
  
  if (dtzp < 0)
    inputerror ();
  
}



void
get_step (void) 
{
  
  printf ("Enter the time step in minutes ( >= 0 ):  ");
  
  scanf ("%lf", &step);
  
  step /= 60;			// Convert to 60 minute hours
  step /= 24;			// Convert to fractions of 24-hour days for internal calculations...
  if (step > dtzp || step < 0)
    inputerror ();		// The step cannot be larger than dtz as this would cause no values to be calculated.
}



void
get_wave_factor (void) 
{
  
  printf ("Enter the wave factor (2-10000): ");
  
  scanf ("%i", &wave_factor);
  
  
  //  if ( wave_factor < 2 || wave_factor > 10000 ) inputerror(); 
} 

int

doublecheck (void) 
{
  
  char answer;
  
  printf
  ("\nThe combination you have chosen will create %f data points. \nDo you wish to continue? (Y/N) ",
  1 + (int) dtzp / step);
  
  answer = getchar ();
  
} 

void

inputerror (void) 
{
  
  printf ("\nError: Invalid input, exiting.\n\n");
  
  exit (EXIT_SUCCESS);
  
} 