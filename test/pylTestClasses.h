#pragma once

#include <cmath>

// This function converts a class between
// its C++ and Python representations
//Vector3 testDataTransfer( Vector3 v );

// You can pass one of these back and forth
// between the interpreter and host program
struct Vector3
{
	// Three float members
	float x{ 0 };
	float y{ 0 };
	float z{ 0 };

	// Vector length
	float length()
	{
		return sqrt( x*x + y*y + z*z );
	}

	// Element access
	float& operator[]( int idx )
	{
		return (&x)[idx];
	}
};

// Expose this class in python
class Foo
{
	Vector3 m_Vec3;

public:
	// Useless default ctor
	// The rest are python callable
	Foo() {}

	// get m_Vec3 member
	Vector3 getVec()
	{
		return m_Vec3;
	}

	// set m_Vec3 member
	void setVec( Vector3 v )
	{
		m_Vec3 = v;
	}

	// normalize m_Vec3 member
	void normalizeVec()
	{
		// testDataTransfer already does this
		m_Vec3 = testDataTransfer( m_Vec3 );
	}
};

