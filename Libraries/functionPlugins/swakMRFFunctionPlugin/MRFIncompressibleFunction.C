/*---------------------------------------------------------------------------*\
|                       _    _  _     ___                       | The         |
|     _____      ____ _| | _| || |   / __\__   __ _ _ __ ___    | Swiss       |
|    / __\ \ /\ / / _` | |/ / || |_ / _\/ _ \ / _` | '_ ` _ \   | Army        |
|    \__ \\ V  V / (_| |   <|__   _/ / | (_) | (_| | | | | | |  | Knife       |
|    |___/ \_/\_/ \__,_|_|\_\  |_| \/   \___/ \__,_|_| |_| |_|  | For         |
|                                                               | OpenFOAM    |
-------------------------------------------------------------------------------
License
    This file is part of swak4Foam.

    swak4Foam is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    swak4Foam is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with swak4Foam; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

Contributors/Copyright:
    2016, 2018 Bernhard F.W. Gschaider <bgschaid@hfd-research.com>

 SWAK Revision: $Id$
\*---------------------------------------------------------------------------*/

#include "MRFIncompressibleFunction.H"

#ifdef FOAM_HAS_IOMRFLIST

#include "FieldValueExpressionDriver.H"

#include "addToRunTimeSelectionTable.H"

#include "fvc.H"

namespace Foam {


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class FieldType>
MRFIncompressibleFunction<FieldType>::MRFIncompressibleFunction(
    const FieldValueExpressionDriver &parentDriver,
    const word &name
):
    swakMRFPluginFunctionBasis(
        parentDriver,
        name,
        FieldType::typeName,
        string("U internalField "+FieldType::typeName)
    )
{
}

// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class FieldType>
void MRFIncompressibleFunction<FieldType>::setArgument(
    label index,
    const string &content,
    const CommonValueExpressionDriver &driver
) {
    assert(index==0);

    field_.set(
        new FieldType(
            dynamic_cast<const FieldValueExpressionDriver &>(
                driver
            ).getResult<FieldType>()
        )
    );
}

template<class FieldType>
void MRFIncompressibleFunction<FieldType>::doEvaluation()
{
    autoPtr<FieldType> pResult(
        new FieldType(
            IOobject(
                "MRFResult",
                mesh().time().timeName(),
                mesh(),
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            field_()
        )
    );
    FieldType &Result=pResult();

    this->manipulate(Result);

    result().setObjectResult(pResult);
}

// * * * * * * * * * * * * * * * Concrete implementations  * * * * * * * * * * * * * //

#define concreteMRF(fName,FType,methodName)             \
class swakMRFPluginFunction_ ## fName ## _inc                       \
: public MRFIncompressibleFunction<FType>                               \
{                                                                       \
public:                                                                 \
    TypeName("swakMRFPluginFunction_" # fName  "_inc");                 \
    swakMRFPluginFunction_ ## fName ## _inc (                           \
        const FieldValueExpressionDriver &parentDriver,                 \
        const word &name                                                \
    ): MRFIncompressibleFunction<FType> (                               \
        parentDriver,                                                   \
        name                                                            \
    ) {}                                                                \
    void manipulate(FType &field) {                                     \
        this->MRF().methodName(field);                                  \
    }                                                                   \
};                                                                      \
defineTypeNameAndDebug(swakMRFPluginFunction_ ## fName ## _inc,0);      \
addNamedToRunTimeSelectionTable(FieldValuePluginFunction,swakMRFPluginFunction_ ## fName ## _inc,name,MRF_ ## fName)

#ifdef FOAM_MRF_NEW_METHOD_NAME
concreteMRF(makeAbsolute,volVectorField,makeAbsolute);
concreteMRF(makeAbsoluteSurf,surfaceScalarField,makeAbsolute);
concreteMRF(makeRelative,volVectorField,makeRelative);
concreteMRF(makeRelativeSurf,surfaceScalarField,makeRelative);
#else
concreteMRF(makeAbsolute,volVectorField,absoluteVelocity);
concreteMRF(makeAbsoluteSurf,surfaceScalarField,absoluteFlux);
concreteMRF(makeRelative,volVectorField,relativeVelocity);
concreteMRF(makeRelativeSurf,surfaceScalarField,relativeFlux);
#endif

class swakCoriolisMRFPluginFunction
: public MRFIncompressibleFunction<volVectorField>
{
public:
    TypeName("swakCoriolisMRFPluginFunction");
    swakCoriolisMRFPluginFunction(
        const FieldValueExpressionDriver &parentDriver,
        const word &name
    ):  MRFIncompressibleFunction<volVectorField>(
        parentDriver,
        name
    ) {}
    void manipulate(volVectorField &) {
        // do nothing
    }
    void doEvaluation() {
            autoPtr<volVectorField> pResult(
                new volVectorField(
                    IOobject(
                        "coriolisMRFResult",
                        mesh().time().timeName(),
                        mesh(),
                        IOobject::NO_READ,
                        IOobject::NO_WRITE
                    ),
                    mesh(),
                    dimensionedVector("zero",dimless,vector::zero),
                    "zeroGradient"
                )
            );
            volVectorField &Result=pResult();

#ifdef FOAM_MRF_CHANGED_CORIOLIS
            MRF().addAcceleration(field_(),Result);
#else
            MRF().addCoriolis(field_(),Result);
#endif
            result().setObjectResult(pResult);
        }

};

defineTypeNameAndDebug(swakCoriolisMRFPluginFunction,0);
addNamedToRunTimeSelectionTable(FieldValuePluginFunction,swakCoriolisMRFPluginFunction,name,MRF_coriolis);
} // namespace

#endif // FOAM_HAS_IOMRFLIST

// ************************************************************************* //
