/*----------------------- -*- C++ -*- ---------------------------------------*\
|                       _    _  _     ___                       | The         |
|     _____      ____ _| | _| || |   / __\__   __ _ _ __ ___    | Swiss       |
|    / __\ \ /\ / / _` | |/ / || |_ / _\/ _ \ / _` | '_ ` _ \   | Army        |
|    \__ \\ V  V / (_| |   <|__   _/ / | (_) | (_| | | | | | |  | Knife       |
|    |___/ \_/\_/ \__,_|_|\_\  |_| \/   \___/ \__,_|_| |_| |_|  | For         |
|                                                               | OpenFOAM    |
-------------------------------------------------------------------------------
License
    This file is part of swak4Foam.

    swak4Foam is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    swak4Foam is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with swak4Foam.  If not, see <http://www.gnu.org/licenses/>.

Description


Contributors/Copyright:
    2006-2018, 2020 Bernhard F.W. Gschaider <bgschaid@hfd-research.com>
    2012 Bruno Santos <wyldckat@gmail.com>

 SWAK Revision: $Id$
\*---------------------------------------------------------------------------*/

#ifndef VALUE_EXPRESSION_DRIVER_I_H
#define VALUE_EXPRESSION_DRIVER_I_H

#include "swak.H"

#include "wallFvPatch.H"
#include "wallPointPatch.H"
#include "fixedValueFvPatchFields.H"
#include "fixedValueFvsPatchFields.H"
#include "zeroGradientFvPatchFields.H"
#include "calculatedFvPatchFields.H"
#include "calculatedFvsPatchFields.H"

#include "foamVersion4swak.H"
#ifndef FOAM_MAPPED_IS_DIRECTMAPPED
#include "mappedFvPatch.H"
#include "mappedWallFvPatch.H"
#else
#include "directMappedFvPatch.H"
#include "directMappedWallFvPatch.H"
#define mappedFvPatch directMappedFvPatch
#define mappedWallFvPatch directMappedWallFvPatch
#endif

#include "volPointInterpolation.H"
#include "interpolatePointToCell.H"

#ifdef FOAM_MAPPED_IS_DIRECTMAPPED
#include "directMappedFvPatch.H"
#include "directMappedPointPatch.H"
#else
#include "mappedFvPatch.H"
#include "mappedPointPatch.H"
#endif

#ifdef FOAM_POINTPATCHFIELD_HAS_FIVE_TEMPLATE_PARAMETERS
#include "FixedValuePointPatchField.H"
#include "ZeroGradientPointPatchField.H"
#include "CalculatedPointPatchField.H"
#else
#include "fixedValuePointPatchFields.H"
#include "zeroGradientPointPatchFields.H"
#include "calculatedPointPatchFields.H"
#endif

#ifdef FOAM_PATCHFIELDTYPE_IN_GEOFIELD_IS_NOW_PATCH
#define PatchFieldType Patch
#define GeometricBoundaryField Boundary
#endif

namespace Foam {

template<class T>
tmp<T>  FieldValueExpressionDriver::getField(const word &name,bool getOldTime)
{
    return tmp<T>(
        this->getOrReadField<T>(
            name,
            true,  // fail if not found
            getOldTime
        ).ptr()
    );
}

template<class T>
tmp<T>  FieldValueExpressionDriver::getPointField(
    const word &name,
    bool getOldTime
)
{
    return tmp<T>(
        this->getOrReadPointField<T>(
            name,
            true,  // fail if not found
            getOldTime
        ).ptr()
    );
}

inline label FieldValueExpressionDriver::size() const
{
    return mesh_.nCells();
}

inline label FieldValueExpressionDriver::pointSize() const
{
    return mesh_.nPoints();
}

inline const fvMesh &FieldValueExpressionDriver::mesh() const
{
    return mesh_;
}

inline bool FieldValueExpressionDriver::canBeValuePatch(const fvPatch &patch)
{
    return
        isA<wallFvPatch>(patch)
        ||
        isType<fvPatch>(patch)
        ||
#ifdef FOAM_MAPPED_IS_DIRECTMAPPED
        isA<directMappedFvPatch>(patch);
#else
        isA<mappedWallFvPatch>(patch)
 	||
        isA<mappedFvPatch>(patch);
#endif
}

inline bool FieldValueExpressionDriver::canBeValuePatch(const pointPatch &patch)
{
    return
        isA<wallPointPatch>(patch)
        ||
        isType<facePointPatch>(patch)
        ||
#ifdef FOAM_MAPPED_IS_DIRECTMAPPED
        isA<directMappedPointPatch>(patch);
#else
        isA<mappedPointPatch>(patch);
#endif
}

template<class T>
void FieldValueExpressionDriver::makePatches
(
    GeometricField<T,fvPatchField,volMesh> &field,
    bool keepPatches,
    const wordList &fixedPatches
) {
  typename GeometricField<T,fvPatchField,volMesh>::GeometricBoundaryField &bf=
      const_cast<typename GeometricField<T,fvPatchField,volMesh>::GeometricBoundaryField&>(field.boundaryField());

  List<fvPatchField<T> *>bfNew(bf.size());

  forAll(bf,patchI) {
    const fvPatch &patch=bf[patchI].patch();

    bool isValuePatch=false;
    forAll(fixedPatches,i) {
      if(fixedPatches[i]==patch.name()) {
	isValuePatch=true;
      }
    }

    if(
        (
            !keepPatches
            ||
            isValuePatch
        )
        &&
        (
            canBeValuePatch(patch)
        )
    ) {
        if(isValuePatch){
            bfNew[patchI]=new fixedValueFvPatchField<T>(patch,field);
        } else {
            bfNew[patchI]=new zeroGradientFvPatchField<T>(patch,field);
        }
    } else {
        bfNew[patchI]=bf[patchI].clone().ptr();
    }
  }

  bf.clear();
  bf.setSize(bfNew.size());
  forAll(bf,i) {
      bf.set(i,bfNew[i]);
  }
}

// code duplication from the template above
template<class T>
void FieldValueExpressionDriver::makePatches
(
    GeometricField<T,fvsPatchField,surfaceMesh> &field,
    bool keepPatches,
    const wordList &fixedPatches
) {
    typename GeometricField<T,fvsPatchField,surfaceMesh>::GeometricBoundaryField &bf=
        const_cast<typename GeometricField<T,fvsPatchField,surfaceMesh>::GeometricBoundaryField&>(field.boundaryField());

  List<fvsPatchField<T> *>bfNew(bf.size());

  forAll(bf,patchI) {
    const fvPatch &patch=bf[patchI].patch();

    bool isValuePatch=false;
    forAll(fixedPatches,i) {
      if(fixedPatches[i]==patch.name()) {
	isValuePatch=true;
      }
    }

    if(
        (
            !keepPatches
            ||
            isValuePatch
        )
        &&
        (
            canBeValuePatch(patch)
        )
    ) {
        if(isValuePatch){
            bfNew[patchI]=new fixedValueFvsPatchField<T>(patch,field);
        } else {
            // this is the only line that is REALLY different
            bfNew[patchI]=new calculatedFvsPatchField<T>(patch,field);
            (*bfNew[patchI])==bf[patchI];
        }
    } else {
        bfNew[patchI]=bf[patchI].clone().ptr();
    }
  }

  bf.clear();
  bf.setSize(bfNew.size());
  forAll(bf,i) {
      bf.set(i,bfNew[i]);
  }
}

// code duplication from the template above
template<class T>
void FieldValueExpressionDriver::makePatches
(
    GeometricField<T,pointPatchField,pointMesh> &field,
    bool keepPatches,
    const wordList &fixedPatches
) {
  typename GeometricField<T,pointPatchField,pointMesh>::GeometricBoundaryField &bf=
      const_cast<typename GeometricField<T,pointPatchField,pointMesh>::GeometricBoundaryField&>(field.boundaryField());

  List<pointPatchField<T> *>bfNew(bf.size());

  forAll(bf,patchI) {
    const pointPatch &patch=bf[patchI].patch();

    bool isValuePatch=false;
    forAll(fixedPatches,i) {
      if(fixedPatches[i]==patch.name()) {
	isValuePatch=true;
      }
    }

    if(
        (
            !keepPatches
            ||
            isValuePatch
        )
        &&
        (
            canBeValuePatch(patch)
        )
    ) {
        if(isValuePatch){
#ifdef FOAM_POINTPATCHFIELD_HAS_FIVE_TEMPLATE_PARAMETERS
            bfNew[patchI]=new FixedValuePointPatchField<pointPatchField,pointMesh,pointPatch,DummyMatrix,T>(patch,field);
#else
            bfNew[patchI]=new fixedValuePointPatchField<T>(patch,field);
#endif
        } else {
#ifdef FOAM_POINTPATCHFIELD_HAS_FIVE_TEMPLATE_PARAMETERS
            bfNew[patchI]=new ZeroGradientPointPatchField<pointPatchField,pointMesh,pointPatch,DummyMatrix,T>(patch,field);
#else
            bfNew[patchI]=new zeroGradientPointPatchField<T>(patch,field);
#endif
        }
    } else {
        bfNew[patchI]=bf[patchI].clone().ptr();
    }
  }

  bf.clear();
  bf.setSize(bfNew.size());
  forAll(bf,i) {
      bf.set(i,bfNew[i]);
  }
}

template<class T>
void FieldValueExpressionDriver::setCalculatedPatches
(
    GeometricField<T,fvPatchField,volMesh> &field,
    T unusedValue
) {
    const typename GeometricField<T,fvPatchField,volMesh>::GeometricBoundaryField &bf=field.boundaryField();

    forAll(bf,patchI) {
        fvPatchField<T> &pf=const_cast<fvPatchField<T>&>(bf[patchI]);

        if(
            typeid(pf)==typeid(calculatedFvPatchField<T>)
        ) {
            pf==pf.patchInternalField();
        }
    }
    field.correctBoundaryConditions();
}

template<class T>
void FieldValueExpressionDriver::setCalculatedPatches
(
    GeometricField<T,fvsPatchField,surfaceMesh> &field,
    T value
) {
    const typename GeometricField<T,fvsPatchField,surfaceMesh>::GeometricBoundaryField &bf=field.boundaryField();

    forAll(bf,patchI) {
        fvsPatchField<T> &pf=const_cast<fvsPatchField<T>&>(bf[patchI]);

        if(
            typeid(pf)==typeid(calculatedFvsPatchField<T>)
        ) {
            //            pf==pf.patchInternalField();
            pf==value;
        }
    }
    //     field.correctBoundaryConditions();
}

template<class T>
void FieldValueExpressionDriver::setCalculatedPatches
(
    GeometricField<T,pointPatchField,pointMesh> &field,
    T unusedValue
) {
    const typename GeometricField<T,pointPatchField,pointMesh>::GeometricBoundaryField &bf=field.boundaryField();
    forAll(bf,patchI) {
        pointPatchField<T> &pf=const_cast<pointPatchField<T>&>(bf[patchI]);

        if(
#ifdef FOAM_POINTPATCHFIELD_HAS_FIVE_TEMPLATE_PARAMETERS
            typeid(pf)==typeid(CalculatedPointPatchField<pointPatchField,pointMesh,pointPatch,DummyMatrix,T>)
#else
            typeid(pf)==typeid(calculatedPointPatchField<T>)
#endif
        ) {
            pf==pf.patchInternalField();
        }
    }
    field.correctBoundaryConditions();
}

template<class T>
void FieldValueExpressionDriver::copyCalculatedPatches
(
    GeometricField<T,fvPatchField,volMesh> &field,
    const GeometricField<T,fvPatchField,volMesh> &orig
) {
    typename GeometricField<T,fvPatchField,volMesh>::GeometricBoundaryField &bf=
        const_cast<typename GeometricField<T,fvPatchField,volMesh>::GeometricBoundaryField&>(field.boundaryField());
    List<fvPatchField<T> *>bfNew(bf.size());

    forAll(bf,patchI) {
        fvPatchField<T> &pf=bf[patchI];

        if(
            typeid(pf)==typeid(calculatedFvPatchField<T>)
        ) {
            pf==pf.patchInternalField();
        }
    }
    //    field.correctBoundaryConditions();
}

template<class T>
void FieldValueExpressionDriver::copyCalculatedPatches
(
    GeometricField<T,fvsPatchField,surfaceMesh> &field,
    const GeometricField<T,fvsPatchField,surfaceMesh> &orig
) {
    typename GeometricField<T,fvsPatchField,surfaceMesh>::GeometricBoundaryField &bf=
        const_cast<typename GeometricField<T,fvsPatchField,surfaceMesh>::GeometricBoundaryField&>(field.boundaryField());

    const typename GeometricField<T,fvsPatchField,surfaceMesh>::GeometricBoundaryField &bfOrig=orig.boundaryField();

    forAll(bf,patchI) {
        fvsPatchField<T> &pf=bf[patchI];
        const fvsPatchField<T> &pfOrig=bfOrig[patchI];

        if(
            typeid(pf)==typeid(calculatedFvsPatchField<T>)
            &&
            typeid(pfOrig)==typeid(calculatedFvsPatchField<T>)
        ) {
            pf==pfOrig;
        }
    }
}

template<class T>
void FieldValueExpressionDriver::copyCalculatedPatches
(
    GeometricField<T,pointPatchField,pointMesh> &field,
    const GeometricField<T,pointPatchField,pointMesh> &orig
) {
    typename GeometricField<T,pointPatchField,pointMesh>::GeometricBoundaryField &bf=
        const_cast<typename GeometricField<T,pointPatchField,pointMesh>::GeometricBoundaryField&>(field.boundaryField());

    List<pointPatchField<T> *>bfNew(bf.size());

    forAll(bf,patchI) {
        pointPatchField<T> &pf=bf[patchI];

        if(
#ifdef FOAM_POINTPATCHFIELD_HAS_FIVE_TEMPLATE_PARAMETERS
            typeid(pf)==typeid(CalculatedPointPatchField<pointPatchField,pointMesh,pointPatch,DummyMatrix,T>)
#else
            typeid(pf)==typeid(calculatedPointPatchField<T>)
#endif
        ) {
            pf==pf.patchInternalField();
        }
    }
    //    field.correctBoundaryConditions();
}

template<class T>
void FieldValueExpressionDriver::setValuePatches
(
    GeometricField<T,fvPatchField,volMesh> &field,
    bool keepPatches,
    const wordList &fixedPatches
) {
  typename GeometricField<T,fvPatchField,volMesh>::GeometricBoundaryField &bf=
      const_cast<typename GeometricField<T,fvPatchField,volMesh>::GeometricBoundaryField&>(field.boundaryField());

  List<fvPatchField<T> *>bfNew(bf.size());

  forAll(bf,patchI) {
    const fvPatch &patch=bf[patchI].patch();

    bool isValuePatch=false;
    forAll(fixedPatches,i) {
      if(fixedPatches[i]==patch.name()) {
	isValuePatch=true;
      }
    }

    if(
        (
            !keepPatches
            ||
            isValuePatch
        )
        &&
        (
            canBeValuePatch(patch)
        )
    ) {
        if(typeid(field.boundaryField()[patchI])==typeid(fixedValueFvPatchField<T>)) {
            fvPatchField<T> &pf=
                const_cast<fvPatchField<T>&>(field.boundaryField()[patchI]);

            pf==pf.patchInternalField();
        }
    }
  }
}

// Code duplication from above ... maybe there is a better way
template<class T>
void FieldValueExpressionDriver::setValuePatches
(
    GeometricField<T,fvsPatchField,surfaceMesh> &field,
    bool keepPatches,
    const wordList &fixedPatches
) {
  typename GeometricField<T,fvsPatchField,surfaceMesh>::GeometricBoundaryField &bf=
      const_cast<typename GeometricField<T,fvsPatchField,surfaceMesh>::GeometricBoundaryField&>(field.boundaryField());

  List<fvsPatchField<T> *>bfNew(bf.size());

  forAll(bf,patchI) {
    const fvPatch &patch=bf[patchI].patch();

    bool isValuePatch=false;
    forAll(fixedPatches,i) {
      if(fixedPatches[i]==patch.name()) {
	isValuePatch=true;
      }
    }

    if(
        (
            !keepPatches
            ||
            isValuePatch
        )
        &&
        (
            canBeValuePatch(patch)
        )
    ) {
        if(typeid(field.boundaryField()[patchI])==typeid(fixedValueFvsPatchField<T>)) {
            fvsPatchField<T> &pf=
                const_cast<fvsPatchField<T>&>(field.boundaryField()[patchI]);

            //            pf==pf.patchInternalField();
            WarningIn(
                "FieldValueExpressionDriver::setValuePatches"
                "("
                "GeometricField<T,fvsPatchField,surfaceMesh> &field,"
                "bool keepPatches,"
                "const wordList &fixedPatches"
                ")"
            ) << "There is no patchInternalField() for fvsPatchField. "
                << "Nothing done for patch " << patch.name()
                << " but setting it to " << pTraits<T>::zero << endl;
            pf==pTraits<T>::zero;
        }
    }
  }
}
// Code duplication from above ... maybe there is a better way
template<class T>
void FieldValueExpressionDriver::setValuePatches
(
    GeometricField<T,pointPatchField,pointMesh> &field,
    bool keepPatches,
    const wordList &fixedPatches
) {
  typename GeometricField<T,pointPatchField,pointMesh>::GeometricBoundaryField &bf=
      const_cast<typename GeometricField<T,pointPatchField,pointMesh>::GeometricBoundaryField&>(field.boundaryField());

  List<pointPatchField<T> *>bfNew(bf.size());

  forAll(bf,patchI) {
    const pointPatch &patch=bf[patchI].patch();

    bool isValuePatch=false;
    forAll(fixedPatches,i) {
      if(fixedPatches[i]==patch.name()) {
	isValuePatch=true;
      }
    }

    if(
        (
            !keepPatches
            ||
            isValuePatch
        )
        &&
        (
            canBeValuePatch(patch)
        )
    ) {
        if(
#ifdef FOAM_POINTPATCHFIELD_HAS_FIVE_TEMPLATE_PARAMETERS
            typeid(field.boundaryField()[patchI])
            ==
            typeid(FixedValuePointPatchField<pointPatchField,pointMesh,pointPatch,DummyMatrix,T>)
#else
            typeid(field.boundaryField()[patchI])
            ==
            typeid(fixedValuePointPatchField<T>)
#endif
        ) {
            pointPatchField<T> &pf=
                const_cast<pointPatchField<T>&>(field.boundaryField()[patchI]);

            pf==pf.patchInternalField();
        }
    }
  }
}

template<class FType>
inline tmp<FType> FieldValueExpressionDriver::makeField(
    const Field<typename FType::value_type> &val
)
{
    return makeFieldInternal<FType>(
        val,
        this->mesh()
    );
}

template<class FType>
inline tmp<FType> FieldValueExpressionDriver::makePointField(
    const Field<typename FType::value_type> &val
)
{
    return makeFieldInternal<FType>(
        val,
        this->pMesh()
    );
}

template<class FType,class Mesh>
inline tmp<FType> FieldValueExpressionDriver::makeFieldInternal(
    const Field<typename FType::value_type> &val,
    const Mesh &actualMesh
)
{
    std::ostringstream buff;
    buff << "field" << pTraits<typename FType::value_type>::typeName;

    tmp<FType> f(
        new FType(
            IOobject
            (
                buff.str(),
                this->time(),
                this->mesh(),
                IOobject::NO_READ,
                IOobject::NO_WRITE,
                false // don't register
            ),
            actualMesh,
            pTraits<typename FType::value_type>::zero,
            "zeroGradient"
        )
    );

    if(val.size()!=f->internalField().size()) {
        FatalErrorIn("FieldValueExpressionDriver::makeField(const Field<typename FType::value_type> &val)")
            << "Size " << val.size()
                << " of the assigned field is not the required "
                << f->internalField().size()
                << endl
                << exit(FatalError);
    }

#ifdef FOAM_NO_DIMENSIONEDINTERNAL_IN_GEOMETRIC
    const_cast<Field<typename FType::value_type>&>(f->internalField().field())
#else
    f->internalField()
#endif
        = val;

    correctField(f);

    return f;
}

template<class FType>
inline tmp<FType> FieldValueExpressionDriver::makeConstantField(
    const typename FType::value_type &val,
    bool makeValuePatches
)
{
    return makeConstantFieldInternal<FType>(
        val,
        mesh_,
        makeValuePatches
    );
}

template<class FType>
inline tmp<FType> FieldValueExpressionDriver::makePointConstantField(
    const typename FType::value_type &val,
    bool makeValuePatches
)
{
    return makeConstantFieldInternal<FType>(
        val,
        this->pMesh(),
        makeValuePatches
    );
}

template<class FType,class Mesh>
inline tmp<FType> FieldValueExpressionDriver::makeConstantFieldInternal(
    const typename FType::value_type &val,
    const Mesh &mesh,
    bool makeValuePatches
)
{
    std::ostringstream buff;
    buff << "constant" << pTraits<typename FType::value_type>::typeName;

    word defaultBC="calculated";
    if(pTraits<typename FType::PatchFieldType>::typeName=="fvPatchField") {
        defaultBC="zeroGradient";
    }
    if(makeValuePatches) {
        defaultBC="fixedValue";
    }

    tmp<FType> f(
        new FType(
            IOobject
            (
                buff.str(),
                time(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE,
                false // don't register
            ),
            mesh,
            val,
            defaultBC
        )
    );
    if(pTraits<typename FType::PatchFieldType>::typeName=="fvsPatchField") {
        this->setCalculatedPatches(const_cast<FType&>(f()),val);
    }

    return f;
}

template<class T>
void FieldValueExpressionDriver::setResult(
    T *r,
    bool isSurfaceField,bool isPointField
)
{
    resultField_.reset(r);

    //    T &result=dynamicCast<T &>(resultField_()); // doesn't work with gcc 4.2

    T &result=dynamic_cast<T &>(resultField_());

    if(!resultDimension_.dimensionless()) {
        result.dimensions().reset(resultDimension_);
    }

    typ_=pTraits<T>::typeName;

    this->result().setResult(
#ifdef FOAM_NO_DIMENSIONEDINTERNAL_IN_GEOMETRIC
        const_cast<Field<typename T::value_type>&>(result.internalField().field()),
#else
        const_cast<Field<typename T::value_type>&>(result.internalField()),
#endif
        isPointField
    );

    isLogical_=false;
    isSurfaceField_=isSurfaceField;
    isPointField_=isPointField;
}

template<class T>
void FieldValueExpressionDriver::setLogicalResult(
    T *r,
    bool isSurfaceField,
    bool isPointField
)
{
    resultField_.reset(r);

    //    T &result=dynamicCast<T &>(resultField_()); // doesn't work with gcc 4.2

    T &result=dynamic_cast<T &>(resultField_());

    if(!resultDimension_.dimensionless()) {
        result.dimensions().reset(resultDimension_);
    }

    //
    typ_=pTraits<T>::typeName;
    if(typ_.find("Field",1)==(typ_.length()-string("Field").length())) {
        if(typ_.find("Scalar")==std::string::npos) {
            FatalErrorIn("FieldValueExpressionDriver::setLogicalResult")
                << "Field of typ " << typ_ << " is not a scalar and "
                    << "can't be interepreted as a boolean" << endl
                    << abort(FatalError);
        }
        typ_=typ_.substr(0,typ_.find("Scalar"))+"LogicalField";
    } else {
        // this is not a GeomtricField. Fall back to old behaviour
        typ_=pTraits<bool>::typeName;
    }

    Field<bool> yesOrNo(result.internalField().size());
    forAll(yesOrNo,i) {
        yesOrNo[i]=toBool(result.internalField()[i]);
    }

    this->result().setResult(yesOrNo,isPointField);

    isLogical_=true;
    isSurfaceField_=isSurfaceField;
    isPointField_=isPointField;
}

template<class T> struct FieldValueExpressionDriver::correctBC {
    inline void operator()(const T &val)
        {
            if(debug) {
                Info << "No way to correct BC for type "
                    << pTraits<T>:: typeName << endl;
            }
            // Doing nothing
        }
};

template<class T> struct FieldValueExpressionDriver::correctBC<GeometricField<T,fvPatchField,volMesh> > {
    typedef GeometricField<T,fvPatchField,volMesh> theType;

    inline void operator()(const theType &val)
        {
            if(debug) {
                Info << "Correcting BC for " << val.name() << " of type "
                    << pTraits<theType>::typeName
                    << endl;
            }
            const_cast<theType&>(
                val
            ).correctBoundaryConditions();
        }
};

template<class T> struct FieldValueExpressionDriver::correctBC<GeometricField<T,pointPatchField,pointMesh> > {
    typedef GeometricField<T,pointPatchField,pointMesh> theType;

    inline void operator()(const theType &val)
        {
            if(debug) {
                Info << "Correcting BC for " << val.name() << " of type "
                    << pTraits<theType>::typeName
                    << endl;
            }
            const_cast<theType&>(
                val
            ).correctBoundaryConditions();
        }
};

template<class T>
const T &FieldValueExpressionDriver::getResult(bool correct) const
{
    if(!resultField_.valid()) {
        FatalErrorIn("FieldValueExpressionDriver::getResult()")
            << "When asking for a " << pTraits<T>::typeName
                << ": No result present"
                << endl
                << exit(FatalError);
    }

    //    return dynamicCast<const T &>(resultField_()); // doesn't work with gcc 4.2
    const T &result=dynamic_cast<const T &>(resultField_());
    if(correct) {
        correctBC<T>()(result);
    }

    return result;
}

template<class T>
bool FieldValueExpressionDriver::resultIsTyp(bool isLogical)
{
    if(!resultField_.valid()) {
        FatalErrorIn("FieldValueExpressionDriver::isTyp(bool isLogical)")
            << "When asking for a " << pTraits<T>::typeName
                << ": No result present"
                << endl
                << exit(FatalError);
    }

    return (
        resultField_().type() == pTraits<T>::typeName
        &&
        isLogical == isLogical_
    );
}

template<class Type>
inline autoPtr<GeometricField<Type,pointPatchField,pointMesh> >
FieldValueExpressionDriver::cellToPointInterpolate(
    const GeometricField<Type,fvPatchField,volMesh> &field
) {
    typedef GeometricField<Type,pointPatchField,pointMesh> rType;

    volPointInterpolation interpol(this->mesh());
    return autoPtr<rType>(interpol.interpolate(field).ptr());
}

template<class Type>
inline autoPtr<GeometricField<Type,fvPatchField,volMesh> >
FieldValueExpressionDriver::pointToCellInterpolate(
    const GeometricField<Type,pointPatchField,pointMesh> &field
) {
    typedef GeometricField<Type,fvPatchField,volMesh> rType;
    autoPtr<rType> result(
        this->makeConstantField<rType>(pTraits<Type>::zero).ptr()
    );
    forAll(result(),cellI) {
        result()[cellI]=interpolatePointToCell(field,cellI);
    }
    result->correctBoundaryConditions();

    return result;
}

} // end namespace

#endif
