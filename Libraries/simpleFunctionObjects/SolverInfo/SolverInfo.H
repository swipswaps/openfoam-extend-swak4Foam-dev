//  OF-extend Revision: $Id$ 
/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright  held by original author
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is based on OpenFOAM.

    OpenFOAM is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

Class
    SolverInfo

Description
    Determines the type of the Solver (whether it is 
    compressible/incompressible, turbulent etc) 

SourceFiles
    SolverInfo.C

\*---------------------------------------------------------------------------*/

#ifndef SolverInfo_H
#define SolverInfo_H

#include "dictionary.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

    class objectRegistry;

/*---------------------------------------------------------------------------*\
                           Class SolverInfo Declaration
\*---------------------------------------------------------------------------*/

class SolverInfo
{
    // Private data

    word phi_;
    
    bool compressible_;

    bool turbulent_;

    bool les_;

    // Private Member Functions

    //- Disallow default bitwise copy construct
    SolverInfo(const SolverInfo&);

    //- Disallow default bitwise assignment
    void operator=(const SolverInfo&);

public:

    //- Construct from basic information and try to determine properties 
    SolverInfo(const objectRegistry &obr,const dictionary& dict);

    word phi() const { return phi_; }

    bool compressible() const { return compressible_; }

    bool turbulent() const { return turbulent_; }

    bool les() const { return les_; }

    void printInfo();
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //