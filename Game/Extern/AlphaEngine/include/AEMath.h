/******************************************************************************/
/*!
\par        Project: Alpha Engine
\file       AEMath.h

\author     Sun Tjen Fam
\date       January 31, 2008

\brief      Header file for the math library.

\copyright  Copyright (C) 2013 DigiPen Institute of Technology. Reproduction 
            or disclosure of this file or its contents without the prior 
            written consent of DigiPen Institute of Technology is prohibited.
*/
/******************************************************************************/

#ifndef AE_MATH_H
#define AE_MATH_H

// ---------------------------------------------------------------------------

#include "AEVec2.h"             //Vector 2D
#include "AEMtx33.h"            //Matrix 3x3
#include "AELineSegment2.h"     //LineSegment 2D

#include <float.h>

#include "math.h"

// ---------------------------------------------------------------------------
#ifdef __cplusplus

extern "C"
{
#endif

// ---------------------------------------------------------------------------

/******************************************************************************/
/*!
\fn         f32 AEDegToRad(f32 x)

\brief      Convert an angle from Degree to Radians.

\param      [in] x
            The angle in Degree to be converted.

\retval     f32
            The angle in Radians after conversion.
*/
/******************************************************************************/
AE_API f32 AEDegToRad(f32 x);

/******************************************************************************/
/*!
\fn         f32 AERadToDeg(f32 x)

\brief      Convert an angle from Radians to Degree.

\param      [in] x
            The angle in Radians to be converted.

\retval     f32
            The angle in Degree after conversion.
*/
/******************************************************************************/
AE_API f32 AERadToDeg(f32 x);

/******************************************************************************/
/*!
\fn         f32 AESin(f32 x)

\brief      Calculate the Sine value of an angle.

\param      [in] x
            The angle in Radians.

\retval     f32
            The value of sin(x).
*/
/******************************************************************************/
AE_API f32 AESin	(f32 x);

/******************************************************************************/
/*!
\fn         f32 AECos(f32 x)

\brief      Calculate the Cosine value of an angle.

\param      [in] x
            The angle in Radians.

\retval     f32
            The value of cos(x).
*/
/******************************************************************************/
AE_API f32 AECos	(f32 x);

/******************************************************************************/
/*!
\fn         f32 AETan(f32 x)

\brief      Calculate the Tangent value of an angle.

\param      [in] x
            The angle in Radians.

\retval     f32
            The value of tan(x).
*/
/******************************************************************************/
AE_API f32 AETan	(f32 x);

/******************************************************************************/
/*!
\fn         f32 AEASin(f32 x)

\brief      Calculate the ArcSine value of an angle.

\param      [in] x
            The angle in Radians.

\retval     f32
            The value of asin(x).
*/
/******************************************************************************/
AE_API f32 AEASin	(f32 x);

/******************************************************************************/
/*!
\fn         f32 AEACos(f32 x)

\brief      Calculate the ArcCosine value of an angle.

\param      [in] x
            The angle in Radians.

\retval     f32
            The value of acos(x).
*/
/******************************************************************************/
AE_API f32 AEACos	(f32 x);

/******************************************************************************/
/*!
\fn         f32 AEATan(f32 x)

\brief      Calculate the ArcTangent value of an angle.

\param      [in] x
            The angle in Radians.

\retval     f32
            The value of atan(x).
*/
/******************************************************************************/
AE_API f32 AEATan	(f32 x);

// ---------------------------------------------------------------------------

//@{ 
//! Macros for the trigo functions that take input in degree
    #define AESinDeg(x)		AESin(AEDegToRad(x))
    #define AECosDeg(x)		AECos(AEDegToRad(x))
    #define AETanDeg(x)		AETan(AEDegToRad(x))
    #define AEASinDeg(x)	AERadToDeg(AEASin(x))
    #define AEACosDeg(x)	AERadToDeg(AEACos(x))
    #define AEATanDeg(x)	AERadToDeg(AEATan(x))
//@}

// ---------------------------------------------------------------------------

/******************************************************************************/
/*!
\fn         u32 AEIsPowOf2(u32 x)

\brief      Check if x is a power of 2.

\details    Powers of 2 are values which can be presented as 2 ^ N,
            where N is any non-negative integer.
            Examples of powers of 2 are:
                - 1     (2 ^ 0)
                - 2     (2 ^ 1)
                - 4     (2 ^ 2)
                - 256   (2 ^ 8)
                - 1024  (2 ^ 10)

\param      [in] x
            The value to be checked.

\retval     u32
            Returns true (non-zero value) if x is a power of 2.
            Else return false (zero).
*/
/******************************************************************************/
AE_API u32		AEIsPowOf2	(u32 x);

/******************************************************************************/
/*!
\fn         u32 AENextPowOf2(u32 x)

\brief      Calculate the next power of 2 that is greater than x.

\details    Powers of 2 are values which can be presented as 2 ^ N,
            where N is any non-negative integer.
            Examples of powers of 2 are:
                - 1     (2 ^ 0)
                - 2     (2 ^ 1)
                - 4     (2 ^ 2)
                - 256   (2 ^ 8)
                - 1024  (2 ^ 10)

\param      [in] x
            The input value.

\retval     u32
            Returns the next power of 2 greater than x.
*/
/******************************************************************************/
AE_API u32		AENextPowOf2(u32 x);

/******************************************************************************/
/*!
\fn         u32 AELogBase2(u32 x)

\brief      Calculate the log2 of x.

\details    Calculate the log2 of x, rounded to the lower integer.

\param      [in] x
            The input value.

\retval     u32
            The log2 of x, rounded to lower integer.
*/
/******************************************************************************/
AE_API u32		AELogBase2	(u32 x);

/******************************************************************************/
/*!
\fn         f32 AEClamp(f32 x, f32 x0, f32 x1)

\brief      Clamp x to between x0 and x1.

\details    If x is lower than the minimum value (x0), return x0.
            If x is higher than the maximum value (x1), return x1.
            Else return x.

\param      [in] x
            The input value.

\param      [in] x0
            The minimum value.

\param      [in] x1
            The maximum value.

\retval     f32
            The clamped value of x.
*/
/******************************************************************************/
AE_API f32		AEClamp		(f32 X, f32 Min, f32 Max);

/******************************************************************************/
/*!
\fn         f32	AEWrap(f32 x, f32 x0, f32 x1)

\brief      Wraparound for x with respect to range (x0 to x1).

\details    If x is lesser than x0, return (x + range).
            If x is higher than x1, return (x - range).

\warning    Wraparound does not work if x is lesser than (x0 - range)
            or if x is greater than (x1 + range).

\param      [in] x
            The input value.

\param      [in] x0
            The lower bound of range

\param      [in] x1
            The upper bound of range.

\retval     f32
            The wraparound value of x with respect to range.
*/
/******************************************************************************/
AE_API f32		AEWrap		(f32 x, f32 x0, f32 x1);

/******************************************************************************/
/*!
\fn         f32	AEMin(f32 x, f32 y)

\brief      Find which of the 2 value is lower.

\details    If x is lower than y, return x.
            If y is lower than x, return y.

\param      [in] x
            The first input value.

\param      [in] y
            The second input value.

\retval     f32
            The lower of the 2 value.
*/
/******************************************************************************/
AE_API f32		AEMin		(f32 x, f32 y);


/******************************************************************************/
/*!
\fn         f32	AEMax(f32 x, f32 y)

\brief      Find which of the 2 value is higher.

\details    If x is higher than y, return x.
            If y is higher than x, return y.

\param      [in] x
            The first input value.

\param      [in] y
            The second input value.

\retval     f32
            The higher of the 2 value.
*/
/******************************************************************************/
AE_API f32		AEMax		(f32 x, f32 y);

/******************************************************************************/
/*!
\fn         s32 AEInRange(f32 x, f32 x0, f32 x1)

\brief      Find if x is in the range (x0 to x1), inclusive.

\details    If x is more than or equal to x0 OR 
            If x is less than or equal to x1, return true.
            Else return false.

\param      [in] x
            The input value.

\param      [in] x0
            The lower bound of range.

\param      [in] x1
            The upper bound of range.

\retval     s32
            Returns true if x is between x0 and x1, inclusive.
            Else return false.
*/
/******************************************************************************/
AE_API s32		AEInRange	(f32 x, f32 x0, f32 x1);

#ifdef __cplusplus 
}
#endif

// ---------------------------------------------------------------------------

#endif // AE_MATH_H
