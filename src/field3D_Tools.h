// Copyright (c) 2011 Prime Focus Film.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the
// distribution. Neither the name of Prime Focus Film nor the
// names of its contributors may be used to endorse or promote
// products derived from this software without specific prior written
// permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.


#ifndef FIELD3DTOOLS_H
#define FIELD3DTOOLS_H

#include <vector>
#include <string>

#include <Field3D/Field3DFile.h>
#include <Field3D/DenseField.h>
#include <Field3D/SparseField.h>
#include <Field3D/MACField.h>
#include <Field3D/Types.h>
#include <Field3D/FieldMetadata.h>
#include <Field3D/InitIO.h>

#include "tinyLogger.h"




namespace Field3DTools {

const float SPARSE_THRESHOLD = 0.0000001 ;

// ---------------------  Infos
void getFieldNames       ( Field3D::Field3DInputFile *file   , std::vector< std::string > &names );
bool getFieldsResolution ( Field3D::Field3DInputFile *inFile , unsigned int (&res)[3]            );

//std::string getScalarFieldValueType( Field3D::Field3DInputFile *file , std::string name );
//FieldRes::dataTypeString()
//Field<T>::typedef Data_T value_type;
//
//
//std::string getScalarFieldClassType( Field3D::Field3DInputFile *file , std::string name );
//FieldBase::className()
//FieldBase::typedef Field<Data_T> class_type;

enum SupportedFieldTypeEnum {
	DenseScalarField_Half   ,
	DenseScalarField_Float  ,
	SparseScalarField_Half  ,
	SparseScalarField_Float ,
	DenseVectorField_Half   ,
	DenseVectorField_Float  ,
	SparseVectorField_Half  ,
	SparseVectorField_Float ,
	MACField_Half           ,
	MACField_Float          ,
	TypeUnsupported
};

enum FieldTypeEnum      { DENSE , SPARSE } ;
enum FieldDataTypeEnum  { FLOAT , HALF   } ;


bool getFieldValueType( Field3D::Field3DInputFile *inFile , std::string name, SupportedFieldTypeEnum &type) ;


template<typename Data_T>
void setFieldProperties(
		Field3D::ResizableField<Data_T> &field      ,
		const std::string               name        ,
		const std::string               attribute   ,
		double                          transform[4][4]
)
{
	// name, attribute
	field.name      = name.c_str();
	field.attribute = attribute.c_str();

	// mapping
	Field3D::MatrixFieldMapping::Ptr mapping(new Field3D::MatrixFieldMapping);
	Field3D::M44d transf(transform);

	// just store the local transform
	mapping->setLocalToWorld(transf);
	field.setMapping(mapping);

}


// ---------------------  Read Field3d field into raw arrays
template< class FieldType ,typename MayaArray >
bool readScalarField(
		Field3D::Field3DInputFile  *in       ,
		const char *      /*fluidName*/      ,
		const char *      fieldName          ,
		MayaArray         &data
)
{

	typedef typename FieldType::value_type ImportType;

	typename Field3D::Field<ImportType >::Vec sl    = in->readScalarLayers<ImportType>(fieldName) ;
	typename FieldType::Ptr                   field = Field3D::field_dynamic_cast< FieldType >(sl[0]);
	if( !field ) {
		ERROR( std::string("Failed to read ") + fieldName + " : Dynamic downcasting failed ");
		return false;
	}

	// non-safe cast to unsigned int : but resolution
	// should'nt be stored as ints in field3D anyway
	Field3D::V3i reso = field->dataResolution();
	unsigned int resolution[3];
	resolution[0] = (unsigned int) reso.x;
	resolution[1] = (unsigned int) reso.y;
	resolution[2] = (unsigned int) reso.z;


	DEBUG( "Start copy " );
	typename FieldType::const_iterator it = field->cbegin();

	for ( ; it != field->cend(); ++it) {
		const unsigned int src = it.x + resolution[0]*it.y + resolution[0]*resolution[1]*it.z;

#if defined(DEBUG_MODE)
		unsigned int arraySize = resolution[0]*resolution[1]*resolution[2] ;
		if( src <  arraySize ) {
#endif
			// TODO : check conversion
			data[src] = (float) *it;

#if defined(DEBUG_MODE)
		}
		else {
			std::stringstream idx,len;
			idx<<src; len<<arraySize;
			ERROR( std::string("Failed to copy channel ") + fieldName + " into field : Index (" + idx.str() + ") is out of bounds (" + len.str()+ ")") ;
			return false;
		}
#endif

	}
	DEBUG( "End copy " );

	return true;
}


template< typename FieldType ,typename MayaArray >
bool readVectorField(
		Field3D::Field3DInputFile  *in       ,
		const char *      /*fluidName*/      ,
		const char *      fieldName          ,
		MayaArray         &data
)
{

	typedef typename FieldType::value_type ImportType;
	typedef typename ImportType::BaseType  DataType;

	typename Field3D::Field<ImportType>::Vec sl    = in->readVectorLayers<DataType>(fieldName)       ;
	typename FieldType::Ptr                  field = Field3D::field_dynamic_cast< FieldType >(sl[0]) ;
	if( !field ) {
		ERROR( std::string("Failed to read ") + fieldName + " : Dynamic downcasting failed ");
		return false;
	}

	// non-safe cast to unsigned int : but resolution
	// should'nt be stored as ints in field3D anyway
	Field3D::V3i reso = field->dataResolution();
	unsigned int resolution[3];
	resolution[0] = (unsigned int) reso.x;
	resolution[1] = (unsigned int) reso.y;
	resolution[2] = (unsigned int) reso.z;


	typename FieldType::const_iterator it   = field->cbegin();
	typename FieldType::const_iterator iend = field->cend();
	for ( ; it != iend; ++it)  {
		data[ 0 + (it.x)*3 + (it.y)*3*resolution[0] + (it.z)*3*resolution[0]*resolution[1] ] = (*it).x;
		data[ 1 + (it.x)*3 + (it.y)*3*resolution[0] + (it.z)*3*resolution[0]*resolution[1] ] = (*it).y;
		data[ 2 + (it.x)*3 + (it.y)*3*resolution[0] + (it.z)*3*resolution[0]*resolution[1] ] = (*it).z;
	}

	return true;
}



template< typename ImportType , typename MayaArray >
bool readMACField(
		Field3D::Field3DInputFile  *in       ,
		const char *      /*fluidName*/      ,
		const char *      fieldName          ,
		MayaArray         &data
)
{

	typename Field3D::Field<FIELD3D_VEC3_T<ImportType> >::Vec    sl    = in->readVectorLayers<ImportType>(fieldName) ;
	typename Field3D::MACField<FIELD3D_VEC3_T<ImportType> >::Ptr field = Field3D::field_dynamic_cast< Field3D::MACField<FIELD3D_VEC3_T<ImportType> > >(sl[0]);
	if( !field ) {
		ERROR( std::string("Failed to read ") + fieldName + " : Dynamic downcasting failed  ");
		return false;
	}

	// non-safe cast to unsigned int : but resolution
	// should'nt be stored as ints in field3D anyway
	Field3D::V3i reso = field->dataResolution();
	unsigned int resolution[3];
	resolution[0] = (unsigned int) reso.x;
	resolution[1] = (unsigned int) reso.y;
	resolution[2] = (unsigned int) reso.z;


	Field3D::V3i s = field->getComponentSize();

	DEBUG( "Start copy " );
	// copy data into MAC field
	Field3D::MACComponent compo[3]={Field3D::MACCompU,Field3D::MACCompV,Field3D::MACCompW};
	unsigned int off=0;
	unsigned int r[3] = {0,0,0};
	for(unsigned int cp=0;cp<3;cp++) {

		if(cp==0) {
			r[0]=resolution[0]+1;
			r[1]=resolution[1];
			r[2]=resolution[2];
			off=0;
		}
		else if(cp==1) {
			r[0]=resolution[0];
			r[1]=resolution[1]+1;
			r[2]=resolution[2];
			off+=s.x ;//r[0]*r[1]*r[2];
		}
		else if(cp==2) {
			r[0]=resolution[0];
			r[1]=resolution[1];
			r[2]=resolution[2]+1;
			off+= s.y; //r[0]*r[1]*r[2];
		}

		unsigned int src=0;
		typename Field3D::MACField<FIELD3D_VEC3_T<ImportType> >::mac_comp_iterator i    = field->begin_comp(compo[cp]);
		typename Field3D::MACField<FIELD3D_VEC3_T<ImportType> >::mac_comp_iterator iend = field->end_comp(compo[cp]);
		for ( ; i != iend; ++i)  {
			src = off + i.x  + i.y*r[0] + i.z*r[0]*r[1];
			data[src]=(*i);
		}

	}

	DEBUG( "End copy " );

	return true;
}


// ---------------------  Write raw arrays into Field3D files
template< typename ExportType >
bool writeDenseScalarField(
		Field3D::Field3DOutputFile *out     ,
		const char *      fluidName         ,
		const char *      fieldName         ,
		unsigned int      res[3]            ,
		double            transform[4][4]   ,
		float *           data
)
{
	if( data == NULL ) {
		ERROR("Array is NULL");
		return false;
	}

	// field declaration
	typename Field3D::DenseField<ExportType>::Ptr field( new Field3D::DenseField<ExportType>() ) ;

	// properties
	Field3DTools::setFieldProperties( *field.get(), fluidName, fieldName, transform);

	// copy channel into the scalar field
	field->setSize(Field3D::V3i(res[0],res[1],res[2]));
	for(unsigned int k=0; k<res[2];k++) {
		for(unsigned int j=0; j<res[1];j++) {
			for(unsigned int i=0; i<res[0];i++) {
				// TODO : check conversion
				ExportType val = (ExportType) *(data+ i + res[0]*j + res[0]*res[1]*k);
				field->fastLValue(i,j,k) = val;
			}
		}
	}

	// write it onto disk
	if( !out->writeScalarLayer<ExportType>(field) ) {
		ERROR( std::string("Problem while writing dense scalar field ") + fieldName + " : Unknown Reason ");
		return false;
	}

	return true;
}



template< typename ExportType >
bool writeSparseScalarField(
		Field3D::Field3DOutputFile *out     ,
		const char *    fluidName           ,
		const char *    fieldName           ,
		unsigned int    res[3]              ,
		double          transform[4][4]     ,
		float *         data
)
{


	// check
	if( data == NULL ) {
		ERROR("Array is NULL");
		return false;
	}

	// field declaration
	typename Field3D::SparseField<ExportType>::Ptr field( new Field3D::SparseField<ExportType>() );

	// properties
	Field3DTools::setFieldProperties( *field.get(), fluidName, fieldName, transform);

	// copy channel into the scalar field
	field->setSize(Field3D::V3i(res[0],res[1],res[2]));
	for(unsigned int k=0; k<res[2];k++) {
		for(unsigned int j=0; j<res[1];j++) {
			for(unsigned int i=0; i<res[0];i++) {
				// TODO : check conversion
				ExportType val=(ExportType) *(data + i + res[0]*j + res[0]*res[1]*k);
				if(val > SPARSE_THRESHOLD ) {
					field->fastLValue(i,j,k) = val;
				}
			}
		}
	}

	// write it onto disk
	if( !out->writeScalarLayer<ExportType>(field) ) {
		ERROR( std::string("Problem while writing sparse scalar field ") + fieldName + " : Unknown Reason ");
		return false;
	}

	return true;
}




template< typename ExportType >
extern bool writeDenseVectorField(
		Field3D::Field3DOutputFile *out     ,
		const char *    fluidName           ,
		const char *    fieldName           ,
		unsigned int    res[3]              ,
		double          transform[4][4]     ,
		const float *   data0               ,
		const float *   data1               ,
		const float *   data2
)
{

	// check
	if( data0 == NULL || data0 == NULL || data2 == NULL ) {
		ERROR("Arrays are NULL");
		return false;
	}

	// field declaration
	typename Field3D::DenseField<FIELD3D_VEC3_T<ExportType> >::Ptr field( new Field3D::DenseField<FIELD3D_VEC3_T<ExportType> > );

	// properties
	Field3DTools::setFieldProperties(*field.get(), fluidName, fieldName, transform);

	// copy channel into the vector field
	field->setSize(Field3D::V3i(res[0],res[1],res[2]));
	for(unsigned int k=0; k<res[2];k++) {
		for(unsigned int j=0; j<res[1];j++) {
			for(unsigned int i=0; i<res[0];i++) {
				// TODO : check conversion
				ExportType a= (ExportType) *(data0 + i + res[0]*j + res[0]*res[1]*k);
				ExportType b= (ExportType) *(data1 + i + res[0]*j + res[0]*res[1]*k);
				ExportType c= (ExportType) *(data2 + i + res[0]*j + res[0]*res[1]*k);
				field->fastLValue(i,j,k) = Imath::Vec3<ExportType>(a,b,c);
			}
		}
	}

	// write it onto disk
	if( !out->writeScalarLayer<FIELD3D_VEC3_T<ExportType> >(field) ) {
		ERROR( std::string("Problem while writing dense vector field ") + fieldName + " : Unknown Reason ");
		return false;
	}

	return true;
}


template< typename ExportType >
extern bool writeSparseVectorField(
		Field3D::Field3DOutputFile *out     ,
		const char *    fluidName           ,
		const char *    fieldName           ,
		unsigned int    res[3]              ,
		double          transform[4][4]     ,
		const float *   data0               ,
		const float *   data1               ,
		const float *   data2
)
{


	// check
	if( data0 == NULL || data0 == NULL || data2 == NULL ) {
		ERROR("Arrays are NULL");
		return false;
	}

	// field declaration
	typename Field3D::DenseField<FIELD3D_VEC3_T<ExportType> >::Ptr field( new Field3D::DenseField<FIELD3D_VEC3_T<ExportType> >() );

	// properties
	Field3DTools::setFieldProperties(*field, fluidName, fieldName, transform);

	// copy channel into the vector field
	field->setSize(Field3D::V3i(res[0],res[1],res[2]));
	for(unsigned int k=0; k<res[2];k++) {
		for(unsigned int j=0; j<res[1];j++) {
			for(unsigned int i=0; i<res[0];i++) {
				// TODO : check conversion
				ExportType a= (ExportType) *(data0 + i + res[0]*j + res[0]*res[1]*k);
				ExportType b= (ExportType) *(data1 + i + res[0]*j + res[0]*res[1]*k);
				ExportType c= (ExportType) *(data2 + i + res[0]*j + res[0]*res[1]*k);

				// it is not very clear how to use a threshold for a vector field
				if(a*a + b*b + c*c > SPARSE_THRESHOLD )
					field->fastLValue(i,j,k) = Imath::Vec3<ExportType>(a,b,c);
			}
		}
	}

	// write it onto disk
	if( !out->writeScalarLayer<FIELD3D_VEC3_T<ExportType> >(field) ) {
		ERROR( std::string("Problem while writing sparse vector field ") + fieldName + " : Unknown Reason ");
		return false;
	}

	return true;
}


template< typename ExportType >
extern bool writeMACVectorField(
		Field3D::Field3DOutputFile *out     ,
		const char *    fluidName           ,
		const char *    fieldName           ,
		unsigned int    res[3]              ,
		double          transform[4][4]     ,
		const float *   vx                  ,
		const float *   vy                  ,
		const float *   vz
)
{

	// check
	if( vx == NULL || vy == NULL || vz == NULL ) {
		ERROR("Arrays are NULL");
		return false;
	}

	// field declaration
	typename Field3D::MACField<FIELD3D_VEC3_T<ExportType> >::Ptr field(new Field3D::MACField<FIELD3D_VEC3_T<ExportType> >());

	// properties
	Field3DTools::setFieldProperties(*field, fluidName, fieldName, transform);

	// copy channel into the vector field
	field->setSize(Field3D::V3i(res[0],res[1],res[2]));

	// do the common job for all components (instead of doing it per component)
	for(unsigned int x = 0; x < res[0]; x++)
		for(unsigned int y = 0; y < res[1]; y++)
			for(unsigned int z = 0; z < res[2]; z++) {
				// TODO : check conversion
				field->u(x,y,z)= (ExportType) *(vx + x + (res[0]+1)*y + (res[0]+1)*res[1]*z) ;
				field->v(x,y,z)= (ExportType) *(vy + x + res[0]*y     + res[0]*(res[1]+1)*z) ;
				field->w(x,y,z)= (ExportType) *(vz + x + res[0]*y     + res[0]*res[1]*z    ) ;
			}

	// and fill the remaining component :u
	for(unsigned int y = 0; y < res[1]; y++)
		for(unsigned int z = 0; z < res[2]; z++)
			field->u(res[0],y,z)= (ExportType) *(vx + res[0] + (res[0]+1)*y + (res[0]+1)*res[1]*z) ;

	// and fill the remaining component : v
	for(unsigned int x = 0; x < res[0]; x++)
		for(unsigned int z = 0; z < res[2]; z++)
			field->v(x,res[1],z)= (ExportType) *(vy + x + res[0]*res[1] + res[0]*(res[1]+1)*z) ;

	// and fill the remaining component : w
	for(unsigned int x = 0; x < res[0]; x++)
		for(unsigned int y = 0; y < res[1]; y++)
			field->w(x,y,res[2])= (ExportType) *(vz + x + res[0]*y + res[0]*res[1]*res[2] ) ;

	// write it onto disk
	if(!out->writeScalarLayer<FIELD3D_VEC3_T<ExportType> >(field)) {
		ERROR( std::string("Problem while writing MAC vector field ") + fieldName + " : Unknown Reason ");
		return false;
	}

	return true;
}






}

#endif
