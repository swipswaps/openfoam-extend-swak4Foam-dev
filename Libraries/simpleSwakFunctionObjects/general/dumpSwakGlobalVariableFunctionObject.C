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
    2011-2013, 2016-2018 Bernhard F.W. Gschaider <bgschaid@hfd-research.com>

 SWAK Revision: $Id:  $
\*---------------------------------------------------------------------------*/

#include "dumpSwakGlobalVariableFunctionObject.H"
#include "addToRunTimeSelectionTable.H"

#include "volFields.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(dumpSwakGlobalVariableFunctionObject, 0);

    addToRunTimeSelectionTable
    (
        functionObject,
        dumpSwakGlobalVariableFunctionObject,
        dictionary
    );



// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

dumpSwakGlobalVariableFunctionObject::dumpSwakGlobalVariableFunctionObject
(
    const word& name,
    const Time& t,
    const dictionary& dict
)
:
    timelineFunctionObject(name,t,dict),
    globalScope_(dict.lookup("globalScope")),
    globalName_(dict.lookup("globalName"))
{
    const string warnSwitch="IKnowThatThisFunctionObjectMayWriteExcessiveAmountsOfData";
    if(!dict.lookupOrDefault<bool>(warnSwitch,false)) {
        WarningIn("dumpSwakGlobalVariableFunctionObject::dumpSwakGlobalVariableFunctionObject")
            << "This functionObject may write huge amounts of data. "
                << "If you understand the risks set the switch " << warnSwitch
                << " to 'true' to get rid of this warning"
                << endl;
    }
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

word dumpSwakGlobalVariableFunctionObject::dirName()
{
    return typeName;
}

wordList dumpSwakGlobalVariableFunctionObject::fileNames()
{
    return wordList(1,name());
}

stringList dumpSwakGlobalVariableFunctionObject::columnNames()
{
    return stringList(1,"No way to know how much data will follow");
}

void dumpSwakGlobalVariableFunctionObject::writeSimple()
{

    if(verbose()) {
        Info << "Global " << name() << " : ";
    }

    ExpressionResult value(
        GlobalVariablesRepository::getGlobalVariables(
                obr_
        ).get(
            globalName_,
            wordList(1,globalScope_)
        )
    );

    word rType(value.valueType());

    if(rType==pTraits<scalar>::typeName) {
        writeTheData<scalar>(value);
    } else if(rType==pTraits<vector>::typeName) {
        writeTheData<vector>(value);
    } else if(rType==pTraits<tensor>::typeName) {
        writeTheData<tensor>(value);
    } else if(rType==pTraits<symmTensor>::typeName) {
        writeTheData<symmTensor>(value);
    } else if(rType==pTraits<sphericalTensor>::typeName) {
        writeTheData<sphericalTensor>(value);
    } else {
        WarningIn("dumpSwakGlobalVariableFunctionObject::writeSimple()")
            << "Don't know how to handle type " << rType
                << endl;
    }

    if(verbose()) {
        Info << endl;
    }
}

} // namespace Foam

// ************************************************************************* //
