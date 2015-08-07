#include <stdlib.h>
#include <math.h>
#include <stdio.h>

/* Written long time ago by Jan Labanowski */

#define RAD 57.29577951

float distance(xyz1, xyz2)
float xyz1[3], xyz2[3];
{
 int i;
 float s, x;
 s = 0.0;
 for(i = 0; i < 3; i++)
   {
    x = xyz1[i] - xyz2[i];
    s = s + x*x;
   }
 return((float)sqrt((double)s));
}

/*===============================================================
   function angle finds angle (in deg) between 3 points:

             xyz1--------xyz2
                        /
                       /
                      /
                     /
                  xyz3
=================================================================*/


float angle(xyz1, xyz2, xyz3)
float xyz1[3], xyz2[3], xyz3[3];
{
 float d12, d23, d13, s, c, x;

 d12 = distance(xyz1,xyz2);
 d23 = distance(xyz2,xyz3);
 d13 = distance(xyz1,xyz3);

 x = 2.0*d12*d23;

 if((x < 0.00001) || (d13 < 0.00001))
    return(0.0);

 c = (d23*d23 + d12*d12 - d13*d13)/x;

 if(1.0 - c*c < 0.00001)
   return(180.0);

 return((float)(RAD*acos((double)c)));
}

/*===================================================================
   function vector_length finds length of a vector 
=====================================================================*/
float vector_length(vector)
float vector[3];
{
 float x, y;
 int i;

 x = 0.0;
 for(i = 0; i < 3; i++)
   {
    y = vector[i];
    x = x + y*y;
   }
 return((float)sqrt((double)x));
}


/*====================================================================

    calculates unit vector from xyz1 to xyz2 and returns distance
    between xyz1 and xyz2 as function value.
======================================================================*/

float unit_vector(xyz1, xyz2, vector)
float xyz1[3], xyz2[3], vector[3];
{
 int k;
 float x;

 x = distance(xyz1,xyz2);
 if(x < 0.0001)
   return(0.0);

 for(k=0; k<3; k++)
   vector[k] = (xyz2[k] - xyz1[k])/x;
   
 return(x);
}


/*=================================================================

      Normalizes a vector and returns its length
===================================================================*/
float normalize(vorig, vnorm)
float vorig[3], vnorm[3];
{
 int k;
 float x;

 x = vector_length(vorig);
 if(x < 0.0001)
   return(0.0);

 for(k = 0; k < 3; k++)
    vnorm[k] = vorig[k]/x;
}

/*==================================================================

   calculated scalar product of v1 and v2
===================================================================*/
float scalar_product(v1, v2)
float v1[3], v2[3];
{
 int k;
 float x;

 x = 0.0;
 for(k = 0; k < 3; k++)
   x = x + v1[k]*v2[k];

 return(x);
}

/*==================================================================

   calculates vector product of v1 and v2 and stores it in v12.
   Returns length of v12 as function value
====================================================================*/
float vector_product(v1, v2, v12)
float v1[3], v2[3], v12[3];
{
 v12[0] = v1[1]*v2[2] - v1[2]*v2[1];
 v12[1] = v1[2]*v2[0] - v1[0]*v2[2];
 v12[2] = v1[0]*v2[1] - v1[1]*v2[0];
 return(vector_length(v12));
}

/*==================================================================

   Function returns torsion angle between xyz1--xyz2
                                                  \
                                                   xyz3--xyz4

===================================================================*/
float torsion(xyz1, xyz2, xyz3, xyz4)
float xyz1[3], xyz2[3], xyz3[3], xyz4[3];
{
 int i;
 float  v12[3], v23[3], v34[3], plane123[3],
        plane234[3], v1234[3], c, s, x, y;

 if((unit_vector(xyz1, xyz2, v12) <= 0.0001) ||
    (unit_vector(xyz2, xyz3, v23) <= 0.0001) ||
    (unit_vector(xyz3, xyz4, v34) <= 0.0001))
   return(0.0);

 if((vector_product(v12,v23,plane123) <= 0.0001) ||
    (vector_product(v23,v34,plane234) <= 0.0001))
   return(0.0);

 x = normalize(plane123,plane123);
 y = normalize(plane234,plane234);

 c = scalar_product(plane123,plane234);
 if(fabs(c+1.0) < 0.0001)
   return(180.0);
 else if(fabs(c-1.0) < 0.0001)
   return(0.0);

 s = vector_product(plane123,plane234,v1234);
 s = normalize(v1234,v1234);
 s = s*scalar_product(v1234,v23);
       
 return((float)(RAD*atan((double)s/c)));
}      

