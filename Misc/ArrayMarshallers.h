/***********************************************************************
ArrayMarshallers - Generic marshaller classes for standard C-style
arrays, with implicit or explicit array sizes.
Copyright (c) 2010 Oliver Kreylos

This file is part of the Miscellaneous Support Library (Misc).

The Miscellaneous Support Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Miscellaneous Support Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Miscellaneous Support Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef MISC_ARRAYMARSHALLERS_INCLUDED
#define MISC_ARRAYMARSHALLERS_INCLUDED

#include <stddef.h>
#include <Misc/Marshaller.h>

namespace Misc {

template <class ValueParam>
class FixedArrayMarshaller // Marshaller class for arrays with a-priori known sizes
	{
	/* Methods: */
	public:
	static size_t getSize(const ValueParam* elements,size_t numElements)
		{
		size_t result=0;
		for(size_t i=0;i<numElements;++i)
			result+=Marshaller<ValueParam>::getSize(elements[i]);
		return result;
		}
	template <class DataSinkParam>
	static void write(const ValueParam* elements,size_t numElements,DataSinkParam& sink)
		{
		for(size_t i=0;i<numElements;++i)
			Marshaller<ValueParam>::write(elements[i],sink);
		}
	template <class DataSourceParam>
	static void read(ValueParam* elements,size_t numElements,DataSourceParam& source)
		{
		for(size_t i=0;i<numElements;++i)
			elements[i]=Marshaller<ValueParam>::read(source);
		}
	};

template <class ValueParam>
class DynamicArrayMarshaller // Marshaller class for arrays with explicit sizes
	{
	/* Methods: */
	public:
	static size_t getSize(const ValueParam* elements,size_t numElements)
		{
		size_t result=sizeof(unsigned int);
		for(size_t i=0;i<numElements;++i)
			result+=Marshaller<ValueParam>::getSize(elements[i]);
		return result;
		}
	template <class DataSinkParam>
	static void write(const ValueParam* elements,size_t numElements,DataSinkParam& sink)
		{
		sink.template write<unsigned int>((unsigned int)numElements);
		for(size_t i=0;i<numElements;++i)
			Marshaller<ValueParam>::write(elements[i],sink);
		}
	template <class DataSourceParam>
	static size_t read(ValueParam* elements,size_t maxNumElements,DataSourceParam& source) // Reads at most maxNumElements elements; returns total number of elements to be read
		{
		size_t numElements=size_t(source.template read<unsigned int>());
		if(maxNumElements>numElements)
			maxNumElements=numElements;
		for(size_t i=0;i<maxNumElements;++i)
			elements[i]=Marshaller<ValueParam>::read(source);
		return numElements;
		}
	template <class DataSourceParam>
	static void discard(size_t numElements,DataSourceParam& source) // Discards a number of elements from the source
		{
		for(size_t i=0;i<numElements;++i)
			Marshaller<ValueParam>::read(source);
		}
	template <class DataSourceParam>
	static size_t read(ValueParam*& elements,DataSourceParam& source) // Reads into newly-allocated array of correct size
		{
		size_t numElements=size_t(source.template read<unsigned int>());
		elements=new ValueParam[numElements];
		for(size_t i=0;i<numElements;++i)
			elements[i]=Marshaller<ValueParam>::read(source);
		return numElements;
		}
	};

}

#endif
