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

    swak4Foam is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    swak4Foam is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with swak4Foam.  If not, see <http://www.gnu.org/licenses/>.

Contributors/Copyright:
    2012-2014, 2016-2018 Bernhard F.W. Gschaider <bgschaid@hfd-research.com>
    2013 Bruno Santos <wyldckat@gmail.com>

 SWAK Revision: $Id$
\*---------------------------------------------------------------------------*/

#include "readAndUpdateFields.H"
#include "dictionary.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(readAndUpdateFields, 0);
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::readAndUpdateFields::readAndUpdateFields
(
    const word& name,
    const objectRegistry& obr,
    const dictionary& dict,
    const bool loadFromFiles
)
:
    name_(name),
    obr_(obr),
    active_(true),
    fieldSet_(),
    correctBoundary_(true)
{
    // Check if the available mesh is an fvMesh otherise deactivate
    if (!isA<fvMesh>(obr_))
    {
        active_ = false;
        WarningIn
        (
            "readAndUpdateFields::readAndUpdateFields"
            "("
                "const word&, "
                "const objectRegistry&, "
                "const dictionary&, "
                "const bool"
            ")"
        )   << "No fvMesh available, deactivating."
            << endl;
    }

    read(dict);
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::readAndUpdateFields::~readAndUpdateFields()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::readAndUpdateFields::timeSet()
{
    // Do nothing
}

void Foam::readAndUpdateFields::read(const dictionary& dict)
{
    if (active_)
    {
        dict.lookup("fields") >> fieldSet_;

        //Info<< type() << " " << name_ << ":" << nl;

        // Clear out any previously loaded fields
        vsf_.clear();
        vvf_.clear();
        vSpheretf_.clear();
        vSymmtf_.clear();
        vtf_.clear();

        psf_.clear();
        pvf_.clear();
        pSpheretf_.clear();
        pSymmtf_.clear();
        ptf_.clear();

        ssf_.clear();
        svf_.clear();
        sSpheretf_.clear();
        sSymmtf_.clear();
        stf_.clear();

        forAll(fieldSet_, fieldI)
        {
            bool found = loadField<scalar>(fieldSet_[fieldI], vsf_, ssf_, psf_);
            found = found || loadField<vector>(fieldSet_[fieldI], vvf_, svf_, pvf_);
            found = found || loadField<sphericalTensor>(fieldSet_[fieldI], vSpheretf_, sSpheretf_, pSpheretf_);
            found = found || loadField<symmTensor>(fieldSet_[fieldI], vSymmtf_, sSymmtf_, pSymmtf_);
            found = found || loadField<tensor>(fieldSet_[fieldI], vtf_, stf_, ptf_);

            if(!found)
            {
                FatalErrorIn("Foam::readAndUpdateFields::read(const dictionary& dict)")
                    << "Field " << fieldSet_[fieldI] << " does not exist"
                        << endl
                        << exit(FatalError);
            }
        }

        correctBoundary_=dict.lookupOrDefault<bool>("correctBoundary",true);

        if(!dict.found("correctBoundary")) {
            WarningIn("Foam::readAndUpdateFields::read(const dictionary& dict)")
                << "No entry 'correctBoundary' in " << dict.name()
                    << ". Defaulting to 'true'" << endl;
        }
    }
}

const Foam::pointMesh &Foam::readAndUpdateFields::pMesh(const polyMesh &mesh)
{
    if(!pMesh_.valid()) {
        pMesh_.set(new pointMesh(mesh));
    }
    return pMesh_();
}

void Foam::readAndUpdateFields::execute()
{
    if(
        active_
        ||
        correctBoundary_
    ) {
        Info << this->name() << " : Correcting " << flush;

        correctBoundaryConditions(vsf_);
        correctBoundaryConditions(vvf_);
        correctBoundaryConditions(vtf_);
        correctBoundaryConditions(vSymmtf_);
        correctBoundaryConditions(vSpheretf_);

        correctBoundaryConditions(psf_);
        correctBoundaryConditions(pvf_);
        correctBoundaryConditions(ptf_);
        correctBoundaryConditions(pSymmtf_);
        correctBoundaryConditions(pSpheretf_);

        Info << endl;
    }
}


void Foam::readAndUpdateFields::end()
{
    execute();
}


#ifdef FOAM_IOFILTER_WRITE_NEEDS_BOOL
bool
#else
void
#endif
Foam::readAndUpdateFields::write()
{
    // Do nothing
#ifdef FOAM_IOFILTER_WRITE_NEEDS_BOOL
    return true;
#endif
}


// ************************************************************************* //
